# LogToQueue
Libreria que envia el log a un puerto serial y a un Queue para poder ser redirigido o enviado a cualquier medio, como MQTT, Otro pueto Serial, a un SD, etc. Se deriba de la clase Stream, por lo que se pueden usar print o println como en cualquier otro Stream

## Informacion General

Por defecto esta libreria biene configurada con un buffer de 256, que al llenarse agrega una linea de retorno automatico para evitar el desborde y que no haga fragmentacion de memoria inecesario.

Por utilizar Queue solo es compatible con sistemas que usen FREERTOS como el ESP32 de Expressive.

El tañano del QUEUE lo defines segun tus necesidades y almacena solo 1 solo caracter del Log, asi que debe de ser largo para alamcenar algunas lineas de Log antes de su proceso.

## Ejemplo
Para usar la libreria tienes que incluirla en tu proyecto, Crear un queue para manejar las respuesta y crear una instancia del LogToQue que en este caso la hemos llamado Log

```cpp
#include <LogToQueue.h>
QueueHandle_t queueLog;
LogToQueue Log;
```

Creamos el Log con un tamño de variable a enviar de 1 Char y con el suficiente espacio para recibir 100 caracteres. Ajustar la cantidad de caracteres a recibir segun tu proyecto.

```cpp
queueLog = xQueueCreate(100, sizeof(char));
```
Inicializamos la libreira indicando el puerto serie donde se enviara el log (Como Referencia), si queremos que se visualice un timestamp y la cola en donde queremos pasar el log, la cola tambien se pasa como referencia, pero por la forma en como trabaja, no se necesita poner & antes.

```cpp
Log.begin(&Serial,true,queueLog);
```

Ya comenzar a usar el Log solo se tiene que usar los metodos conocidos de cualquier Stream como print o  println.

```cpp
Log.println(F("=== Iniciando TractoSmart ==="));
```
Puede activar o desactivar el volcado del log al puerto serial a voluntad usuando
```cpp
Log.setDump(true);
```
Mas informacion en la carpeta ejemplos.