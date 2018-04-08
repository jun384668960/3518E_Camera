#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

#include "p2p_utils.h"
#include "tcp_push.h"
#include "utils_log.h"
#include "utils_common.h"

void on_push_disconnect(void *transport, void* user_data, int status);
void on_push_connect_result(void *transport, void* user_data, int status);
void on_rtmp_event(void *transport, void* user_data, int event);

tcp_push* get_tcp_push_handle();
int tcp_push_connect();
int tcp_push_disconnect();

static void* thread_push_conn(void *pParam);

////////////////////////////////////////////////////////////////
#define MAX_ERROR_STRING_LEN 	256
//#define FILE_DEBUG_W
//#define FILE_DEBUG_R
static tcp_push* s_handle = NULL;

void on_push_disconnect(void *transport, void* user_data, int status)
{
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return;
	
	pHandle->m_status = -1;
	
	LOGI_print("device push connection disconnect, error %d", status); 
	if(pHandle->m_pTransport != NULL)
	{
		gss_dev_push_destroy(pHandle->m_pTransport);
		pHandle->m_pTransport = NULL;
	}
	pHandle->m_rtmpStatus = 2;
}

void on_push_connect_result(void *transport, void* user_data, int status)
{
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return;
		
	if(status == 0)
	{
		LOGI_print("device push connect success! transport:%p",transport);
		pHandle->m_status = 0;
		pHandle->m_pTransport = transport;
	}		
	else
	{
		LOGI_print("failed to device push connect server, error %d", status); 
		gss_dev_push_destroy(transport);
		pHandle->m_pTransport = NULL;
		pHandle->m_status = -1;
		pHandle->m_rtmpStatus = 2;
	}
}

void on_rtmp_event(void *transport, void* user_data, int event)
{
	LOGI_print("on_rtmp_event, event %d", event); 
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return;

	//int event，RTMP事件通知。0 连接成功，1 连接失败， 2 连接断开 3 正在连接。
	pHandle->m_rtmpStatus = event;
}


tcp_push* get_tcp_push_handle()
{
	return s_handle;
}

int tcp_push_connect()
{
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_pTransport)
	{
		LOGW_print("p2p transport already created, destroy it first\r");
		return 0;
	}
	else
	{
		int status = gss_dev_push_connect(&pHandle->m_Cfg, &pHandle->m_pTransport);
		if (status != P2P_SUCCESS) 
		{
			char errmsg[MAX_ERROR_STRING_LEN];
			p2p_strerror(status, errmsg, sizeof(errmsg));
			LOGE_print("create p2p transport failed: %s", errmsg);
			return -1;
		}
		LOGT_print("gss_dev_push_connect pHandle->m_pTransport:%p", pHandle->m_pTransport);
		return 0;
	}
}

int tcp_push_disconnect()
{
	//清除m_Conn
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return -1;
	
	if(!pHandle->m_pTransport)
	{
		LOGE_print("push connection is null");
		return -1;
	}

	LOGI_print("gss_dev_push_destroy m_pTransport:%p", pHandle->m_pTransport);
	if(pHandle->m_pTransport != NULL)
	{
		gss_dev_push_destroy(pHandle->m_pTransport);
		pHandle->m_pTransport = NULL;
	}
	pHandle->m_rtmpStatus = 2;
	
	return 0;
}

static void* thread_push_conn(void *pParam)
{
	tcp_push* pHandle = (tcp_push*)pParam;
	
	while(!pHandle->m_ConnStop)
	{
		//已经连接
		if(pHandle->m_status == 0) 
		{
			usleep(5000*1000);
			continue;
		}

		tcp_push_disconnect();
		int ret = tcp_push_connect();
		if(ret != 0){
			LOGE_print(" connect_turn_server error");
		}

		usleep(5000*1000);
	}
	
	return NULL;
}

p2p_handle* tcp_push_create(char* rtmp)
{
	tcp_push* handle = (tcp_push*)malloc(sizeof(tcp_push));
	memset(handle, 0x0, sizeof(tcp_push));
	
	handle->base.m_type = HANDLE_TYPE_TCP_PUSH;
	handle->base.m_Iwait = 1;
	memset(&handle->m_Cfg, 0, sizeof(handle->m_Cfg));
	memset(&handle->m_Cb, 0, sizeof(handle->m_Cb));
	handle->m_pTransport = NULL;

	handle->m_status = -1;
	handle->m_rtmpStatus = -1;

	memset(handle->m_RtmpUrl, 0, 256);
	
	if(rtmp != NULL && strlen(rtmp) > 0)
	{
		LOGI_print("push rtmp mode url:%s", rtmp);
		strncpy(handle->m_RtmpUrl, rtmp, strlen(rtmp));
		handle->m_RtmpUrl[strlen(rtmp)] = '\0';
		handle->m_rtmpStatus = 2;
		handle->base.m_type = HANDLE_TYPE_TCP_RTMP;
	}

	handle->m_enAutio = 0;
	handle->m_enVideo = 1;
	handle->m_vQuality = 1;
	
	s_handle = handle;
	
	return (p2p_handle*)handle;
}

