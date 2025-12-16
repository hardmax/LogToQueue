/*
 * SimpleManaged Example
 *
 * Ejemplo básico del modo Managed Queue (v1.2.0+)
 * Muestra el uso más simple de beginManaged() y getLine()
 * sin complejidad de multi-tarea.
 *
 * Este ejemplo es perfecto para empezar a usar LogToQueue
 * en modo managed.
 */

#define SerialMon Serial

#include <Arduino.h>
#include <LogToQueue.h>

LogToQueue Log;

void setup() {
    SerialMon.begin(115200);
    delay(1000);

    SerialMon.println("\n========================================");
    SerialMon.println("LogToQueue - Ejemplo Simple Managed Mode");
    SerialMon.println("========================================\n");

    // Inicializar en modo managed
    // La librería crea y gestiona el queue automáticamente
    Log.beginManaged(&SerialMon, true, 500);  // 500 caracteres de queue

    SerialMon.print("Modo managed activo: ");
    SerialMon.println(Log.isQueueManaged() ? "SI" : "NO");
    SerialMon.print("Tamaño del queue: ");
    SerialMon.print(Log.getQueueSize());
    SerialMon.println(" caracteres\n");

    // Escribir algunos logs de ejemplo
    Log.println(F("=== Sistema Iniciado ==="));
    Log.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
    Log.printf("Frecuencia CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Log.println(F("Configuración completada"));

    SerialMon.println("Logs escritos. Ahora los recuperamos...\n");
}

void loop() {
    // PASO 1: Escribir nuevos logs cada segundo
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 1000) {
        lastLog = millis();

        // Generar logs de ejemplo
        int temperatura = random(20, 30);
        int humedad = random(40, 60);

        Log.printf("Temp: %d°C, Humedad: %d%%\n", temperatura, humedad);
        Log.printf("Uptime: %lu segundos\n", millis() / 1000);

        // Agregar un mensaje ocasional
        if (random(100) < 20) {
            Log.println(F("Alerta: Lectura completada"));
        }
    }

    // PASO 2: Recuperar y procesar líneas del log
    char line[256];

    // getLine() es non-blocking por defecto (timeout = 0)
    // Retorna true si hay una línea disponible
    while (Log.getLine(line, sizeof(line))) {
        // Aquí puedes procesar la línea como quieras:
        // - Guardar en SD
        // - Enviar por MQTT
        // - Enviar a servidor web
        // - Almacenar en base de datos

        // En este ejemplo, simplemente la mostramos
        SerialMon.print("[LOG] ");
        SerialMon.println(line);

        // Opcional: Buscar palabras clave en el log
        if (strstr(line, "Alerta") != NULL) {
            SerialMon.println("  -> ¡ALERTA DETECTADA!");
        }
    }

    // PASO 3: Monitorear el estado del queue
    static unsigned long lastMonitor = 0;
    if (millis() - lastMonitor > 5000) {
        lastMonitor = millis();

        UBaseType_t waiting = Log.getQueueMessagesWaiting();
        uint16_t queueSize = Log.getQueueSize();
        int percentage = (waiting * 100) / queueSize;

        SerialMon.printf("\n[MONITOR] Queue ocupado: %d%% (%d/%d caracteres)\n\n",
                        percentage, waiting, queueSize);

        // Advertencia si el queue está muy lleno
        if (percentage > 80) {
            SerialMon.println("[WARN] Queue casi lleno! Aumenta el tamaño o procesa más rápido\n");
        }
    }

    // Pequeño delay para no saturar
    delay(10);
}
