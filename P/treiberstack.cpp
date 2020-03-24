#include "treiberstack.h"
/******************************************************************************
 * Treiber Stack
 * Credit goes to Joe Izraelevitz - Concurrent Programming Class Lecture Notes
 *****************************************************************************/ 

void tstack::print()
{
    node * t = top.load(memory_order_acquire);
    while(t->val != -2)
    {
        printf("%d ", t->val);
        t = t->down;
    }
    printf("\n");
}

void tstack::push(int val)
{
    node * n = new node(val);
    node * t;
    do 
    {
        t = top.load(memory_order_acquire);
        n->down = t;
    } while (!top.compare_exchange_weak(t,n,memory_order_acq_rel));
    // printf("Treiber-Push:%d\n", val);
}
int tstack::pop()
{
    node * t;
    node * n;
    int v = 0;
    do
    {
        t = top.load(memory_order_acquire);
        if (t == NULL)
        {
            return -2;
        }
        n = t->down;
        v = t->val;
    } while (!top.compare_exchange_weak(t,n,memory_order_acq_rel));
    return v;
}
