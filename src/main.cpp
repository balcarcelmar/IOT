#include <ClosedCube_HDC1080.h>

#include <Wire.h>
#include <vector>
#include <iostream>

using namespace std;

ClosedCube_HDC1080 sensor;

int estado = 0;

void setup() {

sensor.begin(0x40);

Serial.begin(115200);

estado = 1;
Serial.println("entrado al estado");

}



void loop() {

  switch(estado)
  {
    case 1:
      double n = sensor.readTemperature();

  }

  /*double temperatura = sensor.readTemperature();

  double humedad = sensor.readHumidity();


  Serial.print("Temperatura = ");

  Serial.print(temperatura);

  Serial.print("ºC  Humedad = ");

  Serial.print(humedad);

  Serial.println("%");

  delay(2000);
  */

}

void chirp()
{

    vector <double> datos;
    double dato;

  for (double n, n = 0; n < 10; n++)
  {
      dato = sensor.readTemperature();
      datos.push_back(dato);
  }
}