#include "msqueue.h"

/******************************************************************************
 * M&S Queue
 * Credit goes to Joe Izraelevitz - Concurrent Programming Class Lecture Notes
 *****************************************************************************/ 
msqueue::msqueue() 
{
    node * dummy = new node(DUMMY);
    dummy->next = NULL;
    head.store(dummy);
    tail.store(dummy);
}

void msqueue::print()
{
    node * h = head.load(); 
    node * t = tail.load(); 
    while (h != t) 
    {
        h = h->next.load();
        printf("%d ", h->val);
    }
    printf("\n");
}

void msqueue::enqueue(int val) 
{
    node * t, * e, * n;
    node * dummy = NULL;
    n = new node(val);
    while(true) 
    {
        t = tail.load();
        e = t->next.load();
        if (t == tail.load())
        {
            if (e == NULL && t->next.compare_exchange_weak(dummy,n)) 
            {
                // printf("MS-EN:%d\n", val);
                break;
            }
        }
        else 
        {
            tail.compare_exchange_weak(t,e);
        }
    }
    tail.compare_exchange_weak(t,n);
}

int msqueue::dequeue() 
{
    node *t, *h, *n;
    while(true){
        h = head.load(); 
        t = tail.load(); 
        n = h->next.load();
        if (h == t) 
        {
            if (n == NULL)
            {
                return -2; // Should be null
            }
            else
            {
                tail.compare_exchange_weak(t,n);
            }
        }
        else
        {
            int ret = n->val;
            if (head.compare_exchange_weak(h,n))
            {
            	// printf("MS-DE:%d\n", ret);
                return ret;
            }
        }
    }
}

