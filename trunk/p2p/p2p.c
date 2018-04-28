#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "p2p_handle.h"
#include "stream_manager.h"
#include "stream_define.h"

#include "utils_log.h"

typedef struct _gos_frame_head
{
	unsigned int	nFrameNo;			// 帧号
	unsigned int	nFrameType;			// 帧类型	gos_frame_type_t
	unsigned int	nCodeType;			// 编码类型 gos_codec_type_t
	unsigned int	nFrameRate;			// 视频帧率，音频采样率
	unsigned int	nTimestamp;			// 时间戳
	unsigned short	sWidth;				// 视频宽
	unsigned short	sHeight;			// 视频高
	unsigned int	reserved;			// 预留
	unsigned int	nDataSize;			// data数据长度
	char			data[0];
}gos_frame_head;

#define P2P_TURN_SERVER  		"119.23.128.209:6001"	
#define P2P_SERVER 				"119.23.128.209"
#define P2P_PORT				34780
#define P2P_UID 				"A99762001001002"
static int s_vsnd_stop = 0;
void* thread_p2p_vsnd(void* param)
{	
	gos_frame_head head;
	memset(&head, 0x0, sizeof(gos_frame_head));
	unsigned char* pData = (unsigned char*)malloc(200*1024);
	unsigned int vframe_no = 0;
	p2p_handle* handle = (p2p_handle*)param;
	shm_stream_t* main_stream = shm_stream_create("p2p_mainread", "mainstream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	while(s_vsnd_stop != 1)
	{
		int is_video = SND_VIDEO_HD;
		int is_key;

		frame_info info;
		unsigned char* frame;
		unsigned int length;
		int ret = shm_stream_get(main_stream, &info, &frame, &length);
		if(ret == 0)
		{
			if(p2p_handle_av_swtich(handle, is_video) == 0)
			{
				head.nCodeType = 12;
				head.nDataSize = info.length;
				head.nFrameNo = vframe_no++;
				head.nFrameRate = 30;
				head.nTimestamp = info.pts/1000;
				head.sWidth = 1280;
				head.sWidth = 720;
				head.nFrameType = info.key == 1? 2 : 1;
				memcpy(pData, &head, sizeof(gos_frame_head));
				memcpy(pData + sizeof(gos_frame_head), frame, length);

				p2p_handle_write(handle, (unsigned char*)pData, length+sizeof(gos_frame_head), is_video, info.pts/1000, info.key == 1? 0 : 1);
			}
			shm_stream_post(main_stream);
			int remains = shm_stream_remains(main_stream);
			if(remains > 6)
				LOGI_print("main_stream remains:%d", shm_stream_remains(main_stream));

		}
		else
		{
			usleep(33*1000);
		}
	}

	if(main_stream != NULL)
	{
		shm_stream_destory(main_stream);
	}
	return NULL;
}

int main()
{
	p2p_handle* handle = p2p_handle_create(P2P_UID);
	if(handle == NULL)
	{
		LOGE_print("p2p_handle_create error");
		return -1;
	}
	int ret = p2p_handle_init(handle,P2P_TURN_SERVER,P2P_SERVER,34780,P2P_UID,"123456",1);
	if(ret != 0)
	{
		LOGE_print("p2p_handle_init error");
		return -1;
	}

	shm_stream_t* audio_stream = shm_stream_create("p2p_audioread", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_AUDIO_MAX_SIZE, SHM_STREAM_READ);
	gos_frame_head head;
	memset(&head, 0x0, sizeof(gos_frame_head));
	unsigned char* pData = (unsigned char*)malloc(200*1024);
	unsigned int vframe_no = 0;
	unsigned int aframe_no = 0;
	pthread_t tid_vsnd;
	if(pthread_create(&tid_vsnd, NULL, thread_p2p_vsnd, handle) != 0)
	{
		LOGE_print("pthread_create thread_p2p_vsnd error");
		exit(0);
	}
	while(1)
	{
		int is_video = SND_AUDIO;
		int is_key;

		frame_info info;
		unsigned char* frame;
		unsigned int length;
		int ret = shm_stream_get(audio_stream, &info, &frame, &length);
		if(ret == 0)
		{
			if(p2p_handle_av_swtich(handle, SND_AUDIO) == 0)
			{
				head.nCodeType = 52;
				head.nDataSize = info.length;
				head.nFrameNo = aframe_no++;
				head.nFrameRate = 8000;
				head.nTimestamp = info.pts/1000;
				head.sWidth = 1280;
				head.sWidth = 720;
				head.nFrameType = 50;
				memcpy(pData, &head, sizeof(gos_frame_head));
				memcpy(pData + sizeof(gos_frame_head), frame, length);

				p2p_handle_write(handle, (unsigned char*)pData, length+sizeof(gos_frame_head), is_video, info.pts/1000, 1);
			}
			shm_stream_post(audio_stream);
			int remains = shm_stream_remains(audio_stream);
			if(remains > 6)
				LOGI_print("audio_stream remains:%d", shm_stream_remains(audio_stream));
		}
		else
		{
			usleep(40*1000);
		}
	}
	if(audio_stream != NULL)
	{
		shm_stream_destory(audio_stream);
	}
	s_vsnd_stop = 1;
	pthread_join(tid_vsnd, NULL);
	
	p2p_handle_uninit(handle);
	p2p_handle_destory(handle);
	
	return 0;
}
