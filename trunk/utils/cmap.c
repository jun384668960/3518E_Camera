#include <stdlib.h>
#include <stdio.h>
#include "cmap.h"

#ifdef __cplusplus
extern "C"{
#endif


void cmap_init(cmap *p)
{
	p->front = (cmapnode *)malloc(sizeof(cmapnode));
	p->front->data = NULL;
	p->rear = p->front;
	p->size = 0;
	p->front->key = 0xffffffff;
	(p->front)->next = null;

}
void cmap_clear(cmap *q)
{
	while (cmap_is_empty(q) != 1)
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
	q->front->key = 0xffffffff;
	q->size = 0;
}
status cmap_destory(cmap *q)
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
status cmap_is_empty(cmap *q)
{
	int v;
	if (q->front == q->rear) 
		v = 1;
	else              
		v = 0;
	return v;
}

status cmap_insert(cmap *q, int key, elem e)
{
	if(cmap_find(q, key) != null) return -2;
		
	q->rear->next = (cmapnode *)malloc(sizeof(cmapnode));
	q->rear = q->rear->next;
	if (!q->rear) 
		return -1;
	q->rear->data = e;
	q->rear->key = key;
	q->rear->next = null;
	q->size += 1;

	return 0;
}

elem cmap_find(cmap *q, int key)
{
	elem v = null;
	cmapnode* p = q->front->next;
	while (p)
	{	
		if(p->key == key)
		{
			v = p->data;
			break;
		}
		p = p->next;
	}

	return v;
}

cmapnode* cmap_index_get(cmap *q, int index)
{
	cmapnode* v = null;
	int idx = 0;
	cmapnode* p = q->front->next;
	while (p)
	{	
		if(idx == index)
		{
			v = p;
			break;
		}
		p = p->next;
		idx++;
	}

	return v;
}


status cmap_erase(cmap *q, int key)
{
	cmapnode* t = q->front;
	cmapnode* p = q->front->next;
	while (p)
	{	
		if(p->key == key)
		{
			t->next = p->next;
			if(q->rear == p)
			{
				q->rear = t;
			}
			
			free(p);

			q->size -= 1;
			return 0;
		}
		t = p;
		p = p->next;
	}

	return -1;
}
int cmap_size(cmap *q)
{
	return q->size;
}

#ifdef __cplusplus
}
#endif

