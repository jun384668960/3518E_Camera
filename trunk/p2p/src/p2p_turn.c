#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

#include "p2p_turn.h"
#include "p2p_conn_client.h"
#include "p2p_utils.h"
#include "p2p_define.h"
#include "utils_log.h"
#include "utils_common.h"

static void on_create_complete(p2p_transport *transport,
					   int status,
					   void *user_data);

static void on_disconnect_server(p2p_transport *transport,
							 int status,
							 void *user_data);

static void on_connect_complete(p2p_transport *transport,
						int connection_id,
						int status,
						void *transport_user_data,
						void *connect_user_data);

static void on_connection_disconnect(p2p_transport *transport,
						  int connection_id,
						  void *transport_user_data,
						  void *connect_user_data);

static void on_accept_remote_connection(p2p_transport *transport,
									int connection_id,
									int conn_flag, //to see conn_flag in p2p_transport_connect 
									void *transport_user_data);

static void on_connection_recv(p2p_transport *transport,
					int connection_id,
					void *transport_user_data,
					void *connect_user_data,
					char* data,
					int len);

static void on_turn_dispatch(void* dispatcher,
					int status, 
					void* user_data, 
					char* server, 
					unsigned short port, 
					unsigned int server_id);
#if !DATA_QSEND
void* thread_data_dispatch(void *pParam);
#endif
static void* thread_client_del(void* param);
static void* thread_turn_dispatch(void *pParam);
static void* thread_turn_conn(void *pParam);

///////////////////////////////////////////////////////////////////////
int turn_dispatch(void** dispatcher, char* user, char* password, char* ds_addr, char* user_data);
int turn_server_connect();
int turn_server_disconnect();
int noticfy_data_sync();
int noticfy_client_error(int id);//通知无效连接
int error_clients_clear();

int do_data_av_send(unsigned char* data, unsigned int len, int id, int flag, p2p_send_model mode);
int do_data_dispatch(MassData* pData);
void loop_data_dispatch();//此线程只为发送server主动发送数据
p2p_trans* get_p2p_trans_handle();

///////////////////////////////////////////////////////////////////////

#define MAX_ERROR_STRING_LEN 	256
#define P2P_SERVER_PORT 		34780
#define DATA_QSEND				1
//#define FILE_DEBUG_W
//#define FILE_DEBUG_R
static p2p_trans* s_handle = NULL;
#if !DATA_QSEND
static int s_MaxLstDataSize = 200;
#endif

void on_create_complete(p2p_transport *transport,
						   int status,
						   void *user_data)
						   
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;

	if (status == P2P_SUCCESS) 
	{
		LOGI_print("p2p connect server successful, transport:%p net state %d\r", transport, p2p_transport_server_net_state(transport));
		pHandle->m_status = 0;
	} 
	else 
	{
		char errmsg[MAX_ERROR_STRING_LEN];
		p2p_strerror(status, errmsg, sizeof(errmsg));
		LOGE_print("p2p connect server failed: %s\r", errmsg);
		pHandle->m_status = -1;
	}
}

void on_disconnect_server(p2p_transport *transport,
								 int status,
								 void *user_data)
{
	int i;
	char errmsg[MAX_ERROR_STRING_LEN];
	p2p_strerror(status, errmsg, sizeof(errmsg));
	LOGI_print("p2p disconnect server, %s\r", errmsg);
	
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;
	
	pHandle->m_status = -1;

	//清除已经连接的client
	cmtx_enter(pHandle->m_mtxConnClients);
	int size = cmap_size(&(pHandle->m_ConnClients));
	for(i=0; i<size; i++)
	{
		cmapnode* node = cmap_index_get(&(pHandle->m_ConnClients), i);
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
	cmap_clear(&(pHandle->m_ConnClients));
	cmtx_leave(pHandle->m_mtxConnClients);
	LOGI_print("p2p disconnect server done\r");

	turn_server_disconnect();
	//重新请求分派服务器 -- 启用线程更合适
	pHandle->m_Dispatched = 0;
}

void on_connect_complete(p2p_transport *transport,
							int connection_id,
							int status,
							void *transport_user_data,
							void *connect_user_data)
{
	if (status == P2P_SUCCESS) 
	{
		char addr[256];
		int len = sizeof(addr);
		p2p_addr_type addr_type;
		p2p_get_conn_remote_addr(transport, connection_id, addr, &len, &addr_type);
		LOGI_print("p2p connect remote user successful, connection id %d %s", connection_id, addr);
	} 
	else 
	{
		char errmsg[MAX_ERROR_STRING_LEN];
		p2p_strerror(status, errmsg, sizeof(errmsg));
		LOGE_print("p2p connect remote user failed: %s, connection id %d", errmsg, connection_id);
	}
}

void on_connection_disconnect(p2p_transport *transport,
							  int connection_id,
							  void *transport_user_data,
							  void *connect_user_data)
{
	LOGI_print("p2p connection is disconnected %d", connection_id);
	pthread_t  pid;
	//从m_Conn中删除
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;

	cmtx_enter(pHandle->m_mtxConnClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_ConnClients), connection_id);

	if(client == NULL)
	{
		LOGE_print("OnConnDisConnect id:%d error", connection_id);
	}
	else
	{
		LOGI_print("OnConnDisConnect id:%d client:%p", connection_id, client);
		cmap_erase(&(pHandle->m_ConnClients), connection_id);
	}
	
	LOGI_print("current clients:%d", cmap_size(&(pHandle->m_ConnClients)));

	cmtx_leave(pHandle->m_mtxConnClients);

	if(client)
		pthread_create_4m(&pid, thread_client_del, client);
}

