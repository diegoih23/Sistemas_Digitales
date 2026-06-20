#include <LiquidCrystal.h>
#include <DHT.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

//==============================
// LCD
//==============================

#define LCD_RS 13
#define LCD_EN 12
#define LCD_D4 14
#define LCD_D5 27
#define LCD_D6 26
#define LCD_D7 25

LiquidCrystal lcd(
    LCD_RS,
    LCD_EN,
    LCD_D4,
    LCD_D5,
    LCD_D6,
    LCD_D7);

//==============================
// DHT22
//==============================

#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//==============================
// Potenciometro
//==============================

#define POT_PIN 34

//==============================
// Buzzer
//==============================

#define BUZZER 18

//==============================
// Estructuras
//==============================
typedef struct
{
    int bpm;

    float temperatura;

    bool sensorOK;

} SignosVitales;

typedef struct
{
    int bpm;

    float temperatura;

    char estado[25];

    char mensaje[40];

    uint8_t nivel;

    int tendenciaPulso;

    int tendenciaTemp;

} Diagnostico;

typedef enum
{
    PULSO_BRADICARDIA_GRAVE,
    PULSO_BRADICARDIA,
    PULSO_NORMAL,
    PULSO_TAQUICARDIA,
    PULSO_TAQUICARDIA_GRAVE

} EstadoPulso;

typedef enum
{
    TEMP_HIPOTERMIA,
    TEMP_BAJA,
    TEMP_NORMAL,
    TEMP_FEBRICULA,
    TEMP_FIEBRE,
    TEMP_FIEBRE_ALTA

} EstadoTemperatura;

//==============================
// Niveles del sistema
//==============================

#define NORMAL 0

#define PRECAUCION 1

#define ALERTA 2

#define EMERGENCIA 3

//==============================
// Colas
//==============================

QueueHandle_t colaSensores;

QueueHandle_t colaLCD;

QueueHandle_t colaSerial;

QueueHandle_t colaBuzzer;

//==============================
// Mutex LCD
//==============================

SemaphoreHandle_t mutexLCD;

//==============================
// Prototipos
//==============================

void TaskSensores(void *pvParameters);

void TaskDiagnostico(void *pvParameters);

void TaskLCD(void *pvParameters);

void TaskTelemetria(void *pvParameters);

void TaskBuzzer(void *pvParameters);

//-------------------------------------------------
// Funciones de clasificacion clinica
//-------------------------------------------------

EstadoPulso clasificarPulso(int bpm)
{
    if (bpm < 40)
        return PULSO_BRADICARDIA_GRAVE;

    if (bpm < 60)
        return PULSO_BRADICARDIA;

    if (bpm <= 100)
        return PULSO_NORMAL;

    if (bpm <= 120)
        return PULSO_TAQUICARDIA;

    return PULSO_TAQUICARDIA_GRAVE;
}

EstadoTemperatura clasificarTemperatura(float t)
{
    if (t < 35)
        return TEMP_HIPOTERMIA;

    if (t < 36)
        return TEMP_BAJA;

    if (t <= 37.2)
        return TEMP_NORMAL;

    if (t < 38)
        return TEMP_FEBRICULA;

    if (t < 39.5)
        return TEMP_FIEBRE;

    return TEMP_FIEBRE_ALTA;
}

//==============================
// Funciones Auxiliares
//==============================

const char *obtenerNivel(uint8_t nivel)
{
    switch (nivel)
    {
    case NORMAL:
        return "NORMAL";

    case PRECAUCION:
        return "PRECAUCION";

    case ALERTA:
        return "ALERTA";

    case EMERGENCIA:
        return "EMERGENCIA";

    default:
        return "DESCONOCIDO";
    }
}

void beep(int frecuencia, int tiempo)
{
    ledcWriteTone(BUZZER, frecuencia);

    vTaskDelay(pdMS_TO_TICKS(tiempo));

    ledcWriteTone(BUZZER, 0);
}

void silencio(int tiempo)
{
    ledcWriteTone(BUZZER, 0);

    vTaskDelay(pdMS_TO_TICKS(tiempo));
}

//==============================
// Setup
//==============================

