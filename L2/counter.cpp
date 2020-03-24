/**************************************************************************//**
 * @file counter.cpp
 * @brief Miniprogram to practice locks and barriers with a counter
 * @build ./counter -t 5 -i=20000 --lock=ticket -o out.txt
 * @author Brandon Lewien
 * @version 1.1
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2019 Brandon Lewien
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Brandon Lewien will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include <mutex> 
#include <atomic>
#include <iostream> 
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace std; 

int numberThreads        = 0;                       // Changed in main
int numberIterations     = 0;                       // Changed in main
volatile int counterCntr = 0;                       // The main variable used for counter increment
int numberOver           = 0;                       // Attempt to solve the overincrement

pthread_barrier_t bar;
pthread_mutex_t mutexLock;


struct timespec start, endTime;

/******************************************************************************
 * Ticket
 *****************************************************************************/
atomic_int next_num       (0);
atomic_int now_serving    (0);
atomic_int atomicThreadID (0);

static void ticketLock()
{
    int my_num = atomic_fetch_add(&next_num, 1);
    while(now_serving.load() != my_num)
    {
        if (counterCntr >= numberIterations)    // This is an attempt to alleviate
        {                                       // the over-increment on counterCntr
            numberOver = 1;
            break;
        }
    }
}
static void ticketUnlock() 
{
    atomic_fetch_add(&now_serving, 1);
}
/******************************************************************************
 * @brief counterTL - A counter with TicketLock microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
static void * counterTL(void * filler)
{
    ticketLock();
    while (counterCntr < numberIterations) 
    {
        if (!numberOver)
        {
            counterCntr++;
        }
        ticketUnlock();
        ticketLock();
    }
    ticketUnlock();
}
/******************************************************************************
 * Test and Set along with Test&TAS
 *****************************************************************************/
atomic_bool tasFlag (false);

static bool tas()
{
    if (tasFlag == false) 
    {
        tasFlag = true;
        return true;
    }
    else
    {
        return false;
    }
}
static void tasLock()
{
    while(!tas());
}
static void ttasLock()
{
    while(tasFlag.load() || !tas());
}
static void tasUnlock()
{
    tasFlag.store(false, memory_order_seq_cst);
}
/******************************************************************************
 * @brief counterTSL - A counter with Test-and-Set Lock microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
static void * counterTSL(void * filler)
{
    tasLock();
    while (counterCntr < numberIterations) 
    {
        counterCntr++;
        tasUnlock();
        tasLock();
    }
    tasUnlock();
}
/******************************************************************************
 * @brief counterTTSL - A counter with Test-and-Test-and-Set Lock microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
static void * counterTTSL(void * filler)
{
    ttasLock();
    while (counterCntr < numberIterations) 
    {
        counterCntr++;
        tasUnlock();
        ttasLock();
    }
    tasUnlock();
}
/******************************************************************************
 * Sense Reversal Barrier
 *****************************************************************************/
atomic_int sense (0);
atomic_int cnt (0);

static void srbWait()
{
    int N = numberThreads;

    thread_local bool my_sense = 0;
    my_sense = !my_sense;
    int cnt_cpy = atomic_fetch_add(&cnt, 1);
    if (cnt_cpy == N - 1)                   // Last to arrive
    {
        cnt.store(0, memory_order_relaxed);
        sense.store(my_sense);
    }
    else                                    // Not last
    {
        while(sense.load() != my_sense);
    }
}
/******************************************************************************
 * @brief counterSRB - A counter with Sense-Reversal Barrier microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
static void * counterSRB(void * filler)
{
    int threadID = atomicThreadID++;
    while (counterCntr < numberIterations) 
    {
        counterCntr++;
        srbWait();
    }
}
/******************************************************************************
 * PThreads
 *****************************************************************************/