void on_accept_remote_connection(p2p_transport *transport,
										int connection_id,
										int conn_flag, //to see conn_flag in p2p_transport_connect 
										void *transport_user_data)
{
	LOGI_print("accept remote connection %d", connection_id);

	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;
	
	//最大连接数控制
	p2p_conn_clnt* client = p2p_conn_clnt_create((p2p_handle*)pHandle, connection_id, NULL);
	if(client != NULL/*p2p_conn_clnt_prepare(client) == 0*/)
	{
		cmtx_enter(pHandle->m_mtxConnClients);
		int ret = cmap_insert(&(pHandle->m_ConnClients), connection_id, client);
		cmtx_leave(pHandle->m_mtxConnClients);
		if(ret != 0)
		{
			LOGE_print("m_ConnClients insert id:%d client:%p error", connection_id,client);
			return;
		}
		LOGI_print("m_ConnClients insert id:%d client:%p success,  clients:%d ", connection_id, client, cmap_size(&(pHandle->m_ConnClients)));
	}
//	else
//	{
//		p2p_conn_clnt_destory(client);
//	}
}

void on_connection_recv(p2p_transport *transport,
						int connection_id,
						void *transport_user_data,
						void *connect_user_data,
						char* data,
						int len)
{
	LOGT_print("on_connection_recv %p %d %d", transport, connection_id, len);
	if(data == NULL || len<=0 || len>32*1024) return; //脏数据
	
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;

	//删除无效链接
//	ClearErrClients();
	
	cmtx_enter(pHandle->m_mtxConnClients);
	p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_ConnClients), connection_id);

	if(client == NULL)
	{
		cmtx_leave(pHandle->m_mtxConnClients);
		LOGE_print("m_ConnClients find id:%d error", connection_id);
	}
	else
	{
		cmtx_leave(pHandle->m_mtxConnClients);
		p2p_conn_clnt_write(client, (unsigned char*)data,len);
	}
}

void on_turn_dispatch(void* dispatcher, int status, 
					void* user_data, 
					char* server, 
					unsigned short port, 
					unsigned int server_id)
{
	LOGI_print("dispatcher:%p status:%d", dispatcher, status);
	p2p_trans* pHandle = get_p2p_trans_handle();
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
		
		turn_server_disconnect();
		int ret = turn_server_connect();
		if(ret != 0){
			LOGE_print(" connect_turn_server error");
		}
	}
	else
	{
//		DisConnectServer();
//		pHandle->m_Dispatched = false;
	}
	destroy_p2p_dispatch_requester(dispatcher);
	pHandle->m_Dispatcher = NULL;
}

#if !DATA_QSEND
void* thread_data_dispatch(void *pParam)
{
	loop_data_dispatch();
	return 0;
}
#endif

void* thread_client_del(void* param)
{
	pthread_detach(pthread_self());
	p2p_conn_clnt* client = (p2p_conn_clnt*)param;
	if(client) p2p_conn_clnt_destory(client);
	
	LOGI_print("client ThreadClientDel sucess!");

	return NULL;
}