void tcp_push_destory(tcp_push* pHandle)
{
	free(pHandle);
}

int tcp_push_init(tcp_push* pHandle, char* dispathce_server, char* server, unsigned short port, char* uid)
{
	LOGT_print("server:%s port:%d uid:%s", server, port, uid);
	if(dispathce_server == NULL || server == NULL || uid == NULL)
	{
		LOGE_print("error, invaid params");
		return -1;
	}

	if((strlen(dispathce_server) <=0 && strlen(server) <= 0) || strlen(uid) <= 0)
	{
		LOGE_print("error, invaid params");
		return -1;
	}

	//TODO 格式检测

	if(pHandle == NULL) return -1;
	
	int ret;

	pHandle->m_Cb.on_disconnect = on_push_disconnect;
	pHandle->m_Cb.on_connect_result = on_push_connect_result;
	pHandle->m_Cb.on_rtmp_event = on_rtmp_event;
	pHandle->m_Cfg.cb = &pHandle->m_Cb;

	strncpy(pHandle->base.m_CfgServer, server, strlen(server));
	pHandle->base.m_CfgServer[strlen(server)] = '\0';
	strncpy(pHandle->base.m_CfgUser, uid, strlen(uid));
	pHandle->base.m_CfgUser[strlen(uid)] = '\0';
	pHandle->base.m_CfgServerPort = port;

	pHandle->m_Cfg.server = pHandle->base.m_CfgServer;
	pHandle->m_Cfg.port = port;
	pHandle->m_Cfg.uid = pHandle->base.m_CfgUser;//TODO 读取配置 获取UID
	pHandle->m_Cfg.user_data = NULL;

	pHandle->m_ConnStop = 0;
	ret = pthread_create_4m(&pHandle->m_pThreadConn, thread_push_conn, pHandle);
	if(ret != 0 ){
		LOGE_print(" create thread_turn_conn error:%s", strerror(ret));
		p2p_uninit();
		return -1;
	}

	return 0;
}

int tcp_push_uninit(tcp_push* pHandle)
{
	if(pHandle->m_pThreadConn)
	{
		pHandle->m_ConnStop = 1;
		pthread_join(pHandle->m_pThreadConn, NULL);
	}

	tcp_push_disconnect();
	
	return 0;
}


