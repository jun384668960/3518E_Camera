#ifndef _TCP_TURN_
#define _TCP_TURN_

#include <pthread.h>
#include "gss_transport.h"
#include "p2p_handle.h"

typedef struct tcp_turn_s
{
	p2p_handle				base;
	
	gss_dev_main_cfg 		m_Cfg;
	gss_dev_main_cb 		m_Cb;
	void*					m_pTransport;

	cmap					m_SigClients;
	CMtx					m_mtxSigClients;
	cmap					m_AvClients;
	CMtx					m_mtxAvClients;
	int 					m_status;

	//内部缓存后进行发送使用
	int 					m_Stop;
	pthread_t				m_pThread;//线程
	CMtx					m_mtxWaitData;
	cqueue					m_lstData;

	cqueue					m_lstErrClnt;
	CMtx					m_mtxErrClnt;
	
	FILE*					m_pFdW;
	FILE*					m_pFdR;

	pthread_t				m_pThreadDispatch;//线程
	pthread_t				m_pThreadMainConn;
	int 					m_DispatchStop;
	int 					m_MainConnStop;
	int 					m_Dispatched;
	void*					m_Dispatcher;
	char					m_DispatchServer[128];

	int						m_enablepush;
	p2p_handle*				m_pusher;
}tcp_turn;

p2p_handle* tcp_turn_create();
void tcp_turn_destory(tcp_turn* handle);

int tcp_turn_init(tcp_turn* handle, char* dispathce_server, char* server, unsigned short port, char* uid);
int tcp_turn_uninit(tcp_turn* handle);
//tcp turn 转发
int tcp_turn_av_write(tcp_turn* handle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);
int tcp_turn_send(void* transport, int id, unsigned char* data, unsigned int len, p2p_send_model mode);
int tcp_turn_clearcache();
int tcp_turn_av_swtich(int type);
int tcp_turn_param_set(int type, char* val);

#endif