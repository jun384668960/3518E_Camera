#include <stdio.h>  
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "ipc_utils.h"
#include "ipc_shm_utils.h"
#include "utils_log.h"

#define SHM_KEY				131415
#define SHM_DEFAULT_SIZE	1024*1024*2

#define MSG_SERVER	1234
#define MSG_RECV1	12341
#define MSG_RECV2	12342
#define MSG_SEND1	12343

typedef struct shm_node_s
{
	int offset;
	int length;
}shm_node_s;

static ipc_shm_handle*  g_handle = NULL;

static int on_conn_recv(struct ipc_clnt_handle_s* clnt, ipc_st data)
{
	LOGD_print("data.type:%d data.form:%d, data.to:%d", data.type, data.form, data.to);
	shm_node_s node;
	memcpy(&node, data.body, sizeof(shm_node_s));

//	char frame[150*1024] = {0};
	if(g_handle != NULL)
	{
		char* frame = NULL;
		LOGD_print("offset:%d length:%d", node.offset, node.length);
		int ret = ipc_shm_read(g_handle, &frame, node.length, node.offset);
		if(ret == 0)
		{
			static FILE* h264 = NULL;
			if(h264 == NULL)
				h264 = fopen("./read_stream0.h264", "wb");
			if(h264 != NULL)
			{
				fwrite(frame, node.length, 1, h264);
				fflush(h264);
			}
		}
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	log_ctrl* log = log_ctrl_create("./shm_utils_read1.log", LOG_DEBUG, 1);
	
	g_handle = ipc_shm_open(SHM_KEY, SHM_DEFAULT_SIZE, 0);
	ipc_clnt_handle* clnt = ipc_clnt_create(MSG_RECV1, on_conn_recv);
	if(clnt == NULL)
	{
		LOGE_print("ipc_clnt_create error");
		return -1;
	}
		

	int ret = ipc_clnt_login(clnt, MSG_SERVER);
	if(ret != 0)
	{
		LOGD_print("ipc_clnt_login error");
		return -1;
	}

	LOGD_print("ipc_clnt_login OK");
	int count = 0;
	while(1)
	{
		usleep(2*1000*1000);
		continue;
	}
	LOGD_print("end");

	ipc_clnt_logout(clnt);
	ipc_clnt_destory(clnt);
	ipc_shm_close(g_handle, 0);
	log_ctrl_destory(log);
	
	return 0;
}

