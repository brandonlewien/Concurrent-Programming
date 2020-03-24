#ifndef TREIBERSTACK_H
#define TREIBERSTACK_H

#include <atomic>
#include <iostream>

using namespace std;


class tstack
{
public:
    class node 
    {
    public:
        node (int v):val(v){}
        int val;
        node * down;
    };
    atomic<node *> top;
    void push(int val);
    int pop();
    void print();
};

#endif