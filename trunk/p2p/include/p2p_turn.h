#ifndef P2P_TURN_H
#define P2P_TURN_H

#include <pthread.h>
#include "p2p_handle.h"

typedef struct p2p_trans_s
{
	p2p_handle				base;
	
	p2p_transport_cb	m_Cb;
	p2p_transport_cfg	m_Cfg;
	p2p_transport*		m_pTransport;

	cmap				m_ConnClients;
	CMtx				m_mtxConnClients;
	int 				m_status;

	int 				m_Stop;
	pthread_t			m_pThread;//线程
	CMtx				m_mtxWaitData;
	cqueue				m_lstData;
	CSem 				m_semWaitData;

	cqueue				m_lstErrClnt;
	CMtx				m_mtxErrClnt;
	FILE*				m_pFdW;
	FILE*				m_pFdR;

	pthread_t			m_pThreadDispatch;//线程
	pthread_t			m_pThreadConn;
	int 				m_DispatchStop;
	int 				m_ConnStop;
	int 				m_Dispatched;
	void*				m_Dispatcher;
	char				m_DispatchServer[128];
}p2p_trans;

int p2p_trans_iwait();
int p2p_trans_send(unsigned char* data, unsigned int len, int id, p2p_send_model mode);
int p2p_trans_clearcache(int id);
int p2p_trans_av_swtich(int type);
int p2p_trans_param_set(int type, char* val);

p2p_handle* p2p_trans_create();
void p2p_trans_destory(p2p_trans* handle);

int p2p_trans_init(p2p_trans* handle, char* dispathce_server, char* server, unsigned short port, char* uid, char* pwd, int tcp);
int p2p_trans_uninit(p2p_trans* handle);
int p2p_trans_write(p2p_trans* handle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);

#endif
