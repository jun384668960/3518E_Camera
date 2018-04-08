#ifndef _TCP_PUSH_H_
#define _TCP_PUSH_H_

#include <pthread.h>
#include "gss_transport.h"
#include "p2p_handle.h"

typedef struct tcp_push_s
{
	p2p_handle				base;
	
	gss_dev_push_conn_cfg 	m_Cfg;
	gss_dev_push_cb 		m_Cb;
	void*					m_pTransport;
	char					m_RtmpUrl[256];
	int						m_rtmpStatus;
	
	int 					m_status;
	
	FILE*					m_pFdW;
	FILE*					m_pFdR;

	pthread_t				m_pThreadConn;
	int 					m_ConnStop;

	int						m_enAutio;
	int						m_enVideo;
	int 					m_vQuality;
}tcp_push;

p2p_handle* tcp_push_create(char* rtmp);
void tcp_push_destory(tcp_push* handle);

int tcp_push_init(tcp_push* handle, char* dispathce_server, char* server, unsigned short port, char* uid);
int tcp_push_uninit(tcp_push* handle);
//只支持h264+aac裸流
int tcp_push_av_write(tcp_push* handle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);
int tcp_push_av_swtich(int type);
int tcp_push_param_set(int type, char* val);

#endif