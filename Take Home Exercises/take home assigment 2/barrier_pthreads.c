#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define DEFAULT_TEST_ITERATIONS 10
#define MAX_THREAD_COUNT 1024


typedef struct barrierLog{
    pthread_mutex_t lockCount;
    pthread_cond_t isPossibleProceedUp;
    pthread_cond_t isPossibleProceedDown;
    int counter;
} barrierLog;


typedef struct barrierLinear{
    pthread_mutex_t lockCount;
    pthread_cond_t isPossibleProceed;
    int count;
} barrierLinear;



barrierLinear linearBarrier;
barrierLog logBarrier[MAX_THREAD_COUNT];

void initializeLogBarrier(barrierLog* barrierNodeList, int maxnoOfThreads){
    for(int i=0; i<maxnoOfThreads; i++){
        barrierNodeList[i].counter = 0;
        pthread_mutex_init(&(barrierNodeList[i].lockCount), NULL);
        pthread_cond_init(&(barrierNodeList[i].isPossibleProceedDown), NULL);
        pthread_cond_init(&(barrierNodeList[i].isPossibleProceedUp), NULL);
    }
}

void initilizeLinearBarrier(barrierLinear* barrier){
    barrier->count = 0;
    pthread_mutex_init(&(barrier->lockCount), NULL);
    pthread_cond_init(&(barrier->isPossibleProceed), NULL);
}


void useLinearBarrier(barrierLinear* barrier, int noOfThreads){
    pthread_mutex_lock(&(barrier->lockCount));
    barrier->count++;
    if(barrier->count != noOfThreads){
        while(pthread_cond_wait(&(barrier->isPossibleProceed), &(barrier->lockCount)) != 0);
    }
    else{
        barrier->count = 0;
        pthread_cond_broadcast(&(barrier->isPossibleProceed));
    }
    pthread_mutex_unlock(&(barrier->lockCount));
}


void useLogBarrier(barrierLog* barrierNodeList, int maxnoOfThreads, int threadID){
    int i = 2;
    int baseID = 0;
    int idx;
    do{
        idx = baseID + threadID / i;
        if(threadID % i == 0){
            pthread_mutex_lock(&(barrierNodeList[idx].lockCount));
            barrierNodeList[idx].counter++;
            while(barrierNodeList[idx].counter < 2)
                pthread_cond_wait(&(barrierNodeList[idx].isPossibleProceedUp), &(barrierNodeList[idx].lockCount));
            pthread_mutex_unlock(&(barrierNodeList[idx].lockCount));
        }
        else{
            pthread_mutex_lock(&(barrierNodeList[idx].lockCount));
            barrierNodeList[idx].counter++;
            if(barrierNodeList[idx].counter == 2)
                pthread_cond_signal(&(barrierNodeList[idx].isPossibleProceedUp));
            while(barrierNodeList[idx].counter != 0)
                pthread_cond_wait(&(barrierNodeList[idx].isPossibleProceedDown), &(barrierNodeList[idx].lockCount));
            pthread_mutex_unlock(&(barrierNodeList[idx].lockCount));
            break;
        }
        baseID = baseID + maxnoOfThreads / i;
        i = i*2;
    } while(i <= maxnoOfThreads);

    i = i/2;
    while(i > 1){
        baseID = baseID - maxnoOfThreads / i;
        idx = baseID + threadID / i;
        pthread_mutex_lock(&(barrierNodeList[idx].lockCount));
        barrierNodeList[idx].counter = 0;
        pthread_mutex_unlock(&(barrierNodeList[idx].lockCount));
        pthread_cond_signal(&(barrierNodeList[idx].isPossibleProceedDown));
        i = i / 2;
    }
}


void* testFunctionLinear(void* v){
    int maxThreads = *((int*)v);
    int i;
    int v1 = 0;
    int v2 = 1000000;
    while(i<10){
        v1 += i;
        v2 -= i;
        i++;
    }
    useLinearBarrier(&linearBarrier, maxThreads);
    return (NULL);
}

void* testFunctionLog(void* v){
    int threadID = ((int*)v)[0];
    int maxThreads = ((int*)v)[1];
    int i;
    int v1 = 0;
    int v2 = 1000000;
    while(i<10){
        v1 += i;
        v2 -= i;
        i++;
    }
    useLogBarrier(logBarrier, maxThreads, threadID);
    return (NULL);
}

int main(int argc, char** argv){
    clock_t startTime, endTime;
    double timeTaken;   
    int i;
    int maxnoOfThreads = atoi(argv[1]);
    if(maxnoOfThreads > MAX_THREAD_COUNT){
        printf("\n Maximum number of threads allowed execced the value. Maximum allowed value is %d\n", MAX_THREAD_COUNT);
        return 0;
    }

    if(!(maxnoOfThreads && (!(maxnoOfThreads & (maxnoOfThreads-1))))){
        printf("\nNumber of threads should be of a power of 2.\n");
        return 0;
    }

    int maxnoOfThreadsArr[maxnoOfThreads];
    pthread_t threads[maxnoOfThreads];
    pthread_attr_t attribute;
    pthread_attr_init (&attribute);


    double totalLinearTime = 0.0;
    double totalLogTime = 0.0;
    int testIt = 0;
    int noOfIterations = DEFAULT_TEST_ITERATIONS;

    if(argc < 2) return 0;

    while(testIt < noOfIterations){
        initilizeLinearBarrier(&linearBarrier);
        startTime = clock();
        for(i=0; i<maxnoOfThreads; i++){
            maxnoOfThreadsArr[i] = maxnoOfThreads;
            pthread_create(&threads[i], &attribute, &testFunctionLinear, (void*)&maxnoOfThreadsArr[i]);
        }
        endTime = clock();
        timeTaken = ((double)(endTime-startTime))/CLOCKS_PER_SEC;
        totalLinearTime += timeTaken;
        i = 0;
        while(i<maxnoOfThreads){
            pthread_join(threads[i], NULL);
            i++;
        }
        
        int arr[maxnoOfThreads][2];
        initializeLogBarrier(logBarrier, maxnoOfThreads);
        startTime = clock();
        i = 0;
        while(i<maxnoOfThreads){
            arr[i][0] = i;
            arr[i][1] = maxnoOfThreads;
            pthread_create(&threads[i], &attribute, &testFunctionLog, (void*)arr[i]);
            i++;
        }
        endTime = clock();
        timeTaken = ((double)(endTime-startTime))/CLOCKS_PER_SEC;

        totalLogTime += timeTaken;
        
        i = 0;
        while(i<maxnoOfThreads){
            pthread_join(threads[i], NULL);            
            i++;
        }
        
        testIt++;
    }

    printf("\nNo of iterations : %d\n", noOfIterations);
    printf("\nAverage time by linear barrier is %lf sec",(totalLinearTime/noOfIterations));
    printf("\nAverage time by by log barrier is  %lf sec\n\n",(totalLogTime/noOfIterations));

    return 0;
}
