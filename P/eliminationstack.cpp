#include "eliminationstack.h"

LLNode * LLhead;
LLNode * LLtail;
LLNode * LLposition;

atomic<int> elimCounter (0);


bool LL_CAS(LLNode * accum, LLNode * dest, LLNode * newval)
{
    if(accum->next == dest->next)
    {
        *dest = *newval;
        return true;
    }
    else
    {
        *accum = *dest;
        return false;
    }
}
void printElim(struct LLNode * n)
{
	while (n !=NULL)
	{
		printf("%d \n", n->data.val);
		n = n->next;
	}
}

void clearElim(void)
{
    LLNode * nd = LLposition;
    nd = (LLNode *)malloc(sizeof(LLNode));
    while(true)    
    {
        if(!LL_CAS(nd, LLposition, LLhead))
        {
            return;
        }
    }
}

void add_LL(LLNode * head)
{
	LLNode * newNode = NULL;
    LLNode * previous = NULL;
	LLNode * traversal = head;
	newNode = (LLNode *)malloc(sizeof(LLNode));
    previous = (LLNode *)malloc(sizeof(LLNode));
	while(traversal->next != NULL)
	{
		traversal = traversal->next;
        previous = traversal;
	}
    traversal->prev = previous;
	traversal->next = newNode;
    LLtail = traversal;
	newNode->next = NULL;
}

void elim_init(void)
{
    LLhead = (LLNode *)malloc(sizeof(LLNode));
    LLtail = (LLNode *)malloc(sizeof(LLNode));
    LLposition = (LLNode *)malloc(sizeof(LLNode));

    LLhead->prev = NULL;
    LLposition = LLhead;
	for(int i = 0; i < 100; i++)
	{
		add_LL(LLhead);
	}

}
/******************************************************************************
 * @brief estack::push - Customized treiber stack but with a limiter on top
 * @param val - the value wanting to be popped
 * @return bool - if the stack is full or not
 *****************************************************************************/ 
bool estack::push(int val)
{
    ++elimCounter;

    node * t;

    if (elimCounter >= MAX_ELIM_SIZE)
    {
        return true;
    }
    node * n = new node(val);

    do 
    {
        t = top.load(memory_order_acquire);

        n->down = t;
    } while (!top.compare_exchange_weak(t,n,memory_order_acq_rel));
    return false;
    // printf("Elimination-Push:%d\n", val);
}
/******************************************************************************
 * @brief estack::push - Customized treiber stack but with a limiter on top
 * @param None
 * @return int - the value or -2 == EMPTY
 *****************************************************************************/ 
int estack::pop()
{
    --elimCounter;

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

/*// Elimination Prototypes, the actual one from the paper
struct Cell {
    Cell *pnext;
    void * pdata;
};
struct ThreadInfo {
    uint16_t id;
    char op;
    Cell cell;
    int spin;
};
struct Simple_Stack{
    Cell * ptop;
};
Simple_Stack S;
void ** location;
int * collision;

bool TryPerformStackOp(ThreadInfo * p)
{
    Cell * phead, *pnext;
    if(p->op == 'H')
    {
        phead = S.ptop;
        p->cell.pnext = phead;
        if (cell_CAS(&S.ptop, phead, &p->cell))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    if (p->op == 'P')
    {
        phead = S.ptop;
        if (phead == NULL)
        {
            p->cell = NULL;
            return true;
        }
        pnext = phead->pnext;
        if (e_CAS(&S.ptop, phead, pnext))
        {
            p->cell =  *phead;
            return true;
        }
        else
        {
            p->cell = NULL;
            return false;
        }
    }
}
void FinishCollision(ThreadInfo * p)
{
    if (p->op == POP_OP)
    {
        p->pcell = location[mypid]->pcell;
        location[mypid] = NULL;
    }
}
void TryCollision(ThreadInfo * p, ThreadInfo * q)
{
    if (p->op == 'H')
    {
        if (e_CAS(&location[him], q, p))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    if (p->op == "P")
    {
        if (e_CAS(&location[him], q, NULL))
        {
            p->cell = q->cell;
            location[mypid] = NULL;
            return true;
        }
        else
        {
            return false;
        }
    }
}

void LesOP(ThreadInfo * p)
{
    while(true) {
        location[mypid] = p;
        int pos = GetPosition(p);
        int him = collision[pos];
        while(!e_CAS(&collision[pos], him, mypid))
        {
            him = collision[pos];
        }
        if (him != )//Empty)
        {
            int q = location[him];
            if (q != NULL && q->id==him && q->op != p->op)
            {
                if (e_CAS(&location[mypid], p, NULL))
                {
                    if(TryCollision(p, q) == true)
                    {
                        break;
                    }
                    else
                    {
                        if (TryPerformStackOp(p) == true)
                        {
                            break;
                        }
                    }
                }
                else {
                    FinishCollision(p);
                    return;
                }
            }
        }
        // delay(p->spin);
        if (e_CAS(!&location[mypid], p, NULL))
        {
            FinishCollision(p);
            break;
        }
        if (TryPerformStackOp(p) == true)
        {
            break;
        }
    }
}

void StackOp(ThreadInfo * p) {
    if (TryPerformStackOp(p) == false)
    {
        LesOP(p);
    }
}*/