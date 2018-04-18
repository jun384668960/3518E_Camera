#ifndef C_QUEUE_H
#define C_QUEUE_H

#define null 0 

typedef int status;
typedef void* elem;

typedef struct cqueuenode_s{
	elem data;
	struct cqueuenode_s *next;
}cqueuenode;
typedef struct
{
	cqueuenode *front;
	cqueuenode *rear;
	int size;
}cqueue;

#ifdef __cplusplus
extern "C"{
#endif

void cqueue_init(cqueue *p);
//将释放节点数据内存
void cqueue_clear(cqueue *q);
//将释放节点数据内存
status cqueue_destory(cqueue *q);
status cqueue_is_empty(cqueue *q);
//外部mallco节点结构
status cqueue_enqueue(cqueue *q, elem e);
//需要手动删除节点
elem cqueue_dequeue(cqueue *q);
elem cqueue_gethead(cqueue *q);
int cqueue_size(cqueue *q);

#ifdef __cplusplus
}
#endif

#endif
