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
	if (q->size == MAX_QUEUE_SIZE) 
	{
		perror("There are no available slots in this queue");
		return;
	}
	else
	{
		q->proc[q->size++] = proc;
	}
	
}

struct pcb_t * dequeue(struct queue_t * q) 
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
	if (q->size == 0)
	{
		perror("There are no process in this queue");
		return NULL;
	}
	struct pcb_t * proc_out = q->proc[0];
	if (q->size == 1)
	{
		q->proc[0] = NULL;
		q->size--;
		return proc_out;
	}
	else
	{
		memmove(&(q->proc[0]), &(q->proc[1]), (q->size - 1) * sizeof(proc_out));
		q->proc[q->size] = NULL;
		q->size--;
		return proc_out;
	}
}
