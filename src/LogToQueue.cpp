#include "LogToQueue.h"
#include <time.h>  // Para time(), localtime_r(), struct tm

void LogToQueue::begin(Print *output, bool showTimestamp, QueueHandle_t q)
{
    _logOutput = output;
    _showTimestamp = showTimestamp;
	_queue = q;
    _ownsQueue = false;  // External queue - user manages it

    // Create mutex for thread-safety
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == NULL) {
        // Error: no se pudo crear mutex
        // Consider disabling or logging error
    }

    this->setBufferSize(255);  // Maximum buffer size for uint8_t
}

void LogToQueue::begin(Print *output, bool showTimestamp, uint16_t queueSize)
{
    // Validate parameters
    if (output == NULL || queueSize == 0) {
        return;
    }

    // Reasonable limit to avoid memory exhaustion
    const uint16_t MAX_QUEUE_SIZE = 2000;
    if (queueSize > MAX_QUEUE_SIZE) {
        output->print(F("[LogToQueue] WARNING: Queue size "));
        output->print(queueSize);
        output->print(F(" clamped to "));
        output->println(MAX_QUEUE_SIZE);
        queueSize = MAX_QUEUE_SIZE;
    }

    // Create internal queue
    QueueHandle_t managedQueue = xQueueCreate(queueSize, sizeof(char));
    if (managedQueue == NULL) {
        output->println(F("[LogToQueue] ERROR: Failed to create queue"));
        return;
    }

    // Call the original begin() with the created queue
    this->begin(output, showTimestamp, managedQueue);

    // Verify mutex was created successfully
    if (_mutex == NULL) {
        // Error: mutex creation failed - cleanup queue
        vQueueDelete(managedQueue);
        _queue = NULL;
        output->println(F("[LogToQueue] ERROR: Failed to create mutex"));
        return;
    }

    // Mark as owned (override the _ownsQueue = false from original begin)
    _ownsQueue = true;
    _managedQueueSize = queueSize;
}

LogToQueue::~LogToQueue()
{
    // Clean up tags (v1.3.0)
    clearTags();

    // Clean up managed queue
    if (_ownsQueue && _queue != NULL) {
        vQueueDelete(_queue);
        _queue = NULL;
        _ownsQueue = false;
        _managedQueueSize = 0;
    }
    // NOTE: If !_ownsQueue, queue is external - user manages it

    // Clean up mutex
    if (_mutex != NULL) {
        vSemaphoreDelete(_mutex);
        _mutex = NULL;
    }

    // Free dynamically allocated buffer
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
}

