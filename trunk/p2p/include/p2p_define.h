#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#define LIST_BUFFER
#define MAX_SPEAK_BUF_SIZE	100*1024
#define MAX_CMD_SIZE		150			//命令缓存列表最大数
#define MAX_RESP_SIZE		256			//返回指令数据包大小

enum{
	STATUS_START = 1,
	STATUS_STOP  = 0,
};

enum{
	VIDEO_STATUS  		= 0,
	VIDEO_QUALITY 		= 1,
	AUDIO_STATUS  		= 2,
	SPEAK_STATUS  		= 3,
	REC_VIDEO_STATUS  	= 4,
	REC_AUDIO_STATUS  	= 5,
};

enum{
	SND_VIDEO_LD		= 0x00F0,
	SND_VIDEO_HD		= 0x00F1,
	SND_VIDEO			= 0x00F2,
	SND_AUDIO			= 0x00F3,
	SND_NOTIC	    	= 0x00F4,
	SND_FILE	    	= 0x00F5,
};

enum PARAMS_SET_E{
	ENABLE_VIDEO		= 0x00F1,
	ENABLE_AUDIO		= 0x00F2,
	ENABLE_VQUALITY		= 0x00F3,
	ENABLE_1VN			= 0x00F4,
	SET_1VN_RTMP_RUL	= 0x00F6,
};

typedef struct Conn_t{
	int id; 		//connection_id
	int vstatus;	//图像状态
	int vquality;	//图像清晰度
	int astatus;	//音频状态
	int pstatus;	//对讲状态
	unsigned char* m_pRevcBuf;
	int m_curPos;
}Conn;

#pragma   pack(1)
typedef struct P2pHead_t
{
	int  			flag;		//消息开始标识
	unsigned int 	size;		//接收发送消息大小(不包括消息头)
	char 			type;		//协议类型1 json 2 json 加密
	char			protoType;	//消息类型1 请求2应答3通知
	int 			msgType;	//IOTYPE消息类型
	char 			reserve[6];	//保留
}P2pHead;
#pragma   pack()

typedef struct RemoteCmd_t{
	int id;
	int len;
	unsigned char* body;
}RemoteCmd;

typedef struct MassData_t{
	int type;
	int len;
	unsigned char* body;
}MassData;

typedef struct CmdRespInfo_t{
	int 	len;		//返回数据长度
	int 	enable;		//是否返回
}CmdRespInfo;

#endif

