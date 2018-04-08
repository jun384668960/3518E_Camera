#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

#include "p2p_define.h"
#include "p2p_utils.h"
#include "tcp_turn.h"
#include "tcp_push.h"
#include "p2p_conn_client.h"
#include "utils_log.h"
#include "utils_common.h"

void on_connect_result(void *transport, void* user_data, int status);
void on_disconnect(void *transport, void* user_data, int status);
void on_accept_signaling_client(void *transport, void* user_data, unsigned short client_conn);
void on_disconnect_signaling_client(void *transport, void* user_data, unsigned short client_conn);  
void on_recv_av_request(void *transport, void* user_data, unsigned int client_conn); 
void on_recv(void *transport, void *user_data, unsigned short client_conn, char* data, int len);

void on_av_connect_result(void *transport, void* user_data, int status);
void on_av_disconnect(void *transport, void* user_data, int status);
void on_av_client_disconnect(void *transport, void *user_data);
void on_av_recv(void *transport, void *user_data, char* data, int len);
void on_tcp_turn_dispatch(void* dispatcher, 
					int status, 
					void* user_data, 
					char* server, 
					unsigned short port, 
					unsigned int server_id);

void* thread_sigconn_del(void* param);

int turn_main_connect();
int turn_main_disconnect();
int turn_av_connect(unsigned int client_conn);

void* thread_tcp_turn_dispatch(void *pParam);
static void* thread_main_conn(void *pParam);
int tcp_turn_dispatch(void** dispatcher, char* user, char* password, char* ds_addr, char* user_data);

int tcp_turn_1v1_av_swtich(int type);
int tcp_turn_1v1_write(tcp_turn* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);
int tcp_turn_1vn_write(tcp_turn* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);
int tcp_turn_data_dispatch(MassData* pData);

tcp_turn* get_tcp_turn_handle();
////////////////////////////////////////////////////////////////
#define MAX_ERROR_STRING_LEN 	256
//#define FILE_DEBUG_W
//#define FILE_DEBUG_R
static tcp_turn* s_handle = NULL;

void on_connect_result(void *transport, void* user_data, int status)
{
	LOGI_print("on_connect_result status:%d transport:%p", status, transport);	
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;
	
	if(status == 0)				
	{
		LOGI_print("device main connect success!");	
		pHandle->m_status = 0;

		//一对多对象初始化
		if(pHandle->m_pusher != NULL){
			tcp_push_init((tcp_push*)pHandle->m_pusher, pHandle->m_DispatchServer, pHandle->base.m_CfgServer, pHandle->base.m_CfgServerPort, pHandle->base.m_CfgUser);
		}
	}
	else
	{
		LOGI_print("failed to device main connect server, error %d", status); 
		pHandle->m_status = -1;
	}
}

void on_disconnect(void *transport, void* user_data, int status)
{
	LOGI_print("device main disconnect, error %d transport:%p", status, transport); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	pHandle->m_status = -1;

	//清空av连接数
	int i;
	cmtx_enter(pHandle->m_mtxAvClients);
	int size = cmap_size(&(pHandle->m_AvClients));
	for(i=0; i<size; i++)
	{
		cmapnode* node = cmap_index_get(&(pHandle->m_AvClients), i);
		if(node)
		{
			p2p_conn_clnt* clnt = (p2p_conn_clnt*)(node->data);
			if(clnt) 
			{
				p2p_conn_clnt_destory(clnt);
				node->data = NULL;
			}
		}
	}
	cmap_clear(&(pHandle->m_AvClients));
	cmtx_leave(pHandle->m_mtxAvClients);
	
	//清除已经连接的client
	cmtx_enter(pHandle->m_mtxSigClients);
	size = cmap_size(&(pHandle->m_SigClients));
	for(i=0; i<size; i++)
	{
		cmapnode* node = cmap_index_get(&(pHandle->m_SigClients), i);
		if(node)
		{
			p2p_conn_clnt* clnt = (p2p_conn_clnt*)(node->data);
			if(clnt) 
			{
				p2p_conn_clnt_destory(clnt);
				node->data = NULL;
			}
		}
	}
	cmap_clear(&(pHandle->m_SigClients));
	cmtx_leave(pHandle->m_mtxSigClients);

	//删除一对多
	if(pHandle->m_pusher != NULL){
		tcp_push_uninit((tcp_push*)pHandle->m_pusher);
	}
	
	LOGI_print("p2p disconnect server done\r");

	turn_main_disconnect();
	//重新请求分派服务器 -- 启用线程更合适
//	pHandle->m_Dispatched = 0;
}

