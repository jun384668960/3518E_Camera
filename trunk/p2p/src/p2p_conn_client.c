#include "p2p_conn_client.h"
#include "p2p_cmd_handle.h"
#include "p2p_turn.h"
#include "tcp_turn.h"
#include "p2p_utils.h"
#include "utils_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

void* thread_cmd_dispatch(void *pParam);

void p2p_conn_clnt_loop(p2p_conn_clnt* clnt);

void cmd_build_write(p2p_conn_clnt* clnt, unsigned char* data, int len);
void cmd_push_back(p2p_conn_clnt* clnt, RemoteCmd cmd);
void cmd_pop_front(p2p_conn_clnt* clnt);
int  cmd_head_get(p2p_conn_clnt* clnt, RemoteCmd** cmd);

#define P2P_CONN_CLIENT

void* thread_cmd_dispatch(void *pParam)
{
	p2p_conn_clnt* clnt = (p2p_conn_clnt*)pParam;
	p2p_conn_clnt_loop(clnt);
	
	return 0;
}	

void p2p_conn_clnt_loop(p2p_conn_clnt* clnt)
{
	int speakOut = 0;
	while(clnt->m_Stop != 1)
	{
		RemoteCmd* cmd = NULL;
		if(cmd_head_get(clnt, &cmd) == 1)
		{
			P2pHead info;
			memcpy(&info, cmd->body, sizeof(P2pHead));
			//对讲数据
			if(info.msgType == SND_AUDIO)
			{
				speakOut = 0;
				int ret = speak_data_handle(clnt, cmd->body, info.size + sizeof(P2pHead), info);
				if(ret != 0)
				{
					LOGE_print("Speaker not open yet !!");
				}
			}
			else
			{
				int ret = cmd_handle(clnt, cmd->body, info.size + sizeof(P2pHead), info);
				if(ret != 0)
				{
					LOGE_print("id:%d cmd_handle msgType:%d error ",clnt->m_id, info.msgType);
				}
			}

			cmd_pop_front(clnt);
		}
		
		speakOut++;
		//太久没有收到对讲数据
		if(speakOut > 200)
		{
			speakOut = 0;
			if(clnt->m_SpeakerDataLen > 0)
			{
				speak_data_send(clnt->m_SpeakerData, clnt->m_SpeakerDataLen);
				memset(clnt->m_SpeakerData, 0x0, MAX_SPEAK_BUF_SIZE);
				clnt->m_SpeakerDataLen = 0;
			}
		}
		usleep(5 * 1000);
	}

	if(clnt->m_SpeakerDataLen > 0)
	{
		speak_data_send(clnt->m_SpeakerData, clnt->m_SpeakerDataLen);
		memset(clnt->m_SpeakerData, 0x0, MAX_SPEAK_BUF_SIZE);
		clnt->m_SpeakerDataLen = 0;
	}
}

int p2p_conn_data_send(p2p_conn_clnt* clnt, unsigned char* data, unsigned int len, p2p_send_model mode)
{
	int ret;
	if(clnt->m_pTransport == NULL)
	{
		ret = p2p_trans_send(data, len, clnt->m_id, mode);
	}
	else
	{
		ret = tcp_turn_send(clnt->m_pTransport, clnt->m_id, data, len, mode);
	}
			
	return ret;
}

int p2p_conn_clearcache(p2p_conn_clnt* clnt)
{
	int ret;
	if(clnt->m_pTransport == NULL)
	{
		ret = p2p_trans_clearcache(clnt->m_id);
	}
	else
	{
		ret = tcp_turn_clearcache();
	}
	LOGI_print("clnt_id:%d p2p_conn_clearcache ret:%d", clnt->m_id, ret);
	return ret;
}

void cmd_build_write(p2p_conn_clnt* clnt, unsigned char* data, int len)
{
	if(private_head_check(data, len) != -1)
	{
		RemoteCmd cmd;
		cmd.id = clnt->m_id;
		cmd.len = len;
		cmd.body = (unsigned char*)malloc(len + MAX_RESP_SIZE/*预留字节*/);
		memcpy(cmd.body, data, len);
		
		cmd_push_back(clnt, cmd);
	}
	else
	{
		LOGE_print("client try to push cmd error, PrivHeadCheck error...");
	}
}

