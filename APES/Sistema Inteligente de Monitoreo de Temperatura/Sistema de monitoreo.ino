/*********************
 * MONITOR LCD DE TEMPERATURA Y LUCES
 * Sensor: LM35
 *********************/

#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int Sensor = A0;
int ValorSensor = 0;
float Temperatura = 0;

int PinRojo = 6;
int PinVerde = 7;
int PinAzul = 8;
int PinSpeaker = 9;

// Variables para millis
unsigned long tiempoAnterior = 0;
const unsigned long intervalo = 500;

void setup() {

  lcd.begin(16, 2);

  pinMode(PinVerde, OUTPUT);
  pinMode(PinRojo, OUTPUT);
  pinMode(PinAzul, OUTPUT);
  pinMode(PinSpeaker, OUTPUT);

}

void loop() {
  if (millis() - tiempoAnterior >= intervalo) {

    tiempoAnterior = millis();
    ValorSensor = analogRead(Sensor);
    Temperatura = (ValorSensor * (500.0 / 1023.0)) - 50.0;

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(Temperatura, 1);
    lcd.print(" C   ");

    digitalWrite(PinAzul, LOW);
    digitalWrite(PinRojo, LOW);
    digitalWrite(PinVerde, LOW);

    lcd.setCursor(0, 1);

    if (Temperatura < 20) {

      lcd.print("Temp. BAJA   ");
      digitalWrite(PinAzul, HIGH);

    } else if (Temperatura > 30) {

      lcd.print("Temp. ALTA   ");
      digitalWrite(PinRojo, HIGH);

    } else {

      lcd.print("Temp. NORMAL ");
      digitalWrite(PinVerde, HIGH);
    }
  }
}