# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LogToQueue is an Arduino library for ESP32 that enables dual-output logging: simultaneously sending log messages to a serial port and a FreeRTOS queue. This allows flexible log handling - logs can be redirected to SD cards, MQTT, alternative serial ports, or any custom destination.

The library extends Arduino's `Print` class, making it compatible with standard `print()` and `println()` methods.

## Platform Requirements

- **Architecture**: ESP32 only (requires FreeRTOS)
- **Framework**: Arduino
- **Build System**: PlatformIO
- **Key Dependency**: FreeRTOS (for QueueHandle_t)

## Build and Development Commands

### Build the project
```bash
pio run
```

### Upload to ESP32-S3
```bash
pio run --target upload
```

### Monitor serial output
```bash
pio run --target monitor
```

### Clean build artifacts
```bash
pio run --target clean
```

### Switch between examples
Edit `platformio.ini` and change the `src_dir` line:
```ini
src_dir = examples/LogToQueue    # or examples/LogToBuffer
```

## Core Architecture

### Main Components

**LogToQueue Class** (`src/LogToQueue.h`, `src/LogToQueue.cpp`)
- Inherits from Arduino's `Print` class
- Core method: `write(uint8_t)` - handles every character written
- Dual output: buffers for serial output, queues for FreeRTOS processing
- Internal buffering (default 256 bytes) prevents memory fragmentation
- Auto-flushes on newline or buffer full

### Key Design Patterns

1. **Buffered Writing**: Characters accumulate in an internal buffer until a newline or buffer overflow occurs, then flush to serial port
2. **Queue Distribution**: Every character (including timestamps) is sent to the FreeRTOS queue immediately for parallel processing
3. **Timestamp Management**: When enabled, prepends 13-character timestamp (`HH:MM:SS.mmm `) to each line, accounting for it in buffer size calculations

### Buffer Management

- Default buffer size: 256 bytes
- If timestamp enabled, effective buffer size is reduced by 13 bytes
- Buffer allocation uses `malloc()` and `realloc()` for dynamic sizing
- Buffer flushes automatically when:
  - Newline character (`\n`) is written
  - Buffer reaches capacity (prevents overflow)

### Critical Implementation Details

**Character Flow**:
```
User calls Log.println()
  → write(uint8_t) called for each character
  → If first char of line & timestamp enabled: printTimestamp()
  → Character sent to queue via xQueueSend()
  → Character added to internal buffer
  → On '\n' or buffer full: sendBuffer() flushes to serial
```

**setDump() Toggle**:
- Controls whether buffered data is written to serial port
- Queue output always continues regardless of this setting
- Useful for disabling serial output while maintaining queue-based logging

## Library Usage Patterns

### Basic Initialization
```cpp
QueueHandle_t queueLog = xQueueCreate(100, sizeof(char));
LogToQueue Log;
Log.begin(&Serial, true, queueLog);  // Serial output, timestamp enabled, queue handle
```

### Reading from Queue
Two patterns demonstrated in examples:

**Character-by-character** (`LogToQueue.ino`):
```cpp
char datoRecibido;
if (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    // Process single character
}
```

**Line-buffered** (`LogToBuffer.ino`):
```cpp
while (xQueueReceive(queueLog, &datoRecibido, 0) == pdTRUE) {
    if (datoRecibido == '\n') {
        // Process complete line in buffer
    }
    // Build line in buffer
}
```

## Configuration Files

### platformio.ini
- Current target: `esp32-S3` (ESP32-S3 DevKit M-1)
- Flash size: 8MB with custom partition table (`particion8MB.csv`)
- LittleFS filesystem enabled
- USB CDC on boot enabled for Serial monitoring
- Library visibility: Uses `lib_extra_dirs = .` to make local library available to examples

### library.properties
- Version: 1.0.2
- Category: Communication
- ESP32 architecture only

## Testing Strategy

When modifying the library:
1. Test with both example sketches (LogToQueue and LogToBuffer)
2. Verify timestamp formatting (13 characters exactly)
3. Test buffer overflow handling (write more than 256 bytes without newline)
4. Validate queue doesn't overflow (adjust queue size in examples if needed)
5. Test setDump() toggle functionality
6. Monitor for memory leaks in buffer reallocation

## Common Pitfalls

- **Queue Size**: Queue stores individual characters, not strings. Size appropriately (e.g., 100 characters for ~3-5 typical log lines)
- **Buffer Size**: If changing buffer size with `setBufferSize()`, remember timestamp consumes 13 bytes
- **Character Encoding**: Library operates on raw bytes; no UTF-8 handling
- **Timestamp Format**: Hardcoded 13-character format - changes require updating buffer size logic
