#ifndef __LOGTOQUEUE_H__
#define __LOGTOQUEUE_H__

#pragma once
#include <Arduino.h>
#include <Print.h>

class LogToQueue : public Print 
{
private:
    QueueHandle_t _queue; //QueueHandle_t queue;
    Print* _logOutput;
    bool _showTimestamp;
    bool _enable = true;
    uint8_t* buffer;
    uint8_t* bufferEnd;
    uint16_t bufferCnt = 0, bufferSize = 0;
    void sendBuffer();
    void printTimestamp();

public:
    void begin(Print *output, bool showTimestamp = true, QueueHandle_t q = NULL);
    ~LogToQueue();

    void setDump(bool enable = true);

    virtual size_t write(uint8_t);
    using Print::write;
    uint16_t getBufferSize();
    boolean setBufferSize(uint16_t size);
};

#endif // __LOGTOQUEUE_H__