void on_accept_signaling_client(void *transport, void* user_data, unsigned short client_conn)
{
	LOGI_print("device signaling accept client, client index %d", client_conn); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	exec_cmd("sh /opt/ipnc/clearCached.sh&");
	//最大连接数控制
	int size = cmap_size(&pHandle->m_SigClients);
	if( size > 5)
	{
		LOGE_print("m_SigClients:%d > 5", size); 
		return;
	}
	p2p_conn_clnt* client = p2p_conn_clnt_create((p2p_handle*)pHandle, client_conn, transport);
	if(client != NULL/*p2p_conn_clnt_prepare(client) == 0*/)
	{
		cmtx_enter(pHandle->m_mtxSigClients);
		int ret = cmap_insert(&(pHandle->m_SigClients), client_conn, client);
		cmtx_leave(pHandle->m_mtxSigClients);
		if(ret != 0)
		{
			LOGE_print("m_SigClients insert id:%d client:%p error", client_conn,client);
			return;
		}
		LOGI_print("m_SigClients insert id:%d client:%p success,  sigclients:%d ", client_conn, client, cmap_size(&(pHandle->m_SigClients)));
	}
	else
	{
//		p2p_conn_clnt_destory(client);
	}
}

void on_disconnect_signaling_client(void *transport, void* user_data, unsigned short client_conn)
{
	LOGI_print("device main signaling client, client index %d transport:%p", client_conn, transport); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	pthread_t  pid;

	cmtx_enter(pHandle->m_mtxSigClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_SigClients), client_conn);

	if(client == NULL)
	{
		LOGE_print("cmap_find id:%d error", client_conn);
	}
	else
	{
		LOGI_print("cmap_find id:%d client:%p", client_conn, client);
		cmap_erase(&(pHandle->m_SigClients), client_conn);
	}
	
	LOGI_print("current sigclients:%d", cmap_size(&(pHandle->m_SigClients)));

	cmtx_leave(pHandle->m_mtxSigClients);

	if(client)
		pthread_create_4m(&pid, thread_sigconn_del, client);
}

void on_recv_av_request(void *transport, void* user_data, unsigned int client_conn)
{
	LOGI_print("on_recv_av_request, client index %d transport:%p", client_conn, transport); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;
	
	turn_av_connect(client_conn);
}

void on_recv(void *transport, void *user_data, unsigned short client_conn, char* data, int len)
{
	LOGI_print("device main receive date, client index %d, length %d transport:%p", client_conn, len, transport); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	cmtx_enter(pHandle->m_mtxSigClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_SigClients), client_conn);

	if(client == NULL)
	{
		cmtx_leave(pHandle->m_mtxSigClients);
		LOGE_print("cmap_find id:%d error", client_conn);
	}
	else
	{
		cmtx_leave(pHandle->m_mtxSigClients);
		p2p_conn_clnt_write(client, (unsigned char*)data,len);
	}
}