void cmd_push_back(p2p_conn_clnt* clnt, RemoteCmd cmd)
{
	//最大缓存数控制
	cmtx_enter(clnt->m_mtxWaitCmd);
	if(cqueue_size(&clnt->m_lstCmd) > MAX_CMD_SIZE)
	{
		LOGW_print("m_lstCmd full....");
		cmtx_leave(clnt->m_mtxWaitCmd);
		return;
	}
	
	RemoteCmd* pCmd = (RemoteCmd*)malloc(sizeof(RemoteCmd));
	memcpy(pCmd, &cmd, sizeof(RemoteCmd));

	cqueue_enqueue(&clnt->m_lstCmd, pCmd);
	cmtx_leave(clnt->m_mtxWaitCmd);
}

void cmd_pop_front(p2p_conn_clnt* clnt)
{
	cmtx_enter(clnt->m_mtxWaitCmd);
	RemoteCmd* pCmd = (RemoteCmd*)cqueue_dequeue(&clnt->m_lstCmd);
	if(pCmd->body != NULL) free(pCmd->body);
	free(pCmd);
	cmtx_leave(clnt->m_mtxWaitCmd);
	LOGT_print("cmd_pop_front OK.... size:%d", cqueue_size(&clnt->m_lstCmd));
}

int cmd_head_get(p2p_conn_clnt* clnt, RemoteCmd** cmd)
{
	cmtx_enter(clnt->m_mtxWaitCmd);
	if(cqueue_is_empty(&clnt->m_lstCmd) == 1)
	{
		//LOGT_print("m_lstCmd is empty\n");
		cmtx_leave(clnt->m_mtxWaitCmd);
		return 0;
	}
	RemoteCmd* pCmd = (RemoteCmd*)cqueue_gethead(&clnt->m_lstCmd);
	*cmd = pCmd;

	cmtx_leave(clnt->m_mtxWaitCmd);
	return 1;
}

int connction_status_get(p2p_conn_clnt* clnt, int type)
{
	int val = 0;
	switch(type)
	{
	case VIDEO_STATUS:
		val = clnt->m_vstatus;
		break;
	case VIDEO_QUALITY:
		val = clnt->m_vquality;
		break;
	case AUDIO_STATUS:
		val = clnt->m_astatus;
		break;
	case SPEAK_STATUS:
		val = clnt->m_pstatus;
		break;
	case REC_VIDEO_STATUS:
		val = clnt->m_rvstatus;
		break;
	case REC_AUDIO_STATUS:
		val = clnt->m_rastatus;
		break;
	default:
		break;
	}
	
	return val;
}

int connction_status_set(p2p_conn_clnt* clnt, int type, int val)
{
	switch(type)
	{
	case VIDEO_STATUS:
		clnt->m_vstatus = val;
		break;
	case VIDEO_QUALITY:
		clnt->m_vquality = val;
		break;
	case AUDIO_STATUS:
		clnt->m_astatus = val;
		break;
	case SPEAK_STATUS:
		clnt->m_pstatus = val;
		break;
	case REC_VIDEO_STATUS:
		clnt->m_rvstatus = val;
		break;
	case REC_AUDIO_STATUS:
		clnt->m_rastatus = val;
		break;
	default:
		break;
	}
	
	return 0;
}

int p2p_conn_clnt_vswitch_get()
{
	return globle_vswitch_get();
}

int p2p_conn_clnt_aswitch_get()
{
	return globle_aswitch_get();
}
int p2p_conn_clnt_vquality_get()
{
	return globle_vquality_get();
}