void* thread_turn_dispatch(void *pParam)
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	
	while(!pHandle->m_DispatchStop)
	{
		//已经连接
		if(pHandle->m_Dispatched && pHandle->m_status == 0) 
		{
			usleep(5000*1000);
			continue;
		}

		//TODO 读取配置 获取分派服务器域名/地址
		LOGI_print("do TurnDispatch:%p user:%s password:%s m_DispatchServer:%s"
				,pHandle->m_Dispatcher, pHandle->m_Cfg.user, pHandle->m_Cfg.password, pHandle->m_DispatchServer);
		
		if(pHandle->m_Dispatcher != NULL) destroy_p2p_dispatch_requester(pHandle->m_Dispatcher);
		int ret = turn_dispatch(&(pHandle->m_Dispatcher), pHandle->m_Cfg.user, pHandle->m_Cfg.password, pHandle->m_DispatchServer, NULL);
		if(ret != 0 ){
			LOGE_print(" turn_dispatch error");
		}

		usleep(5000*1000);
	}
	return NULL;
}

void* thread_turn_conn(void *pParam)
{
	p2p_trans* pHandle = (p2p_trans*)pParam;

	while(!pHandle->m_ConnStop)
	{
		//已经连接
		if(pHandle->m_status == 0) 
		{
			usleep(10000*1000);
			continue;
		}

		turn_server_disconnect();
		int ret = turn_server_connect();
		if(ret != 0){
			LOGE_print(" connect_turn_server error");
		}

		usleep(10000*1000);
	}
	return NULL;
}

int turn_dispatch(void** dispatcher, char* user, char* password, char* ds_addr, char* user_data)
{
	int ret = p2p_request_dispatch_server(
				  user/*char* user*/, 
				  password/*char* password*/, 
				  ds_addr/*char* ds_addr*/, 
				  user_data/*void* user_data*/, 
				  on_turn_dispatch,
				  dispatcher/*void** dispatcher*/);
	if(ret)
	{
		LOGE_print("p2p_request_dispatch_server error");
		return -1;
	}

	LOGT_print("p2p_request_dispatch_server done"); 
	return 0;
}

int turn_server_connect()
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_pTransport)
	{
		LOGW_print("p2p transport already created, destroy it first\r");
		return 0;
	}
	else
	{
		int status = p2p_transport_create(&(pHandle->m_Cfg), &(pHandle->m_pTransport));
		if (status != P2P_SUCCESS) 
		{
			char errmsg[MAX_ERROR_STRING_LEN];
			p2p_strerror(status, errmsg, sizeof(errmsg));
			LOGE_print("create p2p transport failed: %s\r", errmsg);
			return -1;
		}
		LOGW_print("m_pTransport:%p\r", pHandle->m_pTransport);
		return 0;
	}
}

int turn_server_disconnect()
{
	//清除m_Conn
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;

	LOGW_print("m_pTransport:%p\r", pHandle->m_pTransport);
	if(pHandle->m_pTransport)
	{
		p2p_transport_destroy(pHandle->m_pTransport);
		pHandle->m_pTransport = 0;
	}
	
	return 0;
}

int noticfy_data_sync()
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	//清除已经缓存的广播数据
	cmtx_enter(pHandle->m_mtxWaitData);
	while(cqueue_is_empty(&pHandle->m_lstData) == 0)
	{
		MassData* data = (MassData*)cqueue_dequeue(&pHandle->m_lstData);
		if(data->body != NULL) free(data->body);
		free(data);
	}
	cmtx_leave(pHandle->m_mtxWaitData);

	return 0;
}

int  noticfy_client_error(int id)
{
//	p2p_handle* pHandle = get_p2p_handle();
//	if(pHandle == NULL) return -1;
//	
//	cmtx_enter(pHandle->m_mtxErrClnt);
//	clnt_node* node = (clnt_node*)malloc(sizeof(clnt_node));
//	node->id = id;
//	cqueue_enqueue(&pHandle->m_lstErrClnt, node);
//	cmtx_leave(pHandle->m_mtxErrClnt);
	return 0;
}