void on_av_connect_result(void *transport, void* user_data, int status)
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	exec_cmd("sh /opt/ipnc/clearCached.sh&");
	
	if(status == 0)	
	{		
		LOGI_print("device av connect success!");
		//最大连接数控制
		int size = cmap_size(&pHandle->m_AvClients);
		if( size > 5)
		{
			LOGE_print("m_AvClients:%d > 5", size); 
			return;
		}
		
		int key = (int)transport;
		LOGI_print("key:%d", key);
		p2p_conn_clnt* client = p2p_conn_clnt_create((p2p_handle*)pHandle, 0, transport);
		if(client != NULL/*p2p_conn_clnt_prepare(client) == 0*/)
		{
			cmtx_enter(pHandle->m_mtxAvClients);
			int ret = cmap_insert(&(pHandle->m_AvClients), key, client);
			cmtx_leave(pHandle->m_mtxAvClients);
			if(ret != 0)
			{
				LOGE_print("m_AvClients insert key:%d client:%p error", key,client);
				return;
			}
			LOGI_print("m_AvClients insert transport:%p key:%d client:%p success,  avclients:%d ", transport, key, client, cmap_size(&(pHandle->m_AvClients)));
		}
		else
		{
//			p2p_conn_clnt_destory(client);
			gss_dev_av_destroy(transport);
		}
	}	
	else	
	{		
		LOGE_print("%p failed to device av connect server, error %d", transport, status); 		
		gss_dev_av_destroy(transport);
	}
}

void on_av_disconnect(void *transport, void* user_data, int status)
{
	LOGI_print("device av connection disconnect, error %d transport:%p", status, transport);	
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	pthread_t  pid;
	int key = (int)transport;
	
	LOGI_print("key:%d", key);
	cmtx_enter(pHandle->m_mtxAvClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_AvClients), key);
	if(client == NULL)
	{
		LOGE_print("cmap_find id:%d error", key);
	}
	else
	{
		LOGI_print("cmap_find id:%d client:%p", key, client);
		cmap_erase(&(pHandle->m_AvClients), key);
	}
	
	LOGI_print("current avclients:%d", cmap_size(&(pHandle->m_AvClients)));
	cmtx_leave(pHandle->m_mtxAvClients);

	if(client)
		pthread_create_4m(&pid, thread_sigconn_del, client);
	
	gss_dev_av_destroy(transport);
}

void on_av_client_disconnect(void *transport, void *user_data)
{
	LOGI_print("device av connection client disconnect transport:%p", transport);	
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	pthread_t  pid;
	int key = (int)transport;
	LOGI_print("key:%d", key);
	
	cmtx_enter(pHandle->m_mtxAvClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_AvClients), key);
	if(client == NULL)
	{
		LOGE_print("cmap_find id:%d error", key);
	}
	else
	{
		LOGI_print("cmap_find id:%d client:%p", key, client);
		cmap_erase(&(pHandle->m_AvClients), key);
	}
	
	LOGI_print("current avclients:%d", cmap_size(&(pHandle->m_AvClients)));
	cmtx_leave(pHandle->m_mtxAvClients);

	if(client)
		pthread_create_4m(&pid, thread_sigconn_del, client);
	
	gss_dev_av_destroy(transport);
}

void on_av_recv(void *transport, void *user_data, char* data, int len)
{
	LOGI_print("device av connection receive date, length %d transport:%p", len, transport); 
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	int key = (int)transport;
	cmtx_enter(pHandle->m_mtxAvClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&pHandle->m_AvClients, key);

	if(client == NULL)
	{
		cmtx_leave(pHandle->m_mtxAvClients);
		LOGE_print("cmap_find key:%d error", key);
	}
	else
	{
		cmtx_leave(pHandle->m_mtxAvClients);
		p2p_conn_clnt_write(client, (unsigned char*)data, len);
	}
}

void on_tcp_turn_dispatch(void* dispatcher, 
					int status, 
					void* user_data, 
					char* server, 
					unsigned short port, 
					unsigned int server_id)
{
	LOGI_print("dispatcher:%p status:%d", dispatcher, status);
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return;

	if(status == 0)
	{
		pHandle->m_Dispatched = 1;
		LOGI_print("server:%s port:%d", server, port);
		
		//修改m_Cfg
		pHandle->m_Cfg.server = server;
		pHandle->m_Cfg.port = port;
		pHandle->m_Cfg.user_data = user_data;

		strncpy(pHandle->base.m_CfgServer, server, strlen(server));
		pHandle->base.m_CfgServer[strlen(server)] = '\0';
		pHandle->base.m_CfgServerPort = port;
		
		turn_main_disconnect();
		int ret = turn_main_connect();
		if(ret != 0){
			LOGE_print(" connect_turn_server error");
		}
	}
	else
	{
//		DisConnectServer();
//		pHandle->m_Dispatched = false;
	}
	destroy_gss_dispatch_requester(dispatcher);
	pHandle->m_Dispatcher = NULL;
}

