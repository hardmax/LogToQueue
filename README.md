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

Cuando el timestamp está habilitado, 8 bytes se reservan automáticamente para el formato de timestamp (`HH:MM:SS `), dejando 247 bytes disponibles para el contenido del mensaje.

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

## Filtrado por Tags (v1.3.0)

Desde la versión 1.3.0, LogToQueue puede filtrar mensajes por etiquetas (tags) en el puerto serial configurado. **Importante:** El filtrado solo afecta lo que se imprime en el puerto serial; todos los mensajes siguen enviándose al queue sin filtrar.

### Uso Básico

```cpp
#include <LogToQueue.h>

LogToQueue Log;

void setup() {
    Serial.begin(115200);
    Log.beginManaged(&Serial, true, 500);

    // Configurar filtro para solo mostrar [MAIN], [SERIAL] y [GPS]
    Log.setDump("MAIN,SERIAL,GPS");

    // Estos mensajes SE IMPRIMEN en serial:
    Log.println("[MAIN] Sistema iniciado");      // ✓ Pasa filtro
    Log.println("[SERIAL] Puerto abierto");      // ✓ Pasa filtro
    Log.println("[GPS] Coordenadas: 10,20");     // ✓ Pasa filtro

    // Estos mensajes NO SE IMPRIMEN en serial:
    Log.println("[SENSOR] Temperatura: 25C");    // ✗ Tag no permitido
    Log.println("[DEBUG] Valor x=10");           // ✗ Tag no permitido

    // Mensajes sin tag - siempre se imprimen:
    Log.println("Mensaje sin tag");              // ✓ Se imprime

    // IMPORTANTE: TODOS los mensajes (filtrados o no) van al queue
}

void loop() {
    char line[256];

    // Recuperar todos los mensajes del queue (sin filtrar)
    if (Log.getLine(line, sizeof(line))) {
        // Aquí recibes TODOS los mensajes, incluso los filtrados
        procesarLinea(line);
    }
}
```

### API de setDump()

#### Modo Boolean (compatible con versiones anteriores):
```cpp
Log.setDump(true);   // Habilitar todo (sin filtrado)
Log.setDump(false);  // Deshabilitar todo
```

#### Modo Filtrado por Tags (v1.3.0):
```cpp
// Filtrar por tags específicos (separados por comas)
Log.setDump("MAIN,SERIAL,GPS");

// Espacios son ignorados
Log.setDump("MAIN, SERIAL , GPS");  // Equivalente al anterior

// String vacío = sin filtrado (permitir todos)
Log.setDump("");

// NULL = sin filtrado (permitir todos)
Log.setDump(NULL);
```

### Formato de Tags

Los tags deben seguir el formato `[NOMBRE]` al inicio del mensaje:

```cpp
// Tags válidos:
Log.println("[MAIN] Mensaje");       // ✓ Tag válido
Log.println("[GPS] Coordenadas");    // ✓ Tag válido
Log.println("[SENSOR] Datos");       // ✓ Tag válido

// Sin tag (siempre se imprime según setDump):
Log.println("Mensaje sin tag");      // ✓ Se imprime si _enable=true

// Tags inválidos (se tratan como sin tag):
Log.println("MAIN Mensaje");         // Sin corchetes
Log.println("[] Mensaje");           // Tag vacío
Log.println("[ MAIN] Mensaje");      // Espacio antes del tag
```

### Cambiar Filtro Dinámicamente

```cpp
// Inicialmente permitir MAIN y GPS
Log.setDump("MAIN,GPS");
Log.println("[MAIN] Visible");       // ✓ Impreso
Log.println("[GPS] Visible");        // ✓ Impreso
Log.println("[DEBUG] No visible");   // ✗ Bloqueado

// Cambiar a solo DEBUG
Log.setDump("DEBUG");
Log.println("[MAIN] No visible");    // ✗ Bloqueado
Log.println("[DEBUG] Visible");      // ✓ Impreso

// Volver a permitir todos
Log.setDump("");  // O también: Log.setDump(true);
Log.println("[MAIN] Visible");       // ✓ Impreso
Log.println("[DEBUG] Visible");      // ✓ Impreso
```

### Comportamiento del Filtrado

