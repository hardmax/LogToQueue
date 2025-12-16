#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>

QueueHandle_t queueLog;

LogToQueue Log;

char datoRecibido;

void setup() {
  SerialMon.begin(9600);
  delay(3000);
  SerialMon.println("\nIniciando Prueba");
  queueLog = xQueueCreate(100, sizeof(char));

  Log.begin(&SerialMon,true,queueLog);
  Log.println(F("=== Iniciando TractoSmart ==="));
  delay(500);
  Log.println(F("Aqui otro mensaje"));
  SerialMon.println("Aqui los datos recibidos en el queue:\n");
}

void loop() {
  if (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    SerialMon.print((char)datoRecibido);
    //SerialMon.println("=======");
  }
}