/*
 * TagFiltering Example
 *
 * Demonstrates the tag filtering feature (v1.3.0) of LogToQueue.
 * Shows how to filter log messages by tags in the serial output,
 * while all messages still go to the queue.
 *
 * Features demonstrated:
 * - setDump(const char* tags) - Filter by comma-separated tags
 * - Tag-based message filtering with [TAG] format
 * - Dynamic tag configuration
 * - Messages without tags (always printed)
 * - All messages still available in queue via getLine()
 */

#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>

LogToQueue Log;

// Simulated system values
int sensorTemp = 25;
int sensorHumidity = 50;
int gpsLat = 0;
int gpsLon = 0;
int serialData = 0;

void setup() {
    SerialMon.begin(115200);
    delay(1000);

    SerialMon.println("\n========================================");
    SerialMon.println("LogToQueue - Tag Filtering Example");
    SerialMon.println("========================================\n");

    // Initialize in managed mode with 1000 character queue
    Log.begin(&SerialMon, true, 1000);

    SerialMon.print("Modo managed activo: ");
    SerialMon.println(Log.isQueueManaged() ? "SI" : "NO");
    SerialMon.print("Tamaño del queue: ");
    SerialMon.print(Log.getQueueSize());
    SerialMon.println(" caracteres\n");

    // ===========================================
    // PARTE 1: Sin filtrado (comportamiento por defecto)
    // ===========================================
    SerialMon.println("=== PARTE 1: Sin filtrado (todos los tags) ===\n");

    // Escribir logs con diferentes tags
    Log.println(F("[MAIN] Sistema iniciado"));
    Log.println(F("[SERIAL] Puerto serie configurado"));
    Log.println(F("[GPS] GPS inicializando"));
    Log.println(F("[SENSOR] Sensores calibrados"));
    Log.println(F("[DEBUG] Valores por defecto cargados"));
    Log.println(F("Mensaje sin tag - siempre visible"));

    delay(2000);

    // ===========================================
    // PARTE 2: Filtrar solo MAIN, SERIAL y GPS
    // ===========================================
    SerialMon.println("\n\n=== PARTE 2: Filtrar solo [MAIN], [SERIAL], [GPS] ===");
    SerialMon.println("Ejecutando: Log.setDump(\"MAIN,SERIAL,GPS\");\n");

    Log.setDump("MAIN,SERIAL,GPS");

    // Estos mensajes se imprimen en serial (tags permitidos)
    Log.println(F("[MAIN] Configuración completada"));
    Log.println(F("[SERIAL] Datos recibidos correctamente"));
    Log.println(F("[GPS] Posición adquirida"));

    // Estos NO se imprimen en serial (tags bloqueados)
    Log.println(F("[SENSOR] Temperatura: 25°C"));
    Log.println(F("[DEBUG] Variable x = 123"));

    // Sin tag - se imprime (según configuración actual)
    Log.println(F("Mensaje sin tag - siempre visible"));

    delay(2000);

    // ===========================================
    // PARTE 3: Cambiar filtro a solo DEBUG
    // ===========================================
    SerialMon.println("\n\n=== PARTE 3: Cambiar filtro a solo [DEBUG] ===");
    SerialMon.println("Ejecutando: Log.setDump(\"DEBUG\");\n");

    Log.setDump("DEBUG");

    Log.println(F("[MAIN] Este mensaje NO aparece"));
    Log.println(F("[DEBUG] Este mensaje SI aparece"));
    Log.println(F("[SENSOR] Este mensaje NO aparece"));
    Log.println(F("[DEBUG] Valor de prueba: 456"));
    Log.println(F("Mensaje sin tag - siempre visible"));

    delay(2000);

    // ===========================================
    // PARTE 4: Desactivar filtrado (permitir todos)
    // ===========================================
    SerialMon.println("\n\n=== PARTE 4: Desactivar filtrado (todos permitidos) ===");
    SerialMon.println("Ejecutando: Log.setDump(\"\"); // String vacío\n");

    Log.setDump("");  // String vacío = sin filtrado

    Log.println(F("[MAIN] Ahora todos los tags son visibles"));
    Log.println(F("[SERIAL] Este mensaje se ve"));
    Log.println(F("[GPS] Este mensaje también"));
    Log.println(F("[SENSOR] Y este también"));
    Log.println(F("[DEBUG] Todos los mensajes visibles"));

    delay(2000);

    // ===========================================
    // PARTE 5: Verificar queue (todos los mensajes están ahí)
    // ===========================================
    SerialMon.println("\n\n=== PARTE 5: Verificar Queue ===");
    SerialMon.println("IMPORTANTE: Todos los mensajes están en el queue,");
    SerialMon.println("independientemente del filtro de tags.\n");

    UBaseType_t waiting = Log.getQueueMessagesWaiting();
    SerialMon.printf("Caracteres en queue: %d/%d\n", waiting, Log.getQueueSize());
    SerialMon.println("Recuperando primeras 10 líneas del queue:\n");

    char line[256];
    for (int i = 0; i < 10 && Log.getLine(line, sizeof(line)); i++) {
        SerialMon.print("Queue[");
        SerialMon.print(i);
        SerialMon.print("]: ");
        SerialMon.println(line);
    }

    delay(2000);

    // ===========================================
    // PARTE 6: Preparar demostración continua
    // ===========================================
    SerialMon.println("\n\n=== PARTE 6: Demostración continua ===");
    SerialMon.println("Filtrando solo [MAIN] y [SENSOR]");
    SerialMon.println("Ejecutando: Log.setDump(\"MAIN,SENSOR\");\n");

    Log.setDump("MAIN,SENSOR");

    SerialMon.println("Entrando en loop() - generando logs cada 2 segundos...\n");
}

void loop() {
    static unsigned long lastLog = 0;

    if (millis() - lastLog > 2000) {
        lastLog = millis();

        // Simular cambios en sensores
        sensorTemp = 20 + random(0, 15);
        sensorHumidity = 40 + random(0, 30);
        gpsLat += random(-5, 5);
        gpsLon += random(-5, 5);
        serialData += 10;

        // Generar logs con diferentes tags
        // Solo [MAIN] y [SENSOR] se imprimen según filtro actual
        Log.printf("[MAIN] Uptime: %lu segundos\n", millis() / 1000);
        Log.printf("[SENSOR] Temp=%d°C Hum=%d%%\n", sensorTemp, sensorHumidity);
        Log.printf("[GPS] Posición: %d,%d\n", gpsLat, gpsLon);
        Log.printf("[SERIAL] Datos recibidos: %d bytes\n", serialData);
        Log.printf("[DEBUG] Heap libre: %d bytes\n", ESP.getFreeHeap());

        // Mensaje sin tag cada 3 iteraciones
        static int counter = 0;
        counter++;
        if (counter % 3 == 0) {
            Log.println(F("--- Sistema operando normalmente ---"));
        }
    }

    // Procesar líneas del queue (opcional)
    // Descomentar para ver todas las líneas que pasan por el queue
    /*
    char line[256];
    while (Log.getLine(line, sizeof(line))) {
        SerialMon.print("[QUEUE] ");
        SerialMon.println(line);
    }
    */

    // Mostrar estadísticas cada 10 segundos
    static unsigned long lastStats = 0;
    if (millis() - lastStats > 10000) {
        lastStats = millis();

        UBaseType_t waiting = Log.getQueueMessagesWaiting();
        SerialMon.printf("\n[STATS] Queue ocupación: %d/%d (%d%%)\n\n",
                        waiting, Log.getQueueSize(),
                        (int)(waiting * 100 / Log.getQueueSize()));
    }

    delay(10);
}
