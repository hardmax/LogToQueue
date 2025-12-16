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

## Thread-Safety (v1.1.0+)

Desde la versión 1.1.0, LogToQueue es **thread-safe** y puede ser llamado de forma segura desde múltiples tareas de FreeRTOS concurrentemente. Las operaciones internas están protegidas por un mutex.

**Ejemplo de uso multi-tarea:**
```cpp
LogToQueue Log;

void taskA(void *param) {
    while (1) {
        Log.println("Mensaje desde Task A");
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void taskB(void *param) {
    while (1) {
        Log.println("Mensaje desde Task B");
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);
    queueLog = xQueueCreate(500, sizeof(char));
    Log.begin(&Serial, true, queueLog);

    xTaskCreate(taskA, "TaskA", 2048, NULL, 1, NULL);
    xTaskCreate(taskB, "TaskB", 2048, NULL, 1, NULL);
}
```

## Límite de Buffer

**Tamaño máximo del buffer:** 255 bytes

Cuando el timestamp está habilitado, 13 bytes se reservan automáticamente para el formato de timestamp (`HH:MM:SS.mmm `), dejando 243 bytes disponibles para el contenido del mensaje.

Si intentas configurar un buffer mayor a 255 bytes, se limitará automáticamente al máximo permitido.

## Comportamiento de Queue Lleno

Cuando el queue de FreeRTOS está lleno, LogToQueue implementa un **comportamiento circular automático**:

- Se elimina el carácter más antiguo del queue
- Se inserta el nuevo carácter en su lugar
- Esto asegura que los mensajes nuevos nunca se pierden

Este comportamiento garantiza que siempre tendrás los datos más recientes disponibles, incluso cuando el queue alcanza su capacidad máxima.

**Recomendación:** Dimensiona tu queue apropiadamente según la tasa de generación de logs en tu aplicación.

## Optimizaciones de Memoria (v1.1.0+)

La versión 1.1.0 incluye optimizaciones significativas:

- Reducción del tamaño del objeto en 2 bytes (eliminación de puntero redundante)
- Optimización del buffer de timestamp (ahorro de 7 bytes en stack por llamada)
- Corrección de memory leak en el destructor
- Verificación mejorada de asignación de memoria

**Antes vs Después:**
- Tamaño de objeto: 22 bytes → 20 bytes
- Buffer timestamp: 20 bytes → 14 bytes
- Heap adicional: +80 bytes (mutex para thread-safety)

## Notas Importantes

- Esta biblioteca requiere FreeRTOS y solo es compatible con ESP32
- El mutex se crea automáticamente en `begin()` y se destruye en el destructor
- Las operaciones de queue son thread-safe por diseño de FreeRTOS
- El buffer interno está protegido por mutex para uso multi-tarea seguro

## Modo Managed Queue (v1.2.0+)

Desde la versión 1.2.0, LogToQueue puede gestionar su propio queue interno, simplificando el código y proporcionando una API más conveniente.

### Uso Básico - Modo Managed

```cpp
#include <LogToQueue.h>

LogToQueue Log;

void setup() {
    Serial.begin(115200);

    // La librería crea y gestiona el queue internamente
    Log.beginManaged(&Serial, true, 500);  // 500 caracteres de queue

    Log.println("=== Sistema Iniciado ===");
}

void loop() {
    char line[256];

    // Recuperar líneas completas fácilmente
    if (Log.getLine(line, sizeof(line))) {
        // Procesar línea completa
        Serial.print("Log: ");
        Serial.println(line);
    }

    delay(100);
}
```

### API de getLine()

La API `getLine()` simplifica la recuperación de líneas completas del log:

**Firma:**
```cpp
bool getLine(char* buffer, size_t maxLen, TickType_t timeout = 0)
```

**Parámetros:**
- `buffer`: Buffer donde se almacenará la línea
- `maxLen`: Tamaño máximo del buffer
- `timeout`: Timeout en ticks (0 = non-blocking, por defecto)