int error_clients_clear()
{
#if 0
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	if(cqueue_is_empty(&pHandle->m_lstErrClnt) == 1)  return 0;
	
	cmtx_enter(pHandle->m_mtxErrClnt);
	while(cqueue_is_empty(&pHandle->m_lstErrClnt) == 0)
	{
		clnt_node* node = (clnt_node*)cqueue_dequeue(&pHandle->m_lstErrClnt);
		int id = node->id;
		
		cmtx_enter(pHandle->m_mtxConnClients);

		p2p_conn_clnt* client = (p2p_conn_clnt*)cmap_find(&(pHandle->m_ConnClients), id);
		if(client)
		{
			cmap_erase(&(pHandle->m_ConnClients), id);
			p2p_conn_clnt_destory(client);
		}
		
		cmtx_leave(pHandle->m_mtxConnClients);

		free(node);
	}

	cqueue_clear(&pHandle->m_lstErrClnt);
	cmtx_leave(pHandle->m_mtxErrClnt);
#endif
	return 0;
}

int do_data_av_send(unsigned char* data, unsigned int len, int id, int flag, p2p_send_model mode)
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	P2pHead* head = (P2pHead*)data;
	int error;
	int sended = p2p_transport_av_send(pHandle->m_pTransport, id, (char*)data, (int)len, flag ,mode, &error);
	if(sended > 0)
	{
//		LOGT_print("p2p send successful! msgType:%x size:%u\r", head->msgType, head->size);
		return 0;
	}
	else
	{
		LOGE_print("p2p send failed %d %d  msgType:%x size:%u\r", sended, error, head->msgType, head->size);
		if(sended == 0)//链接失效
		{
			return -2;
		}
		return -1;
	}

}

int do_data_dispatch(MassData* pData)
{
	int i;
	int ret = 0;
	cqueue mlstClnt;
		
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;

	cqueue_init(&mlstClnt);
	if(pData->type == SND_VIDEO || pData->type == SND_AUDIO || pData->type == SND_NOTIC){
		cmtx_enter(pHandle->m_mtxConnClients);

		int size = cmap_size(&pHandle->m_ConnClients);
		for(i=0; i<size; i++)
		{
			cmapnode* mnode = cmap_index_get(&pHandle->m_ConnClients, i);
			if(mnode == NULL) continue;
			
			p2p_conn_clnt* client = (p2p_conn_clnt*)(mnode->data);
			if(client == NULL) continue;

			int clntId = mnode->key;

			//NOTIC. m_mtxConnClients.Leave(); as soon as possible
			if((pData->type == SND_VIDEO && client->m_vstatus == STATUS_START)
				|| (pData->type == SND_AUDIO && client->m_astatus == STATUS_START)
//			if((pData->type == SND_VIDEO)
//				|| (pData->type == SND_AUDIO)
				|| (pData->type == SND_NOTIC))
			{
//				LOGT_print("send data.type:%x", pData->type);
				clnt_node* qnode = (clnt_node*)malloc(sizeof(clnt_node));
				qnode->id = clntId;
				cqueue_enqueue(&mlstClnt, qnode);
			}
		}

		cmtx_leave(pHandle->m_mtxConnClients);

		//分发
		while(cqueue_is_empty(&mlstClnt) == 0)
		{
			clnt_node* qnode = (clnt_node*)cqueue_dequeue(&mlstClnt);
			int id = qnode->id;

			if(pData->type == SND_VIDEO || pData->type == SND_AUDIO)
			{
				ret = do_data_av_send((unsigned char*)pData->body, pData->len, id, pData->type==SND_VIDEO?1:0, P2P_SEND_NONBLOCK);
			}
			else
			{
				ret = p2p_trans_send((unsigned char*)pData->body, pData->len, id, P2P_SEND_NONBLOCK);
			}
				
			if(ret == -2)
			{
				//记住此连接失效
//				noticfy_client_error(id);
			}
			
			free(qnode);
		}
	}
	
	cqueue_destory(&mlstClnt);	

	return ret;
}

void loop_data_dispatch()
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return;
	
	while(!pHandle->m_Stop)
	{
		int ret = csem_wait_timeout(pHandle->m_semWaitData, 200);
		if(ret != 0)
		{
//			LOGT_print("===========>>> csem_wait timeout");
			continue;
		}
		cmtx_enter(pHandle->m_mtxWaitData);
		if(cqueue_is_empty(&pHandle->m_lstData) == 0)
		{
			MassData* pData = (MassData*)cqueue_gethead(&pHandle->m_lstData);
			cmtx_leave(pHandle->m_mtxWaitData);

			do_data_dispatch(pData);
			
#ifdef FILE_DEBUG_R
			LOGT_print("loop data type:%x len:%d", pData->type, pData->len);
			if(pHandle->m_pFdR) fwrite(pData->body + sizeof(P2pHead), 1, pData->len - sizeof(P2pHead), m_pFdR);
#endif
			cmtx_enter(pHandle->m_mtxWaitData);

			pData = (MassData*)cqueue_dequeue(&pHandle->m_lstData);
			if(pData->body != NULL) free(pData->body);
			if(pData) free(pData);
		}
		cmtx_leave(pHandle->m_mtxWaitData);

		//删除无效链接
		error_clients_clear();

//		usleep(1000);
	}
}

