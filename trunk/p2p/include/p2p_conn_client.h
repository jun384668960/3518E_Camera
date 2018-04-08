#ifndef P2P_CONN_CLIENT_H
#define P2P_CONN_CLIENT_H

#include <pthread.h>
#include "p2p_handle.h"
#include "p2p_define.h"
#include "lock_utils.h"
#include "cqueue.h"

struct p2p_handle_t;
typedef struct __sd_playback_t
{
	int sid;				//通道
	int avIndex;			//tutk 消息通道
	int type;				// 0 是获取历史数据预览图1是实时播放历史数据
	unsigned int utctime;	//app 定位时间
	int duration;			//剪接时长
}sd_playback_t;

typedef struct sd_file_info_t
{
	char 			name[64];
	unsigned int    time;
	unsigned int 	pts_b;
}sd_file_info;

typedef struct p2p_conn_clnt_t
{
	int 				m_vstatus;
	int 				m_vquality;
	int 				m_astatus;
	int 				m_pstatus;
	struct p2p_handle_t* m_pServer;
	void* 				m_pTransport;
	int 				m_id;
	int					m_vQuality;	//请求的视频清晰度
	unsigned short		m_Width; 	// 视频宽
	unsigned short		m_Height;	// 视频高

	cqueue				m_lstCmd;
	int					m_Stop;
	pthread_t       	m_pThread;	//线程
	CMtx				m_mtxWaitCmd;

////////////////////////////////////////////////////////////////
	int					m_UploadStop;
	pthread_t       	m_pUploadThread;//线程
	char 				m_UploadFileName[128];
	unsigned int 		m_packetNum;

	cqueue				m_lstFdReSnd;
	CMtx				m_mtxFdReSnd;

	unsigned char*		m_SpeakerData;
	int					m_SpeakerDataLen;

	int 				m_rvstatus;				//录像视频状态
	int 				m_rastatus;				//录像音频状态
	int					m_playBackStop;
	pthread_t       	m_playBackThread;		//线程
	int					m_playBackPrevImgStop;
	pthread_t       	m_playBackPrevImgThread;		//线程
	sd_playback_t 		m_playBackAttr;
	int					m_cutSndVFrame;
	unsigned int		m_cutSndVPts;
	unsigned int		m_cutSndVPtsRef;
	unsigned int		m_cutSndAPts;
	unsigned int		m_cutSndAPtsRef;
	sd_playback_t 		m_playBackPrevImgAttr;
	int					m_playBackReflush;
	int					m_playBackPrevImgReflush;
}p2p_conn_clnt;


int p2p_conn_clnt_vswitch_get();
int p2p_conn_clnt_aswitch_get();
int p2p_conn_clnt_vquality_get();

p2p_conn_clnt* p2p_conn_clnt_create(struct p2p_handle_t* server, int id, void* transport);
void p2p_conn_clnt_destory(p2p_conn_clnt* clnt);

int p2p_conn_clnt_prepare(p2p_conn_clnt* clnt);
int p2p_conn_clnt_write(p2p_conn_clnt* clnt, unsigned char* data, int len);
int p2p_conn_data_send(p2p_conn_clnt* clnt, unsigned char* data, unsigned int len, p2p_send_model mode);
int p2p_conn_clearcache(p2p_conn_clnt* clnt);
int connction_status_get(p2p_conn_clnt* clnt, int type);
int connction_status_set(p2p_conn_clnt* clnt, int type, int val);

#endif