int tcp_push_av_write(tcp_push* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	if(pHandle == NULL) return -1;
	if(pHandle->m_status != 0) return -2; 	//服务尚未链接成功
	if(data == NULL || len == 0 || len > 250*1024) return -1; //脏数据
	
	int is_audio = 0;
	int ret = -1;
	if(tcp_push_av_swtich(type) != 0)
	{
		return -1;
	}
	
	if(type == SND_AUDIO) is_audio = 1;
	if(type == SND_VIDEO_HD || type == SND_VIDEO_LD)
	{
		type = SND_VIDEO;
	}
	
	if(strlen(pHandle->m_RtmpUrl) > 0)
	{
		if(pHandle->m_rtmpStatus == 1 || pHandle->m_rtmpStatus == 2)
		{
			if(type == SND_VIDEO && is_key == 1)
			{
				LOGT_print("==== %d ==== %p %s", __LINE__, pHandle->m_pTransport, pHandle->m_RtmpUrl);
				gss_dev_push_rtmp (pHandle->m_pTransport, pHandle->m_RtmpUrl);
				pHandle->m_rtmpStatus = 3;
			}
		}
		
		if(pHandle->m_rtmpStatus == 0)
		{
			//等待I帧
			if(type == SND_VIDEO){
				if(is_key == 0 && pHandle->base.m_Iwait == 1)
					return -3;
				if(is_key == 1 && pHandle->base.m_Iwait == 1){
					LOGT_print("wait i frame done");
					pHandle->base.m_Iwait = 0;
				}
			}else if(type == SND_AUDIO){
				if(pHandle->base.m_Iwait == 1){
					return -2;
				}
			}
	
//			LOGT_print("gss_dev_push_send data[0]:%x data[1]:%x data[2]:%x data[3]:%x data[4]:%x is_audio:%d, time_stamp:%u, is_key:%d len:%d "
//				, data[0], data[1], data[2], data[3], data[4], is_audio, time_stamp, is_key, len);
			ret = gss_dev_push_send(pHandle->m_pTransport, (char*)data, len, is_audio, time_stamp, is_key, P2P_SEND_NONBLOCK);
		}
	}
	else
	{
		//等待I帧
		if(type == SND_VIDEO){
			if(is_key == 0 && pHandle->base.m_Iwait == 1)
				return -3;
			if(is_key == 1 && pHandle->base.m_Iwait == 1){
				LOGT_print("wait i frame done");
				pHandle->base.m_Iwait = 0;
			}
		}else if(type == SND_AUDIO){
			if(pHandle->base.m_Iwait == 1){
				return -2;
			}
		}
		
		P2pHead head = private_head_format(len, type, 0, 1);
		unsigned char frame[250*1024];
		memset(frame, 0, 250*1024);
		memcpy(frame, &head, sizeof(P2pHead));
		memcpy(frame + sizeof(P2pHead), data, len);
			
//		LOGT_print("gss_dev_push_send is_audio:%d is_key:%d time_stamp:%u len:%d "
//				, is_audio, is_key, time_stamp, len);
		ret = gss_dev_push_send(pHandle->m_pTransport, (char*)frame, len+sizeof(P2pHead), is_audio, time_stamp, is_key, P2P_SEND_NONBLOCK);
	}

	//I帧丢失
	if(ret != 0 && is_key == 1 && type == SND_VIDEO)
	{	
		LOGE_print("send i frame error, skip flow p/b frame ret:%d",ret);
		pHandle->base.m_Iwait = 1;
	}
	
	return ret;
}

int tcp_push_av_swtich(int type)
{
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_status == 0)
	{
		if(type == SND_AUDIO && pHandle->m_enAutio == STATUS_STOP)
		{
//			LOGW_print("type:0x%x m_enAutio:%d",type, STATUS_STOP);
			return -1;
		}
		if((type == SND_VIDEO_HD || type == SND_VIDEO_LD) && pHandle->m_enVideo == STATUS_STOP)
		{
//			LOGW_print("type:0x%x m_enVideo:%d",type, STATUS_STOP);
			return -1;
		}
	 	else if(type == SND_VIDEO_HD && pHandle->m_vQuality == 1)
		{
//			LOGW_print("type:0x%x m_vQuality:1",type);
			return -1;
		}
		else if(type == SND_VIDEO_LD && pHandle->m_vQuality == 0)
		{
//			LOGW_print("type:0x%x m_vQuality:0",type);
			return -1;
		}
	
		return 0;
	}
	else
	{
		return -1;
	}
}

int tcp_push_param_set(int type, char* val)
{
	if(val == NULL) return -1;
	
	tcp_push* pHandle = get_tcp_push_handle();
	if(pHandle == NULL) return -1;
	
	switch(type){
		case ENABLE_AUDIO:
		{
			LOGI_print("ENABLE_AUDIO val:%s", val);
			pHandle->m_enAutio = atoi(val);
			break;
		}
		case ENABLE_VIDEO:
		{
			LOGI_print("ENABLE_VIDEO val:%s", val);
			pHandle->m_enVideo = atoi(val);
			pHandle->base.m_Iwait = 1;
			break;
		}
		case ENABLE_VQUALITY:
		{
			LOGI_print("ENABLE_VQUALITY val:%s", val);
			pHandle->m_vQuality = atoi(val);
			if(pHandle->m_enVideo == 1)
				pHandle->base.m_Iwait = 1;
			
			break;
		}
		case SET_1VN_RTMP_RUL:
		{
			LOGI_print("SET_1VN_RTMP_RUL val:%s", val);
			//开启rtmp模式 并且设置url
			if(strlen(val) != 0)
			{
				strncpy(pHandle->m_RtmpUrl, val, strlen(val));
				pHandle->m_RtmpUrl[strlen(val)] = '\0';
				pHandle->m_rtmpStatus = 2;
				pHandle->base.m_type = HANDLE_TYPE_TCP_RTMP;
			}
			else
			{
				pHandle->base.m_type = HANDLE_TYPE_TCP_PUSH;
			}
			break;
		}
		default:
			break;
	}
	return 0;
}

