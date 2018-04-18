#include <stdlib.h>
#include <stdio.h>
#include "cqueue.h"

#ifdef __cplusplus
extern "C"{
#endif

void cqueue_init(cqueue *p)
{
	p->front = (cqueuenode *)malloc(sizeof(cqueuenode));
	p->front->data = NULL;
	p->rear = p->front;
	p->size = 0;
	(p->front)->next = null;
}

status cqueue_destory(cqueue *q)
{
	while (q->front)
	{
		q->rear = q->front->next;
		if(q->front->data)
			free(q->front->data);
		free(q->front);
		q->front = q->rear;
	}
	q->size = 0;

	return 0;
}

void cqueue_clear(cqueue *q)
{
	while (cqueue_is_empty(q) != 1)
	{
		q->rear = q->front->next;
		if(q->front->data)
			free(q->front->data);
		free(q->front);
		q->front = q->rear;
	}
	if(q->front->data)
		free(q->front->data);
		
	q->front->data = NULL;
	q->rear = q->front;
	q->size = 0;
	(q->front)->next = null;
}

status cqueue_is_empty(cqueue *q)
{
	int v;
	if (q->front == q->rear) 
		v = 1;
	else              
		v = 0;
	return v;
}


elem cqueue_gethead(cqueue *q)
{
	elem v;
	if (q->front == q->rear)
		v = null;
	else
		v = (q->front)->next->data;
	return v;
}

status cqueue_enqueue(cqueue *q, elem e)
{
	q->rear->next = (cqueuenode *)malloc(sizeof(cqueuenode));
	q->rear = q->rear->next;
	if (!q->rear) 
		return -1;
	q->rear->data = e;
	q->rear->next = null;
	q->size += 1;

	return 0;
}

elem cqueue_dequeue(cqueue *q)
{
	cqueuenode *p;
	elem e;
	if (q->front == q->rear)
		return null;
	else 
		p = (q->front)->next;
	
	(q->front)->next = p->next;
	e = p->data;
	if (q->rear == p)
		q->rear = q->front;
	free(p);
	q->size -= 1;
	
	return(e);
}

int cqueue_size(cqueue *q)
{
	return q->size;
}

#ifdef __cplusplus
}
#endif