p2p_trans* get_p2p_trans_handle()
{
	return s_handle;
}

p2p_handle* p2p_trans_create()
{
	p2p_trans* handle = (p2p_trans*)malloc(sizeof(p2p_trans));
	memset(handle, 0x0, sizeof(p2p_trans));

	handle->base.m_type = HANDLE_TYPE_P2P;
	handle->base.m_CfgServerPort = P2P_SERVER_PORT;
	handle->base.m_Iwait = 1;
	
	handle->m_pTransport = NULL;
	handle->m_status = -1;
	handle->m_Stop = 0;
	handle->m_pFdW = NULL;
	handle->m_pFdR = NULL;
	handle->m_DispatchStop = 0;
	handle->m_ConnStop = 0;
	handle->m_Dispatched = 0;

	handle->m_pThread = 0;
	handle->m_pThreadDispatch = 0;;//线程
	handle->m_pThreadConn = 0;
	handle->m_Dispatcher = NULL;
	
	memset(&handle->m_Cfg, 0, sizeof(handle->m_Cfg));
	memset(&handle->m_Cb, 0, sizeof(handle->m_Cb));

#ifdef FILE_DEBUG_W
	handle->m_pFdW = fopen("/opt/httpServer/lighttpd/htdocs/sd/handlew.media","wb");
#endif
#ifdef FILE_DEBUG_R
	handle->m_pFdR = fopen("/opt/httpServer/lighttpd/htdocs/sd/handler.media","wb");
#endif
	
	handle->m_mtxWaitData = cmtx_create();
	handle->m_semWaitData = csem_create(1, 999);

	handle->m_mtxErrClnt = cmtx_create();
	handle->m_mtxConnClients = cmtx_create();
	
	cqueue_init(&handle->m_lstErrClnt);	
	cqueue_init(&handle->m_lstData);	
	cmap_init(&handle->m_ConnClients);
	
	s_handle = handle;

	return (p2p_handle*)handle;
}

void p2p_trans_destory(p2p_trans* handle)
{
#ifdef FILE_DEBUG_W
	fclose(handle->m_pFdW);
#endif
#ifdef FILE_DEBUG_R
	fclose(handle->m_pFdR);
#endif

	cqueue_destory(&handle->m_lstErrClnt);
	cqueue_destory(&handle->m_lstData);
	cmap_destory(&handle->m_ConnClients);
	
	cmtx_delete(handle->m_mtxWaitData);
	csem_delete(handle->m_semWaitData);
	cmtx_delete(handle->m_mtxErrClnt);
	cmtx_delete(handle->m_mtxConnClients);

	free(handle);
	s_handle = NULL;
}

