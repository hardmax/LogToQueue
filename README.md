# LogToQueue

Librería para ESP32 que envía cada mensaje de log simultáneamente al puerto serial y a un queue de FreeRTOS. Desde el queue podés redirigir los logs a SD, MQTT, otro puerto serial, o cualquier destino que necesites.

Extiende la clase `Print` de Arduino, por lo que usás `print()`, `println()` y `printf()` igual que siempre.

## Instalación

Buscá **LogToQueue** en el Library Manager de Arduino IDE o PlatformIO y hacé clic en instalar.

O en `platformio.ini`:
```ini
lib_deps = hardmax/LogToQueue
```

## Compatibilidad

- **Hardware:** ESP32 (requiere FreeRTOS)
- **Framework:** Arduino
- **IDE:** Arduino IDE / PlatformIO

---

## Uso rápido

### Modo managed (recomendado)

La librería crea y gestiona el queue internamente. Más simple, menos código.

```cpp
#include <LogToQueue.h>

LogToQueue Log;

void setup() {
    Serial.begin(115200);
    Log.begin(&Serial, true, 500);  // Puerto, timestamp, tamaño del queue

    Log.println("[MAIN] Sistema iniciado");
    Log.printf("[SENSOR] Temperatura: %.1f°C\n", 25.3);
}

void loop() {
    char linea[256];

    if (Log.getLine(linea, sizeof(linea))) {
        // Aquí procesás la línea: guardar en SD, enviar por MQTT, etc.
        Serial.print("Procesado: ");
        Serial.println(linea);
    }
}
```

### Modo externo (legacy)

Vos creás y manejás el queue. Compatible con código anterior a v1.2.0.

```cpp
#include <LogToQueue.h>

QueueHandle_t queueLog;
LogToQueue Log;

void setup() {
    Serial.begin(115200);
    queueLog = xQueueCreate(500, sizeof(char));
    Log.begin(&Serial, true, queueLog);

    Log.println("[MAIN] Sistema iniciado");
}

void loop() {
    char ch;
    char linea[256];
    int idx = 0;

    while (xQueueReceive(queueLog, &ch, 0) == pdTRUE) {
        if (ch == '\n') {
            linea[idx] = '\0';
            // Procesás la línea completa
            idx = 0;
        } else if (idx < 255) {
            linea[idx++] = ch;
        }
    }
}
```

---

## Funcionalidades

### Timestamp automático

Habilitado con el segundo parámetro de `begin()`. Usa el RTC interno del ESP32:

```
14:30:15 [MAIN] Sistema iniciado
14:30:16 [SENSOR] Temperatura: 25.3°C
```

Sin sincronización muestra `00:00:00` incrementando desde el boot. Para hora real, sincronizá con NTP antes de llamar a `begin()`:

```cpp
#include <WiFi.h>
#include <time.h>

WiFi.begin("SSID", "PASSWORD");
while (WiFi.status() != WL_CONNECTED) delay(500);

configTime(0, 0, "pool.ntp.org");
setenv("TZ", "CST5", 1);  // UTC-5 Perú. Ajustá según tu zona horaria.
tzset();

Log.begin(&Serial, true, 500);  // Ahora muestra hora real
```

Zonas horarias comunes:

| Zona | Código |
|---|---|
| UTC | `UTC0` |
| Perú / Ecuador / Colombia (UTC-5) | `CST5` |
| México (UTC-6) | `CST6` |
| Argentina (UTC-3) | `WART3` |
| España (UTC+1) | `CET-1` |

### Filtrado por tags

Controlá qué mensajes se imprimen en serial sin afectar el queue. El queue recibe **todos** los mensajes siempre.

```cpp
// Solo imprimir en serial los tags MAIN y GPS
Log.setDump("MAIN,GPS");

Log.println("[MAIN] Visible en serial");    // ✓ Se imprime
Log.println("[GPS] Visible en serial");     // ✓ Se imprime
Log.println("[DEBUG] No en serial");        // ✗ Filtrado (pero sí va al queue)
Log.println("Sin tag, siempre visible");    // ✓ Se imprime

// Volver a mostrar todo
Log.setDump(true);

// Desactivar serial por completo
Log.setDump(false);
```

El tag debe estar al inicio del mensaje con formato `[NOMBRE]`:

```cpp
Log.println("[MAIN] correcto");     // ✓ Tag reconocido
Log.println("MAIN mensaje");        // ✗ Sin corchetes, se trata como sin tag
```

### Thread-safety

La librería usa un mutex interno. Podés llamar a `Log` desde múltiples tareas FreeRTOS de forma segura:

```cpp
void taskA(void *param) {
    while (1) {
        Log.println("[SENSOR] Leyendo sensor");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void taskB(void *param) {
    while (1) {
        Log.println("[RED] Enviando datos");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void setup() {
    Serial.begin(115200);
    Log.begin(&Serial, true, 500);
    xTaskCreate(taskA, "Sensor", 2048, NULL, 1, NULL);
    xTaskCreate(taskB, "Red", 2048, NULL, 1, NULL);
}
```

### Queue circular

Cuando el queue se llena, el carácter más antiguo se descarta automáticamente para dar lugar al nuevo. Los mensajes más recientes nunca se pierden.

---

## API

```cpp
// Inicialización
void begin(Print* output, bool showTimestamp, uint16_t queueSize);   // Modo managed
void begin(Print* output, bool showTimestamp, QueueHandle_t queue);  // Modo externo

// Lectura (modo managed)
bool getLine(char* buffer, size_t maxLen, TickType_t timeout = 0);

// Control de serial
void setDump(bool enable);        // Activar/desactivar serial
void setDump(const char* tags);   // Filtrar por tags ("TAG1,TAG2")

// Buffer
uint8_t getBufferSize();
bool setBufferSize(uint8_t size); // Máximo: 255 bytes

// Estado del queue
UBaseType_t getQueueMessagesWaiting() const;
bool isQueueManaged() const;
uint16_t getQueueSize() const;
```

---

## Ejemplos

Ver la carpeta `examples/` para ejemplos completos:

- `LogToQueue/` — Uso básico con queue externo, lectura carácter a carácter
- `LogToBuffer/` — Lectura por líneas completas con buffer
