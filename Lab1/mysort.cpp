/**************************************************************************//**
 * @file mysort.cpp
 * @brief General Mergesort and bucket sort algorithm developed for a class
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

#include <iostream> 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <string.h>
#include <math.h>
#include <atomic>
#include <mutex> 
#include <list>
#include <algorithm>  
#include <vector>

using namespace std; 

#define MAX 2500000
int upper[MAX];

typedef struct  
{
    int *** localizedBuckets;
    int ** globalBuckets;
    int ** localBucketLeftovers;
    int * globalLocationTracker;
    int bucketIndexSize;
}arrayPointerMix;
struct timespec start, endTime;

int numberInBucket[MAX] = {0};
int numberThreads = 0;                                  // # threads that gets set in main from argv
int numElements   = 0;                                  // # items that gets set in main from counter
int a[MAX]        = {0};                                // Initial array set large
int globalCounter = 0;                                  // Backend debug
atomic_int part (0);                                    // Handles the global thread assigner (should be unique)
atomic_int lockingBool (0);                             // Handles the lock across threads
mutex backToBucketLock;  
pthread_barrier_t bar;



/******************************************************************************
 * @brief bucketSort The not so bucket that sorts completely...
 *        takes an array and groups each item with a group (range)
 * @param low  - bottom part of array set
 *        high - top part of array set
 *        threadPart - Which thread is being used, should be unique
 * @return none
 *****************************************************************************/
