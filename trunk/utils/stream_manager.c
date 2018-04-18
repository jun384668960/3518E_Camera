#include <string.h>
#include <sys/shm.h>
#include <sys/types.h>    
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "stream_manager.h"
#include "utils_log.h"

shm_stream_t* shm_stream_create(char* id, char* name, int users, int infos, int size, SHM_STREAM_MODE_E mode)
{
	int i;
	shm_stream_t* handle = (shm_stream_t*)malloc(sizeof(shm_stream_t));

	//映射共享内存
	void* addr = shm_stream_mmap(handle, name, users*sizeof(shm_user_t)+infos*sizeof(shm_info_t)+size);
	if(addr == NULL)
	{
		LOGE_print("sshm_stream_mmap error");
		free(handle);
		return NULL;
	}

	handle->sem = csem_open(name, 1);
	handle->mode = mode;
	handle->max_frames = infos;
	handle->size = size;
	handle->user_array = (char*)addr;
	handle->info_array = handle->user_array + users*sizeof(shm_user_t);
	handle->base_addr  = handle->info_array + infos*sizeof(shm_info_t);
	snprintf(handle->name, 20, "%s", name);

	csem_wait(handle->sem);
	shm_user_t* user = (shm_user_t*)handle->user_array;
	
	if(mode == SHM_STREAM_WRITE || mode == SHM_STREAM_WRITE_BLOCK)
	{	
		//写模式默认使用user[0]
		user[0].index = 0;
		user[0].offset = 0;
		user[0].users = 0;

		snprintf(user[0].id, 32, "%s", id);
		for(i=1; i<users; i++)	//	初始化其他模式的读下标
		{
			if(strlen(user[i].id) != 0)
			{
				LOGI_print("reader user[%d].id:%s", i, user[i].id);
				user[i].index = user[0].index;
				user[0].users++;
			}
			printf("%d=>%s ", i, user[i].id);
		}
		printf("\n");
	}
	else
	{
		//如果是重复注册
		for (i=1; i<users; i++)
		{
			if (strncmp(user[i].id, id, 32) == 0)
			{
				handle->index = i;
				user[i].index = user[0].index;
				goto shm_stream_create_done;
			}
		}

		//查找空位插入  若超过users则会出现异常
		for (i=1; i<users; i++)
		{
			if (strlen(user[i].id) == 0)			
			{
				handle->index = i;
				user[i].index = user[0].index;
				user[0].users++;
				snprintf(user[i].id, 32, "%s", id);
				LOGI_print("reader user[%d].id:%s", i, user[i].id);
				
				break;
			}
		}

		for(i=1; i<users; i++)	//	初始化其他模式的读下标
		{
			printf("%d=>%s ", i, user[i].id);
		}
		printf("\n");
	}

shm_stream_create_done:
	csem_post(handle->sem);
	return handle;
}

void shm_stream_destory(shm_stream_t* handle)
{
	csem_wait(handle->sem);
	shm_user_t* user = (shm_user_t *)handle->user_array;

	if(handle->mode == SHM_STREAM_READ)
		user[0].users--;

	memset(user[handle->index].id, 0, 32);
	shmdt(handle->user_array);

	csem_post(handle->sem);
	csem_close(handle->sem);
	
	if(handle != NULL)
		free(handle);
}

int shm_stream_put(shm_stream_t* handle, frame_info info, unsigned char* data, unsigned int length)
{
	//如果没有人想要数据 则不put
	if(shm_stream_readers(handle) == 0)
	{
		return -1;
	}
	
	unsigned int head;
	shm_user_t* users = (shm_user_t*)handle->user_array;
	shm_info_t* infos = (shm_info_t*)handle->info_array;

	csem_wait(handle->sem);
	head = users[0].index % handle->max_frames;
	
	memcpy(&infos[head].info, &info, sizeof(frame_info));
	infos[head].lenght = length;
	if(length + users[0].offset > handle->size) 	//addr不够存储了， 重头存储
	{
		infos[head].offset = 0;
		users[0].offset = 0;
	}
	else
	{
		infos[head].offset = users[0].offset;
	}
	memcpy(handle->base_addr+infos[head].offset, data, length);
	
	users[0].offset += length;
	users[0].index = (users[0].index + 1 ) % handle->max_frames;
//	LOGI_print("users[0].offset:%d users[0].index:%d", users[0].offset, users[0].index);
//	LOGI_print("users[%d].lenght:%d users[%d].offset:%d", head, infos[head].lenght, head, infos[head].offset);
	
	csem_post(handle->sem);
	return 0;
}

