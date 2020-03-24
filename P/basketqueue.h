#ifndef BASKETQUEUE_H
#define BASKETQUEUE_H

#include <iostream>
#include <atomic>

extern pthread_mutex_t singleGlobalLock;

#define MAX_HOPS 3  // In dequeue

struct node_t;      // Forward defined
struct pointer_t {
    node_t * ptr;
    bool deleted;
    uint16_t tag;
};
struct queue_t {
    pointer_t tail;
    pointer_t head;
};
struct node_t {
    int value;
    pointer_t next;
};

void init_queue(queue_t * q);
bool Basket_Enqueue(queue_t * q, int val);
void free_chain(queue_t * q, pointer_t head, pointer_t new_head);
int Basket_Dequeue(queue_t * q);

#endif