#ifndef ELiMINATIONSTACK_H
#define ELiMINATIONSTACK_H

#include <atomic>
#include "sgl.h"
#include "treiberstack.h"

using namespace std;

#define MAX_LOCATION 6
#define MAX_ELIM_SIZE 100
#define DUMMY 0

struct elim_t
{
	int val;
	bool used;
};
struct LLNode
{
	elim_t data;
	struct LLNode * next;
    struct LLNode * prev;
};


void printElim(struct LLNode * n);
void clearElim(void);

void add_LL(LLNode * head);
bool LL_CAS(LLNode * accum, LLNode * dest, LLNode * newval);

void elim_init(void);


////////////////////////////////////////////////////////////////


class estack
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
    bool push(int val);
    int pop();
};

struct elimSGL
{
	sglStackStruct * sgl;
	estack	       * elim;
};

struct elimTreiber
{
	tstack * treiber;
	estack * elim;
};

#endif