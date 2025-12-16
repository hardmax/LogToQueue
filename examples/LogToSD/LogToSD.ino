/*
 * LogToSD Example
 *
 * Ejemplo que muestra cómo usar LogToQueue en modo managed
 * para guardar logs en una tarjeta SD.
 *
 * Este es un caso de uso muy común: guardar todos los logs
 * del sistema en un archivo para análisis posterior.
 *
 * Hardware requerido:
 * - ESP32
 * - Módulo SD card (SPI)
 */

#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>
#include <SD.h>
#include <SPI.h>

// Configuración pines SD Card (ajusta según tu hardware)
#define SD_CS_PIN 5

LogToQueue Log;
File logFile;
bool sdAvailable = false;

void setup() {
    SerialMon.begin(115200);
    delay(1000);

    SerialMon.println("\n============================");
    SerialMon.println("LogToQueue -> SD Card");
    SerialMon.println("============================\n");

    // Inicializar SD card
    if (!SD.begin(SD_CS_PIN)) {
        SerialMon.println("[ERROR] SD Card no encontrada!");
        sdAvailable = false;
    } else {
        SerialMon.println("[OK] SD Card inicializada");
        sdAvailable = true;

        // Mostrar info de la SD
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        SerialMon.printf("Tamaño SD: %llu MB\n", cardSize);
        SerialMon.printf("Espacio usado: %llu MB\n", SD.usedBytes() / (1024 * 1024));
        SerialMon.println();
    }

    // Inicializar LogToQueue en modo managed
    // Queue de 1000 caracteres para buffer robusto
    Log.beginManaged(&SerialMon, true, 1000);

    SerialMon.println("LogToQueue iniciado en modo managed");
    SerialMon.printf("Queue size: %d caracteres\n\n", Log.getQueueSize());

    // Logs iniciales
    Log.println(F("=== Sistema de Logging a SD Iniciado ==="));
    Log.printf("Fecha: %s %s\n", __DATE__, __TIME__);
    Log.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
    Log.printf("SD disponible: %s\n", sdAvailable ? "SI" : "NO");

    if (sdAvailable) {
        // Abrir archivo de log en modo append
        logFile = SD.open("/datalog.txt", FILE_APPEND);
        if (logFile) {
            SerialMon.println("Archivo datalog.txt abierto para escritura\n");
            logFile.println("\n\n=== Nueva sesión iniciada ===");
            logFile.flush();
        } else {
            SerialMon.println("[ERROR] No se pudo abrir datalog.txt\n");
            sdAvailable = false;
        }
    }
}

void loop() {
    // PASO 1: Generar logs del sistema
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 2000) {
        lastLog = millis();

        // Simular lecturas de sensores
        float temperatura = 20.0 + random(0, 100) / 10.0;
        int humedad = random(30, 70);
        int luz = random(0, 1024);

        // Escribir logs
        Log.printf("Sensor: T=%.1f°C H=%d%% L=%d\n",
                   temperatura, humedad, luz);

        // Log adicional cada 10 segundos
        if (random(100) < 20) {
            Log.println(F("Status: Sistema operando normalmente"));
        }
    }

    // PASO 2: Recuperar líneas del log y guardar en SD
    char line[256];
    int linesWritten = 0;

    while (Log.getLine(line, sizeof(line))) {
        // Guardar en SD si está disponible
        if (sdAvailable && logFile) {
            logFile.println(line);
            linesWritten++;

            // Hacer flush cada 5 líneas para asegurar escritura
            if (linesWritten % 5 == 0) {
                logFile.flush();
            }
        }

        // También mostrar en Serial
        SerialMon.print("[SD] ");
        SerialMon.println(line);
    }

    // PASO 3: Monitoreo periódico
    static unsigned long lastMonitor = 0;
    if (millis() - lastMonitor > 10000) {
        lastMonitor = millis();

        // Estadísticas del queue
        UBaseType_t waiting = Log.getQueueMessagesWaiting();
        SerialMon.printf("\n[STATS] Queue ocupación: %d/%d caracteres\n",
                        waiting, Log.getQueueSize());

        // Estadísticas de la SD
        if (sdAvailable) {
            SerialMon.printf("[STATS] SD espacio usado: %llu MB\n",
                           SD.usedBytes() / (1024 * 1024));

            // Cerrar y reabrir archivo periódicamente para asegurar integridad
            if (logFile) {
                logFile.close();
            }
            logFile = SD.open("/datalog.txt", FILE_APPEND);
            if (!logFile) {
                SerialMon.println("[ERROR] No se pudo reabrir archivo!");
                sdAvailable = false;
            }
        }

        SerialMon.println();

        // Log de status
        Log.println(F("--- Checkpoint: Sistema activo ---"));
    }

    delay(10);
}
