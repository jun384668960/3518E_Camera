#ifndef UTILS_COMMON_H
#define UTILS_CPMMON_H
#ifdef __cplusplus
extern "C"{
#endif
#include <stdint.h>
#include <pthread.h>

void localtime_mp4name_get(char* filename);
//获取osd时间戳字符串
void localtime_string_get(char* tstr);
//执行控制台指令
int exec_cmd(const char *cmd);		
//获取系统剩余内存
unsigned long get_system_mem_freeKb();		
//获取目录剩余存储空间
unsigned long long get_system_tf_freeKb(char* dir);
//获取系统自启动时间到现在的tick数
int64_t get_tick_count();
//创建4M栈大小的线程
int pthread_create_4m(pthread_t *pt_id, void *(*proc)(void *), void* arg);

#ifdef __cplusplus
}
#endif
#endif
