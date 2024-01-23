# LogToQueue
Biblioteca que envía el log a un puerto serial y a un Queue para poder ser redirigido o enviado a cualquier medio, como MQTT, otro puerto Serial, a un SD, etc. Se deriva de la clase Stream, por lo que se pueden usar print o println como en cualquier otro Stream.

## Informacion General

Por defecto esta biblioteca viene configurada con un buffer de 256 bytes, que al llenarse agrega una linea de retorno automatico para evitar el desborde y que no haga fragmentacion de memoria innecesario.

Por utilizar Queue solo es compatible con sistemas que usen FREERTOS como el ESP32.

El tamaño del QUEUE lo defines segun tus necesidades y almacena solo 1 caracter del Log, así que debe de ser largo para almacenar algunas lineas de Log antes de su proceso.

## Ejemplo
Para usar la biblioteca tienes que incluirla en tu proyecto, crear un queue para manejar las respuestas y crear una instancia del LogToQueue que en este caso la hemos llamado Log:
```cpp
#include <LogToQueue.h>
QueueHandle_t queueLog;
LogToQueue Log;
```

Creamos una cola donde recibir el Log  con el suficiente espacio para recibir 100 caracteres. Ajustar la cantidad de caracteres a recibir segun tu proyecto.

```cpp
queueLog = xQueueCreate(100, sizeof(char));
```

Inicializamos la biblioteca indicando el puerto serie donde se enviará el log (Como Referencia), si queremos que se visualice un timestamp y la cola en donde queremos pasar el log, la cola tambien se pasa como referencia, pero por la forma en como trabaja, no se necesita poner & antes.
```cpp
Log.begin(&Serial,true,queueLog);
```

Ya comenzar a usar el Log solo se tiene que usar los metodos conocidos de cualquier Stream como print o println.

```cpp
Log.println(F("=== Iniciando TractoSmart ==="));
```

Puede activar o desactivar el volcado del log al puerto serial a voluntad usando:
```cpp
Log.setDump(true);
```
Para recibir los datos en el Queue:
```cpp
 if (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    SerialMon.print(datoRecibido);      //aqui hacemos lo que querramos con el dato recibido
 }
```
Tambien podemos almacenar todo lo enviado hasta encontrar un retorno de carro y guardarlo en un SD o cualquier cosa que necesitemos
```cpp
while (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    if (datoRecibido == '\n'){          //Si se imprime una nueva linea se ejecuta o se hace algo con el buffer
      bufferDatos[indice] = '\0';       //Añadimos el caracter termino del array (para que no se imprima el resto del array)
      SerialMon.println(bufferDatos);   //Aqui hacemos lo que queramos con el bufferDatos q contiene toda la lniea enviada
      indice = 0;
      continue;                         //Salimos del bucle para continuar con otros procesos
    }
    bufferDatos[indice] = datoRecibido;
    indice++;
  }
```

Mas informacion en la carpeta ejemplos.