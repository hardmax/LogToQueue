#ifndef __LOGTOQUEUE_H__
#define __LOGTOQUEUE_H__

#pragma once
#include <Arduino.h>
#include <Print.h>

class LogToQueue : public Print
{
private:
    QueueHandle_t _queue = NULL; // QueueHandle_t queue;
    SemaphoreHandle_t _mutex = NULL; // Mutex for thread-safety
    bool _ownsQueue = false; // Queue ownership flag
    uint16_t _managedQueueSize = 0; // Size for managed queue
    Print *_logOutput;
    bool _showTimestamp = false;
    bool _enable = true;
    uint8_t *buffer = 0;
    uint8_t bufferCnt = 0, bufferSize = 0;
    void sendBuffer();
    void printTimestamp();

    // Tag filtering (v1.3.0)
    char** _allowedTags = NULL;     // Array de tags permitidos
    uint8_t _tagCount = 0;          // Número de tags configurados
    uint8_t _maxTags = 10;          // Máximo de tags soportados

    bool isTagAllowed(const char* buffer, uint8_t len) const;
    void clearTags();

public:
    void begin(Print *output, bool showTimestamp = true, QueueHandle_t q = NULL);
    ~LogToQueue();

    void setDump(bool enable = true);
    void setDump(const char* tags); // Tag filtering (v1.3.0)

    virtual size_t write(uint8_t);
    using Print::write;
    uint8_t getBufferSize();
    boolean setBufferSize(uint8_t size);

    // Managed queue mode (v1.2.0)
    void beginManaged(Print *output, bool showTimestamp = true, uint16_t queueSize = 500);

    // Line retrieval API (v1.2.0)
    bool getLine(char* buffer, size_t maxLen, TickType_t timeout = 0);

    // Utility methods (v1.2.0)
    UBaseType_t getQueueMessagesWaiting() const;
    bool isQueueManaged() const { return _ownsQueue; }
    uint16_t getQueueSize() const { return _managedQueueSize; }
};

#endif // __LOGTOQUEUE_H__