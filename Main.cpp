#include "monitor.h"
#include <stdio.h>
#include <queue>
#include <pthread.h>
#include <iostream>

#define numberOfBuffers 5 // can't use global variable
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
        //for(auto it = 0; 1==1; it++) // for test purpose
        {
                usleep((IndepRand() % 10));
                unsigned int randomBuffer;
                while(1 == 1)
                {
                        randomBuffer = IndepRand() % numberOfBuffers;
                        if(arrayOfBuffers[randomBuffer].checkBufferSize() != 0)
                        {
                                arrayOfBuffers[randomBuffer].unlockBufferAfterChoosing();
                                break;
                        }
                        arrayOfBuffers[randomBuffer].unlockBufferAfterChoosing();
                }
                arrayOfBuffers[randomBuffer].getMessage();
                printf("Consumer has eaten element from buffer nr: %d\n", randomBuffer);
        }
        printf("Consumer processed all messages\n");
}

void producer(size_t producerID)
{
        for(auto it = 0; it < numberOfProducedMessages; it++)
        //for(auto it = 0; 1==1; it++) // for test purpose
        {
                usleep(IndepRand() % 100);
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
                for(auto i = 0; i < numberOfBuffers; i++)
                        arrayOfBuffers[i].unlockBufferAfterChoosing();
                arrayOfBuffers[nrMinBuffer].putMessage(message);
                printf("Producer nr %ld produce message nr %d in buffer %d\n", producerID, message, nrMinBuffer);
        }
}

void * consumerP(void * notUsed)
{
        consumer();
}

void * producerP(void * producerIdPtr)
{
        unsigned int producerID = *(unsigned int *) producerIdPtr;
        free(producerIdPtr);
        producer(producerID);
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
        pthread_t consumer;
        pthread_t *producers = new pthread_t[numberOfProducers];
        pthread_create(&consumer, NULL, consumerP, (void *) nullptr);
        for(auto it = 0; it < numberOfProducers; it++)
                pthread_create(&producers[it], NULL, producerP, (void*) new unsigned int((it+1)));
        for(auto it = 0; it < numberOfProducers; it++)
                pthread_join(producers[it], NULL);
        pthread_join(consumer, NULL);
        return 0;
}