void* thread_sigconn_del(void* param)
{
	pthread_detach(pthread_self());
	p2p_conn_clnt* client = (p2p_conn_clnt*)param;
	if(client) p2p_conn_clnt_destory(client);
	
	LOGI_print("client ThreadClientDel sucess!");

	return NULL;
}

int turn_main_connect()
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_pTransport)
	{
		LOGW_print("p2p transport already created, destroy it first\r");
		return 0;
	}
	else
	{
		int status = gss_dev_main_connect(&pHandle->m_Cfg, &pHandle->m_pTransport);
		if (status != P2P_SUCCESS) 
		{
			char errmsg[MAX_ERROR_STRING_LEN];
			p2p_strerror(status, errmsg, sizeof(errmsg));
			LOGE_print("create p2p transport failed: %s\r", errmsg);
			return -1;
		}
		return 0;
	}
}

int turn_main_disconnect()
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	if(!pHandle->m_pTransport)
	{
		LOGE_print("main connection is null");
		return -1;
	}
	gss_dev_main_destroy(pHandle->m_pTransport);
	pHandle->m_pTransport = NULL;
	
	return 0;
}

int turn_av_connect(unsigned int client_conn)
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	int result;
	void* avConn;
	gss_dev_av_cfg cfg;
	gss_dev_av_cb cb;

	cfg.server = pHandle->base.m_CfgServer;
	cfg.port = pHandle->base.m_CfgServerPort;
	cfg.uid = pHandle->base.m_CfgUser;
	cfg.user_data = NULL;
	cfg.client_conn = client_conn;
	cfg.cb = &cb;

	cb.on_recv = on_av_recv;
	cb.on_disconnect = on_av_disconnect;
	cb.on_connect_result= on_av_connect_result;
	cb.on_client_disconnect= on_av_client_disconnect;

	result = gss_dev_av_connect(&cfg, &avConn); 
	if(result != 0)
		LOGE_print("client_av_connect_server return %d", result);
	LOGI_print("gss_dev_av_connect client_conn:%d avConn %p", client_conn, avConn);
	
	return 0;
}

void* thread_tcp_turn_dispatch(void *pParam)
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	
	while(!pHandle->m_DispatchStop)
	{
		//已经连接
		if(pHandle->m_Dispatched && pHandle->m_status == 0) 
		{
			usleep(5000*1000);
			continue;
		}

		//TODO 读取配置 获取分派服务器域名/地址
		LOGI_print("====================================");
		LOGI_print("do TurnDispatch:%p uid:%s DispatchServer:%s"
				,pHandle->m_Dispatcher, pHandle->m_Cfg.uid, pHandle->m_DispatchServer);
		
		if(pHandle->m_Dispatcher != NULL) destroy_gss_dispatch_requester(pHandle->m_Dispatcher);
		int ret = tcp_turn_dispatch(&(pHandle->m_Dispatcher), pHandle->m_Cfg.uid, "", pHandle->m_DispatchServer, NULL);
		if(ret != 0 ){
			LOGE_print(" turn_dispatch error");
		}

		usleep(5000*1000);
	}
	return NULL;
}

static void* thread_main_conn(void *pParam)
{
	tcp_turn* pHandle = (tcp_turn*)pParam;
	
	while(!pHandle->m_MainConnStop)
	{
		//已经连接
		if(pHandle->m_status == 0) 
		{
			usleep(10000*1000);
			continue;
		}

		turn_main_disconnect();
		int ret = turn_main_connect();
		if(ret != 0){
			LOGE_print(" connect_turn_server error");
		}

		usleep(10000*1000);
	}

	return NULL;
}

