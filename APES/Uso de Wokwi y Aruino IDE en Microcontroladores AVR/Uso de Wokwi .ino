//declaración de variables 
int LED_PIN = 13; 
byte estadoLed = 0; 
int contador = 0; 
long numeroLargo = 123456; 
float numeroDecimal = 3.14; 
bool encendido = false; 
 
void setup() { 
  pinMode(LED_PIN, OUTPUT); 
  Serial.begin(9600); 
  //operaciones bitwise 
  byte a = 0b00001111; 
  byte b = 0b00000101; 
 
  Serial.println("Operaciones Bitwise:"); 
  Serial.print("AND: "); 
  Serial.println(a & b, BIN); 
  Serial.print("OR: "); 
  Serial.println(a | b, BIN); 
  Serial.print("XOR: "); 
  Serial.println(a ^ b, BIN); 
  Serial.print("NOT a: "); 
  Serial.println(~a, BIN); 
  Serial.print("Shift Left (1 << 2): "); 
  Serial.println(1 << 2, BIN); 
  Serial.print("Shift Right (8 >> 1): "); 
  Serial.println(8 >> 1, BIN); 
  //activar primer bit 
  estadoLed = estadoLed | (1 << 0); 
} 
void loop() { 
  //alternar estado con XOR 
  estadoLed = estadoLed ^ 0b00000001; 
  //verificar estado con AND 
  if ((estadoLed & 1) == 1) { 
    digitalWrite(LED_PIN, HIGH); 
    encendido = true; 
  } else { 
    digitalWrite(LED_PIN, LOW); 
    encendido = false; 
  } 
  delay(500); 
  //contador circular de 0 a 7 
  contador = (contador + 1) % 8; 
  Serial.print("Shift dinámico: "); 
  Serial.println(1 << contador, BIN); 
  delay(500); 
} 