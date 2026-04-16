//Declaracion de variables
const int leds[] = {2, 3, 4, 5, 6, 7};
const int boton = 8;

int estadoBoton = 0;
int modo = 0;  


void setup() {
  for (int i = 0; i < 6; i++) {
    pinMode(leds[i], OUTPUT);
  }

  pinMode(boton, INPUT);
}

void loop() {
  leerBoton();
  ejecutarPatron();
}

void leerBoton() {
  if (digitalRead(boton) == HIGH) {
    modo++;
    if (modo > 4) modo = 0;
    delay(300);
  }
}

void ejecutarPatron() {
  switch (modo) {
    case 0:
      secuencia();
      break;
    case 1:
      persecucion();
      break;
    case 2:
      parpadeo();
      break;
    case 3:
      aleatorio();
      break;
    case 4:
      onda();
      break;
  }
}

void secuencia() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(leds[i], HIGH);
    delay(100);
    digitalWrite(leds[i], LOW);
  }
}

void persecucion() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(leds[i], HIGH);
    delay(80);
    digitalWrite(leds[i], LOW);
  }

  for (int i = 4; i >= 1; i--) {
    digitalWrite(leds[i], HIGH);
    delay(80);
    digitalWrite(leds[i], LOW);
  }
}

void parpadeo() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(leds[i], HIGH);
  }
  delay(200);

  for (int i = 0; i < 6; i++) {
    digitalWrite(leds[i], LOW);
  }
  delay(200);
}

void aleatorio() {
  int led = random(0, 6);
  digitalWrite(leds[led], HIGH);
  delay(100);
  digitalWrite(leds[led], LOW);
}

void onda() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(leds[i], HIGH);
    if (i > 0) digitalWrite(leds[i - 1], LOW);
    delay(100);
  }
}