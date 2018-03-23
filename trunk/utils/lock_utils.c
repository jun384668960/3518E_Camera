#include "lock_utils.h"
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C"{
#endif

CSem csem_create(int initial_count, int max_count)
{
	sem_t *sem = (sem_t*)malloc(sizeof(sem_t));
	sem_init(sem, 0, initial_count);
	return sem;
}

CSem csem_open(char* name, int initial_count)
{
	sem_t* sem = sem_open(name, O_CREAT, 0X666, 1);
	sem_unlink(name);
	return sem;
}

int csem_delete(void* handle)
{
//	assert( NULL != handle);
	sem_destroy((sem_t*)handle);
	free(handle);
	return 0;
}

int csem_close(void* handle)
{
//	assert( NULL != handle);
	sem_close((sem_t*)handle);
	return 0;
}

int csem_getcount(void* handle, int *count)
{
//	assert( NULL != handle);
	sem_getvalue((sem_t *)handle, count);
	return 0;
}

int csem_post(void* handle)
{
//	assert( NULL != handle);
    return sem_post((sem_t*)handle);
}

int csem_wait_timeout(void* handle, unsigned int timeout)
{
//	assert( NULL != handle);
	struct timespec abstime;
	clock_gettime( CLOCK_REALTIME,&abstime );

	abstime.tv_sec  += timeout/1000;
	abstime.tv_nsec += (timeout%1000)*1000000;

	abstime.tv_sec += abstime.tv_nsec/1000000000;
	abstime.tv_nsec = abstime.tv_nsec%1000000000;

	do{
	if( sem_timedwait((sem_t*)handle, &abstime) == 0 ) return 0;
	if( errno != EINTR ) break;
	}while(1);
	return -1;
}

int csem_wait(void* handle)
{
//	assert( NULL != handle);
	return sem_wait((sem_t*)handle);
}


CMtx cmtx_create()
{
	pthread_mutex_t *mtx = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(mtx, &attr);
	pthread_mutexattr_destroy(&attr);
	
	return mtx;
}

void cmtx_delete(CMtx handle)
{
//	assert( NULL != handle);
	pthread_mutex_destroy((pthread_mutex_t*)handle);
	free(handle);
}

void cmtx_enter(CMtx handle)
{
	pthread_mutex_lock((pthread_mutex_t*)handle);
}

void cmtx_leave(CMtx handle)
{
	pthread_mutex_unlock((pthread_mutex_t*)handle);
}

#ifdef __cplusplus
}
#endif

