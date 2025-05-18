#include <SD.h>        
#include "DHT.h"       
#include <math.h>      

#define pinDHT11 6
#define tipoDHT DHT11
DHT dht11(pinDHT11, tipoDHT);  // Inicialización del sensor DHT11 en el pin 6

#define SENSOR_RUIDO A0        // Pin analógico para el sensor de ruido
#define NUM_MUESTRAS 100       // Número de muestras para calcular el nivel de ruido
#define VREF 0.02              // Voltaje de referencia para el cálculo en dB

// Variables para simular la fecha y hora de los registros
int dia = 7, mes = 4, anio = 2025;
int hora = 14, minuto = 6;

// Variables de control de tiempo y manejo de archivos
unsigned long lastMeasurementTime = 0;
File ArchivoCSV;

// Devuelve el número de días del mes especificado 
int diasEnMes(int mes) {
  switch (mes) {
    case 4: case 6: case 9: case 11: return 30;
    case 2: return 28;
    default: return 31;
  }
}

// Simula el avance de 20 minutos en la fecha/hora configurada
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
  delay(1000);  // Espera inicial para estabilizar el sistema

  // Inicializa la tarjeta SD; si falla, se detiene el programa
  if (!SD.begin(4)) {
    Serial.println("Error al acceder a tarjeta SD.");
    return;
  }
  Serial.println("Acceso correcto a tarjeta SD.");

  dht11.begin();  // Inicializa el sensor DHT11

  // Si el archivo CSV no existe, lo crea y escribe la cabecera de columnas
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

  // Cada 20 minutos (1,200,000 ms), realiza una lectura y guarda los datos
  if (millis() - lastMeasurementTime >= 1200000) {
    Serial.println("¡20 minutos pasados! Registrando...");
    lastMeasurementTime = millis();

    // Lectura de sensores de temperatura y humedad
    int temp = dht11.readTemperature();
    int hum = dht11.readHumidity();

    // Verifica si las lecturas son válidas
    if (isnan(temp) || isnan(hum)) {
      Serial.println("ERROR: Sensor DHT no responde.");
      return;
    }

    // Procesamiento del nivel de ruido: se obtiene el voltaje pico y se convierte a dB
    float voltajeBase = 0, voltajePico = 0;
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

    // Formateo de fecha y hora para el registro
    char fecha[11], horaTexto[6];
    sprintf(fecha, "%02d/%02d/%d", dia, mes, anio);
    sprintf(horaTexto, "%02d:%02d", hora, minuto);

    // Escritura de los datos en el archivo CSV
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

    // Impresión de los datos en el Monitor Serial
    Serial.print(">> "); Serial.print(fecha); Serial.print(" ");
    Serial.print(horaTexto); Serial.print(" - Temp: ");
    Serial.print(temp); Serial.print("°C, Hum: ");
    Serial.print(hum); Serial.print("%, Ruido: ");
    Serial.println(dB, 2);

    avanzar20Min();  // Simula el avance de 20 minutos
  }

  delay(1000);  // Espera antes de verificar nuevamente el tiempo
}

