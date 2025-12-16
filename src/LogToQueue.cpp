#include "LogToQueue.h"

void LogToQueue::begin(Print *output, bool showTimestamp, QueueHandle_t q)
{
    _logOutput = output;
    _showTimestamp = showTimestamp;
	_queue = q;

    // Create mutex for thread-safety
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == NULL) {
        // Error: no se pudo crear mutex
        // Consider disabling or logging error
    }

    this->setBufferSize(255);  // Maximum buffer size for uint8_t
}

LogToQueue::~LogToQueue()
{
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
    // 1. Queue operations (thread-safe by FreeRTOS)
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

    // 2. Buffer operations (protect with mutex)
    if (_mutex != NULL && xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {

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
                this->buffer[this->bufferCnt++] = character;
            }
        }

        xSemaphoreGive(_mutex);
    }

    return 1;
}

uint8_t LogToQueue::getBufferSize()
{
    return this->bufferSize;
}

boolean LogToQueue::setBufferSize(uint8_t size)
{
    if (_showTimestamp) size -= 13;
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
            _logOutput->write(this->buffer, this->bufferCnt);
            _logOutput->println();
        }
        this->bufferCnt = 0;
    }
}

void LogToQueue::printTimestamp()
{
    // ASSUMPTION: Caller holds _mutex

    // Division constants
    const unsigned long MSECS_PER_SEC       = 1000;
    const unsigned long SECS_PER_MIN        = 60;
    const unsigned long SECS_PER_HOUR       = 3600;
    const unsigned long SECS_PER_DAY        = 86400;

    // Total time
    const unsigned long msecs               =  millis();
    const unsigned long secs                =  msecs / MSECS_PER_SEC;

    // Time in components
    const unsigned long MilliSeconds        =  msecs % MSECS_PER_SEC;
    const unsigned long Seconds             =  secs  % SECS_PER_MIN ;
    const unsigned long Minutes             = (secs  / SECS_PER_MIN) % SECS_PER_MIN;
    const unsigned long Hours               = (secs  % SECS_PER_DAY) / SECS_PER_HOUR;

    // Time as string (optimized size: 13 chars + null terminator)
    char timestamp[14];
    sprintf(timestamp, "%02d:%02d:%02d.%03d ", Hours, Minutes, Seconds, MilliSeconds);

    _logOutput->print(timestamp);

    // Send timestamp to queue with circular behavior
    if (_queue != NULL) {
        for (int i = 0; i < 13; i++)
        {
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