int tcp_turn_dispatch(void** dispatcher, char* user, char* password, char* ds_addr, char* user_data)
{
	int ret = gss_request_dispatch_server(
				  user/*char* user*/, 
				  password/*char* password*/, 
				  ds_addr/*char* ds_addr*/, 
				  user_data/*void* user_data*/, 
				  on_tcp_turn_dispatch,
				  dispatcher/*void** dispatcher*/);
	if(ret)
	{
		LOGE_print("gss_request_dispatch_server error:%d", ret);
		return -1;
	}

	LOGT_print("gss_request_dispatch_server done"); 
	return 0;
}

int tcp_turn_1v1_av_swtich(int type)
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	int av_count = 0;
	cmtx_enter(pHandle->m_mtxAvClients);
	av_count = cmap_size(&pHandle->m_AvClients);
	cmtx_leave(pHandle->m_mtxAvClients);

	if(av_count == 0)
	{
		return -1;
	}

	if(type == SND_AUDIO && p2p_conn_clnt_aswitch_get() == STATUS_STOP)
	{
//		LOGW_print("type:0x%x m_aSwitch:%d",type, STATUS_STOP);
		return -1;
	}
	if((type == SND_VIDEO_HD || type == SND_VIDEO_LD) && p2p_conn_clnt_vswitch_get() == STATUS_STOP)
	{
//		LOGW_print("type:0x%x m_vSwitch:%d",type, STATUS_STOP);
		return -1;
	}
	else if(type == SND_VIDEO_HD && p2p_conn_clnt_vquality_get() == 1)
	{
//		LOGW_print("type:0x%x m_vQualitySingle:1",type);
		return -1;
	}
	else if(type == SND_VIDEO_LD && p2p_conn_clnt_vquality_get() == 0)
	{
//		LOGW_print("type:0x%x m_vQualitySingle:0",type);
		return -1;
	}

	return 0;
}

int tcp_turn_1v1_write(tcp_turn* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	//是否订阅
	if(tcp_turn_1v1_av_swtich(type) != 0)
	{
		return -1;
	}
	
	if(type == SND_VIDEO_HD || type == SND_VIDEO_LD)
	{
		type = SND_VIDEO;
	}
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
	
	MassData mass;
	mass.type = type;
	mass.len = len + sizeof(P2pHead);
	mass.body = frame;

	int ret = tcp_turn_data_dispatch(&mass);
	if(ret != 0 && is_key == 1 && type == SND_VIDEO)
	{	
		LOGE_print("send i frame error, skip flow p/b frame");
		pHandle->base.m_Iwait = 1;
	}

	return 0;
}

int tcp_turn_1vn_write(tcp_turn* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	if(pHandle->m_pusher != NULL && pHandle->m_enablepush == 1)
	{
		if(pHandle->m_pusher->m_type == HANDLE_TYPE_TCP_RTMP)
		{
			if(type == SND_VIDEO_HD || type == SND_VIDEO_LD)
			{
				tcp_push_av_write((tcp_push*)pHandle->m_pusher, data, len, type, time_stamp, is_key);
			}
		}
		else
		{
			tcp_push_av_write((tcp_push*)pHandle->m_pusher, data, len, type, time_stamp, is_key);
		}
	}

	return 0;
}

