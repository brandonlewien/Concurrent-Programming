#include "sgl.h"

pthread_mutex_t singleGlobalLock;


/******************************************************************************
 * SGL Stack
 *****************************************************************************/ 
void Print_Stack(std::stack <int> * s)
{
    while(!s->empty())
    {
        printf("%d ", s->top());
        s->pop();
    }
    printf("\n");
}
/******************************************************************************
 * @brief SGL_Stack_Push - Simple global lock around the push. Uses
 *                         stack library for faster prototyping.
 * @param stack - the stack passed in
 *        item  - the integer wanting to be in the queue
 * @return none
 *****************************************************************************/  
void SGL_Stack_Push(std::stack<int> * stack, int item)
{
    pthread_mutex_lock(&singleGlobalLock);
    stack->push(item);
    pthread_mutex_unlock(&singleGlobalLock);
    // printf("SGL-Push:%d\n", item);
}
/******************************************************************************
 * @brief SGL_Stack_Pop - Simple global lock around the pop. Uses
 *                        stack library for faster prototyping.
 * @param stack - the stack passed in
 * @return none
 *****************************************************************************/ 
void SGL_Stack_Pop(std::stack<int> * stack)
{   
    pthread_mutex_lock(&singleGlobalLock);
    if (!stack->empty()) 
    {
        // printf("SGL-Pop:%d\n", (*stack).top()); 
        stack->pop(); 
    } 
    else
    {
    	// printf("Stack Successfully emptied!\n");
    }
    pthread_mutex_unlock(&singleGlobalLock);
}

/******************************************************************************
 * SGL Queue
 *****************************************************************************/
 void Print_Queue(std::queue <int> * q)
{
    while(!q->empty())
    {
        printf("%d ", q->front());
        q->pop();
    }
    printf("\n");
}
/******************************************************************************
 * @brief SGL_Queue_Enqueue - Simple global lock around the push. Uses
 *                            queue library for faster prototyping.
 * @param queue - the queue passed in
 *        item  - the integer wanting to be in the queue
 * @return none
 *****************************************************************************/  
void SGL_Queue_Enqueue(std::queue<int> * queue, int item)
{
    pthread_mutex_lock(&singleGlobalLock);
    (*queue).push(item);
    pthread_mutex_unlock(&singleGlobalLock);
    // printf("SGL-E:%d\n", item);
}
/******************************************************************************
 * @brief SGL_Queue_Dequeue - Simple global lock around the pop. Uses
 *                            queue library for faster prototyping.
 * @param queue - the queue passed in
 * @return none
 *****************************************************************************/  
void SGL_Queue_Dequeue(std::queue<int> * queue)
{   
    pthread_mutex_lock(&singleGlobalLock);
    if (!queue->empty()) 
    {
        // printf("SGL-D:%d\n", (*queue).front()); 
        queue->pop(); 
    } 
    else
    {
    	// printf("Stack successfully emptied\n");
    }
    pthread_mutex_unlock(&singleGlobalLock);
}
