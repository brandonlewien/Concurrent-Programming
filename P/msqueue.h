#ifndef MSQUEUE_H
#define MSQUEUE_H

#include <atomic>
#include <iostream>

#define DUMMY 0

using namespace std;

class msqueue 
{
    public:
    class node 
    {
        public:
        node (int v) : val(v){}
        int val; 
        atomic<node *> next;
    };
    atomic<node *> head, tail;
    msqueue();
    void enqueue(int val);
    void print();
    int dequeue();
};

#endif