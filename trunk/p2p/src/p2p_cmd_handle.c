#include "p2p_cmd_handle.h"
#include "p2p_utils.h"
#include "utils_log.h"

//////////////////////////////////////////////////
#include "command_define.h"
//////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

static int uploadBufferSize = 5*1024;
static int uploadPacketSize = 3*1024;

static int s_vQualitySingle = 0;		//请求的视频清晰度
static int s_aSwitch = STATUS_STOP;		//请求了音频
static int s_vSwitch = STATUS_STOP;		//请求了视频

int globle_vquality_init()
{
	return s_vQualitySingle;
}
int globle_vquality_get()
{
	return s_vQualitySingle;
}
int globle_vswitch_get()
{
	return s_vSwitch;
}
int globle_aswitch_get()
{
	return s_vSwitch;
}

int cmd_handle(p2p_conn_clnt* clnt, void* data, int len,  P2pHead info)
{
	unsigned char* cmd = (unsigned char*)data + sizeof(P2pHead);
	unsigned int cmdLen = len - sizeof(P2pHead);
	CmdRespInfo respInfo;
	respInfo.enable = 0;
	respInfo.len = 0;
	LOGT_print("msgType:0x%x cmdLen:%d", info.msgType, cmdLen);
	
	switch(info.msgType)
	{
	case IOTYPE_USER_IPCAM_VIDEOSTART:
		{
			cmd_video_start(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	case IOTYPE_USER_IPCAM_VIDEOSTOP:
	case TMP_IOTYPE_USER_IPCAM_STOP:
		{
			cmd_video_stop(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	case IOTYPE_USER_IPCAM_AUDIOSTART:
	case TMP_IOTYPE_USER_IPCAM_AUDIOSTART:
		{
			cmd_audio_start(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	case IOTYPE_USER_IPCAM_AUDIOSTOP:
	case TMP_IOTYPE_USER_IPCAM_AUDIOSTOP:
		{
			cmd_audio_stop(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	case IOTYPE_USER_IPCAM_SPEAKERSTART:
		{
			cmd_speak_start(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	case IOTYPE_USER_IPCAM_SPEAKERSTOP:
		{
			cmd_speak_stop(clnt, cmd, cmdLen, &respInfo);
		}
		break;
	default:
		LOGE_print("unsupport msgType:%d", info.msgType);
		break;
	}

	//发送回应信息??
	if(respInfo.enable != 0)
	{
		P2pHead* head = (P2pHead*)data;
		head->msgType += 1;
		head->size = respInfo.len;
		len = head->size + sizeof(P2pHead);
			
		int ret = p2p_conn_data_send(clnt, data, len, P2P_SEND_NONBLOCK);
		if(ret == -2)
		{
			LOGE_print("send return %d connection error",ret);
			clnt->m_Stop = 1;
			//通知连接已经失效
//			m_pServer->NoticfyErrClient(m_id);
		}
	}
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
int cmd_video_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_START clnt:%p", clnt);
	//可以发送视频，并且设定要哪一路
	int ret;
	// 1. 清晰度
	int quality = s_vQualitySingle;//connction_status_get(clnt, VIDEO_QUALITY);
	// 2. 鉴权
	// 3. 时间同步
	// 4. 请求一个I帧 更快显示图像

	connction_status_set(clnt, VIDEO_STATUS, STATUS_START);
	s_vSwitch = STATUS_START;

	//等待I帧发送
	clnt->m_pServer->m_Iwait = 1;

	return 0;
}
int cmd_video_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_STOP");
	connction_status_set(clnt, VIDEO_STATUS, STATUS_STOP);
	s_vSwitch = STATUS_STOP;

	//等待I帧发送
	clnt->m_pServer->m_Iwait = 1;

	return 0;
}
int cmd_audio_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_AUDIOSTART");
	connction_status_set(clnt, AUDIO_STATUS, STATUS_START);
	s_aSwitch = STATUS_START;
	
	return 0;
}
int cmd_audio_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_AUDIOSTOP");
	connction_status_set(clnt, AUDIO_STATUS, STATUS_STOP);
	s_aSwitch = STATUS_STOP;
	
	return 0;
}
int cmd_speak_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_SPEAKERSTART");

	return 0;
}
int cmd_speak_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_SPEAKERSTOP");

	return 0;
}
int cmd_stream_ctrl_get(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_GETSTREAMCTRL_REQ clnt:%p", clnt);

	return 0;
}

int cmd_stream_ctrl_set(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_SETSTREAMCTRL_REQ clnt:%p", clnt);
	
	return 0;
}
int cmd_PTZ_ctrl(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_PTZ_COMMAND");
	
	return 0;
}
int cmd_get_record_file_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_GET_RECORDFILE_START_REQ");
	
	return 0;
}
int cmd_get_record_file_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_GET_RECORDFILE_STOP_REQ");
	
	return 0;
}
int cmd_get_record_file_lock(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_LOCK_RECORDFILE_REQ");

	return 0;
}
int cmd_get_record_file_resend(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_FILE_RESEND_REQ");
	
	return 0;
}

int cmd_play_record(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_PLAY_RECORD_REQ %p", clnt);
	
	return 0;
}

int cmd_stop_play_record(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo)
{
	LOGI_print("IOTYPE_USER_IPCAM_STOP_PLAY_RECORD_REQ %p", clnt);
	
	return 0;
}


int speak_data_send(unsigned char* data, unsigned int lenght)
{
	LOGI_print("data:%p length:%u", data, lenght);

	return 0;
}
int speak_data_handle(p2p_conn_clnt* clnt, void* data, int len,  P2pHead info)
{
	LOGT_print("get speader data,len:%d info:%d",len, info.size);
	
	return 0;
}

