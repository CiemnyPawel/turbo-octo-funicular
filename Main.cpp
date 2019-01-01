#include "monitor.h"
#include <stdio.h>
#include <queue>
#include <pthread.h>

#define numberOfBuffers 20 // can't use global variable
size_t buffersLength = 10;
size_t numberOfProducedMessages = 20;
size_t numberOfProducers = 10;

class buffer : public Monitor
{
public:
buffer(){
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
        if(bufferQueue.size() == buffersLength)
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
        if(bufferQueue.size() == buffersLength - 1)
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
};

buffer arrayOfBuffers[numberOfBuffers];

unsigned int IndepRand()
{
        FILE * F = fopen("/dev/urandom", "r");
        if(!F)
        {
                printf("Cannot open urandom...\n");
                abort();
        }
        unsigned int randomValue;
        fread((char *) &randomValue, 1, sizeof(unsigned int), F);
        fclose(F);
        return randomValue;
}

void consumer()
{
        size_t numberOfAllMessages = numberOfProducedMessages * numberOfProducers;
        for(auto it = 0; it < numberOfAllMessages; it++)
        {
                usleep((IndepRand() % 10000));
                unsigned int randomBuffer = IndepRand() % numberOfBuffers;
                arrayOfBuffers[randomBuffer].getMessage();
                printf("Consumer has eaten element from buffer nr: %d\n", randomBuffer);
        }
        printf("Consumer processed all messages");
}

void producer(size_t producerID)
{
        for(auto it = 0; it < numberOfProducedMessages; it++)
        {
                usleep(IndepRand() % 100000);
                unsigned int message = producerID * 1000 + IndepRand() % 100;
                unsigned int temp, nrMinBuffer, minValue;
                minValue = buffersLength +1;
                for(auto i = 0; i < numberOfBuffers; i++)
                {
                        temp = arrayOfBuffers[i].checkBufferSize();
                        if(temp < minValue)
                        {
                                minValue = temp;
                                nrMinBuffer = i;
                        }
                }
                arrayOfBuffers[nrMinBuffer].putMessage(message);
                for(auto i = 0; i < numberOfBuffers; i++)
                        arrayOfBuffers[i].unlockBufferAfterChoosing();
                printf("Producer nr %ld produce message nr %d\n", producerID, message);
        }
}
int main(int ArgC, char ** ArgV)
{
        if(ArgC != 4)
        {
                printf("%s =: Liczba producentow || Liczba wiadomosci od kazdego producenta || Dlugosc buforow\n", ArgV[0]);
                return 1;
        }
        numberOfProducers = (size_t) atoi(ArgV[1]);
        numberOfProducedMessages = (size_t) atoi(ArgV[2]);
        buffersLength = (size_t) atoi(ArgV[3]);

        return 0;
}