p2p_conn_clnt* p2p_conn_clnt_create(struct p2p_handle_t* server, int id, void* transport)
{
	p2p_conn_clnt* clnt = (p2p_conn_clnt*)malloc(sizeof(p2p_conn_clnt));

	clnt->m_pServer = server;
	clnt->m_pTransport = transport;
	clnt->m_id = id;
	clnt->m_SpeakerData = NULL;
	clnt->m_vstatus = 0;
	clnt->m_vquality = 0;
	clnt->m_astatus = 0;
	clnt->m_pstatus = 0;

	clnt->m_Stop = 1;
	clnt->m_UploadStop = 1;
	clnt->m_vQuality = 0;
	clnt->m_Width = 0;
	clnt->m_Height = 0;

	clnt->m_pThread = 0;
	clnt->m_pUploadThread = 0;

	clnt->m_SpeakerData = (unsigned char*)malloc(MAX_SPEAK_BUF_SIZE);
	memset(clnt->m_SpeakerData, 0x0, MAX_SPEAK_BUF_SIZE);
	clnt->m_SpeakerDataLen = 0;

	clnt->m_rvstatus = 0;
	clnt->m_rastatus = 0;
	clnt->m_playBackStop = 0;
	clnt->m_playBackThread = 0;
	clnt->m_playBackPrevImgStop = 0;
	clnt->m_playBackPrevImgThread = 0;
	clnt->m_playBackReflush = 0;
	clnt->m_playBackPrevImgReflush = 0;
	clnt->m_cutSndVFrame = 0;
	clnt->m_cutSndVPts = 0;
	clnt->m_cutSndVPtsRef = 0;
	clnt->m_cutSndAPts = 0;
	clnt->m_cutSndAPtsRef = 0;
	clnt->m_mtxWaitCmd = cmtx_create();
	clnt->m_mtxFdReSnd = cmtx_create();

	cqueue_init(&clnt->m_lstCmd); 
	cqueue_init(&clnt->m_lstFdReSnd); 

	LOGI_print("clnt:%p", clnt);
	return clnt;
}
void p2p_conn_clnt_destory(p2p_conn_clnt* clnt)
{
	LOGI_print("clnt:%p", clnt);
	if(clnt->m_playBackThread)
	{
		LOGT_print(" pthread_join m_playBackThread:%p",clnt->m_playBackThread);
		clnt->m_playBackStop = 1;
		pthread_join(clnt->m_playBackThread, NULL);
		clnt->m_playBackThread = 0;
	}

	if(clnt->m_playBackPrevImgThread)
	{
		LOGT_print(" pthread_join m_playBackPrevImgThread:%p",clnt->m_playBackPrevImgThread);
		clnt->m_playBackPrevImgStop = 1;
		pthread_join(clnt->m_playBackPrevImgThread, NULL);
		clnt->m_playBackPrevImgThread = 0;
	}
	
	if(clnt->m_pUploadThread)
	{
		LOGT_print(" pthread_join m_pUploadThread:%p",clnt->m_pUploadThread);
		clnt->m_UploadStop = 1;
		pthread_join(clnt->m_pUploadThread, NULL);
		clnt->m_pUploadThread = 0;
	}
		
	if(clnt->m_pThread)
	{
		LOGT_print(" pthread_join m_pThread:%p",clnt->m_pThread);
		clnt->m_Stop = 1;
		pthread_join(clnt->m_pThread, NULL);
		clnt->m_pThread = 0;
	}

	LOGT_print("pthread_join one");
	cmtx_enter(clnt->m_mtxWaitCmd);
	while(cqueue_is_empty(&clnt->m_lstCmd) == 0)
	{
		RemoteCmd* qnode = (RemoteCmd*)cqueue_dequeue(&clnt->m_lstCmd);
		if(qnode->body != NULL) free(qnode->body);
		
		free(qnode);
	}
	cqueue_clear(&clnt->m_lstCmd);
	cmtx_leave(clnt->m_mtxWaitCmd);

	if(clnt->m_SpeakerData != NULL) free(clnt->m_SpeakerData);

	cqueue_destory(&clnt->m_lstFdReSnd);	
	cqueue_destory(&clnt->m_lstCmd);	
	cmtx_delete(clnt->m_mtxFdReSnd);
	cmtx_delete(clnt->m_mtxWaitCmd);

	LOGI_print("destory done");
	
	free(clnt);
}

int p2p_conn_clnt_prepare(p2p_conn_clnt* clnt)
{
	//此处不合理 
	
	if(globle_vquality_get() == -1)
	{
		int quality = globle_vquality_init();
		connction_status_set(clnt, VIDEO_QUALITY, quality);
		LOGI_print("init quality:%d", quality);
	}
	else
	{
		int quality = globle_vquality_get();
		LOGI_print("current quality:%d", quality);
	}

	//创建数据操作线程
	clnt->m_Stop = 0;
	int isucc = pthread_create_4m(&clnt->m_pThread, thread_cmd_dispatch, clnt);
	if(isucc != 0 ){
		LOGE_print(" create thread_cmd_dispatch error:%s", strerror(isucc));
		clnt->m_pThread = 0;
		return -1;
	}
	LOGI_print(" create m_pThread:%d", clnt->m_pThread);

	return 0;
}

int p2p_conn_clnt_write(p2p_conn_clnt* clnt, unsigned char* data, int len)
{
	if(clnt->m_pThread == 0)
	{
		p2p_conn_clnt_prepare(clnt);
	}
	
	cmd_build_write(clnt, data, len);
	return 0;
}

