#include <Arduino.h>
#include <ClosedCube_HDC1080.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <vector>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// ======== CONFIGURACIÓN WiFi ========
const char* ssid = "UPBWiFi";
const char* password = "";  // Coloca aquí la contraseña de tu WiFi
const char* server = "3.83.48.126";  // Dirección de tu servidor

ClosedCube_HDC1080 sensor;

static const int RX = 2, TX = 0;
static const uint32_t GPSBaud = 9600;
SoftwareSerial ss(RX, TX);

TinyGPSPlus gps;

int estado = 0;
std::vector<double> datos;
double pruning = 0.0;       

// Declaración de funciones
std::vector<double> chirp();
double mean(const std::vector<double>& datos);
String bundling(double pruning);
void sleepArduino(unsigned long seconds);
String GPSdata();
void sendDataToServer(double pruning, const String& gpsInfo);

// Función smartDelay para procesar datos del GPS
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

void setup() {
  Serial.begin(115200);
  sensor.begin(0x40);
  ss.begin(GPSBaud);

  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  estado = 1;
  Serial.println("Setup completado. Estado inicial = 1");
}

void loop() {
  switch (estado) {
    case 1:
      Serial.println("=== Estado 1: Chirp ===");
      datos = chirp(); 
      estado = 2;  
      break;

    case 2:
      Serial.println("=== Estado 2: Mean ===");
      pruning = mean(datos); 
      estado = 3; 
      break;

    case 3: {
      Serial.println("=== Estado 3: Bundling y Envío ===");
      String tempState = bundling(pruning);
      String gpsInfo = GPSdata();
      Serial.println(gpsInfo);
      
      // Enviar datos al servidor
      sendDataToServer(pruning, gpsInfo);
      
      // Espera 5 segundos antes de continuar
      delay(5000);
      estado = 4;     
      break;
    }
      
    case 4:
      Serial.println("=== Estado 4: Sleep ===");
      sleepArduino(1);
      estado = 1;
      break;
  }
}

std::vector<double> chirp() {
  std::vector<double> tempDatos;
  tempDatos.reserve(10); 
  for (int i = 0; i < 10; i++) {
    double lectura = sensor.readTemperature();
    tempDatos.push_back(lectura);
    Serial.print("Lectura ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(lectura);
    smartDelay(10);
  }
  return tempDatos;
}

double mean(const std::vector<double>& datos) {
  if (datos.empty()) {
    return 0.0; 
  }
  double suma = 0.0;
  for (double valor : datos) {
    suma += valor;
  }
  double promedio = suma / datos.size();
  Serial.print("Media calculada: ");
  Serial.println(promedio);
  smartDelay(0);
  return promedio;
}

String bundling(double pruning) {
  String temperature;
  if (pruning <= 15.0) {
    temperature = "cold";
  } else if (pruning < 25.0) {
    temperature = "tempered";
  } else {
    temperature = "hot";
  }
  Serial.print("Temperatura: ");
  Serial.print(pruning);
  Serial.print(" ºC - Estado: ");
  Serial.println(temperature);
  smartDelay(0);
  return temperature;
}

void sleepArduino(unsigned long seconds) {
  Serial.print("Esperando... ");
  Serial.print(seconds);
  Serial.println(" segundo(s).");
  smartDelay(seconds * 1000UL);
}

String GPSdata() {
  if (!gps.location.isValid()) {
    return "GPS no válido o sin señal.";
  }
  String gpsInfo = "=== Datos GPS ===\n";
  if (gps.time.isValid()) {
    gpsInfo += "Hora UTC: " + String(gps.time.hour()) + ":" +
               String(gps.time.minute()) + ":" + 
               String(gps.time.second()) + "\n";
  } else {
    gpsInfo += "Hora UTC: No disponible\n";
  }
  gpsInfo += "Latitud: " + String(gps.location.lat(), 6) + "\n";
  gpsInfo += "Longitud: " + String(gps.location.lng(), 6) + "\n";
  if (gps.satellites.isValid()) {
    gpsInfo += "Satélites en uso: " + String(gps.satellites.value()) + "\n";
  } else {
    gpsInfo += "Satélites en uso: No disponible\n";
  }
  if (gps.hdop.isValid()) {
    gpsInfo += "Precisión HDOP: " + String(gps.hdop.value()) + "\n";
  } else {
    gpsInfo += "Precisión HDOP: No disponible\n";
  }
  if (gps.altitude.isValid()) {
    gpsInfo += "Altitud: " + String(gps.altitude.meters()) + " m\n";
  } else {
    gpsInfo += "Altitud: No disponible\n";
  }
  return gpsInfo;
}

void sendDataToServer(double pruning, const String& gpsInfo) {
  // Solo intentamos enviar si estamos conectados a WiFi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    
    // Iniciar la conexión al endpoint /sensordata en tu servidor
    http.begin(client, String("http://") + server + "/sensordata");
    http.addHeader("Content-Type", "application/json");

    // Se arma el payload JSON con dos campos: temperature y gpsInfo
    String payload = "{";
    payload += "\"temperature\": \"" + bundling(pruning) + "\"\n" ;
    payload += "\"gpsInfo\": \"" + gpsInfo + "\"";
    payload += "}";

    // Imprimir el payload y su longitud
    Serial.print("Payload: ");
    Serial.println(payload);
    Serial.print("Longitud del payload: ");
    Serial.println(payload.length());


    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error en la petición HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi no conectado. No se envían datos.");
  }
}
