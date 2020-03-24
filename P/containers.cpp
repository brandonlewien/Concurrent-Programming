/******************************************************************************
 * @file containers.cpp
 * @brief Included: 
 *          SGL Stack/Queue            : sglstack or sglqueue
 *          Treiber Stack              : treiber
 *          Michael and Scott Queue    : ms
 *          Elimination                : e_sgl or e_t
 *          Flat-Combining Stack/Queue : fc
 *          Baskets Queue              : basket
 * @run make all; ./containers -t <# threads> -l <# loops/iterations> <above>
 * @author Brandon Lewien
 * @version 1.0
 ******************************************************************************
 * @section License
 * (C) Copyright 2019 Brandon Lewien
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

#include "treiberstack.h"
#include "basketqueue.h"
#include "msqueue.h"
#include "eliminationstack.h"

#include <string.h> // strcmp
#include <stdio.h>

using namespace std;

// #define BACK_TO_BACK    // Undefine for doing all pushes/enqueues then pops/dequeues instead of back to back

extern pthread_mutex_t singleGlobalLock;
extern LLNode * LLhead;
extern LLNode * LLtail;
extern LLNode * LLposition;

typedef enum
{
    SGL_S_e, 
    SGL_Q_e,
    TREIBER_e,
    ESGL_e,
    ET_e,
    BASKET_e,
    MS_e
}test;

int numberThreads = 0;  // Globally defined, set in main
int numberLoops   = 0;  // Globally defined, set in main but divided by the number of threads
atomic<int> missed (0); // Self tracker for the Elimination stack

struct timespec start, endTime; 

/******************************************************************************
 * @brief _ThreadHandler - Interface between threads and library.
 *                         Back to Back and All then All are used.
 * Back to Back - Push/Enqueue then Pop/Dequeue... etc.
 * All then All - Push/Enqueue all, then Pop/Dequeue all
 * @param stack - the stack/queue passed in as a struct
 * @return void *, nothing.
 *****************************************************************************/  
void * SGL_Stack_ThreadHandler(void * stack)
{
    sglStackStruct * stackTC = (sglStackStruct *)stack;

#ifdef BACK_TO_BACK
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        SGL_Stack_Push(stackTC->lifoStack, iterations);
        SGL_Stack_Pop(stackTC->lifoStack);
    }
    while (!stackTC->lifoStack->empty()) 
    {
        SGL_Stack_Pop(stackTC->lifoStack);
    }
#else
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        SGL_Stack_Push(stackTC->lifoStack, iterations);
    }
    while (!stackTC->lifoStack->empty()) 
    {
        SGL_Stack_Pop(stackTC->lifoStack);
    }
#endif 
}
void * SGL_Queue_ThreadHandler(void * queue)
{
    sglQueueStruct * queueTC = (sglQueueStruct *)queue;

#ifdef BACK_TO_BACK
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        SGL_Queue_Enqueue(queueTC->fifoQueue, iterations);
        SGL_Queue_Dequeue(queueTC->fifoQueue);
    }
    while (!queueTC->fifoQueue->empty())
    {
        SGL_Queue_Dequeue(queueTC->fifoQueue);
    }
#else
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        SGL_Queue_Enqueue(queueTC->fifoQueue, iterations);
    }
    while (!queueTC->fifoQueue->empty())
    {
        SGL_Queue_Dequeue(queueTC->fifoQueue);
    }
#endif 
}
void * Treiber_ThreadHandler(void * object)
{
    tstack * objectC = (tstack *)object;
#ifdef BACK_TO_BACK
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        objectC->push(iterations);
        int val = (*objectC).pop();
    }
#else
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        objectC->push(iterations);
    }
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        int val = (*objectC).pop();
        // printf("Treiber.Pop:%d\n", val);
    }
#endif 
    // printf("Treiber Thread Exiting\n");
}
void * MS_ThreadHandler(void * object)
{
    msqueue * objectC = (msqueue *)object;
#ifdef BACK_TO_BACK
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        objectC->enqueue(iterations);
        objectC->dequeue();
    }
    int val = 0;
    while(val != -2)
    {
        val = (*objectC).dequeue();
        // printf("MS-DE:%d\n", val);
    }
#else   
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        objectC->enqueue(iterations);
    }
    int val = 0;
    while(val != -2)
    {
        val = objectC->dequeue();
        // printf("MS-DE:%d\n", val);
    }
#endif 
}
/******************************************************************************
 * Refer to writeup for Basket queue.
 *****************************************************************************/ 