void setup()
{

    Serial.begin(115200);

    dht.begin();

    pinMode(BUZZER, OUTPUT);

    digitalWrite(BUZZER, LOW);
    ledcAttach(BUZZER, 2000, 8);

    lcd.begin(16, 2);

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Monitor Medico");

    lcd.setCursor(0, 1);
    lcd.print("Inicializando");

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    delay(2000);

    colaSensores = xQueueCreate(1, sizeof(SignosVitales));

    colaLCD = xQueueCreate(1, sizeof(Diagnostico));

    colaSerial = xQueueCreate(1, sizeof(Diagnostico));

    colaBuzzer = xQueueCreate(1, sizeof(Diagnostico));

    mutexLCD = xSemaphoreCreateMutex();

    if (
        colaSensores == NULL ||
        colaLCD == NULL ||
        colaSerial == NULL ||
        colaBuzzer == NULL ||
        mutexLCD == NULL)
    {
        lcd.clear();
        lcd.print("Error Sistema");

        while (true)
            ;
    }

    xTaskCreatePinnedToCore(

        TaskSensores,

        "Sensores",

        4096,

        NULL,

        3,

        NULL,

        1

    );

    xTaskCreatePinnedToCore(

        TaskDiagnostico,

        "Diagnostico",

        4096,

        NULL,

        3,

        NULL,

        1

    );

    xTaskCreatePinnedToCore(

        TaskLCD,

        "LCD",

        4096,

        NULL,

        2,

        NULL,

        1

    );

    xTaskCreatePinnedToCore(

        TaskTelemetria,

        "Telemetria",

        4096,

        NULL,

        1,

        NULL,

        1

    );

    xTaskCreatePinnedToCore(

        TaskBuzzer,

        "Buzzer",

        2048,

        NULL,

        2,

        NULL,

        1

    );
}

//==============================
// Loop
//==============================

void loop() {}

//==============================
// Tarea Sensores
//==============================

