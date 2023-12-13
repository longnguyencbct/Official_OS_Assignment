#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int empty(struct queue_t * q) 
{
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) 
{
    /* TODO: put a new process to queue [q] */
    if (q->size == MAX_QUEUE_SIZE) {
        return;
    }
    q->size++;
    int pos=q->size-1;
    while(pos) {
        if( q->proc[pos-1]->priority < proc->priority) {
            break;
        }
        q->proc[pos]=q->proc[pos-1];
        --pos;
    }
    q->proc[pos] = proc;
}

struct pcb_t * dequeue(struct queue_t * q) 
{
    /* TODO: return a pcb whose prioprity is the highest
     * in the queue [q] and remember to remove it from q
     * */
    if (q->size){
        return q->proc[--q->size];
    }
    return NULL;
}
