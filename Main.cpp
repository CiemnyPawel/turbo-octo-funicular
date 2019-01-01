#include "monitor.h"
#include <stdio.h>
#include <queue>
#include <pthread.h>

class buffer : public Monitor
{
public:
buffer(char Id, unsigned int bufferLength) : id(Id), bufferLength(bufferLength)
{
}

unsigned int checkBufferSize()
{
        enter();
        return (unsigned int) bufferQueue.size();
}

void unlockBufferAfterChoosing()
{
        leave();
}

void putMessage(unsigned int message)
{
        enter();
        if(bufferQueue.size() == bufferLength)
                wait(notFull);
        bufferQueue.push(message);
        if(bufferQueue.size() == 1)
                signal(notEmpty);
        leave();
}

unsigned int getMessage()
{
        enter();
        if(bufferQueue.size() == 0)
                wait(notEmpty);
        unsigned int message = bufferQueue.front();
        bufferQueue.pop();
        if(bufferQueue.size() == bufferLength - 1)
        {
                signal(notFull);
        }
        leave();
        return message;
}

private:
Condition notFull;
Condition notEmpty;
std::queue<unsigned int> bufferQueue;
const char id;
const unsigned int bufferLength;
};