int tcp_turn_data_dispatch(MassData* pData)
{
	int i;
	int ret = 0;
	cqueue mlstClnt;

	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	if(pData->type == SND_VIDEO || pData->type == SND_AUDIO || pData->type == SND_NOTIC)
	{
		cqueue_init(&mlstClnt);
		cmtx_enter(pHandle->m_mtxAvClients);
		int size = cmap_size(&pHandle->m_AvClients);
		for(i=0; i<size; i++)
		{
			cmapnode* mnode = cmap_index_get(&pHandle->m_AvClients, i);
			if(mnode == NULL) continue;
			p2p_conn_clnt* client = (p2p_conn_clnt*)(mnode->data);
			if(client == NULL) continue;
			int clntId = mnode->key;

			//NOTIC. m_mtxConnClients.Leave(); as soon as possible
			if((pData->type == SND_VIDEO && client->m_vstatus == STATUS_START)
				|| (pData->type == SND_AUDIO && client->m_astatus == STATUS_START)
				|| (pData->type == SND_NOTIC))
			{
//				LOGT_print("send data.type:%x", pData->type);
				clnt_node* qnode = (clnt_node*)malloc(sizeof(clnt_node));
				qnode->id = clntId;
				cqueue_enqueue(&mlstClnt, qnode);
			}
		}
		cmtx_leave(pHandle->m_mtxAvClients);

		//分发
		while(cqueue_is_empty(&mlstClnt) == 0)
		{
			clnt_node* qnode = (clnt_node*)cqueue_dequeue(&mlstClnt);
			int id = qnode->id;
			
//			static FILE* mp4 = NULL;
//			if(mp4 == NULL) mp4 = fopen("/mnt/root/send.mp4","wb");
//			if(mp4) fwrite(data + sizeof(P2pHead), 1, head->size, mp4);
			LOGT_print("gss_dev_av_send, id:0x%x pData->len:%d",id, pData->len);
			ret = gss_dev_av_send((void*)id, (char*)pData->body, pData->len, P2P_SEND_NONBLOCK, GSS_REALPLAY_DATA);
			if(ret != 0)
			{
				LOGE_print("gss_dev_av_send error, ret:%d",ret);
			}
			free(qnode);
		}

		cqueue_destory(&mlstClnt);	
	}

	return ret;
}

tcp_turn* get_tcp_turn_handle()
{
	return s_handle;
}

p2p_handle* tcp_turn_create()
{
	tcp_turn* handle = (tcp_turn*)malloc(sizeof(tcp_turn));
	memset(handle, 0x0, sizeof(tcp_turn));

	handle->base.m_type = HANDLE_TYPE_TCP;
	handle->base.m_Iwait = 1;
	memset(&handle->m_Cfg, 0, sizeof(handle->m_Cfg));
	memset(&handle->m_Cb, 0, sizeof(handle->m_Cb));
	handle->m_pTransport = NULL;

	handle->m_status = -1;
	handle->m_Stop = 0;
	handle->m_pThread = 0;//线程
	handle->m_pFdW = NULL;
	handle->m_pFdR = NULL;

	handle->m_pThreadDispatch = 0;//线程
	handle->m_pThreadMainConn = 0;
	handle->m_DispatchStop = 0;
	handle->m_MainConnStop = 0;;
	handle->m_Dispatched = 0;
	handle->m_Dispatcher = NULL;
	memset(&handle->m_DispatchServer, 0, 128);

	//创建一对多对象
	handle->m_pusher = tcp_push_create("");
	handle->m_enablepush = 0;
	
#ifdef FILE_DEBUG_W
	handle->m_pFdW = fopen("/opt/httpServer/lighttpd/htdocs/sd/handlew.media","wb");
#endif
#ifdef FILE_DEBUG_R
	handle->m_pFdR = fopen("/opt/httpServer/lighttpd/htdocs/sd/handler.media","wb");
#endif

	handle->m_mtxSigClients = cmtx_create();
	handle->m_mtxAvClients = cmtx_create();
	handle->m_mtxWaitData = cmtx_create();
	handle->m_mtxErrClnt = cmtx_create();

	cqueue_init(&handle->m_lstData);	
	cqueue_init(&handle->m_lstErrClnt);	
	cmap_init(&handle->m_SigClients);
	cmap_init(&handle->m_AvClients);

	s_handle = handle;

	return (p2p_handle*)handle;
}

