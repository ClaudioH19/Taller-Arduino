// ============================================================
// Monitoreo Ciclo + Envio HTTP a servidor SQLite
// Placa: Arduino UNO R4 WiFi
// Servidor: http://38.242.251.218:10000/api/monitoreo
// ============================================================

#include <DHT.h>
#include <WiFiS3.h>

// -------------------- WiFi --------------------
const char WIFI_SSID[] = "Utalca-visitas";
const char WIFI_PASS[] = "";

// -------------------- Servidor API --------------------
const char API_HOST[] = "38.242.251.218";
const int API_PORT = 8300;
const char API_PATH[] = "/api/monitoreo";

// -------------------- Pines --------------------
#define PIN_DHT        3
#define PIN_LED_VERDE  4
#define PIN_LED_AZUL   5
#define PIN_LED_ROJO   2
#define PIN_BUZZER     8
#define PIN_TRIG       6
#define PIN_ECHO       7

// -------------------- Sensor --------------------
DHT dht(PIN_DHT, DHT11);

// -------------------- Configuracion --------------------
const int DISTANCIA_LIMITE = 5;

const unsigned long TIEMPO_DIA = 10000;
const unsigned long TIEMPO_TARDE = 5000;
const unsigned long TIEMPO_NOCHE = 8000;

const bool SIMULAR_CURVA_DIARIA = false;
const float INTENSIDAD_RUIDO = 1.0;

const unsigned long INTERVALO_ENVIO = 5000;
const unsigned long WIFI_RETRY_MS = 10000;
const unsigned long HTTP_TIMEOUT_MS = 5000;

// -------------------- Variables --------------------
unsigned long tiempoInicio = 0;
unsigned long ultimoEnvio = 0;
unsigned long ultimoIntentoWiFi = 0;

float temperatura = 0;
float humedad = 0;
float distancia = -1;

WiFiClient client;

// ============================================================
// WIFI
// ============================================================
void iniciarWiFi() {
  Serial.print("Conectando a WiFi publica: ");
  Serial.println(WIFI_SSID);

  if (strlen(WIFI_PASS) > 0) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  } else {
    WiFi.begin(WIFI_SSID);
  }
}

void gestionarWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long ahora = millis();

  if (ahora - ultimoIntentoWiFi >= WIFI_RETRY_MS) {
    ultimoIntentoWiFi = ahora;
    iniciarWiFi();
  }
}

// ============================================================
// FUNCION - RUIDO SIMULADO
// ============================================================
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
  delay(500);

  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_AZUL, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  dht.begin();
  randomSeed(analogRead(A0));

  iniciarWiFi();

  Serial.println("Sistema listo");
  tiempoInicio = millis();
}

// ============================================================
// LOOP PRINCIPAL
// ============================================================
void loop() {
  gestionarWiFi();

  unsigned long tiempoActual = millis();
  unsigned long tiempoCiclo = tiempoActual - tiempoInicio;
  unsigned long relojInternoSegundos = tiempoActual / 1000;

  temperatura = dht.readTemperature();
  humedad = dht.readHumidity();

  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error leyendo DHT11");
    delay(1000);
    return;
  }

  // -------------------- DIA --------------------
  if (tiempoCiclo < TIEMPO_DIA) {
    distancia = -1;

    digitalWrite(PIN_LED_VERDE, HIGH);
    digitalWrite(PIN_LED_AZUL, LOW);
    digitalWrite(PIN_LED_ROJO, LOW);
    noTone(PIN_BUZZER);

    temperatura = temperatura + generarRuido(15, 31);
    humedad     = humedad - generarRuido(10, 41);
  }

  // -------------------- TARDE --------------------
  else if (tiempoCiclo < TIEMPO_DIA + TIEMPO_TARDE) {
    distancia = -1;

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
  Serial.print(humedad);
  Serial.print(",");
  Serial.print(distancia);
  Serial.print(",");
  Serial.println(relojInternoSegundos);

  if (tiempoActual - ultimoEnvio >= INTERVALO_ENVIO) {
    enviarDatosAPI(
      temperatura,
      humedad,
      distancia,
      relojInternoSegundos
    );

    ultimoEnvio = tiempoActual;
  }

  if (tiempoCiclo >= TIEMPO_DIA + TIEMPO_TARDE + TIEMPO_NOCHE) {
    tiempoInicio = millis();
  }

  delay(500);
}

// ============================================================
// ENVIO HTTP A API
// ============================================================
void enviarDatosAPI(
  float temp,
  float hum,
  float dist,
  unsigned long relojSegundos
) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No se envia: WiFi no conectado");
    return;
  }

  String json = "{";
  json += "\"temperatura\":" + String(temp, 2) + ",";
  json += "\"humedad\":" + String(hum, 2) + ",";
  json += "\"distancia_cm\":" + String(dist, 2) + ",";
  json += "\"reloj_interno_segundos\":" + String(relojSegundos);
  json += "}";

  Serial.println("Enviando datos por HTTP...");
  Serial.println(json);

  client.stop();

  if (!client.connect(API_HOST, API_PORT)) {
    Serial.println("No se pudo conectar al servidor");
    return;
  }

  client.print("POST ");
  client.print(API_PATH);
  client.println(" HTTP/1.1");

  client.print("Host: ");
  client.print(API_HOST);
  client.print(":");
  client.println(API_PORT);

  client.println("Content-Type: application/json");
  client.println("Accept: application/json");

  client.print("Content-Length: ");
  client.println(json.length());

  client.println("Connection: close");
  client.println();
  client.print(json);

  unsigned long inicio = millis();

  while (millis() - inicio < HTTP_TIMEOUT_MS) {
    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    if (!client.connected()) {
      break;
    }
  }

  client.stop();

  Serial.println();
  Serial.println("Envio HTTP finalizado");
}

// ============================================================
// PROXIMIDAD Y BUZZER
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

  Serial.print("[Distancia]: ");
  Serial.println(distancia);

  if (distancia > 0 && distancia <= DISTANCIA_LIMITE) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
  }
}