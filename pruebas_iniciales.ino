#include <SD.h>
#include "DHT.h"
#include <math.h>

#define pinDHT11 6
#define tipoDHT DHT11
DHT dht11(pinDHT11, tipoDHT);

#define SENSOR_RUIDO A0
#define NUM_MUESTRAS 100
#define VREF 0.02

//Se configuró una fecha y hora de inicio de puesta en marcha del dispositivo.
int dia = 4;
int mes = 4;
int anio = 2025;
int hora = 13;
int minuto = 0;

unsigned long lastMeasurementTime = 0;
File ArchivoCSV;

//Bloque de código que sirve para simular el paso del tiempo
int diasEnMes(int mes) {
  switch (mes) {
    case 4: case 6: case 9: case 11: return 30;
    case 2: return 28;
    default: return 31;
  }
}

//Bloque de código que define cada cuanto tiempo se realizará la medición.
void avanzar20Min() {
  minuto += 20;
  if (minuto >= 60) {
    minuto -= 60;
    hora++;
    if (hora >= 24) {
      hora = 0;
      dia++;
      if (dia > diasEnMes(mes)) {
        dia = 1;
        mes++;
        if (mes > 12) mes = 1;
      }
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  if (!SD.begin(4)) {
    Serial.println("Error al acceder a tarjeta SD.");
    return;
  }

  Serial.println("Acceso correcto a tarjeta SD.");
  dht11.begin();

  if (!SD.exists("dats.csv")) {
    ArchivoCSV = SD.open("dats.csv", FILE_WRITE);
    if (ArchivoCSV) {
      ArchivoCSV.println("Dia,Hora,Temp(Celcius),Humedad(%),Ruido(dB)");
      ArchivoCSV.close();
      Serial.println("Archivo creado con encabezado.");
    } else {
      Serial.println("ERROR: No se pudo crear el archivo CSV.");
    }
  }
}

void loop() {
  Serial.println("Loop activo...");

  if (millis() - lastMeasurementTime >= 2000) {  //Realiza el registro cada 2 segundos suponiendo que ya han pasado 20 minutos
    Serial.println("¡20 minutos pasados! Registrando...");
    lastMeasurementTime = millis();

    int temp = dht11.readTemperature();
    int hum = dht11.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("ERROR: Sensor DHT no responde.");
      return;
    }

    float voltajeBase = 0;
    float voltajePico = 0;

    for (int i = 0; i < NUM_MUESTRAS; i++) {
      int lectura = analogRead(SENSOR_RUIDO);
      float voltaje = (lectura * 5.0) / 1023;
      voltajeBase += voltaje;
      if (voltaje > voltajePico) voltajePico = voltaje;
      delayMicroseconds(100);
    }

    voltajeBase /= NUM_MUESTRAS;
    float voltajeRuido = max(voltajePico - voltajeBase, VREF);
    float dB = 20 * log10(voltajeRuido / VREF);

    char fecha[11];
    sprintf(fecha, "%02d/%02d/%d", dia, mes, anio);

    char horaTexto[6];
    sprintf(horaTexto, "%02d:%02d", hora, minuto);

    ArchivoCSV = SD.open("dats.csv", FILE_WRITE);
    if (ArchivoCSV) {
      ArchivoCSV.print(fecha); ArchivoCSV.print(",");
      ArchivoCSV.print(horaTexto); ArchivoCSV.print(",");
      ArchivoCSV.print(temp); ArchivoCSV.print(",");
      ArchivoCSV.print(hum); ArchivoCSV.print(",");
      ArchivoCSV.println(dB, 2);
      ArchivoCSV.close();
      Serial.println("Registro guardado correctamente.");
    } else {
      Serial.println("ERROR: No se pudo abrir el archivo para escritura.");
    }

    Serial.print(">> "); Serial.print(fecha); Serial.print(" ");
    Serial.print(horaTexto); Serial.print(" - Temp: ");
    Serial.print(temp); Serial.print("°C, Hum: ");
    Serial.print(hum); Serial.print("%, Ruido: ");
    Serial.println(dB, 2);

    avanzar20Min();
  }

  delay(1000);
}
