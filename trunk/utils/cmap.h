#ifndef C_MAP_H
#define C_MAP_H

#define null 0 

typedef int status;
typedef void* elem;

typedef struct cmapnode_s{
	int key;
	elem data;
	struct cmapnode_s *next;
}cmapnode;
typedef struct
{
	cmapnode *front;
	cmapnode *rear;
	int size;
}cmap;

#ifdef __cplusplus
extern "C"{
#endif
void cmap_init(cmap *p);
//将释放节点数据内存
void cmap_clear(cmap *q);
//将释放节点数据内存
status cmap_destory(cmap *q);
status cmap_is_empty(cmap *q);
status cmap_insert(cmap *q, int key, elem e);
//将释放节点数据内存
status cmap_erase(cmap *q, int key);
elem cmap_find(cmap *q, int key);
cmapnode* cmap_index_get(cmap *q, int index);
int cmap_size(cmap *q);
#ifdef __cplusplus
}
#endif

#endif