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
    Print *_logOutput;
    bool _showTimestamp = false;
    bool _enable = true;
    uint8_t *buffer = 0;
    uint8_t bufferCnt = 0, bufferSize = 0;
    void sendBuffer();
    void printTimestamp();

public:
    void begin(Print *output, bool showTimestamp = true, QueueHandle_t q = NULL);
    ~LogToQueue();

    void setDump(bool enable = true);

    virtual size_t write(uint8_t);
    using Print::write;
    uint8_t getBufferSize();
    boolean setBufferSize(uint8_t size);
};

#endif // __LOGTOQUEUE_H__