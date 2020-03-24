/******************************************************************************
 * @file mysort.cpp
 * @brief Mergesort with OpenMP
 * @run ./mysort <input.txt> -o <output.txt>
 * @author Brandon Lewien
 * @version 1.2
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2019 Brandon Lewien
 ******************************************************************************
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

#include <string.h>  // strcmp()
#include <stdio.h>
#include <time.h>    // clock
#include <stdlib.h>  // atoi()

using namespace std;

#define MAX 2500000
int numElements = 0; // Globally keep the number of elements in the file from incrementer
int a[MAX] = {0};    // Array that stores all unsorted then sorted integers

struct timespec start, endTime;

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
    int numLeftElements  = mid - low + 1;
    int numRightElements = high - mid;
    int leftIterator     = 0;
    int rightIterator    = 0;
    int mainIterator     = low;

    #pragma omp parallel
    {
        // storing values in left part 
        #pragma omp for
        for (int i = 0; i < numLeftElements; i++) 
        {
            left[i] = a[i + low]; 
        }
        // storing values in right part 
        #pragma omp for
        for (int i = 0; i < numRightElements; i++) 
        {
            right[i] = a[i + mid + 1]; 
        }
        #pragma omp single
        {
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
        }
        // threadingShift is an interesting variable.
        // The first usage is as a temp variable for the threading, where
        // my intention is to make left/RightIterator be equal to itself.
        // Typically I'd do iterator = iterator but since OpenMP does the 
        // thread creation and splitting for a for loop, leftIterator's original
        // value cannot be guaranteed to be the same after every iteration
        // since the split is different.
        // Along with this, the count for mainIterator used to be used as a 
        // decrement inside the for loop after storing the iterator value into array 'a'.
        // To make the value of mainIterator match the value of the respected iterator
        // I just matched the iterators with a main iterator shift, relying on the for
        // loop to do the increments.
        int threadingShift = leftIterator;
        #pragma omp for
        for (leftIterator = threadingShift; leftIterator < numLeftElements; leftIterator++)
        {
            a[mainIterator + (leftIterator - threadingShift)] = left[leftIterator]; 
        }     
        mainIterator = mainIterator + (leftIterator - threadingShift);

        threadingShift = rightIterator;
        #pragma omp for
        for (rightIterator = threadingShift; rightIterator < numRightElements; rightIterator++)
        {
            a[mainIterator + (rightIterator - threadingShift)] = right[rightIterator]; 
        }
        mainIterator = mainIterator + (rightIterator - threadingShift);
    }
} 
/******************************************************************************
 * @brief merge_sort A general 2 recursive mergesort caller with the low and high
 * @param low - bottom/first element of the range for array a.
 *        high - top/top element of the range for array a.
 * @return none
 *****************************************************************************/ 
void merge_sort(int low, int high) 
{ 
    int mid = low + (high - low) / 2; 
    if (low < high) 
    { 
        merge_sort(low, mid); 
        merge_sort(mid + 1, high); 
        merge(low, mid, high); 
    } 
}

int main(int argc, char* argv[]) 
{
    // If first argument is "--name" print my name
    if (strcmp(argv[1], "--name") == 0)
    {
        printf("Brandon Lewien\n");
        return 0;
    }
    // Handling misinputs
    if (argv[1] == NULL || 
        argv[2] == NULL || 
        argv[3] == NULL)
    {
        printf("Missing parameters inputted!\n");
        return -1;
    }

    FILE * filePointer;
    filePointer = fopen(argv[1],"r");
    // If the file doesn't exist return error
    if (filePointer == NULL) 
    {
        printf("filePointer == NULL\n");
        return -1;
    }

    int incrementer = 0;                                        // Counter for number of things in file
    char integer[MAX];                                          // Gets integer from file
    while (fgets(integer, sizeof(integer), filePointer) != NULL) 
    {
        a[incrementer] = atoi(integer);
        ++incrementer;
    }
    numElements = incrementer;
    fclose(filePointer);

    int low = 0; 
    int high = numElements - 1;
    clock_gettime(CLOCK_MONOTONIC, &start);
    merge_sort(low, high);                                      // Start of the Merge Sort Parallelism
    clock_gettime(CLOCK_MONOTONIC,&endTime);
                                                                
/*    unsigned long long elapsed_ns;                            // Print timer stuff for testing purposes
    elapsed_ns = (endTime.tv_sec-start.tv_sec)*1000000000 + (endTime.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);*/

    int flag = false;                                           // Debugging purposes

    if (strcmp(argv[2], "-o") == 0)
    {
        filePointer = fopen(argv[3], "w");
        if (filePointer == NULL) 
        {
            return -1;
        }
        for (int i = 0; i < incrementer; ++i)
        {
            if (i < incrementer - 1)                            // Debugging purposes
            {
                if (a[i] > a[i + 1])                            // Quick test for sorting check
                {
                    // printf("Failed\n");
                    // printf("%d, %d\n", a[i], a[i+1]);
                    flag = true;
                }
            }
            fprintf(filePointer, "%d\n", a[i]);
        }
        fclose(filePointer);
        if (flag == false)                                      // Debugging purposes 
        {
            // printf("Pass\n");                                
        }
        return 0;
    }
  return 0;
}
