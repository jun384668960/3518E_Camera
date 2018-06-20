#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "MP4Encoder.h"
#include "stream_manager.h"
#include "stream_define.h"

#include "utils_log.h"
#include "utils_common.h"

#define	EN_AUDIO	0
int g_W = 1280;
int g_H = 720;

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
	#if EN_AUDIO
	shm_stream_t* audio_stream = shm_stream_create("rec_audioread", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_AUDIO_MAX_SIZE, SHM_STREAM_READ);
	#endif
	shm_stream_t* main_stream = shm_stream_create("rec_mainread", "mainstream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	unsigned char* pData = (unsigned char*)malloc(200*1024);
	while(1)
	{
		bool Iwait = false;
		bool IwaitOver = false;
		bool vTrackSet = false;
		#if EN_AUDIO
		bool aTrackSet = false;
		#endif
		MP4EncoderResult result;
		MP4Encoder encoder;
		//根据当前时间取得文件名
		char filename[64] = {0};
		localtime_mp4name_get(filename);
		LOGI_print("start recording file %s", filename);
		
		result = encoder.MP4CreateFile(filename, 15);
		
		while(1)
		{
			frame_info info;
			unsigned char* frame;
			unsigned int length;
			int ret = shm_stream_get(main_stream, &info, &frame, &length);
			if(ret == 0)
			{
				//如果还没有取得I帧，并且当前非I帧
				if(!Iwait && info.key == 1)
				{
					LOGW_print("Wait for I frame Iwait:%d info.key:%d", Iwait, info.key);
					shm_stream_post(main_stream);
				}
				else
				{
					if(IwaitOver && info.key == 5)
					{
						LOGW_print("WriteH264Data IwaitOver:%d", IwaitOver);
						break;
					}
						
					
					memcpy(pData, frame, length);
					Iwait = true;
					if(!vTrackSet)
					{
						result = AddH264Track(encoder, pData, length);
						LOGW_print("AddH264Track error:%d", result);
						vTrackSet = true;
						#if EN_AUDIO
						result = AddALawTrack(encoder);
						LOGW_print("AddALawTrack error:%d", result);
						aTrackSet = true;
						#endif
					}
						
					result = WriteH264Data(encoder, pData, info);
					shm_stream_post(main_stream);
					if(result == MP4ENCODER_ERROR(MP4ENCODER_WARN_RECORD_OVER))
					{	
						//直到下一个I帧出现
						IwaitOver = true;
					}
					else if(result != MP4ENCODER_ENONE)
					{
						LOGW_print("WriteH264Data error:%d", result);
						break;
					}
				}
				
				#if EN_AUDIO
				while(1)
				{
					
					ret = shm_stream_get(audio_stream, &info, &frame, &length);
					if(ret == 0)
					{
						if(!aTrackSet)
						{
							shm_stream_post(audio_stream);
						}
						else
						{
							memcpy(pData, frame, length);
							result = WriteALawData(encoder, pData, info);
							shm_stream_post(audio_stream);
							if(result != MP4ENCODER_ENONE)
							{
								LOGW_print("WriteALawData error:%d %d", result, shm_stream_remains(audio_stream));
//								break;
							}
						}

					}
					else
					{
						break;
					}
				}
				#endif
			}
			else
			{
				usleep(5*1000);
			}
		}
		
		encoder.MP4ReleaseFile();
	}

	free(pData);
	if(main_stream != NULL)
		shm_stream_destory(main_stream);
	#if EN_AUDIO
	if(audio_stream != NULL)
		shm_stream_destory(audio_stream);
	#endif
	return 0;
}