int p2p_trans_init(p2p_trans* pHandle, char* dispathce_server, char* server, unsigned short port, char* uid, char* pwd, int tcp)
{
	if(dispathce_server == NULL || server == NULL || uid == NULL || pwd == NULL)
	{
		LOGE_print("error, invaid params");
		return -1;
	}

	if((strlen(dispathce_server) <=0 && strlen(server) <= 0) || strlen(uid) <= 0 || strlen(pwd)<=0)
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

	pHandle->m_Cb.on_create_complete = on_create_complete;
	pHandle->m_Cb.on_disconnect_server = on_disconnect_server;
	pHandle->m_Cb.on_connect_complete = on_connect_complete;
	pHandle->m_Cb.on_connection_disconnect = on_connection_disconnect;
	pHandle->m_Cb.on_accept_remote_connection = on_accept_remote_connection;
	pHandle->m_Cb.on_connection_recv = on_connection_recv;
	pHandle->m_Cfg.cb = &pHandle->m_Cb;

	strncpy(pHandle->base.m_CfgServer, server, strlen(server));
	pHandle->base.m_CfgServer[strlen(server)] = '\0';
	strncpy(pHandle->base.m_CfgUser, uid, strlen(uid));
	pHandle->base.m_CfgUser[strlen(uid)] = '\0';
	strncpy(pHandle->base.m_CfgPwd, pwd, strlen(pwd));
	pHandle->base.m_CfgPwd[strlen(pwd)] = '\0';
	pHandle->base.m_CfgServerPort = port;
	
	pHandle->m_Cfg.server = pHandle->base.m_CfgServer;
	pHandle->m_Cfg.port = port;
	pHandle->m_Cfg.user = pHandle->base.m_CfgUser;//TODO 读取配置 获取UID
	pHandle->m_Cfg.password = pHandle->base.m_CfgPwd;
	pHandle->m_Cfg.use_tcp_connect_srv = tcp;
	if(pHandle->m_Cfg.user == NULL)//strlen(m_Cfg.user)==0
		pHandle->m_Cfg.terminal_type = P2P_CLIENT_TERMINAL;
	else
		pHandle->m_Cfg.terminal_type = P2P_DEVICE_TERMINAL;

	if(strlen(dispathce_server) > 0){
		strncpy(pHandle->m_DispatchServer, dispathce_server, strlen(dispathce_server));
		pHandle->m_DispatchServer[strlen(dispathce_server)] = '\0';
		//请求分派服务器
		pHandle->m_DispatchStop = 0;
		ret = pthread_create_4m(&pHandle->m_pThreadDispatch, thread_turn_dispatch, pHandle);
		if(ret != 0 ){
			LOGE_print(" create thread_turn_dispatch error:%s", strerror(ret));
			p2p_uninit();
			return -1;
		}
	}
	else
	{//直接连接TURN服务器
		pHandle->m_ConnStop = 0;
		ret = pthread_create_4m(&pHandle->m_pThreadConn, thread_turn_conn, pHandle);
		if(ret != 0 ){
			LOGE_print(" create thread_turn_conn error:%s", strerror(ret));
			p2p_uninit();
			return -1;
		}
	}

#if !DATA_QSEND
	//创建数据操作线程
	pthread_attr_t   attr;  
	struct   sched_param   param;  
	pthread_attr_init(&attr);  
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);  //SCHED_FIFO SCHED_RR
	param.sched_priority   =   99;  
	pthread_attr_setschedparam(&attr, &param);  

	pHandle->m_Stop = 0;
	ret = pthread_create(&pHandle->m_pThread, &attr, thread_data_dispatch, pHandle);
	if(ret != 0 ){
		LOGE_print(" create thread_data_dispatch error:%s", strerror(ret));
		p2p_uninit();
		pthread_attr_destroy(&attr);  
		return -1;
	}
	pthread_attr_destroy(&attr); 
#endif

	return 0;
}

int p2p_trans_uninit(p2p_trans* pHandle)
{
	int i;
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_pThreadConn)
	{
		pHandle->m_ConnStop = 1;
		pthread_join(pHandle->m_pThreadConn, NULL);
		pHandle->m_pThreadConn = 0;
	}
	if(pHandle->m_pThreadDispatch)
	{
		pHandle->m_DispatchStop = 1;
		pthread_join(pHandle->m_pThreadDispatch, NULL);
		pHandle->m_pThreadDispatch = 0;
		pHandle->m_Dispatched = 0;
	}
	
	//退出广播线程
	if(pHandle->m_pThread){
		pHandle->m_Stop = 1;
		pthread_join(pHandle->m_pThread, NULL);
	}

	//清除已经连接的client
	cmtx_enter(pHandle->m_mtxConnClients);
	
	int size = cmap_size(&pHandle->m_ConnClients);
	for(i=0; i<size; i++)
	{
		cmapnode* node = cmap_index_get(&pHandle->m_ConnClients, i);
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
	cmap_clear(&pHandle->m_ConnClients);

	cmtx_leave(pHandle->m_mtxConnClients);

	//清除已经缓存的广播数据
	noticfy_data_sync();

	if(pHandle->m_Dispatcher) destroy_p2p_dispatch_requester(pHandle->m_Dispatcher);
	turn_server_disconnect();
	p2p_uninit();
	return 0;
}