void * Basket_ThreadHandler(void * queueInput)
{
    queue_t * queue = (queue_t *)queueInput;
    for (int iterations = 0; iterations < numberLoops; ++iterations)
    {
        Basket_Enqueue(queue, iterations);
    }
    int temp = 0;
    while (temp != -2)
    {
        temp = Basket_Dequeue(queue);
        printf("Basket-DE:%d\n", temp);
    }
}
/******************************************************************************
 * The Elimination threads are a bit confusing partly because one performs
 * very terrible with a lot of misses across the threads vs one that has 
 * very little misses. More is discussed in the writeup.
 * Missed gets print just for performance purposes. This tells how many times
 * the elimination stack had to resort to just pushing the item to the main
 * stack.
 *****************************************************************************/ 
void * E_SGL_Stack_ThreadHandler(void * stack)
{
    elimSGL * stackTC = (elimSGL *)stack;
    int iterations;
#ifdef BACK_TO_BACK
    for (iterations = 0; iterations < numberLoops; ++iterations)
    {
        if (stackTC->elim->push(iterations))
        {   
            SGL_Stack_Push(stackTC->sgl->lifoStack, iterations);
        }
        stackTC->elim->pop();
    }
    int val = 0;
    while(val != -2)
    {
        val = stackTC->elim->pop();
        SGL_Stack_Push(stackTC->sgl->lifoStack, iterations);
    }

    while (!stackTC->sgl->lifoStack->empty()) 
    {
        SGL_Stack_Pop(stackTC->sgl->lifoStack);
        missed++;
    } 
#else
    for (iterations = 0; iterations < numberLoops; ++iterations)
    {
        if (stackTC->elim->push(iterations))
        {   
            SGL_Stack_Push(stackTC->sgl->lifoStack, iterations);
            stackTC->elim->pop();
        }
    }
    int val = 0;
    while(val != -2)
    {
        val = stackTC->elim->pop();
        SGL_Stack_Push(stackTC->sgl->lifoStack, iterations);
    }

    while (!stackTC->sgl->lifoStack->empty()) 
    {
        SGL_Stack_Pop(stackTC->sgl->lifoStack);
        missed++;
    } 
#endif
    // printf("Missed %d\n", (int)missed);
}

void * E_Treiber_Stack_ThreadHandler(void * stack)
{
    elimTreiber * stackTC = (elimTreiber *)stack;
    int iterations;
#ifdef BACK_TO_BACK
    for (iterations = 0; iterations < numberLoops; ++iterations)
    {
        if (stackTC->elim->push(iterations))
        {   
            stackTC->treiber->push(iterations);
        }
        stackTC->elim->pop();
    }
    int val = 0;
    while(val != -2)
    {
        val =stackTC->elim->pop();
        // printf("val %d\n", val);
        stackTC->treiber->push(iterations);
    }
    int realVal = 0;
    while(realVal != -2)
    {
        realVal = stackTC->treiber->pop();
        missed++;
    } 
#else
    for (iterations = 0; iterations < numberLoops; ++iterations)
    {
        if (stackTC->elim->push(iterations))
        {   
            stackTC->treiber->push(iterations);
            stackTC->elim->pop();
        }
    }
    int val = 0;
    while(val != -2)
    {
        val = stackTC->elim->pop();
        // printf("val %d\n", val);
        stackTC->treiber->push(iterations);
    }
    int realVal = 0;
    while(realVal != -2)
    {
        realVal = stackTC->treiber->pop();
        missed++;
    } 
#endif
    // printf("Missed %d\n", (int)missed);
}
// Basic "Does it Run?" Tests
static bool test1(int * counter, int which, int numberThreadsLocal, int numIter)
{
    numberLoops = numIter;
    pthread_t threads[numberThreadsLocal]; 

    switch(which)
    {
        case(SGL_S_e):
        {
            // SGL Stack
            std::stack<int> lifoStack;
            sglStackStruct stackStruct;
            stackStruct.lifoStack = &lifoStack;

            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, SGL_Stack_ThreadHandler, &stackStruct); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            }
            break;
        }
        case(SGL_Q_e):
        {
            // SGL Queue
            std::queue<int> fifoQueue;
            sglQueueStruct queueStruct;
            queueStruct.fifoQueue = &fifoQueue;
            queueStruct.loops = numberLoops;

            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, SGL_Queue_ThreadHandler, &queueStruct); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            } 
            break;
        }
        case(TREIBER_e):
        {
            // Treiber Stack
            tstack TreiberStack;
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, Treiber_ThreadHandler, &TreiberStack); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            } 
            break;
        }
        case(MS_e):
        {
            // MS queue
            msqueue msqueueObject;  // Create the Object
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, MS_ThreadHandler, &msqueueObject); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            }
            break;
        }
        case(BASKET_e):
        {
            // Basket queue
            queue_t * queueInput = new queue_t;
            init_queue(queueInput);
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, Basket_ThreadHandler, queueInput); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            }
            delete queueInput;
            break;
        }
        case(ESGL_e):
        {
            // Elimination SGL Stack
            std::stack<int> lifoStack;
            sglStackStruct stackStruct;
            stackStruct.lifoStack = &lifoStack;
            estack estackObject;  // Create the Object
            elimSGL threadPassIn;
            threadPassIn.sgl = &stackStruct;
            threadPassIn.elim = &estackObject;

            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, E_SGL_Stack_ThreadHandler, &threadPassIn); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            }
            break;
        }
        case(ET_e):
        {
            // Elimination Treiber Stack
            tstack TreiberStack;
            estack estackObject;  // Create the Object
            elimTreiber threadPassIn;
            threadPassIn.treiber = &TreiberStack;
            threadPassIn.elim = &estackObject;

            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_create(&threads[numThreads], NULL, E_Treiber_Stack_ThreadHandler, &threadPassIn); 
            }
            for (int numThreads = 0; numThreads < numberThreadsLocal; ++numThreads)
            {
                pthread_join(threads[numThreads], NULL); 
            }
            break;
        }
    }
    ++(*counter);
    return true;
}