void tcp_turn_destory(tcp_turn* handle)
{
#ifdef FILE_DEBUG_W
	fclose(handle->m_pFdW);
#endif
#ifdef FILE_DEBUG_R
	fclose(handle->m_pFdR);
#endif

	if(handle->m_pusher){
		tcp_push_destory((tcp_push*)handle->m_pusher);
	}
	
	cqueue_destory(&handle->m_lstData);
	cqueue_destory(&handle->m_lstErrClnt);
	cmap_destory(&handle->m_SigClients);
	cmap_destory(&handle->m_AvClients);
	
	cmtx_delete(handle->m_mtxSigClients);
	cmtx_delete(handle->m_mtxAvClients);
	cmtx_delete(handle->m_mtxWaitData);
	cmtx_delete(handle->m_mtxErrClnt);
	
	free(handle);
	s_handle = NULL;
}

int tcp_turn_init(tcp_turn* pHandle, char* dispathce_server, char* server, unsigned short port, char* uid)
{
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
	LOGT_print("dispathce_server:%s server:%s port:%d uid:%s", dispathce_server, server, port, uid);

	//TODO 格式检测
	if(pHandle == NULL) return -1;
	
	int ret;
	
	ret = p2p_init(NULL);
	if(ret != 0)
	{
		LOGE_print("p2p_init error");
		return ret;
	}

	p2p_log_set_level(0);
	
	//设置全局属性
	int maxRecvLen = 1024*1024;
	int maxClientCount = 10;
	p2p_set_global_opt(P2P_MAX_CLIENT_COUNT, &maxClientCount, sizeof(int));
	p2p_set_global_opt(P2P_MAX_RECV_PACKAGE_LEN, &maxRecvLen, sizeof(int));

	pHandle->m_Cb.on_connect_result = on_connect_result;
	pHandle->m_Cb.on_disconnect = on_disconnect;
	pHandle->m_Cb.on_accept_signaling_client = on_accept_signaling_client;
	pHandle->m_Cb.on_disconnect_signaling_client = on_disconnect_signaling_client;
	pHandle->m_Cb.on_recv_av_request = on_recv_av_request;
	pHandle->m_Cb.on_recv = on_recv;
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

	if(strlen(dispathce_server) > 0){
		strncpy(pHandle->m_DispatchServer, dispathce_server, strlen(dispathce_server));
		pHandle->m_DispatchServer[strlen(dispathce_server)] = '\0';
		//请求分派服务器
		pHandle->m_DispatchStop = 0;
		ret = pthread_create_4m(&pHandle->m_pThreadDispatch, thread_tcp_turn_dispatch, pHandle);
		if(ret != 0 ){
			LOGE_print(" create thread_turn_dispatch error:%s", strerror(ret));
			p2p_uninit();
			return -1;
		}
	}
	else
	{//直接连接TURN服务器
		pHandle->m_MainConnStop = 0;
		ret = pthread_create_4m(&pHandle->m_pThreadMainConn, thread_main_conn, pHandle);
		if(ret != 0 ){
			LOGE_print(" create thread_turn_conn error:%s", strerror(ret));
			p2p_uninit();
			return -1;
		}
	}

	return 0;
}

int tcp_turn_uninit(tcp_turn* pHandle)
{
	if(pHandle->m_pThreadMainConn)
	{
		pHandle->m_MainConnStop = 1;
		pthread_join(pHandle->m_pThreadMainConn, NULL);
	}
	
	if(pHandle->m_pThreadDispatch)
	{
		pHandle->m_DispatchStop = 1;
		pthread_join(pHandle->m_pThreadDispatch, NULL);
		pHandle->m_pThreadDispatch = 0;
		pHandle->m_Dispatched = 0;
	}

	//清除已经连接的client
	cmtx_enter(pHandle->m_mtxSigClients);
	int size = cmap_size(&(pHandle->m_SigClients));
	int i;
	for(i=0; i<size; i++)
	{
		cmapnode* node = cmap_index_get(&(pHandle->m_SigClients), i);
		if(node)
		{
			p2p_conn_clnt* clnt = (p2p_conn_clnt*)(node->data);
			if(clnt) 
			{
				p2p_conn_clnt_destory(clnt);
				node->data = NULL;
			}
		}
	}
	cmap_clear(&(pHandle->m_SigClients));
	cmtx_leave(pHandle->m_mtxSigClients);

	//清空av连接数
	cmtx_enter(pHandle->m_mtxAvClients);
	cmap_clear(&(pHandle->m_AvClients));
	cmtx_leave(pHandle->m_mtxAvClients);

//	if(pHandle->m_Dispatcher) destroy_p2p_dispatch_requester(pHandle->m_Dispatcher);
	turn_main_disconnect();

	p2p_uninit();
	
	return 0;
}

