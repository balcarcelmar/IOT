#include <Arduino.h>
#include <ClosedCube_HDC1080.h>
#include <Wire.h>
#include <vector>

// ----- Objeto del sensor -----
ClosedCube_HDC1080 sensor;

// ----- Variables globales -----
int estado = 0;
std::vector<double> datos;  // Almacenará las lecturas de temperatura
double pruning = 0.0;       // Almacenará la media de las lecturas

// ----- Declaraciones de funciones -----
std::vector<double> chirp();
double mean(const std::vector<double>& datos);
String bundling(double pruning);
void sleepArduino(unsigned long seconds);

void setup() {
  Serial.begin(115200);
  sensor.begin(0x40);

  estado = 1;
  Serial.println("Setup completado. Estado inicial = 1");
}

void loop() {
  switch (estado) {
    case 1:
      Serial.println("=== Estado 1: Chirp ===");
      datos = chirp();   // Captura 10 lecturas y las guarda en 'datos'
      estado = 2;        // Pasa al siguiente estado
      break;

    case 2:
      Serial.println("=== Estado 2: Mean ===");
      pruning = mean(datos);  // Calcula la media de las lecturas
      estado = 3;             // Pasa al siguiente estado
      break;

    case 3:
      Serial.println("=== Estado 3: Bundling ===");
      bundling(pruning);      // Determina si está 'cold', 'tempered' o 'hot'
      estado = 4;             // Pasa al siguiente estado
      break;

    case 4:
      Serial.println("=== Estado 4: Sleep ===");
      sleepArduino(1);        // Pausa de 1 segundo
      estado = 1;             // Regresa al estado 1 (reinicia ciclo)
      break;
  }
}

// ----- 1) Función de captura de datos -----
std::vector<double> chirp() {
  std::vector<double> tempDatos;
  tempDatos.reserve(10);  // Reserva espacio para 10 lecturas

  for (int i = 0; i < 10; i++) {
    double lectura = sensor.readTemperature();
    tempDatos.push_back(lectura);
    Serial.print("Lectura ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(lectura);

    delay(200); // Pequeña pausa entre lecturas
  }
  return tempDatos;
}

// ----- 2) Función para calcular la media -----
double mean(const std::vector<double>& datos) {
  if (datos.empty()) {
    return 0.0;  // Evita división por cero
  }
  double suma = 0.0;
  for (double valor : datos) {
    suma += valor;
  }
  double promedio = suma / datos.size();
  Serial.print("Media calculada: ");
  Serial.println(promedio);
  return promedio;
}

// ----- 3) Función que determina el estado en base a la media -----
String bundling(double pruning) {
  String temperature;

  if (pruning <= 15.0) {
    temperature = "cold";
  } 
  else if (pruning < 25.0) {
    temperature = "tempered";
  } 
  else {
    temperature = "hot";
  }

  Serial.print("Temperatura: ");
  Serial.print(pruning);
  Serial.print(" ºC - Estado: ");
  Serial.println(temperature);

  return temperature;
}

// ----- 4) Función de "sleep" para Arduino (en segundos) -----
void sleepArduino(unsigned long seconds) {
  Serial.print("Esperando... ");
  Serial.print(seconds);
  Serial.println(" segundo(s).");
  delay(seconds * 1000UL); // delay() recibe milisegundos
}
