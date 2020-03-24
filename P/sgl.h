#ifndef SGH_H
#define SGH_H

#include <stack>
#include <iostream>
#include <queue>
#include <pthread.h>

typedef struct {
    int loops;
    std::stack<int> * lifoStack;
}sglStackStruct;

typedef struct {
    int loops;
    std::queue<int> * fifoQueue;
}sglQueueStruct;

 
void SGL_Stack_Push(std::stack<int> * stack, int item);
void SGL_Stack_Pop(std::stack<int> * stack);

void SGL_Queue_Enqueue(std::queue<int> * queue, int item);
void SGL_Queue_Dequeue(std::queue<int> * queue);

#endif