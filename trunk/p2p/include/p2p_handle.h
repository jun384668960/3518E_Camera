#ifndef _P2P_HANDLE_H_
#define _P2P_HANDLE_H_

#include <stdio.h>
#include "p2p_define.h"
#include "p2p_transport.h" 
#include "p2p_dispatch.h"
#include "cqueue.h"
#include "cmap.h"
#include "lock_utils.h"

enum HANDLE_TYPE_E
{
	HANDLE_TYPE_NONE		= -1,
	HANDLE_TYPE_TCP 		= 0,
	HANDLE_TYPE_P2P			= 1,
	HANDLE_TYPE_TCP_PUSH 	= 2,
	HANDLE_TYPE_TCP_RTMP 	= 3,
};

typedef struct clnt_node_s{
	int id;
}clnt_node;

typedef struct p2p_handle_t
{
	int					m_type;
	
	char				m_CfgServer[64];
	char				m_CfgUser[64];
	char				m_CfgPwd[64];
	unsigned short		m_CfgServerPort;
	int					m_Iwait;
}p2p_handle;

/*
	根据uid区分服务类型
返回值: int 
			HANDLE_TYPE_P2P	走p2p模式
			HANDLE_TYPE_TCP 走TCP turn模式
参数: char* uid 唯一标识UID，非NULL
用户可使用此接口选择对应服务的dispathce server， turn server 或者 port
*/
int p2p_handle_uid_type(char* uid);
/*
	创建服务句柄
返回值: 句柄指针
参数:	char* uid 	唯一标识UID，非NULL
*/
p2p_handle* p2p_handle_create(char* uid);

/*
	销毁服务
返回值: 无
参数:	p2p_handle* handle	句柄指针
*/
void p2p_handle_destory(p2p_handle* handle);

/*
	初始化服务
返回值: int
			0	成功
			非0	失败
参数:	p2p_handle* handle		句柄指针
		char* dispathce_server	分派服务器地址(格式:IP:port)，非NULL，当为""字符串时，服务器强制使用TURN服务
		char* server			Turn服务IP，非NULL
		unsigned short port		Turn服务端口
		char* uid				唯一标识UID，非NULL
		char* pwd				Turn服务密码
		int tcp					P2p Turn链接服务器是否使用TCP链接方式，当且仅当使用P2P模式时有效
初始化后，将自动链接服务器建立服务，且内部带有了断线重连机制
*/
int p2p_handle_init(p2p_handle* handle, char* dispathce_server, char* server, unsigned short port, char* uid, char* pwd, int tcp);

/*
	服务反初始化
返回值: int
			0	成功
			非0	失败
参数:	p2p_handle* handle		句柄指针
*/
int p2p_handle_uninit(p2p_handle* handle);
/*
	主动推送数据澹(消息/媒体数据)
返回值: int
			0	成功
			非0	失败
参数:	p2p_handle* handle		句柄指针
		unsigned char* data		数据
		unsigned int len		数据长度
		int type				数据类型，同p2p_handle_av_swtich的参数type
		unsigned int time_stamp	媒体数据时间戳
		char is_key				媒体数据是否关键帧
*/
int p2p_handle_write(p2p_handle* handle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key);

/*
	获取音视频是否开启传输
返回值: int
			0	成功
			非0	失败
参数:	p2p_handle* handle		句柄指针
		int type
				SND_AUDIO		音频
				SND_VIDEO_HD	高清视频
				SND_VIDEO_LD	标清视频
*/
int p2p_handle_av_swtich(p2p_handle* handle, int type);

/*
	设置属性
返回值: int
			0	成功
			非0	失败
参数:	p2p_handle* handle		句柄指针
		int type
				ENABLE_AUDIO		音频
				ENABLE_VIDEO		视频
		int val
				当type=ENABLE_AUDIO	0表示禁止音频 1表示开启音频
				当type=ENABLE_VIDEO	0表示禁止视频 1表示开启视频
				更多请参考PARAMS_SET_E
*/
int p2p_handle_param_set(p2p_handle* handle, int type, char* val);

#endif
