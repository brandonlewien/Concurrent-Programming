#include "basketqueue.h"

/******************************************************************************
 * Basket Queue
 *****************************************************************************/

/******************************************************************************
 * @brief init_queue - Initializes a temp node nd
 * @param q   - the custom queue passed in
 * @return none
 *****************************************************************************/ 
void init_queue(queue_t * q)
{
    node_t * nd = new node_t;
    nd->next.ptr = NULL;
    nd->next.deleted = 0;
    nd->next.tag = 0;

    q->tail.ptr = nd;
    q->tail.deleted = 0;
    q->tail.tag = 0;

    q->head.ptr = nd;
    q->head.deleted = 0;
    q->head.tag = 0;

}
void backoff_scheme()
{
	printf("backoff\n");
}
/******************************************************************************
 * @brief Basket_Enqueue - Enqueue function from Moshe, Ori, and Nir's paper
 * @param q   - the custom queue passed in
 *        val - value that is placed into the queue
 * @return will return true or continue to spin
 *****************************************************************************/ 
bool Basket_Enqueue(queue_t * q, int val)
{
    node_t * nd = new node_t;
    nd->value = val;
    while(true)
    {
        pointer_t tail = q->tail;
        pointer_t next = tail.ptr->next;

        pointer_t temp;
        temp.ptr = nd;
        temp.deleted = 0;
        temp.tag = tail.tag+1;
        if (tail.ptr == q->tail.ptr)
        {
	        if (next.ptr == NULL)
	        {
	            nd->next.ptr = NULL;
	            nd->next.deleted = 0;
	            nd->next.tag = tail.tag+2;
				if (__sync_bool_compare_and_swap(&tail.ptr->next.ptr, next.ptr, temp.ptr) &&
				   (__sync_bool_compare_and_swap(&tail.ptr->next.deleted, next.deleted, temp.deleted)) &&
				   (__sync_bool_compare_and_swap(&tail.ptr->next.tag, next.tag, temp.tag)))
	            {
	                __sync_bool_compare_and_swap(&q->tail.ptr, tail.ptr, temp.ptr);
	                __sync_bool_compare_and_swap(&q->tail.deleted, tail.deleted, temp.deleted);
	                __sync_bool_compare_and_swap(&q->tail.tag, tail.tag, temp.tag);
	                printf("Basket-EN:%d\n", val);
	                return true;
	            }
	            next = tail.ptr->next;
	            while((next.tag == tail.tag+1) && (!next.deleted))
	            {
	                backoff_scheme();
	                nd->next = next;
	                if (__sync_bool_compare_and_swap(&tail.ptr->next.ptr, next.ptr, temp.ptr) &&
	                   (__sync_bool_compare_and_swap(&tail.ptr->next.deleted, next.deleted, temp.deleted)) &&
	                   (__sync_bool_compare_and_swap(&tail.ptr->next.tag, next.tag, temp.tag)))
	                {
	                	// printf("Basket-EN:%d\n", val);
	                    return true;
	                }
	                next = tail.ptr->next;
	            }
	        }
	        else
	        {
	            while ((next.ptr->next.ptr != NULL) && (q->tail.ptr==tail.ptr))
	            {
	                next = next.ptr->next;
	            }
	        	temp.ptr = next.ptr;
	            __sync_bool_compare_and_swap(&q->tail.ptr, tail.ptr, temp.ptr);
	            __sync_bool_compare_and_swap(&q->tail.deleted, tail.deleted, temp.deleted);
	            __sync_bool_compare_and_swap(&q->tail.tag, tail.tag, temp.tag);
	        }
	    }
    }
    return false;
}
/******************************************************************************
 * @brief free_chain - Frees the leftover
 * @param q        - the custom queue passed in
 *        head     - starting point
 *        new_head - replace the old with this
 * @return none
 *****************************************************************************/ 
void free_chain(queue_t * q, pointer_t head, pointer_t new_head)
{
	printf("here\n");
	pointer_t temp;
    temp.ptr = new_head.ptr;
    temp.deleted = 0;
    temp.tag = head.tag+1;
	if (__sync_bool_compare_and_swap(&q->head.ptr, head.ptr, temp.ptr) &&
		(__sync_bool_compare_and_swap(&q->head.deleted, head.deleted, temp.deleted)) &&
		(__sync_bool_compare_and_swap(&q->head.tag, head.tag, temp.tag)))
	{
		while (head.ptr != new_head.ptr)
		{
			// reclaim node head.ptr
			head = head.ptr->next;
		}
	}
}

/******************************************************************************
 * @brief Basket_Dequeue - Dequeue function from Moshe, Ori, and Nir's paper
 * @param q - the custom queue passed in
 * @return the value or empty == -2
 *****************************************************************************/ 
int Basket_Dequeue(queue_t * q)
{
    while(true)
    {
        pointer_t head = q->head;
        pointer_t tail = q->tail;
        pointer_t next = head.ptr->next;

        pointer_t temp;
        temp.ptr = next.ptr;
        temp.deleted = 0;
        temp.tag = tail.tag+1;
        if (head.ptr == q->head.ptr)
        {
	        if (head.ptr == tail.ptr)
	        {
	        	printf("never here\n");
	        	if (next.ptr == NULL)
	        	{
	        		return -2;
	        	}
	        	while ((next.ptr->next.ptr != NULL) && (q->tail.ptr == tail.ptr))
	        	{
	        		next = next.ptr->next;
	        	}
	        	__sync_bool_compare_and_swap(&q->tail.ptr, tail.ptr, temp.ptr);
	        	__sync_bool_compare_and_swap(&q->tail.deleted, tail.deleted, temp.deleted);
	        	__sync_bool_compare_and_swap(&q->tail.tag, tail.tag, temp.tag);
	        }
			else
			{
				pointer_t iter = head;
				int hops = 0;
				while ((next.deleted && (iter.ptr != tail.ptr)) && (q->head.ptr == head.ptr))
				{
	        		iter = next;
	        		next = iter.ptr->next;
	        		hops++;
	        	}
	        	if (q->head.ptr != head.ptr)
	        	{
	        		continue;
	        	}
	        	else if(iter.ptr == tail.ptr)
	        	{
	        		free_chain(q, head, iter);
	        	}
	        	else
	        	{
	        		int value = next.ptr->value;
	    			temp.deleted = 1;
	    			temp.tag = next.tag+1;
	        		if (__sync_bool_compare_and_swap(&iter.ptr->next.ptr, next.ptr, temp.ptr) &&
	        	       (__sync_bool_compare_and_swap(&iter.ptr->next.deleted, next.deleted, temp.deleted)) &&
	        		   (__sync_bool_compare_and_swap(&iter.ptr->next.tag, next.tag, temp.tag)))
	        		{
	        			printf("val\n");
	        			if (hops >= MAX_HOPS)
	        			{
	        				free_chain(q, head, next);
	        			}
	        			return value;
	        		}
	        		backoff_scheme();
	        	}
	        }
	    }
    }
}
