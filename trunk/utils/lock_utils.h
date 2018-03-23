#ifndef LOCK_UTILS_H
#define LOCK_UTILS_H

#include <pthread.h> 
#include <semaphore.h>

#ifdef __cplusplus
extern "C"{
#endif

///////////////////////////////////////////////////////////////////
//–≈∫≈¡ø
typedef void* CSem;
CSem  csem_create(int initial_count, int maxcount);
CSem  csem_open(char* name, int initial_count);
int   csem_delete(CSem handle);
int   csem_close(CSem handle);
int   csem_wait(CSem handle);
int   csem_wait_timeout(void* handle, unsigned int timeout/*∫¡√Î*/);
int   csem_post(CSem handle);
int   csem_getcount(CSem handle, int *count);


///////////////////////////////////////////////////////////////////
//ª•≥‚À¯
typedef void* CMtx;
CMtx cmtx_create();
void cmtx_delete(CMtx handle);
void cmtx_enter(CMtx handle);
void cmtx_leave(CMtx handle);

#ifdef __cplusplus
}
#endif

#endif
