#include "LogToQueue.h"

void LogToQueue::begin(Print *output, bool showTimestamp, QueueHandle_t q)
{
    _logOutput = output;
    _showTimestamp = showTimestamp;
	_queue = q;
    this->setBufferSize(256);
}

LogToQueue::~LogToQueue()
{
    
}

size_t LogToQueue::write(uint8_t character)
{   
    if (_showTimestamp && this->bufferCnt==0) printTimestamp();
    xQueueSend(_queue,&character,0);
    if (character == '\n') // when newline is printed we send the buffer
    {
        this->sendBuffer();
    }
    else
    {
        if (this->bufferCnt < this->bufferSize) // add char to end of buffer
        {
            *(this->bufferEnd++) = character;
            this->bufferCnt++;
        }
        else // buffer is full, first send&reset buffer and then add char to buffer
        {
            this->sendBuffer();
            *(this->bufferEnd++) = character;
            this->bufferCnt++;
        }
    }
    return 1;
}

uint16_t LogToQueue::getBufferSize()
{
    return this->bufferSize;
}

boolean LogToQueue::setBufferSize(uint16_t size)
{
    if (size == 0)
    {
        return false;
    }
    if (this->bufferSize == 0)
    {
        this->buffer = (uint8_t *)malloc(size);
        this->bufferEnd = this->buffer;
    }
    else
    {
        uint8_t *newBuffer = (uint8_t *)realloc(this->buffer, size);
        if (newBuffer != NULL)
        {
            this->buffer = newBuffer;
            this->bufferEnd = this->buffer;
        }
        else
        {
            return false;
        }
    }
    this->bufferSize = size;
    return (this->buffer != NULL);
}

void LogToQueue::sendBuffer()
{
    if (this->bufferCnt > 0)
    {
        _logOutput->write(this->buffer, this->bufferCnt);
        _logOutput->println();        
        this->bufferCnt=0;
    }
    this->bufferEnd=this->buffer;
}

void LogToQueue::printTimestamp()
{
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

  // Time as string
  char timestamp[20];
  sprintf(timestamp, "%02d:%02d:%02d.%03d ", Hours, Minutes, Seconds, MilliSeconds);

  _logOutput->print(timestamp);
  
  for (int i = 0; i < 13; i++)
  {
    xQueueSend(_queue,&timestamp[i],0);
  }
  
  
  //
}

