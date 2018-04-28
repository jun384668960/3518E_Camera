#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "utils_log.h"
#include "stream_manager.h"
#include "stream_define.h"

int main()
{
	shm_stream_t* handle = shm_stream_create("write", "stream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_WRITE);
	if(handle == NULL)
	{
		LOGE_print("shm_stream_create error");
		return -1;
	}

	int count = 0;
	while(1)
	{
		unsigned char data[1024] = {0};
		unsigned int length;
		sprintf(data, "======================%d", count);
		length = strlen(data);
		
		frame_info info;
		info.length = length;
		info.pts = count;
		
		int ret = shm_stream_put(handle,info,data,length);
		if(ret != 0)
		{
			LOGE_print("shm_stream_put error");
		}
		LOGI_print("data=>string:%s length:%d", data, length);
		LOGI_print("info=>length:%d pts:%d", info.length, info.pts);
		
		count++;
		usleep(1000*1000);
	}
	
	shm_stream_destory(handle);
	return 0;
}
