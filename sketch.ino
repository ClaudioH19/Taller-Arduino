// ============================================================
// Monitoreo Ciclo 
// ============================================================
// Modulos:
// 1. Dia / tarde / noche
// 2. Humedad y temperatura
// 3. Proximidad y buzzer
// ============================================================

#include <DHT.h>

// -------------------- Pines --------------------
#define PIN_DHT        2
#define PIN_LED_VERDE  4
#define PIN_LED_AZUL   5
#define PIN_LED_ROJO   6
#define PIN_BUZZER     8
#define PIN_TRIG       9
#define PIN_ECHO       10

// -------------------- Sensor --------------------
DHT dht(PIN_DHT, DHT22);  // Cambiar a DHT11 si usas el sensor azul del kit

// -------------------- Configuracion --------------------
const int DISTANCIA_LIMITE = 5;    // cm

const unsigned long TIEMPO_DIA = 10000;     // 10 segundos
const unsigned long TIEMPO_TARDE = 5000;    // 5 segundos
const unsigned long TIEMPO_NOCHE = 8000;    // 5 segundos

const bool SIMULAR_CURVA_DIARIA = false;     // false = lectura directa del sensor
const float INTENSIDAD_RUIDO = 1.0;         // 0.0 a 1.0 mantiene o reduce el ruido

// -------------------- Variables --------------------
unsigned long tiempoInicio = 0;

float temperatura = 0;
float humedad = 0;
float distancia = 0;

float generarRuido(int minimoDecimas, int maximoDecimas) {
  if (!SIMULAR_CURVA_DIARIA) {
    return 0.0;
  }

  return (random(minimoDecimas, maximoDecimas) / 10.0) * INTENSIDAD_RUIDO;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  dht.begin();
  randomSeed(analogRead(A0));
  Serial.println("Sistema listo");
  tiempoInicio = millis();
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  unsigned long tiempoActual = millis();
  unsigned long tiempoCiclo = tiempoActual - tiempoInicio;

  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();

  // -------------------- DIA --------------------
  if (tiempoCiclo < TIEMPO_DIA) {

    digitalWrite(PIN_LED_VERDE, HIGH);
    digitalWrite(PIN_LED_AZUL, LOW);
    digitalWrite(PIN_LED_ROJO, LOW);

    noTone(PIN_BUZZER);
    temperatura = temperatura + generarRuido(15, 31);
    humedad     = humedad - generarRuido(10, 41); 
  }

  // -------------------- TARDE --------------------
  else if (tiempoCiclo < TIEMPO_DIA + TIEMPO_TARDE) {
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_AZUL, LOW);
    digitalWrite(PIN_LED_ROJO, HIGH);

    noTone(PIN_BUZZER);
    temperatura = temperatura + generarRuido(2, 11);
    humedad     = humedad + generarRuido(-10, 11);     
  }

  // -------------------- NOCHE --------------------
  else {
    digitalWrite(PIN_LED_VERDE, LOW);
    digitalWrite(PIN_LED_AZUL, HIGH);
    digitalWrite(PIN_LED_ROJO, LOW);
    
    temperatura = temperatura - generarRuido(10, 31);
    humedad     = humedad + generarRuido(10, 41); 
    
    moduloProximidad();
  }

  Serial.print("DATA,");
  Serial.print(temperatura);
  Serial.print(",");
  Serial.println(humedad);

  // Reinicia el ciclo completo
  if (tiempoCiclo >= TIEMPO_DIA + TIEMPO_TARDE + TIEMPO_NOCHE) {
    tiempoInicio = millis();
  }

  delay(500);
}


// ============================================================
// MODULO - PROXIMIDAD Y BUZZER
// ============================================================
void moduloProximidad() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);

  digitalWrite(PIN_TRIG, LOW);

  long duracion = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duracion == 0) {
    distancia = -1;
  } else {
    distancia = duracion * 0.034 / 2;
  }

  Serial.print("[Distancia]:");
  Serial.println(distancia);
  if (distancia > 0 && distancia <= DISTANCIA_LIMITE) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
  }
}