**Retorna:**
- `true`: Si se recuperó al menos un carácter
- `false`: Si el queue está vacío o hubo error

**Características:**
- **Non-blocking por defecto**: Retorna inmediatamente si no hay datos
- **Timeout configurable**: Espera hasta que llegue una línea completa
- **Líneas parciales**: Retorna datos aunque no encuentre `\n`
- **Thread-safe**: Múltiples escritores, un lector

### Comparación de Modos

| Característica | Modo Legacy | Modo Managed |
|---|---|---|
| **Creación del queue** | Usuario con `xQueueCreate()` | Automática con `beginManaged()` |
| **Gestión de memoria** | Usuario libera con `vQueueDelete()` | Automática en destructor |
| **Recuperación de datos** | Manual con `xQueueReceive()` | Simple con `getLine()` |
| **Líneas de código** | ~15 líneas | ~5 líneas |
| **Compatibilidad** | ✅ Totalmente compatible | ✅ Nueva funcionalidad |

### Ejemplo Multi-Tarea con Modo Managed

```cpp
#include <LogToQueue.h>

LogToQueue Log;

// Tarea productora - genera logs
void taskProducer(void *param) {
    while (1) {
        Log.printf("Sensor: %d\n", analogRead(A0));
        Log.println("Estado: OK");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Tarea consumidora - procesa logs
void taskConsumer(void *param) {
    char line[256];

    while (1) {
        // Monitorear ocupación del queue
        UBaseType_t waiting = Log.getQueueMessagesWaiting();
        if (waiting > 400) {  // 80% de 500
            Serial.println("[WARN] Queue casi lleno!");
        }

        // Procesar líneas con timeout de 100ms
        while (Log.getLine(line, sizeof(line), 100 / portTICK_PERIOD_MS)) {
            // Guardar en SD, enviar por MQTT, etc.
            Serial.println(line);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup() {
    Serial.begin(115200);

    // Crear queue de 500 caracteres
    Log.beginManaged(&Serial, true, 500);

    // Crear tareas producer/consumer
    xTaskCreate(taskProducer, "Producer", 2048, NULL, 1, NULL);
    xTaskCreate(taskConsumer, "Consumer", 4096, NULL, 1, NULL);
}

void loop() {
    // Tareas manejan todo
}
```

### Métodos Adicionales (v1.2.0)

**Monitoreo del Queue:**
```cpp
UBaseType_t waiting = Log.getQueueMessagesWaiting();
// Retorna cantidad de caracteres esperando en el queue
```

**Verificar Modo:**
```cpp
bool managed = Log.isQueueManaged();
// Retorna true si el queue es gestionado por la librería
```

**Obtener Tamaño:**
```cpp
uint16_t size = Log.getQueueSize();
// Retorna el tamaño del queue managed (0 si es externo)
```

### Migración de Modo Legacy a Managed

**Antes (v1.1.0):**
```cpp
QueueHandle_t queueLog;

void setup() {
    Serial.begin(115200);
    queueLog = xQueueCreate(500, sizeof(char));
    Log.begin(&Serial, true, queueLog);
}

void loop() {
    char ch;
    char line[256];
    int idx = 0;

    while (xQueueReceive(queueLog, &ch, 0) == pdTRUE) {
        if (ch == '\n') {
            line[idx] = '\0';
            procesarLinea(line);
            idx = 0;
        } else if (idx < 255) {
            line[idx++] = ch;
        }
    }
}
```

**Después (v1.2.0):**
```cpp
void setup() {
    Serial.begin(115200);
    Log.beginManaged(&Serial, true, 500);
}

void loop() {
    char line[256];

    if (Log.getLine(line, sizeof(line))) {
        procesarLinea(line);
    }
}
```

**Resultado**: -8 líneas de código, gestión automática, código más limpio.