int tcp_turn_av_write(tcp_turn* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_status != 0) return -2; 	//服务尚未链接成功
	if(data == NULL || len == 0 || len > 250*1024) return -1; //脏数据

	//一对多
	tcp_turn_1vn_write(pHandle, data, len, type, time_stamp, is_key);

	//一对一
	tcp_turn_1v1_write(pHandle, data, len, type, time_stamp, is_key);
	
#ifdef FILE_DEBUG_W
	LOGT_print("data type:%x len:%d", mass.type, mass.len);
	if(m_pFdW) fwrite(mass.body + sizeof(P2pHead), 1, len, pHandle->m_pFdW);
#endif

	return 0;
}

int tcp_turn_send(void* transport, int id, unsigned char* data, unsigned int len, p2p_send_model mode)
{
	P2pHead* head = (P2pHead*)data;
	
	LOGT_print("flag:%x, msgType:%x, protoType:%d, size:%u, type:%d, len:%d",
		head->flag, head->msgType, head->protoType, head->size, head->type, len);

//	if(head->msgType == SND_VIDEO_HD || head->msgType == SND_VIDEO_LD
//		|| head->msgType == SND_AUDIO){
//		
//		static FILE* mp4 = NULL;
//		if(mp4 == NULL) mp4 = fopen("/opt/httpServer/lighttpd/htdocs/sd/send.mp4","wb");
//		if(mp4) fwrite(data + sizeof(P2pHead), 1, head->size, mp4);
//	}

	int sended = 0;

	if(id != 0)
	{
		sended = gss_dev_main_send(transport, id, (char*)data, (int)len, mode);
	}
	else
	{
		sended = gss_dev_av_send(transport, (char*)data, (int)len, mode, GSS_COMMON_DATA);
	}
	
	if(sended == 0)
	{
		LOGT_print("tcp send successful!\r");
		return 0;
	}
	else
	{
		LOGE_print("tcp send failed %d\r", sended);
		return sended;
	}
}

int tcp_turn_clearcache()
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	gss_dev_av_clean_buf(pHandle->m_pTransport);

	return 0;
}

int tcp_turn_av_swtich(int type)
{
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;

	int push = -1;
	int send = -1;
	if(pHandle->m_enablepush == 1)
	{
		push = tcp_push_av_swtich(type);
	}

	send = tcp_turn_1v1_av_swtich(type);
//	LOGE_print("type %d push:%d send:%d", type, push, send);
	if(push == -1 && send == -1)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int tcp_turn_param_set(int type, char* val)
{
	if(val == NULL) return -1;
	
	tcp_turn* pHandle = get_tcp_turn_handle();
	if(pHandle == NULL) return -1;
	
	switch(type){
		case ENABLE_1VN:
		{
			pHandle->m_enablepush = atoi(val);
			LOGI_print("ENABLE_1VN enbale %s", val);
			break;
		}
		case ENABLE_AUDIO:
		case ENABLE_VIDEO:
		case ENABLE_VQUALITY:
		case SET_1VN_RTMP_RUL:
		{
			//开启rtmp模式 并且设置url
			LOGI_print("set tcp_push type val %s", val);
			return tcp_push_param_set(type, val);
			break;
		}
		default:
			break;
	}
	
	return 0;
}

