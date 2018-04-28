#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils_log.h"
#include "stream_manager.h"
#include "stream_define.h"

int main()
{
	shm_stream_t* handle = shm_stream_create("read2", "stream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	if(handle == NULL)
	{
		LOGE_print("shm_stream_create error");
		return -1;
	}

	int count = 0;
	while(1)
	{
		unsigned char* data;
		unsigned int length;
		frame_info info;
		unsigned char string[1024] = {0};
		
		int ret = shm_stream_get(handle, &info, &data, &length);
		if(ret != 0)
		{
			LOGE_print("shm_stream_get error");
		}
		else
		{
			strncpy(string, data, length);
			LOGI_print("data=>string:%s length:%d", string, length);
			LOGI_print("info=>length:%d pts:%d", info.length, info.pts);
		}
		int cnt = shm_stream_remains(handle);
		LOGI_print("shm_stream_remains cnt:%d", cnt);
		
		usleep(1000*1000);
	}
	
	shm_stream_destory(handle);
	return 0;
}