void bucketSort(int low, int high, int threadPart, arrayPointerMix * arrayPassIn) 
{ 
    int bucketTracker[numElements] = {0}; // Storing things into a local bucket tracker
    int newLocation;
    for (int i = low; i <= high; i++) 
    { 
        // Put in local bucket
        for (newLocation = 0; newLocation < numberThreads; ++newLocation)
        {
            if(a[i] > (arrayPassIn->bucketIndexSize * newLocation) && a[i] <= (arrayPassIn->bucketIndexSize * (newLocation + 1) + 1))
            {
                arrayPassIn->localizedBuckets[threadPart][newLocation][bucketTracker[newLocation]] = a[i];
                break;
            }
        }
        // Increment number of elements for specific bucket tracking
        numberInBucket[newLocation]++;
        bucketTracker[newLocation]++;
        lockingBool = (backToBucketLock.try_lock());
        // Put into global bucket
        // if (lockingBool == 1) 
        // {
        //     // Minor bug where if the number of elements is too small program will segfault
        //     if (numElements > numberThreads)
        //     {
        //         arrayPassIn->globalBuckets[newLocation][arrayPassIn->globalLocationTracker[newLocation]] = a[i];
        //         arrayPassIn->globalLocationTracker[newLocation]++;
        //     }
        //     backToBucketLock.unlock();
        //     // Self check to see how much was missed
        //     globalCounter++;
        //     bucketTracker[newLocation] = (int)bucketTracker[newLocation] - 1;
        // }
    }
    // Missed these from the global, do in main.
    for (int i = 0; i < numElements; i++)
    {
        arrayPassIn->localBucketLeftovers[threadPart][i] = bucketTracker[i];
    }
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
    // storing values in left part 
    for (int i = 0; i < numLeftElements; i++) 
        left[i] = a[i + low]; 
  
    // storing values in right part 
    for (int i = 0; i < numRightElements; i++) 
        right[i] = a[i + mid + 1]; 
  
    int mainIterator = low; 
  
    while (leftIterator < numLeftElements && rightIterator < numRightElements) { 
        if (left[leftIterator] <= right[rightIterator]) 
            a[mainIterator++] = left[leftIterator++]; 
        else
            a[mainIterator++] = right[rightIterator++]; 
    } 
  
    while (leftIterator < numLeftElements) { 
        a[mainIterator] = left[leftIterator]; 
        mainIterator++;
        leftIterator++;
    } 
  
    while (rightIterator < numRightElements) { 
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

/******************************************************************************
 * @brief threadingHandler Splits the array evenly across all threads
 *        and throws all splits into bucketSort.
 * @param arg - placeholder, not used.
 * @return none
 *****************************************************************************/
void * threadingHandler(void * arrayPassIn) 
{ 
    arrayPointerMix * arrayIn = (arrayPointerMix *)arrayPassIn;

    // part will iterate after setting threadPart, meaning this will start at 0
    int threadPart = part++; 
    int low = (threadPart * numElements) / numberThreads; 
    int high = (((threadPart + 1) * numElements) / numberThreads) - 1; 
    bucketSort(low, high, threadPart, arrayIn);
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
    if (argv[1] == NULL)
    {
        printf("No parameters inputted!\n");
        return -1;
    }
    FILE * filePointer;
    filePointer = fopen(argv[1],"r");
    // If the file doesn't exist return error
    if (filePointer == NULL) 
    {
        return -1;
    }

    while (fgets(integer, sizeof(integer), filePointer) != NULL) 
    {
        a[incrementer] = atoi(integer);
        ++incrementer;
    }
    fclose(filePointer);
    numElements = incrementer; // Change the global to fit to number of elements

    if (argv[4] != NULL)
    {
        if (strcmp(argv[4], "-t") == 0)
        {
            if (argv[5] == NULL)
            {
                printf("No Number of threads!\n");
                return -1;
            }
            char *p;
            int conv = strtol(argv[5], &p, 10);
            numberThreads = conv;
            
        }
    }
    if (argv[6] != NULL)
    {
        if (strcmp(argv[6], "--alg=bucket") == 0)
        {
            clock_gettime(CLOCK_MONOTONIC,&start);
        /* Local copies of each bucket for each thread, matching 
             * Thread, Bucket ID, Item in Bucket
             */ 
            int x = numberThreads, y = numberThreads, z = numElements;
            int *** localizedBuckets = new int ** [x];
            for (int i = 0; i < x; ++i) 
            {
                localizedBuckets[i] = new int * [y];
                for (int j = 0; j < y; ++j)
                {
                    localizedBuckets[i][j] = new int[z]();
                }
            }
            arrayPassIn.localizedBuckets = localizedBuckets;
    printf("here\n");

            /* The global copy of buckets, the one the user should check
             * Bucket ID, Item in Bucket
             */
            int ** globalBuckets = new int * [numberThreads];
            for (int i = 0; i < numberThreads; ++i)
            {
                globalBuckets[i] = new int[numElements]();
            }
            arrayPassIn.globalBuckets = globalBuckets;
    printf("here1\n");

            /* The Kludge to handle missed buckets
             * Thread, Item in Bucket
             */
            int ** localBucketLeftovers = new int * [numberThreads];
            for (int i = 0; i < numberThreads; ++i)
            {
                localBucketLeftovers[i] = new int[numElements]();
            }
            arrayPassIn.localBucketLeftovers = localBucketLeftovers;
    printf("here2\n");

            /* Tracker that takes the threadID and handles incrementation of the global bucket 
             * Item in bucket
             */
            int * globalLocationTracker = new int[numElements](); 
            arrayPassIn.globalLocationTracker = globalLocationTracker;
            
            arrayPassIn.bucketIndexSize = a[numElements - 1] / numberThreads;
            printf("Bucket Index Size: %d\n", arrayPassIn.bucketIndexSize);
            backToBucketLock.unlock();
            // generating random values in array 
        /*    for (int i = 0; i < numElements; i++) 
                a[i] = rand() % 100;
            sort(a, a+numElements);*/

            pthread_t threads[numberThreads]; 
            for (int i = 0; i < numberThreads; ++i) 
            {
                printf("Created thread %d\n", i+2);

                pthread_create(&threads[i], NULL, threadingHandler, &arrayPassIn); 
            }
            for (int i = 0; i < numberThreads; ++i) 
            {
                printf("Joined thread %d\n", i+2);
                pthread_join(threads[i], NULL); 
            }

            // For each thread scan the Buckets for any missing ones
            for (int i = 0; i < numberThreads; i++) 
            {
                for (int j = 0; j < numElements; j++)
                {
                    while (localBucketLeftovers[i][j] != 0)
                    {
                        globalBuckets[j][globalLocationTracker[j]] = localizedBuckets[i][j][localBucketLeftovers[i][j]];
                        localBucketLeftovers[i][j]--;
                        // GlobalCounter at end should be == elements
                        globalCounter++;
                    }
                    // Sort the buckets
                    if (localBucketLeftovers[i][j] == 0)
                    {
                        sort(globalBuckets[i], globalBuckets[i]+numberInBucket[i]); 
                    }
                } 
            }

            // Uncomment for Debug
/*            for (int i = 0; i < numberThreads; i++) 
            {
                for (int j = 0; j < numElements; j++)
                {
                    printf("%d ", globalBuckets[i][j]);
                }
                printf("\n");
            }
            printf("globalCounter: %d\n", globalCounter);*/

            // Cleanup
            for (int i = 0; i < x; ++i) 
            {
                for (int j = 0; j < y; ++j)
                {
                    delete[] localizedBuckets[i][j];
                }
                delete[] localizedBuckets[i];
            }
            delete[] localizedBuckets;

  
            for (int i = 0; i < numberThreads; ++i)
            {
                delete[] globalBuckets[i];
            }
            delete[] globalBuckets;


            for (int i = 0; i < numberThreads; ++i)
            {
                delete[] localBucketLeftovers[i];
            }
            delete[] localBucketLeftovers;
            delete[] globalLocationTracker; 

            // Timer
            clock_gettime(CLOCK_MONOTONIC,&endTime);
            unsigned long long elapsed_ns;
            elapsed_ns = (endTime.tv_sec-start.tv_sec)*1000000000 + (endTime.tv_nsec-start.tv_nsec);
            printf("Elapsed (ns): %llu\n",elapsed_ns);
            double elapsed_s = ((double)elapsed_ns)/1000000000.0;
            printf("Elapsed (s): %lf\n",elapsed_s);
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
                // printf("Cycling middle: %d max: %d\n", upper[i], upper[i+1]);

            }
            for (int i = 0; i < incrementer; ++i)
            {
                printf("%d ", a[i]); 
            } 
            printf("\n"); 
            pthread_barrier_destroy(&bar);
        }
    }
    

    // mergeSort(incrementer, incrementer/2, rawFinal);
    if (argv[2] != NULL)
    {
        if (strcmp(argv[2], "-o") == 0)
        {
            if (argv[3] == NULL)
            {
                printf("No Third Input!\n");
                return -1;
            }

            filePointer = fopen(argv[3], "w");
            if (filePointer == NULL) 
            {
                return -1;
            }
            for (int i = 0; i < incrementer; ++i)
            {
                fprintf(filePointer, "%d\n", a[i]);
            }
            fclose(filePointer);
            return 0;
        }
    }
    for (int i = 0; i < incrementer; ++i)
    {
        printf("%d ", a[i]); 
    } 
    printf("\n"); 

    return 0;
}