| Configuración | Mensaje | ¿Se imprime en Serial? | ¿Va al Queue? |
|---|---|---|---|
| `setDump(true)` | `[MAIN] Test` | ✓ Sí | ✓ Sí |
| `setDump(false)` | `[MAIN] Test` | ✗ No | ✓ Sí |
| `setDump("MAIN")` | `[MAIN] Test` | ✓ Sí | ✓ Sí |
| `setDump("MAIN")` | `[DEBUG] Test` | ✗ No | ✓ Sí |
| `setDump("MAIN")` | `Sin tag` | ✓ Sí | ✓ Sí |

**Regla importante:** El queue recibe TODOS los mensajes sin importar el filtro. El filtro solo afecta lo que se imprime en el puerto serial configurado.

### Timestamps y Filtrado

Los timestamps se filtran correctamente junto con los mensajes:

```cpp
Log.beginManaged(&Serial, true, 500);  // Timestamp habilitado
Log.setDump("MAIN");

Log.println("[MAIN] Test");    // Serial: "14:30:15 [MAIN] Test"
Log.println("[DEBUG] Test");   // Serial: (nada, filtrado con timestamp)
```

**No hay timestamps huérfanos** - el timestamp solo aparece si el mensaje pasa el filtro.

### Límites y Consideraciones

- **Máximo de tags:** 10 tags simultáneos (configurable internamente)
- **Longitud máxima de tag:** 20 caracteres
- **Memoria adicional:** ~6 bytes en objeto + ~120 bytes heap si se usan tags
- **Thread-safety:** `setDump()` debe llamarse desde `setup()` o desde una sola tarea

### Ejemplo Completo

Ver `examples/TagFiltering/TagFiltering.ino` para un ejemplo completo que demuestra:
- Filtrado por múltiples tags
- Cambio dinámico de filtros
- Verificación de que todos los mensajes van al queue
- Uso práctico en sistemas con múltiples subsistemas

## Sincronización del RTC (v1.4.0)

Desde la versión 1.4.0, LogToQueue usa el RTC interno del ESP32 en lugar de `millis()`. Esto permite mostrar fecha y hora real cuando el RTC está sincronizado.

### Uso Básico

```cpp
#include <LogToQueue.h>

LogToQueue Log;

void setup() {
    Serial.begin(115200);

    // RTC NO sincronizado: muestra epoch (00:00:00)
    Log.beginManaged(&Serial, true, 500);
    Log.println("Sistema iniciando");  // "00:00:00 Sistema iniciando"
}
```

### Sincronización con NTP

Para mostrar fecha/hora real, sincroniza el RTC con NTP:

```cpp
#include <LogToQueue.h>
#include <WiFi.h>
#include <time.h>

LogToQueue Log;

void setup() {
    Serial.begin(115200);

    // Conectar WiFi
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");

    // Configurar NTP para sincronizar RTC
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Opcional: Configurar zona horaria (UTC-5 para Perú)
    setenv("TZ", "CST5", 1);
    tzset();

    // Esperar sincronización
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.println("RTC sincronizado con NTP");
    } else {
        Serial.println("Error sincronizando RTC");
    }

    // Inicializar LogToQueue (ahora con hora real)
    Log.beginManaged(&Serial, true, 500);
    Log.println("Sistema iniciado");  // "14:30:15 Sistema iniciado"
}
```

### Formato de Timestamp

El timestamp usa el formato `HH:MM:SS ` (8 caracteres):

```
14:30:15 [MAIN] Sistema iniciado
14:30:16 [SENSOR] Temperatura: 25C
14:31:45 [GPS] Coordenadas actualizadas
```

**Nota:** Si el RTC no está sincronizado, mostrará `00:00:00 ` y se incrementará desde el boot (como época Unix 1970-01-01).

### Zonas Horarias Comunes

```cpp
// UTC (sin ajuste)
setenv("TZ", "UTC0", 1);

// Perú (UTC-5)
setenv("TZ", "CST5", 1);

// México (UTC-6)
setenv("TZ", "CST6", 1);

// Argentina (UTC-3)
setenv("TZ", "WART3", 1);

// España (UTC+1)
setenv("TZ", "CET-1", 1);

// Aplicar cambio
tzset();
```

### Ventajas del RTC vs millis()

| Característica | millis() (v1.3.0) | RTC (v1.4.0) |
|---|---|---|
| Formato | HH:MM:SS.mmm (13 chars) | HH:MM:SS (8 chars) |
| Precisión | Milisegundos | Segundos |
| Fecha/hora real | ❌ No | ✅ Sí (con sincronización) |
| Buffer disponible | 242 bytes | 247 bytes (+5) |
| Reinicia en boot | ✅ Sí | ❌ No (si sincronizado) |