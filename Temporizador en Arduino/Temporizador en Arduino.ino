// Pines
const int pin555 = 2;
const int pinFF  = 3;

// Variables canal 555
int estadoActual555 = LOW;
int estadoAnterior555 = LOW;
unsigned long tiempoAnterior555 = 0;
unsigned long periodo555 = 0;

unsigned long tHigh555 = 0;
unsigned long tLow555 = 0;
unsigned long tiempoCambio555 = 0;

// Variables canal FF
int estadoActualFF = LOW;
int estadoAnteriorFF = LOW;
unsigned long tiempoAnteriorFF = 0;
unsigned long periodoFF = 0;

unsigned long tHighFF = 0;
unsigned long tLowFF = 0;
unsigned long tiempoCambioFF = 0;

void setup() {
  Serial.begin(9600);

  pinMode(pin555, INPUT);
  pinMode(pinFF, INPUT);
}

void loop() {
  unsigned long tiempoActual = millis();

  // =========================
  // CANAL 555
  // =========================
  estadoActual555 = digitalRead(pin555);

  // Detección de flanco ascendente
  if (estadoActual555 == HIGH && estadoAnterior555 == LOW) {
    periodo555 = tiempoActual - tiempoAnterior555;
    tiempoAnterior555 = tiempoActual;
  }

  // Medición HIGH/LOW
  if (estadoActual555 != estadoAnterior555) {
    if (estadoActual555 == HIGH) {
      tLow555 = tiempoActual - tiempoCambio555;
    } else {
      tHigh555 = tiempoActual - tiempoCambio555;
    }
    tiempoCambio555 = tiempoActual;
  }

  estadoAnterior555 = estadoActual555;

  // =========================
  // CANAL FLIP-FLOP
  // =========================
  estadoActualFF = digitalRead(pinFF);

  if (estadoActualFF == HIGH && estadoAnteriorFF == LOW) {
    periodoFF = tiempoActual - tiempoAnteriorFF;
    tiempoAnteriorFF = tiempoActual;
  }

  if (estadoActualFF != estadoAnteriorFF) {
    if (estadoActualFF == HIGH) {
      tLowFF = tiempoActual - tiempoCambioFF;
    } else {
      tHighFF = tiempoActual - tiempoCambioFF;
    }
    tiempoCambioFF = tiempoActual;
  }

  estadoAnteriorFF = estadoActualFF;

  // =========================
  // CÁLCULOS
  // =========================
  float frecuencia555 = (periodo555 > 0) ? 1000.0 / periodo555 : 0;
  float duty555 = (tHigh555 + tLow555 > 0) ? (tHigh555 * 100.0) / (tHigh555 + tLow555) : 0;

  float frecuenciaFF = (periodoFF > 0) ? 1000.0 / periodoFF : 0;
  float dutyFF = (tHighFF + tLowFF > 0) ? (tHighFF * 100.0) / (tHighFF + tLowFF) : 0;

  // =========================
  // SALIDA
  // =========================
  Serial.print("555 -> Freq: ");
  Serial.print(frecuencia555);
  Serial.print(" Hz | Duty: ");
  Serial.print(duty555);
  Serial.print(" % || ");

  Serial.print("FF -> Freq: ");
  Serial.print(frecuenciaFF);
  Serial.print(" Hz | Duty: ");
  Serial.print(dutyFF);
  Serial.println(" %");

  
}