void TaskSensores(void *pvParameters)
{
    SignosVitales datos;

    const byte NUM_MUESTRAS = 15;

    while (true)
    {
        long suma = 0;

        // Promedio movil del potenciometro
        for (byte i = 0; i < NUM_MUESTRAS; i++)
        {
            suma += analogRead(POT_PIN);

            vTaskDelay(pdMS_TO_TICKS(5));
        }

        int lecturaPromedio = suma / NUM_MUESTRAS;

        // Conversion del potenciometro al rango fisiologico completo
        datos.bpm = map(lecturaPromedio, 0, 4095, 30, 180);

        // Lectura del DHT22
        datos.temperatura = dht.readTemperature();

        // Validacion del sensor
        if (isnan(datos.temperatura))
        {
            datos.sensorOK = false;
            datos.temperatura = 0;
        }
        else
        {
            datos.sensorOK = true;
        }

        // Enviar siempre el dato mas reciente
        xQueueOverwrite(colaSensores, &datos);

        // Sincronizado con el tiempo minimo de lectura del DHT22
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

//==============================
// Tarea Diagnostico
//==============================
void TaskDiagnostico(void *pvParameters)
{
    SignosVitales datos;
    Diagnostico diag;

    byte contadorAlerta = 0;
    byte contadorEmergencia = 0;

    static bool primeraLectura = true;
    static int ultimoPulso = 0;
    static float ultimaTemp = 0;

    while (true)
    {
        if (xQueueReceive(colaSensores, &datos, portMAX_DELAY) == pdTRUE)
        {
            diag.bpm = datos.bpm;
            diag.temperatura = datos.temperatura;

            EstadoPulso pulso = clasificarPulso(datos.bpm);
            EstadoTemperatura temp = clasificarTemperatura(datos.temperatura);

            if (primeraLectura)
            {
                diag.tendenciaPulso = 0;
                diag.tendenciaTemp = 0;

                ultimoPulso = datos.bpm;
                ultimaTemp = datos.temperatura;

                primeraLectura = false;
            }
            else
            {
                if (datos.bpm > ultimoPulso)
                    diag.tendenciaPulso = 1;
                else if (datos.bpm < ultimoPulso)
                    diag.tendenciaPulso = -1;
                else
                    diag.tendenciaPulso = 0;

                if (datos.temperatura > ultimaTemp)
                    diag.tendenciaTemp = 1;
                else if (datos.temperatura < ultimaTemp)
                    diag.tendenciaTemp = -1;
                else
                    diag.tendenciaTemp = 0;

                ultimoPulso = datos.bpm;
                ultimaTemp = datos.temperatura;
            }

            //-------------------------
            // Error del sensor
            //-------------------------

            if (!datos.sensorOK)
            {
                strcpy(diag.estado, "ERROR");
                strcpy(diag.mensaje, "Sensor DHT22");
                diag.nivel = EMERGENCIA;

                xQueueOverwrite(colaLCD, &diag);
                xQueueOverwrite(colaSerial, &diag);
                xQueueOverwrite(colaBuzzer, &diag);

                continue;
            }

            //-------------------------
            // Estado por defecto
            //-------------------------

            strcpy(diag.estado, "NORMAL");
            strcpy(diag.mensaje, "Paciente estable");
            diag.nivel = NORMAL;

            //-------------------------
            // Motor de decision
            //-------------------------

            if (
                pulso == PULSO_BRADICARDIA_GRAVE ||
                pulso == PULSO_TAQUICARDIA_GRAVE ||
                temp == TEMP_HIPOTERMIA ||
                temp == TEMP_FIEBRE_ALTA ||
                (pulso == PULSO_TAQUICARDIA && temp == TEMP_FIEBRE) ||
                (pulso == PULSO_BRADICARDIA && temp == TEMP_BAJA))
            {
                contadorEmergencia++;
                contadorAlerta = 0;
            }
            else if (
                pulso == PULSO_BRADICARDIA ||
                pulso == PULSO_TAQUICARDIA ||
                temp == TEMP_FIEBRE)
            {
                contadorAlerta++;
                contadorEmergencia = 0;
            }
            else if (
                temp == TEMP_FEBRICULA ||
                temp == TEMP_BAJA)
            {
                contadorEmergencia = 0;
                contadorAlerta = 0;

                strcpy(diag.estado, "PRECAUCION");
                strcpy(diag.mensaje, "Monitorear");
                diag.nivel = PRECAUCION;
            }
            else
            {
                contadorEmergencia = 0;
                contadorAlerta = 0;
            }

            //-------------------------
            // Confirmacion
            //-------------------------

            if (contadorEmergencia >= 3)
            {
                strcpy(diag.estado, "EMERGENCIA");
                strcpy(diag.mensaje, "Ayuda inmediata");
                diag.nivel = EMERGENCIA;
            }
            else if (contadorAlerta >= 3)
            {
                if (pulso == PULSO_BRADICARDIA)
                {
                    strcpy(diag.estado, "BRADICARDIA");
                    strcpy(diag.mensaje, "Revise pulso");
                }
                else if (pulso == PULSO_TAQUICARDIA)
                {
                    strcpy(diag.estado, "TAQUICARDIA");
                    strcpy(diag.mensaje, "Revise pulso");
                }
                else
                {
                    strcpy(diag.estado, "FIEBRE");
                    strcpy(diag.mensaje, "Temp elevada");
                }

                diag.nivel = ALERTA;
            }

            //-------------------------
            // Enviar diagnostico
            //-------------------------

            xQueueOverwrite(colaLCD, &diag);
            xQueueOverwrite(colaSerial, &diag);
            xQueueOverwrite(colaBuzzer, &diag);
        }
    }
}

//==============================
// Tarea LCD
//==============================

void TaskLCD(void *pvParameters)
{
    Diagnostico diag;

    bool pantalla = false;

    while (true)
    {
        if (xQueuePeek(colaLCD, &diag, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(mutexLCD, pdMS_TO_TICKS(100)))
            {
                lcd.setCursor(0, 0);
                lcd.print("                ");

                lcd.setCursor(0, 1);
                lcd.print("                ");

                if (!pantalla)
                {
                    //-------------------------
                    // Pantalla 1
                    //-------------------------

                    lcd.setCursor(0, 0);
                    lcd.print("Pulso:");
                    lcd.print(diag.bpm);
                    lcd.print(" BPM");

                    lcd.setCursor(0, 1);
                    lcd.print("Temp:");
                    lcd.print(diag.temperatura, 1);
                    lcd.print((char)223);
                    lcd.print("C");
                }
                else
                {
                    //-------------------------
                    // Pantalla 2
                    //-------------------------

                    lcd.setCursor(0, 0);

                    switch (diag.nivel)
                    {
                    case NORMAL:
                        lcd.print("[ OK ]");
                        break;

                    case PRECAUCION:
                        lcd.print("[ PRECAUCION ]");
                        break;

                    case ALERTA:
                        lcd.print("[ ALERTA ]");
                        break;

                    case EMERGENCIA:
                        lcd.print("[ EMERGENCIA ]");
                        break;
                    }

                    lcd.setCursor(0, 1);

                    lcd.print(diag.mensaje);
                }

                xSemaphoreGive(mutexLCD);

                pantalla = !pantalla;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

//==============================
// Tarea Telemetria
//==============================

void TaskTelemetria(void *pvParameters)
{
    Diagnostico diag;

    unsigned long numeroMuestra = 0;

    while (true)
    {
        if (xQueuePeek(colaSerial, &diag, portMAX_DELAY) == pdTRUE)
        {
            numeroMuestra++;

            unsigned long segundosTotales = millis() / 1000;

            unsigned long horas = segundosTotales / 3600;
            unsigned long minutos = (segundosTotales % 3600) / 60;
            unsigned long segundos = segundosTotales % 60;

            Serial.println();
            Serial.println("====================================");
            Serial.println(" MONITOR MEDICO ESP32");
            Serial.println("====================================");

            Serial.print("Paciente     : 001");
            Serial.println();

            Serial.print("Pulso        : ");
            Serial.print(diag.bpm);
            Serial.println(" BPM");

            Serial.print("Temperatura  : ");
            Serial.print(diag.temperatura, 1);
            Serial.println(" C");

            Serial.print("Estado       : ");
            Serial.println(diag.estado);

            Serial.print("Mensaje      : ");
            Serial.println(diag.mensaje);

            Serial.print("Nivel        : ");
            Serial.println(obtenerNivel(diag.nivel));

            Serial.print("Pulso        : ");

            switch (diag.tendenciaPulso)
            {
            case 1:
                Serial.println("Subiendo (^)");
                break;

            case -1:
                Serial.println("Bajando (v)");
                break;

            default:
                Serial.println("Estable (=)");
            }

            Serial.print("Temperatura  : ");

            switch (diag.tendenciaTemp)
            {
            case 1:
                Serial.println("Subiendo (^)");
                break;

            case -1:
                Serial.println("Bajando (v)");
                break;

            default:
                Serial.println("Estable (=)");
            }

            Serial.print("Uptime       : ");

            if (horas < 10)
                Serial.print("0");
            Serial.print(horas);
            Serial.print(":");

            if (minutos < 10)
                Serial.print("0");
            Serial.print(minutos);
            Serial.print(":");

            if (segundos < 10)
                Serial.print("0");
            Serial.println(segundos);

            Serial.println("====================================");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

//==============================
// Tarea Buzzer
//==============================

void TaskBuzzer(void *pvParameters)
{
    Diagnostico diag;

    while (true)
    {
        if (xQueuePeek(colaBuzzer, &diag, portMAX_DELAY) == pdTRUE)
        {
            switch (diag.nivel)
            {
                //-----------------------
                // NORMAL
                //-----------------------

            case NORMAL:

                silencio(500);

                break;

                //-----------------------
                // PRECAUCION
                //-----------------------

            case PRECAUCION:

                beep(1200, 150);

                silencio(2850);

                break;

                //-----------------------
                // ALERTA
                //-----------------------

            case ALERTA:

                beep(1500, 150);

                silencio(120);

                beep(1500, 150);

                silencio(1080);

                break;

                //-----------------------
                // EMERGENCIA
                //-----------------------

            case EMERGENCIA:

                beep(2200, 120);

                silencio(100);

                beep(2200, 120);

                silencio(100);

                beep(2200, 120);

                silencio(800);

                break;
            }
        }
    }
}