int shm_stream_get(shm_stream_t* handle, frame_info* info, unsigned char** data, unsigned int* length)
{
	volatile unsigned int tail, head;

	csem_wait(handle->sem);
	shm_user_t* users = (shm_user_t*)handle->user_array;
	head = users[0].index % handle->max_frames;
	tail = users[handle->index].index % handle->max_frames;
//	LOGI_print("head:%d tail:%d", head, tail);

	if (head != tail)
	{
		shm_info_t* infos = (shm_info_t*)handle->info_array;
		memcpy(info, &infos[tail].info, sizeof(frame_info));
		*data = (unsigned char*)(handle->base_addr + infos[tail].offset);
		*length = infos[tail].lenght;

//		users[handle->index].index = (tail + 1 ) % handle->max_frames;
		csem_post(handle->sem);
		return 0;
	}
	else
	{
		*length = 0;
	
		csem_post(handle->sem);
		return -1;
	}
}

int shm_stream_post(shm_stream_t* handle)
{
	volatile unsigned int tail, head;

	csem_wait(handle->sem);
	shm_user_t* users = (shm_user_t*)handle->user_array;
	head = users[0].index % handle->max_frames;
	tail = users[handle->index].index % handle->max_frames;

	if (head != tail)
	{
		users[handle->index].index = (tail + 1 ) % handle->max_frames;
	}
	csem_post(handle->sem);

	return 0;
}

int shm_stream_sync(shm_stream_t* handle)
{
	volatile unsigned int tail, head;
	shm_user_t *user;

	csem_wait(handle->sem);
	user = (shm_user_t*)handle->user_array;
	head = user[0].index % handle->max_frames;
	tail = user[handle->index].index % handle->max_frames;
	if(head != tail)
	{
		user[handle->index].index = user[0].index % handle->max_frames;
	}
	csem_post(handle->sem);

	return 0;
}

int shm_stream_remains(shm_stream_t* handle)
{	
    int ret;
	csem_wait(handle->sem);

	volatile unsigned int tail, head;
	shm_user_t *user;

	user = (shm_user_t*)handle->user_array;
	head = user[0].index % handle->max_frames;
	tail = user[handle->index].index % handle->max_frames;

    ret = (head + handle->max_frames - tail)% handle->max_frames;

	csem_post(handle->sem);
	return ret;

}

int shm_stream_readers(shm_stream_t* handle)
{
	int ret;
	csem_wait(handle->sem);

	shm_user_t* user = (shm_user_t*)handle->user_array;
	ret = user[0].users;
	
	csem_post(handle->sem);
	return ret;
}

void* shm_stream_mmap(shm_stream_t* handle, char* name, unsigned int size)
{
	int fd, shmid;
	void *memory;
	struct shmid_ds buf;
	
	char filename[32];
	snprintf(filename, 32, "/dev/.%s", name);
	if((fd = open(filename, O_RDWR|O_CREAT|O_EXCL)) > 0)
	{
		close(fd);
	}
	else
	{
		LOGW_print("open name:%s error errno:%d %s", filename, errno, strerror(errno));
	}
	
	shmid = shmget(ftok(filename, 'g'), size, IPC_CREAT);
	if(shmid == -1)
	{
		LOGE_print("shmget errno:%d %s", errno, strerror(errno));
		return NULL;
	}
	
	memory = shmat(shmid, NULL, 0);
	if (memory == (void *)-1)
    {
        LOGE_print("shmat failed errno:%d %s", errno, strerror(errno));
        return NULL;
    }
	
	shmctl(shmid, IPC_STAT, &buf);
	if (buf.shm_nattch == 1)
	{
		LOGI_print("shm_nattch:%d", buf.shm_nattch);
		memset(memory, 0, size);
	}
	else
	{
		LOGI_print("shm_nattch:%d", buf.shm_nattch);
	}
	
	return memory;
}