static bool testSuite(int * counter)
{
    test ff;
/******************************************************************************
* 100 LEVEL TESTS
******************************************************************************/
    // 1 thread 5 elements
        ff = SGL_Q_e;
    test1(counter, ff, 1, 5);
        ff = SGL_S_e;
    test1(counter, ff, 1, 5);
        ff = ET_e;
    test1(counter, ff, 1, 5);
        ff = TREIBER_e;
    test1(counter, ff, 1, 5);
        ff = ESGL_e;
    test1(counter, ff, 1, 5);
        ff = BASKET_e;
    test1(counter, ff, 1, 5);
        ff = MS_e;
    test1(counter, ff, 1, 5);
    // 2 threads 5 elements
        ff = SGL_Q_e;
    test1(counter, ff, 2, 5);
        ff = SGL_S_e;
    test1(counter, ff, 2, 5);
        ff = ET_e;
    test1(counter, ff, 2, 5);
        ff = TREIBER_e;
    test1(counter, ff, 2, 5);
        ff = ESGL_e;
    test1(counter, ff, 2, 5);
        ff = BASKET_e;
    test1(counter, ff, 2, 5);
        ff = MS_e;
    test1(counter, ff, 2, 5);
    // 16 threads
        ff = ET_e;
    test1(counter, ff, 16, 5);
        ff = SGL_Q_e;
    test1(counter, ff, 16, 5);
        ff = SGL_S_e;
    test1(counter, ff, 16, 5);
        ff = TREIBER_e;
    test1(counter, ff, 16, 5);
        ff = ESGL_e;
    test1(counter, ff, 16, 5);
        ff = BASKET_e;
    test1(counter, ff, 16, 5);
        ff = MS_e;
    test1(counter, ff, 16, 5);
/******************************************************************************
* 200 LEVEL TESTS
******************************************************************************/
    // Testing 1 thread, 200000 elements
        ff = ET_e;
    test1(counter, ff, 1, 200000);
        ff = SGL_Q_e;
    test1(counter, ff, 1, 200000);
        ff = SGL_S_e;
    test1(counter, ff, 1, 200000);
        ff = TREIBER_e;
    test1(counter, ff, 1, 200000);
        ff = ESGL_e;
    test1(counter, ff, 1, 200000);
        ff = BASKET_e;
    test1(counter, ff, 1, 200000);
        ff = MS_e;
    test1(counter, ff, 1, 200000);
    // 2 threads
        ff = SGL_Q_e;
    test1(counter, ff, 2, 200000);
        ff = SGL_S_e;
    test1(counter, ff, 2, 200000);
        ff = TREIBER_e;
    test1(counter, ff, 2, 200000);
        ff = ESGL_e;
    test1(counter, ff, 2, 200000);
        ff = BASKET_e;
    test1(counter, ff, 2, 200000);
        ff = MS_e;
    test1(counter, ff, 2, 200000);
    // 4 threads, 200000 elements
        ff = SGL_Q_e;
    test1(counter, ff, 4, 200000);
        ff = SGL_S_e;
    test1(counter, ff, 4, 200000);
        ff = TREIBER_e;
    test1(counter, ff, 4, 200000);
        ff = ESGL_e;
    test1(counter, ff, 4, 200000);
        ff = BASKET_e;
    test1(counter, ff, 4, 200000);
        ff = MS_e;
    test1(counter, ff, 4, 200000);
}

