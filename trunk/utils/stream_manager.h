#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H

#include "lock_utils.h"

typedef enum{
	SHM_STREAM_READ = 1,
	SHM_STREAM_WRITE,
	SHM_STREAM_WRITE_BLOCK,
}SHM_STREAM_MODE_E;

typedef struct
{
	int type;
	int key;
	unsigned int 		length;
	unsigned long long 	pts;
	char reserved[12];
}frame_info;

typedef struct 
{
	char			id[32];	//	用户标识
	unsigned int	index;	//	当前读写info_array下标
	unsigned int	offset;	//	当前数据存储偏移 只用作写模式
	unsigned int	users;	//	读用户数
}shm_user_t;

typedef struct 
{
	unsigned int	offset;		//	数据存储偏移
	unsigned int	lenght;		//	数据长度
	frame_info		info;		//	数据info
}shm_info_t;

typedef struct 
{
//private
	char*	user_array;			//	用户数组初始地址
	char*	info_array;			//	信息数组初始地址
	char*	base_addr;			//	数据初始地址
	CSem	sem;
	char	name[20];			//	创建的共享文件名
	unsigned int index;			//	用户在userArray中的下标
	unsigned int max_frames;	//	最大的帧缓冲数，主要是frameArray数组元素个数
	unsigned int size;
	SHM_STREAM_MODE_E mode;
}shm_stream_t;

//public
shm_stream_t* shm_stream_create(char* id, char* name, int users, int infos, int size, SHM_STREAM_MODE_E mode);
void shm_stream_destory(shm_stream_t* handle);

int shm_stream_put(shm_stream_t* handle, frame_info info, unsigned char* data, unsigned int length);
int shm_stream_get(shm_stream_t* handle, frame_info* info, unsigned char** data, unsigned int* length);
int shm_stream_post(shm_stream_t* handle);
int shm_stream_sync(shm_stream_t* handle);
int shm_stream_remains(shm_stream_t* handle);
int shm_stream_readers(shm_stream_t* handle);

//private
void* shm_stream_mmap(shm_stream_t* handle, char* name, unsigned int size);

#endif