int p2p_trans_write(p2p_trans* pHandle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	if(pHandle == NULL) return -1;
	
	if(pHandle->m_status != 0) return -2; 	//服务尚未链接成功
	if(data == NULL || len == 0 || len > 250*1024) return -1; //脏数据

	//是否订阅
	if(p2p_trans_av_swtich(type) != 0)
	{
		return -1;
	}

	if(type == SND_VIDEO_LD || type == SND_VIDEO_HD)
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
#if DATA_QSEND
	unsigned char frame[250*1024];
	memset(frame, 0, 250*1024);
	memcpy(frame, &head, sizeof(P2pHead));
	memcpy(frame + sizeof(P2pHead), data, len);
	
	MassData mass;
	mass.type = type;
	mass.len = len + sizeof(P2pHead);
	mass.body = frame;
	
	int ret = do_data_dispatch(&mass);
	if(ret != 0 /*&& is_key == 1 */&& type == SND_VIDEO)
	{	
		LOGE_print("send i/p frame error, skip flow p/b frame");
		pHandle->base.m_Iwait = 1;
	}
#else
	//最大缓存数控制
	cmtx_enter(pHandle->m_mtxWaitData);
	if(cqueue_size(&pHandle->m_lstData) > s_MaxLstDataSize)
	{
		LOGW_print("m_lstData full....");
		cmtx_leave(pHandle->m_mtxWaitData);
		return -1;
	}
	cmtx_leave(pHandle->m_mtxWaitData);

	MassData* mass = (MassData*)malloc(sizeof(MassData));
	mass->type = type;
	mass->len = len + sizeof(P2pHead);
	mass->body = (unsigned char*)malloc(len + sizeof(P2pHead));
	memset(mass->body, 0, len + sizeof(P2pHead));
	
	memcpy(mass->body, &head, sizeof(P2pHead));
	memcpy(mass->body + sizeof(P2pHead), data, len);
	
	cmtx_enter(pHandle->m_mtxWaitData);
	cqueue_enqueue(&pHandle->m_lstData, mass);
	cmtx_leave(pHandle->m_mtxWaitData);
	csem_post(pHandle->m_semWaitData);
#endif

#ifdef FILE_DEBUG_W
	LOGT_print("data type:%x len:%d", mass.type, mass.len);
	if(m_pFdW) fwrite(mass.body + sizeof(P2pHead), 1, len, pHandle->m_pFdW);
#endif

	return 0;
}

int p2p_trans_send(unsigned char* data, unsigned int len, int id, p2p_send_model mode)
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	P2pHead* head = (P2pHead*)data;
	
	LOGT_print("m_pTransport:%p id:%d flag:%x, msgType:%x, protoType:%d, type:%d, size:%u",
		pHandle->m_pTransport, id, head->flag, head->msgType, head->protoType, head->type, head->size);

	int error;
	int sended = p2p_transport_send(pHandle->m_pTransport, id, (char*)data, (int)len, mode, &error);
	if(sended > 0)
	{
//		LOGT_print("p2p send successful! msgType:%x size:%u\r", head->msgType, head->size);
		return 0;
	}
	else
	{
		LOGE_print("p2p send failed %d %d  msgType:%x size:%u\r", sended, error, head->msgType, head->size);
		if(sended == 0)//链接失效
		{
			return error;
		}
		return error;
	}

}

int p2p_trans_clearcache(int id)
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	p2p_set_conn_opt(pHandle->m_pTransport, id, P2P_RESET_BUF, NULL, 0);

	return 0;
}

int p2p_trans_av_swtich(int type)
{
	if(type == SND_AUDIO && p2p_conn_clnt_aswitch_get() == STATUS_STOP)
	{
		//LOGW_print("type:0x%x m_aSwitch:%d",type, STATUS_STOP);
		return -1;
	}
	if((type == SND_VIDEO_HD || type == SND_VIDEO_LD) && p2p_conn_clnt_vswitch_get() == STATUS_STOP)
	{
		//LOGW_print("type:0x%x m_vSwitch:%d",type, STATUS_STOP);
		return -1;
	}
 	else if(type == SND_VIDEO_HD && p2p_conn_clnt_vquality_get() == 1)
	{
		//LOGW_print("type:0x%x m_vQualitySingle:1",type);
		return -1;
	}
	else 
		if(type == SND_VIDEO_LD && p2p_conn_clnt_vquality_get() == 0)
	{
		//LOGW_print("type:0x%x m_vQualitySingle:0",type);
		return -1;
	}

	return 0;
}

int p2p_trans_param_set(int type, char* val)
{
	p2p_trans* pHandle = get_p2p_trans_handle();
	if(pHandle == NULL) return -1;
	
	return 0;
}