/******************************************************************************
 * @brief counterLockPthread - A counter with Pthread mutex locks microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
static void * counterLockPthread(void * filler)
{
    int threadID = atomicThreadID++;
    while (counterCntr < numberIterations) 
    {
        while (counterCntr % (numberThreads - 1) != threadID)
        {
            // printf("threadID: %d counterCntr %d \n", threadID, counterCntr);
            if (counterCntr >= numberIterations) break;
        }
        pthread_mutex_lock(&mutexLock);
        counterCntr++;
        pthread_mutex_unlock(&mutexLock);
    }
}
/******************************************************************************
 * @brief counterBarPthread - A counter with PThread Barriers microbenchmark
 * @param None
 * @return none
 *****************************************************************************/
bool flagOut = false; // States if we are done don't do a wait.
static void * counterBarPthread(void * filler)
{
    int threadID = atomicThreadID++;
    pthread_barrier_wait(&bar);
    for (int i = 0; i < numberIterations; i++)
    {
        if (i % numberThreads == threadID)
        {
            counterCntr++;
        }
        pthread_barrier_wait(&bar);
    }
}

int main(int argc, char* argv[]) 
{
    (void)argc;
    char *p; // Temp pointer;

    // If first argument is "--name" print my name
    if (strcmp(argv[1], "--name") == 0)
    {
        printf("Brandon Lewien\n");
        return 0;
    }
    // Handling misinputs
    if (argv[1] == NULL || 
        argv[2] == NULL || 
        argv[3] == NULL || 
        argv[4] == NULL || 
        argv[5] == NULL)
    {
        printf("Missing parameters inputted!\n");
        return -1;
    }

    // Get number threads
    if (strcmp(argv[1], "-t") == 0)
    {
        numberThreads = strtol(argv[2], &p, 10);
    }
    int digit = 0;
    char * iteratorSeperator = argv[3];
    // Requirement of -i=<#number> for argument
    if (iteratorSeperator[0] == '-')
    {
        if (iteratorSeperator[1] == 'i')
        {
            if (iteratorSeperator[2] == '=')
            {
                int tempIncrement = 3;
                for (int i = tempIncrement; i < (int)strlen(iteratorSeperator); i++)
                {
                    digit = iteratorSeperator[i] - '0';
                    numberIterations = numberIterations * 10 + digit;
                }
            }
        }
    }
    
    pthread_t threads[numberThreads]; 
    // The next conditional statements will pratically be repeats.
    // This of course could have been done more efficiently,
    // but for the sake of time I decided to take the very easy
    // route.
    if (strcmp(argv[4], "--bar=sense") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterSRB, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else if (strcmp(argv[4], "--bar=pthread") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        pthread_barrier_init(&bar, NULL, numberThreads);   
        // pthread_barrier_init(&bar, NULL, numberThreads - 1);   
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterBarPthread, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }
        // pthread_barrier_destroy(&bar);
        pthread_barrier_destroy(&bar);
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else if (strcmp(argv[4], "--lock=tas") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterTSL, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else if (strcmp(argv[4], "--lock=ttas") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterTTSL, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else if (strcmp(argv[4], "--lock=ticket") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterTL, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else if (strcmp(argv[4], "--lock=mcs") == 0)
    {
        return -1;
    }

    else if (strcmp(argv[4], "--lock=pthread") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC,&start);
        pthread_mutex_init(&mutexLock, NULL);   
        for (int i = 0; i < numberThreads; i++)
        {
            pthread_create(&threads[i], NULL, counterLockPthread, (void*)NULL); 
        }
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
            // printf("Joined thread %d\n", i+2);
        }
        pthread_mutex_destroy(&mutexLock);
        clock_gettime(CLOCK_MONOTONIC,&endTime);
    }

    else
    {
        printf("Argv[4] inputted wrong\n");
        return -1;
    }
    printf("counter %d\n", counterCntr);

    unsigned long long elapsed_ns;
    elapsed_ns = (endTime.tv_sec-start.tv_sec)*1000000000 + (endTime.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);

    // Only for Debug

    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);

    if (strcmp(argv[5], "-o") == 0)
    {
        FILE * filePointer;

        filePointer = fopen(argv[6], "w");
        if (filePointer == NULL) 
        {
            return -1;
        }
        fprintf(filePointer, "%d\n", counterCntr);
        fclose(filePointer);
        return 0;
    }

    return 0;
}