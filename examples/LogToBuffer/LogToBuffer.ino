#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>

QueueHandle_t queueLog;

LogToQueue Log;

char datoRecibido = ' ';
char bufferDatos[100];
int indice = 0;

void setup() {
  SerialMon.begin(115200);
  delay(1000);
  SerialMon.println("\nIniciando Prueba");
  queueLog = xQueueCreate(100, sizeof(char));

  Log.begin(&SerialMon,true,queueLog);
  Log.println(F("=== Iniciando TractoSmart ==="));
  delay(500);
  Log.println(F("Aqui otro mensaje"));
  SerialMon.println("Aqui los datos recibidos en el queue:\n");
}

void loop() {
  while (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    if (datoRecibido == '\n'){          //Si se imprime una nueva linea se ejecuta o se hace algo con el buffer
      bufferDatos[indice] = '\0';       //Añadimos el caracter termino del array (para que no se imprima el resto del array)
      SerialMon.println(bufferDatos);
      indice = 0;
      continue;                         //Salimos del bucle para continuar con otros procesos
    }
    bufferDatos[indice] = datoRecibido;
    indice++;
  }
}