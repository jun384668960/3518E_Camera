#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "MP4Encoder.h"
#include "stream_manager.h"
#include "stream_define.h"

#include "utils_log.h"
#include "utils_common.h"

int g_W = 1280;
int g_H = 720;
uint8_t g_spspps[] = {
	0x00,0x00,0x00,0x01,0x67,0x42,0x00,0x1f,0x9d,0xa8,0x14,0x01,0x6e,0x9b,0x80,0x80,0x80,0x81,
	0x00,0x00,0x00,0x01,0x68,0xce,0x3c,0x80
};
MP4EncoderResult AddH264Track(MP4Encoder &encoder, uint8_t *sData, int nSize)
{
	return encoder.MP4AddH264Track(sData, nSize, g_W, g_H);

	return MP4ENCODER_ERROR(MP4ENCODER_E_ADD_VIDEO_TRACK);
}

MP4EncoderResult AddALawTrack(MP4Encoder &encoder)
{
	return encoder.MP4AddALawTrack(NULL, 0);
}

MP4EncoderResult WriteH264Data(MP4Encoder &encoder, uint8_t *sData, frame_info info)
{
	MP4EncoderResult result;

	return encoder.MP4WriteH264Data(sData, info.length, info.pts);
}

MP4EncoderResult WriteALawData(MP4Encoder &encoder, uint8_t *sData, frame_info info)
{
	MP4EncoderResult result;

	return encoder.MP4WriteALawData(sData, info.length, info.pts);
}

int main(int argc, const char *argv[])
{
	
	
	shm_stream_t* audio_stream = shm_stream_create("rec_audioread", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_AUDIO_MAX_SIZE, SHM_STREAM_READ);
	shm_stream_t* main_stream = shm_stream_create("rec_mainread", "mainstream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	unsigned char* pData = (unsigned char*)malloc(200*1024);
	while(1)
	{
		MP4EncoderResult result;
		MP4Encoder encoder;
		//根据当前时间取得文件名
		char filename[64] = {0};
		localtime_mp4name_get(filename);
		LOGI_print("start recording file %s", filename);
		
		result = encoder.MP4CreateFile(filename, 55);
		result = AddH264Track(encoder, g_spspps, sizeof(g_spspps)/sizeof(g_spspps[0]));
		result = AddALawTrack(encoder);
		while(1)
		{
			frame_info info;
			unsigned char* frame;
			unsigned int length;
			int ret = shm_stream_get(main_stream, &info, &frame, &length);
			if(ret == 0)
			{
				memcpy(pData, frame, length);
				result = WriteH264Data(encoder, pData, info);
				shm_stream_post(main_stream);
				if(result != MP4ENCODER_ENONE)
				{
					LOGW_print("WriteH264Data error:%d", result);
					break;
				}

				while(1)
				{
					ret = shm_stream_get(audio_stream, &info, &frame, &length);
					if(ret == 0)
					{
						memcpy(pData, frame, length);
						result = WriteALawData(encoder, pData, info);
						shm_stream_post(audio_stream);
						if(result != MP4ENCODER_ENONE)
						{
							LOGW_print("WriteALawData error:%d", result);
							break;
						}
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				usleep(20*1000);
			}
		}
		
		encoder.MP4ReleaseFile();
	}

	free(pData);
	if(main_stream != NULL)
		shm_stream_destory(main_stream);
	if(audio_stream != NULL)
		shm_stream_destory(audio_stream);
	return 0;
}