size_t LogToQueue::write(uint8_t character)
{
    // 1. Buffer operations (protect with mutex)
    if (_mutex != NULL && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {

        // Check if we need timestamp BEFORE sending to queue
        if (_showTimestamp && this->bufferCnt == 0) {
            printTimestamp();
        }

        if (character == '\n') {
            this->sendBuffer();
        } else {
            if (this->bufferCnt < this->bufferSize) {
                this->buffer[this->bufferCnt++] = character;
            } else {
                // Buffer full: flush and add character
                this->sendBuffer();
                // After flush, check if we need timestamp
                if (_showTimestamp && this->bufferCnt == 0) {
                    printTimestamp();
                }
                // Verify buffer has space before adding character
                if (this->bufferCnt < this->bufferSize) {
                    this->buffer[this->bufferCnt++] = character;
                }
            }
        }

        xSemaphoreGive(_mutex);
    }

    // 2. Queue operations (after timestamp has been sent to queue)
    if (_queue != NULL) {
        BaseType_t queueResult = xQueueSend(_queue, &character, 0);
        if (queueResult != pdTRUE) {
            // Queue full - circular behavior: remove oldest, add new
            uint8_t discarded;
            if (xQueueReceive(_queue, &discarded, 0) == pdTRUE) {
                xQueueSend(_queue, &character, 0);
            }
        }
    }

    return 1;
}

uint8_t LogToQueue::getBufferSize()
{
    return this->bufferSize;
}

boolean LogToQueue::setBufferSize(uint8_t size)
{
    if (_showTimestamp) size -= 9;  // Reserve 9 bytes for "HH:MM:SS " timestamp
    if (size == 0) {
        return false;
    }

    // Acquire mutex for buffer reallocation
    bool mutexAcquired = false;
    if (_mutex != NULL) {
        mutexAcquired = (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE);
    }

    boolean result = false;

    if (this->bufferSize == 0) {
        this->buffer = (uint8_t *)malloc(size);
        result = (this->buffer != NULL);
    } else {
        uint8_t *newBuffer = (uint8_t *)realloc(this->buffer, size);
        if (newBuffer != NULL) {
            this->buffer = newBuffer;
            result = true;
        } else {
            // Keep old buffer intact on failure
            result = false;
        }
    }

    if (result) {
        this->bufferSize = size;
        this->bufferCnt = 0;  // Reset count on resize
    }

    if (mutexAcquired) {
        xSemaphoreGive(_mutex);
    }

    return result;
}

void LogToQueue::sendBuffer()
{
    // ASSUMPTION: Caller holds _mutex
    if (this->bufferCnt > 0)
    {
        if (_enable) {
            // Apply tag filter (v1.3.0)
            // Buffer now includes timestamp (if enabled), so filter checks both
            if (isTagAllowed((const char*)this->buffer, this->bufferCnt)) {
                _logOutput->write(this->buffer, this->bufferCnt);
                _logOutput->println();
            }
            // If tag is not allowed, message is discarded for serial output
            // (but was already sent to queue in write())
        }
        this->bufferCnt = 0;
    }
}

void LogToQueue::printTimestamp()
{
    // ASSUMPTION: Caller holds _mutex

    // Obtener tiempo del RTC interno
    time_t now = time(NULL);
    struct tm timeinfo = {0};  // Initialize to zero
    localtime_r(&now, &timeinfo);

    // Ensure values are in valid range to prevent buffer overflow
    int hour = timeinfo.tm_hour % 24;
    int min = timeinfo.tm_min % 60;
    int sec = timeinfo.tm_sec % 60;
    if (hour < 0) hour = 0;
    if (min < 0) min = 0;
    if (sec < 0) sec = 0;

    // Formato: "HH:MM:SS " (9 caracteres: 8 de tiempo + 1 espacio)
    char timestamp[16];  // Extra space for safety
    sprintf(timestamp, "%02d:%02d:%02d ", hour, min, sec);

    // Copiar timestamp al buffer interno (9 caracteres)
    for (int i = 0; i < 9 && this->bufferCnt < this->bufferSize; i++) {
        this->buffer[this->bufferCnt++] = timestamp[i];
    }

    // Enviar timestamp al queue (9 caracteres)
    if (_queue != NULL) {
        for (int i = 0; i < 9; i++) {
            BaseType_t queueResult = xQueueSend(_queue, &timestamp[i], 0);
            if (queueResult != pdTRUE) {
                // Queue full - circular behavior: remove oldest, add new
                uint8_t discarded;
                if (xQueueReceive(_queue, &discarded, 0) == pdTRUE) {
                    xQueueSend(_queue, &timestamp[i], 0);
                }
            }
        }
    }
}

void LogToQueue::setDump(bool enable)
{
    _enable = enable;
}

bool LogToQueue::getLine(char* buffer, size_t maxLen, TickType_t timeout)
{
    // Validate parameters
    if (buffer == NULL || maxLen == 0) {
        return false;
    }

    // Check queue exists
    if (_queue == NULL) {
        buffer[0] = '\0';
        return false;
    }

    size_t idx = 0;
    char ch;
    TickType_t startTime = xTaskGetTickCount();

    // Read characters until newline or buffer full
    while (idx < maxLen - 1) {
        // Calculate remaining timeout
        TickType_t elapsed = xTaskGetTickCount() - startTime;
        TickType_t remainingTimeout = (timeout > 0 && elapsed < timeout)
                                      ? (timeout - elapsed)
                                      : 0;

        // Try to receive character
        BaseType_t result = xQueueReceive(_queue, &ch, remainingTimeout);

        if (result != pdTRUE) {
            // Queue empty or timeout
            break;
        }

        // Check for line terminator
        if (ch == '\n') {
            buffer[idx] = '\0';
            return true;  // Complete line found
        }

        // Add character to buffer
        buffer[idx++] = ch;

        // Reset timeout for inter-character gaps
        if (timeout > 0) {
            startTime = xTaskGetTickCount();
        }
    }

    // Null-terminate partial or complete line
    buffer[idx] = '\0';

    // Return true if we got at least one character
    return (idx > 0);
}

UBaseType_t LogToQueue::getQueueMessagesWaiting() const
{
    if (_queue == NULL) {
        return 0;
    }
    return uxQueueMessagesWaiting(_queue);
}

// Tag filtering implementation (v1.3.0)

void LogToQueue::clearTags()
{
    if (_allowedTags != NULL) {
        // Liberar cada string individual
        for (uint8_t i = 0; i < _tagCount; i++) {
            if (_allowedTags[i] != NULL) {
                free(_allowedTags[i]);
                _allowedTags[i] = NULL;
            }
        }

        // Liberar array de punteros
        free(_allowedTags);
        _allowedTags = NULL;
    }

    _tagCount = 0;
}

void LogToQueue::setDump(const char* tags)
{
    // Limpiar tags anteriores
    clearTags();

    // NULL o string vacío = sin filtrado (permitir todos)
    if (tags == NULL || tags[0] == '\0') {
        _enable = true;
        return;
    }

    // Habilitar dump con filtrado
    _enable = true;

    // Contar tags (separados por comas)
    uint8_t count = 1;
    const char* p = tags;
    while (*p != '\0' && count < _maxTags) {
        if (*p == ',') count++;
        p++;
    }

    // Asignar array de punteros
    _allowedTags = (char**)malloc(count * sizeof(char*));
    if (_allowedTags == NULL) {
        _tagCount = 0;
        return;  // Error de memoria
    }

    // Parsear y copiar cada tag
    _tagCount = 0;
    const char* start = tags;
    const char* end = tags;

    while (*end != '\0' && _tagCount < count) {
        // Buscar próxima coma o fin de string
        while (*end != '\0' && *end != ',') {
            end++;
        }

        // Calcular longitud del tag
        size_t tagLen = end - start;

        // Saltar espacios al inicio
        while (tagLen > 0 && *start == ' ') {
            start++;
            tagLen--;
        }

        // Saltar espacios al final
        while (tagLen > 0 && *(start + tagLen - 1) == ' ') {
            tagLen--;
        }

        // Copiar tag si no está vacío
        if (tagLen > 0) {
            _allowedTags[_tagCount] = (char*)malloc(tagLen + 1);
            if (_allowedTags[_tagCount] != NULL) {
                strncpy(_allowedTags[_tagCount], start, tagLen);
                _allowedTags[_tagCount][tagLen] = '\0';
                _tagCount++;
            }
        }

        // Avanzar al siguiente tag
        if (*end == ',') {
            end++;
            start = end;
        }
    }
}

bool LogToQueue::isTagAllowed(const char* buffer, uint8_t len) const
{
    // Si no hay tags configurados, permitir todo
    if (_tagCount == 0 || _allowedTags == NULL) {
        return true;
    }

    // Si el buffer está vacío, permitir
    if (len == 0 || buffer == NULL) {
        return true;
    }

    // Detectar y saltar timestamp si existe (formato: "HH:MM:SS ")
    // Timestamp tiene 9 caracteres: "00:00:00 " (un espacio al final)
    uint8_t offset = 0;
    if (len >= 9 &&
        buffer[2] == ':' && buffer[5] == ':' && buffer[7] == ' ') {
        // Es muy probable que sea un timestamp, saltarlo
        offset = 9;
    }

    // Verificar si hay suficientes caracteres después del timestamp
    if (offset >= len) {
        return true;  // Sin mensaje después del timestamp
    }

    // Puntero al inicio del mensaje (después del timestamp si existe)
    const char* msg = buffer + offset;
    uint8_t msgLen = len - offset;

    // Si el mensaje no comienza con '[', no tiene tag (permitir)
    if (msgLen == 0 || msg[0] != '[') {
        return true;  // Mensajes sin tag se permiten
    }

    // Buscar fin del tag ']'
    uint8_t tagEndPos = 0;
    for (uint8_t i = 1; i < msgLen && i < 20; i++) {  // Máximo 20 chars para tag
        if (msg[i] == ']') {
            tagEndPos = i;
            break;
        }
    }

    // Si no hay ']' o está vacío, no es un tag válido
    if (tagEndPos == 0 || tagEndPos <= 1) {
        return true;  // No es un tag válido, permitir
    }

    // Extraer tag sin corchetes
    uint8_t tagLen = tagEndPos - 1;

    // Comparar con cada tag permitido
    for (uint8_t i = 0; i < _tagCount; i++) {
        if (_allowedTags[i] != NULL) {
            size_t allowedLen = strlen(_allowedTags[i]);

            // Comparar longitud y contenido
            if (tagLen == allowedLen &&
                strncmp(_allowedTags[i], msg + 1, tagLen) == 0) {
                return true;  // Tag encontrado en lista permitida
            }
        }
    }

    // Tag no está en la lista permitida
    return false;
}