int main(int argc, char* argv[]) 
{
    (void)argc;
    // Create the pthread mutex used in SGL
    if (pthread_mutex_init(&singleGlobalLock, NULL) != 0)
    {
        printf("SGL Mutex failed\n");
        return -1;
    }
    if (argv[1] != NULL)
    {
    	if (strcmp(argv[1], "test") == 0)
    	{
    		int counter = 0;
    		if (testSuite(&counter))
    		{
    			printf("Passed %d tests\n", counter);
    			return 1;
    		}
    		else
    		{
    			printf("Error occurred in testing\n");
    			return -1;
    		}
    	}
        else if (strcmp(argv[1], "-h") == 0)
        {
            printf("\n");
            printf("Normal single run command:\n");
            printf("    ./containers -t <# threads> -l <# loops/iterations> <above>\n");
            printf("    <above> could be any of the following:\n");
            printf("    treiber, ms, e_sgl, e_t, basket, sglqueue, sglstack\n");
            printf("\n");
            printf("Automated Test Command\n");
            printf("    ./containers test\n");
            printf("\n");

            return 0;
        }
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

    // Get number of threads
    if (strcmp(argv[1], "-t") == 0)
    {
        char *p;
        numberThreads = strtol(argv[2], &p, 10);
    }
    pthread_t threads[numberThreads]; 
    // Get number of iterations/loops. This becomes porportional across all the sum of threads
    if (strcmp(argv[3], "-l") == 0)
    {
        char *p;
        numberLoops = strtol(argv[4], &p, 10);
        numberLoops = numberLoops / numberThreads;
    }
    // Start the timer
    clock_gettime(CLOCK_MONOTONIC,&start);

    if (strcmp(argv[5], "sglstack") == 0)
    {
        // SGL Stack
		std::stack<int> lifoStack;
		sglStackStruct stackStruct;
		stackStruct.lifoStack = &lifoStack;

		for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
		{
		    pthread_create(&threads[numThreads], NULL, SGL_Stack_ThreadHandler, &stackStruct); 
		}
		for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
		{
		    pthread_join(threads[numThreads], NULL); 
		}
    }
    else if (strcmp(argv[5], "sglqueue") == 0)
    {
	    // SGL Queue
	    std::queue<int> fifoQueue;
	    sglQueueStruct queueStruct;
	    queueStruct.fifoQueue = &fifoQueue;
	    queueStruct.loops = numberLoops;

	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_create(&threads[numThreads], NULL, SGL_Queue_ThreadHandler, &queueStruct); 
	    }
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_join(threads[numThreads], NULL); 
	    } 
    }
    else if (strcmp(argv[5], "treiber") == 0)
    {
        // Treiber Stack
    	tstack TreiberStack;
        TreiberStack.push(-2);
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_create(&threads[numThreads], NULL, Treiber_ThreadHandler, &TreiberStack); 
	    }
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_join(threads[numThreads], NULL); 
	    } 
            TreiberStack.print();

	}
    else if (strcmp(argv[5], "ms") == 0)
    {
	    // MS queue
	    msqueue msqueueObject;  // Create the Object
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_create(&threads[numThreads], NULL, MS_ThreadHandler, &msqueueObject); 
	    }
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_join(threads[numThreads], NULL); 
	    }
        msqueueObject.print();

	}
    else if (strcmp(argv[5], "basket") == 0)
    {
        // Basket queue
    	queue_t * queueInput = new queue_t;
    	init_queue(queueInput);
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_create(&threads[numThreads], NULL, Basket_ThreadHandler, queueInput); 
	    }
	    for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
	    {
	        pthread_join(threads[numThreads], NULL); 
	    }
	    delete queueInput;
	}
    else if (strcmp(argv[5], "e_sgl") == 0)
    {
        // Elimination SGL Stack
        std::stack<int> lifoStack;
        sglStackStruct stackStruct;
        stackStruct.lifoStack = &lifoStack;
        estack estackObject;  // Create the Object
        elimSGL threadPassIn;
        threadPassIn.sgl = &stackStruct;
        threadPassIn.elim = &estackObject;

        for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
        {
            pthread_create(&threads[numThreads], NULL, E_SGL_Stack_ThreadHandler, &threadPassIn); 
        }
        for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
        {
            pthread_join(threads[numThreads], NULL); 
        }
    }
    else if (strcmp(argv[5], "e_t") == 0)
    {
        // Elimination Treiber Stack
        tstack TreiberStack;
        estack estackObject;  // Create the Object
        elimTreiber threadPassIn;
        threadPassIn.treiber = &TreiberStack;
        threadPassIn.elim = &estackObject;

        for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
        {
            pthread_create(&threads[numThreads], NULL, E_Treiber_Stack_ThreadHandler, &threadPassIn); 
        }
        for (int numThreads = 0; numThreads < numberThreads; ++numThreads)
        {
            pthread_join(threads[numThreads], NULL); 
        }
    }

    clock_gettime(CLOCK_MONOTONIC,&endTime);
    unsigned long long elapsed_ns;
    elapsed_ns = (endTime.tv_sec-start.tv_sec)*1000000000 + (endTime.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);

    return 1;
}