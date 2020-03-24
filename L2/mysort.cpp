/**************************************************************************//**
 * @file mysort.cpp
 * @brief General bucket sort algorithm with multithreading possibilities 
 * build: ./mysort source.txt -o out.txt -t 10 --alg=bucket --bar=pthread

 * @author Brandon Lewien
 * @version 1.3
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

#include <iostream> 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <mutex> 
#include <algorithm>  // sort()

using namespace std; 

#define MAX 2500000

int upper[MAX];

typedef struct  
{
    int ** globalBuckets;
    int ** globalTracker;
    int *  rangeTracker;
}arrayPointerMix;

struct timespec start, endTime;

int numberThreads = 0;                                  // # threads that gets set in main from argv
int numElements   = 0;                                  // # items that gets set in main from counter
int a[MAX]        = {0};                                // Initial array set large
int aFinal[MAX]   = {0};                                // Final array set large
atomic_int part (0);                                    // Handles the global thread assigner (should be unique)
pthread_barrier_t bar;
mutex backToBucketLock;  

/******************************************************************************
 * Ticket
 *****************************************************************************/
atomic_int next_num (0);
atomic_int now_serving (0);

static void ticketLock()
{
    int my_num = atomic_fetch_add(&next_num, 1);
    while(now_serving.load() != my_num);
}
static void ticketUnlock() 
{
    atomic_fetch_add(&now_serving, 1);
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
 * @brief (General) threadingHandler Splits the array evenly across all threads
 *        and throws all splits into bucketSort.
 * @param arg - placeholder, not used.
 * @return none
 *****************************************************************************/
/******************************************************************************
 * @brief (General) bucketSort The not so bucket that sorts completely...
 *        takes an array and groups each item with a group (range).
 *        The different bucketsorts have different locking/barriers.
 * @param low  - bottom part of array set
 *        high - top part of array set
 *        threadPart - Which thread is being used, should be unique
 * @return none
 *****************************************************************************/
/******************************************************************************
 * PThread Lock
 *****************************************************************************/
static void bucketSort_LP(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                backToBucketLock.lock();
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                backToBucketLock.unlock();
                break;
            }
        }
    }
    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }

}
void * threadingHandler_LP(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_LP(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * Barrier PThread
 *****************************************************************************/
static void bucketSort_BP(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    pthread_barrier_wait(&bar);
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                break;
            }
        }
    }
    pthread_barrier_wait(&bar); // Might not have even elements, therefore can stall if inside for loop
    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }
    pthread_barrier_wait(&bar);
}
void * threadingHandler_BP(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_BP(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * Barrier SRB
 *****************************************************************************/
static void bucketSort_SRB(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                break;
            }
        }
    }
    srbWait();

    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }
}
void * threadingHandler_SRB(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_SRB(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * TAS
 *****************************************************************************/
static void bucketSort_TAS(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                tasLock();
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                tasUnlock();
                break;
            }
        }
    }
    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }
}
void * threadingHandler_TAS(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_TAS(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * TTAS
 *****************************************************************************/
static void bucketSort_TTAS(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                ttasLock();
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                tasUnlock();
                break;

            }
        }
    }
    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }
}
void * threadingHandler_TTAS(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_TTAS(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * Ticket
 *****************************************************************************/
static void bucketSort_Ticket(int low, int high, int threadID, arrayPointerMix * threadArgs)
{
    // Know where you are in the global buckets with the offset.
    int localTracker[numberThreads];
    for (int i = 0; i < numberThreads; i++)
    {
        localTracker[i] = threadArgs->rangeTracker[threadID];
    }
    for (int i = low; i <= high; i++)
    {
        for (int j = 0; j < numberThreads; j++)
        {
            if ((threadArgs->rangeTracker[j + 1] > a[i]) && (threadArgs->rangeTracker[j] <= a[i]))
            {
                threadArgs->globalBuckets[j][localTracker[j]] = a[i];
                localTracker[j]++;
                break;
            }
        }
    }
    ticketLock();
    for (int i = 0; i < numberThreads; i++)
    {
        threadArgs->globalTracker[threadID][i] = localTracker[i];
    }
    ticketUnlock();
}
void * threadingHandler_Ticket(void * arg)
{ 
    arrayPointerMix * convertArg = (arrayPointerMix *)arg;
    int threadPart = part++;
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    bucketSort_Ticket(low, high, threadPart, convertArg);
} 
/******************************************************************************
 * @brief merge General merge algorithm
 * @param low  - bottom part of array set
 *        mid  - the middle of the set
 *        high - top part of array set
 * @return none
 *****************************************************************************/
void merge(int low, int mid, int high) 
{ 
    int left[mid - low + 1]; 
    int right[high - mid]; 
  
    int numLeftElements = mid - low + 1;
    int numRightElements = high - mid;
    int leftIterator = 0;
    int rightIterator = 0;
    // Storing values in left part 
    for (int i = 0; i < numLeftElements; i++) 
    {
        left[i] = a[i + low]; 
    }
  
    // Storing values in right part 
    for (int i = 0; i < numRightElements; i++) 
    {
        right[i] = a[i + mid + 1]; 
    }
  
    int mainIterator = low; 
  
    while (leftIterator < numLeftElements && rightIterator < numRightElements) 
    { 
        if (left[leftIterator] <= right[rightIterator])
        {
            a[mainIterator++] = left[leftIterator++]; 
        } 
        else
        {
            a[mainIterator++] = right[rightIterator++];
        }     
    } 
  
    while (leftIterator < numLeftElements) 
    { 
        a[mainIterator] = left[leftIterator]; 
        mainIterator++;
        leftIterator++;
    } 
  
    while (rightIterator < numRightElements) 
    { 
        a[mainIterator] = right[rightIterator]; 
        mainIterator++;
        rightIterator++;
    } 
} 
  
void merge_sort(int low, int high) 
{ 
    int mid = low + (high - low) / 2; 
    if (low < high) { 
        merge_sort(low, mid); 
        merge_sort(mid + 1, high); 
        merge(low, mid, high); 
    } 
} 

/******************************************************************************
 * @brief threadingHandlerMerge Splits the array evenly across all threads
 *        and throws all splits into mergeSort.
 * @param arg - placeholder, not used.
 * @return none
 *****************************************************************************/  
void * threadingHandlerMerge(void* arg) 
{ 
    /* The very first thread will be set to 0. The next threads will be
     * +1 from the previous. threadPart will allow for an even-ish
     * distribution split between the threads and the work to be done.
     */
    int threadPart = part++;

    int low, high;
    low = (threadPart * numElements) / numberThreads; 
    high = (((threadPart + 1) * numElements) / numberThreads) - 1;
    pthread_barrier_wait(&bar);
    int mid = low + (high - low) / 2; 
    /* We use the upper as a global in order to save the specific thread
     * splits for the final merge in main. We only need the upper because
     * the upper of specific splits can be used as the middle and also the 
     * end. The lower will always be 0 for the merge.
     */
    upper[threadPart] = high;
    if (low < high) { 
        merge_sort(low, mid); 
        merge_sort(mid + 1, high); 
        merge(low, mid, high); 
    } 
} 

int main(int argc, char* argv[]) 
{
    arrayPointerMix arrayPassIn;
    (void)argc;
    int incrementer = 0;
    char integer[MAX];
    // If first argument is "--name" print my name
    if (strcmp(argv[1], "--name") == 0)
    {
        printf("Brandon Lewien\n");
        return 24;
    }
    // Handling misinputs
    if (argv[1] == NULL || 
        argv[2] == NULL || 
        argv[3] == NULL || 
        argv[4] == NULL || 
        argv[6] == NULL ||
        argv[7] == NULL) 
    {
        printf("Missing parameters inputted!\n");
        return -1;
    }

    FILE * filePointer;
    filePointer = fopen(argv[1],"r");
    // If the file doesn't exist return error
    if (filePointer == NULL) 
    {
        printf("filepointer == NULL\n");
        return -1;
    }

    while (fgets(integer, sizeof(integer), filePointer) != NULL) 
    {
        a[incrementer] = atoi(integer);
        ++incrementer;
    }
    fclose(filePointer);

    numElements = incrementer; // Change the global to fit to number of elements
    sort(a, a+numElements);    // We assume sorted.

    if (strcmp(argv[4], "-t") == 0)
    {
        char *p;
        numberThreads = strtol(argv[5], &p, 10);
    }

    if (strcmp(argv[6], "--alg=bucket") == 0)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_t threads[numberThreads]; 
        arrayPointerMix arrayPassIn;

        /* The global copy of buckets, the one the user should check
         * Bucket ID, Item in Bucket
         */
        int ** globalBuckets = new int * [numberThreads];
        for (int i = 0; i < numberThreads; ++i)
        {
            globalBuckets[i] = new int[numElements]();
        }
        arrayPassIn.globalBuckets = globalBuckets;

        int * rangeTracker = new int[numberThreads + 1]();
        arrayPassIn.rangeTracker = rangeTracker;

        int ** globalTracker = new int * [numberThreads];
        for (int i = 0; i < numberThreads; ++i)
        {
            globalTracker[i] = new int[numElements]();
        }
        arrayPassIn.globalTracker = globalTracker;

        for (int i = 0; i < numberThreads; i++)
        {
            arrayPassIn.rangeTracker[i] = (i * numElements) / numberThreads; 
        }
        arrayPassIn.rangeTracker[numberThreads] = MAX;

        if (numberThreads > numElements)
            numberThreads = numElements;    // Kludge since we don't handle this case
        
        if (strcmp(argv[7], "--bar=sense") == 0)
        {
           for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_SRB, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL); 
        }
        else if (strcmp(argv[7], "--bar=pthread") == 0)
        {
            pthread_barrier_init(&bar, NULL, numberThreads);   
            for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_BP, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL); 
            pthread_barrier_destroy(&bar);
        }
        else if (strcmp(argv[7], "--lock=tas") == 0)
        {
            for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_TAS, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL);  
        }
        else if (strcmp(argv[7], "--lock=ttas") == 0)
        {
            for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_TTAS, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL); 
        }
        else if (strcmp(argv[7], "--lock=ticket") == 0)
        {
            for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_Ticket, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL); 
        }
        else if (strcmp(argv[7], "--lock=mcs") == 0)
        {
            return -1;
        }
        else if (strcmp(argv[7], "--lock=pthread") == 0)
        {
            for (int i = 0; i < numberThreads; ++i) 
                pthread_create(&threads[i], NULL, threadingHandler_LP, (void *)&arrayPassIn); 
          
            for (int i = 0; i < numberThreads; ++i) 
                pthread_join(threads[i], NULL); 
        }
        else 
        {
            printf("Wrong lock/barrier inputted\n");
            return -1;
        }

        int printBuckets[numberThreads][numElements] = {};
        int m[numberThreads] = {};
        // This is necessary to squash the 0's.
        for (int i = 0; i < numberThreads; i++)
        {
            for (int j = 0; j < numberThreads; j++)
            {
                if (globalTracker[i][j] > rangeTracker[i])
                {
                    for (int k = rangeTracker[i]; k < globalTracker[i][j]; k++)
                    {
                        printBuckets[j][m[j]] = globalBuckets[j][k];
                        m[j]++;
                    }
                }
            }
        }
        int finalCounter = 0;
        for (int i = 0; i < numberThreads; i++)
        {
            for (int j = 0; j < numElements; j++)
            {
                if (printBuckets[i][j] != 0) // We assume that there should be no 0s
                {
                    aFinal[finalCounter] = printBuckets[i][j];
                    finalCounter++;
                }
            }
        }
        sort(aFinal, aFinal+numElements);    // Sort the final

        // Timer
        clock_gettime(CLOCK_MONOTONIC,&endTime);
        unsigned long long elapsed_ns;
        elapsed_ns = (endTime.tv_sec-start.tv_sec)*1000000000 + (endTime.tv_nsec-start.tv_nsec);
        printf("Elapsed (ns): %llu\n",elapsed_ns);
    }

    else if (strcmp(argv[6], "--alg=fj") == 0)
    {
        pthread_barrier_init(&bar, NULL, numberThreads);   
        pthread_t threads[numberThreads]; 
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_create(&threads[i], NULL, threadingHandlerMerge, (void*)NULL); 
        }
      
        for (int i = 0; i < numberThreads; ++i) 
        {
            pthread_join(threads[i], NULL); 
        }

        for (int i = 0; i < numberThreads - 1; i++) 
        {
            merge(0, upper[i], upper[i+1]);
        }
        for (int i = 0; i < incrementer; ++i)
        {
            printf("%d ", a[i]); 
        } 
        printf("\n"); 
        pthread_barrier_destroy(&bar);
    }

    if (strcmp(argv[2], "-o") == 0)
    {
        filePointer = fopen(argv[3], "w");
        if (filePointer == NULL) 
        {
            return -1;
        }
        for (int i = 0; i < incrementer; ++i)
        {
            fprintf(filePointer, "%d\n", aFinal[i]);
        }
        fclose(filePointer);
        return 0;
    }
    for (int i = 0; i < incrementer; ++i)
    {
        printf("%d ", a[i]); 
    } 
    printf("\n"); 

    return 0;
}