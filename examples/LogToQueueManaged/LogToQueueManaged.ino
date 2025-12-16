/*
 * LogToQueueManaged Example
 *
 * Demonstrates managed queue mode (v1.2.0+) where the library
 * creates and manages its own queue internally, and provides
 * a simple getLine() API to retrieve complete log lines.
 *
 * Features demonstrated:
 * - beginManaged() - automatic queue creation
 * - getLine() - simple line retrieval
 * - Queue monitoring with getQueueMessagesWaiting()
 * - Multi-task logging (producer/consumer pattern)
 */

#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>

LogToQueue Log;

// Simulated sensor counter
volatile int sensorValue = 0;

// Producer task - generates log messages
void taskProducer(void *param) {
    int counter = 0;

    while (1) {
        // Generate various log messages
        Log.printf("Mensaje %d: Sensor=%d\n", counter++, sensorValue++);

        if (counter % 5 == 0) {
            Log.println(F("=== Checkpoint cada 5 mensajes ==="));
        }

        if (counter % 10 == 0) {
            Log.printf("Estado del sistema: %s\n", "OK");
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

// Consumer task - processes log lines
void taskConsumer(void *param) {
    char line[256];
    int linesProcessed = 0;

    while (1) {
        // Monitor queue occupancy
        UBaseType_t waiting = Log.getQueueMessagesWaiting();
        uint16_t queueSize = Log.getQueueSize();

        if (waiting > queueSize * 0.8) {
            SerialMon.printf("[WARN] Queue %d%% lleno (%d/%d)\n",
                           (int)(waiting * 100 / queueSize),
                           waiting, queueSize);
        }

        // Process lines with 100ms timeout
        while (Log.getLine(line, sizeof(line), 100 / portTICK_PERIOD_MS)) {
            // Here you could:
            // - Save to SD card
            // - Send via MQTT
            // - Store in database
            // - Send to cloud

            // For this example, just count and display
            linesProcessed++;

            SerialMon.print("[");
            SerialMon.print(linesProcessed);
            SerialMon.print("] ");
            SerialMon.println(line);
        }

        // Display statistics every 10 seconds
        static unsigned long lastStats = 0;
        if (millis() - lastStats > 10000) {
            lastStats = millis();
            SerialMon.printf("\n[STATS] Lineas procesadas: %d, Queue: %d/%d\n\n",
                           linesProcessed, waiting, queueSize);
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup() {
    SerialMon.begin(115200);
    delay(1000);

    SerialMon.println("\n\n=================================");
    SerialMon.println("LogToQueue Managed Mode Example");
    SerialMon.println("=================================\n");

    // Initialize with managed queue (500 characters)
    // The library creates and manages the queue internally
    Log.beginManaged(&SerialMon, true, 500);

    SerialMon.print("Queue managed: ");
    SerialMon.println(Log.isQueueManaged() ? "SI" : "NO");
    SerialMon.print("Queue size: ");
    SerialMon.println(Log.getQueueSize());
    SerialMon.println();

    // Log initial message
    Log.println(F("=== Sistema Iniciado ==="));
    Log.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());

    // Create producer and consumer tasks
    xTaskCreate(taskProducer, "Producer", 2048, NULL, 1, NULL);
    xTaskCreate(taskConsumer, "Consumer", 4096, NULL, 1, NULL);

    SerialMon.println("Tareas creadas. Logging iniciado...\n");
}

void loop() {
    // Tasks handle everything
    // Main loop can do other work or just idle
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
