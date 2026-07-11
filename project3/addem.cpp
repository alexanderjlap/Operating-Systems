#include <iostream>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <string.h> 

using namespace std;

const int MAX_THREADS = 10;
const int MAX_RANGE = 40;

const int RANGE = 1;
const int ALLDONE = 2;

struct Message {
    int iFrom;
    int type;
    int value1;
    int value2;
};

sem_t sem_s[MAX_THREADS + 1];
sem_t sem_r[MAX_THREADS + 1];
Message mailboxes[MAX_THREADS + 1];

void sendMsg(int iTo, Message &message);
void recvMsg(int iFrom, Message &message);
void* sumThread(void* arg);

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Invalid input - 2 arguments must be given\n";
        return 1;
    }

    const int numberOfThreads = atoi(argv[1]);
    if (numberOfThreads <= 0) {
        cout << "You need to specify 1 or more threads\n";
        return 1;
    }

    if (numberOfThreads > MAX_THREADS) {
        cout << "Too many threads. The maximum is 10\n";
        return 1;
    }

    pthread_t threads[MAX_THREADS + 1];
    const int maxValue = atoi(argv[2]);

    for (int i = 0; i <= numberOfThreads; ++i) {
        sem_init(&sem_s[i], 0, 1);
        sem_init(&sem_r[i], 0, 0);
    }

    for (int i = 1; i <= numberOfThreads; ++i) {
        if (pthread_create(&threads[i], nullptr, sumThread, reinterpret_cast<void*>(i)) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    int range = maxValue / numberOfThreads;
    for (int i = 0; i < numberOfThreads - 1; ++i) {
        Message message;
        message.iFrom = 0;
        message.type = RANGE;
        message.value1 = i * range + 1;
        message.value2 = (i + 1) * range;
        sendMsg(i + 1, message);
    }

    Message message;
    message.iFrom = 0;
    message.type = RANGE;
    message.value1 = (numberOfThreads - 1) * range + 1;
    message.value2 = maxValue;
    sendMsg(numberOfThreads, message);

    int fullSum = 0;

    for (int i = 1; i <= numberOfThreads; ++i) {
        Message message;
        recvMsg(0, message);
        if (message.type != ALLDONE) {
            cout << "Messages sent back improperly\n";
        }
        fullSum += message.value1;
    }

    cout << "The total for 1 to " << maxValue << " using " << numberOfThreads << " threads is " << fullSum << ".\n";

    for (int i = 1; i <= numberOfThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    for (int i = 0; i <= numberOfThreads; ++i) {
        sem_destroy(&sem_s[i]);
        sem_destroy(&sem_r[i]);
    }

    return 0;
}

void sendMsg(int iTo, Message &message) {
    sem_wait(&sem_s[iTo]);
    memcpy(&mailboxes[iTo], &message, sizeof(Message));
    sem_post(&sem_r[iTo]);
}

void recvMsg(int iFrom, Message &message) {
    sem_wait(&sem_r[iFrom]);
    memcpy(&message, &mailboxes[iFrom], sizeof(Message));
    sem_post(&sem_s[iFrom]);
}

void* sumThread(void* arg) {
    int threadID = reinterpret_cast<intptr_t>(arg);
    int localSum = 0;

    Message message;
    recvMsg(threadID, message);

    if (message.type != RANGE) {
        cout << "Improper message type received by thread " << threadID << endl;
        pthread_exit(nullptr);
    }

    int start = message.value1;
    int end = message.value2;

    for (int i = start; i <= end; ++i) {
        localSum += i;
    }

    message.iFrom = threadID;
    message.type = ALLDONE;
    message.value1 = localSum;
    sendMsg(0, message);

    pthread_exit(nullptr);
}
