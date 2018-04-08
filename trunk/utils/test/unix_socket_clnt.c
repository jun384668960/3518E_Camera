#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "unix_socket.h"
#include "utils_log.h"

static void on_recv(void* sock, char* data, unsigned int len, int fd)
{
	LOGI_print("%d %s %d", fd, data, len);
}

static void on_status(void* param, int fd, int status)
{
	unix_socket_t* sock = (unix_socket_t*)param;
	LOGI_print("%d %d", fd, status);
//	if(status == -1)
//	{
//		while(sock->status != 1)
//		{
//			LOGI_print("%d %d", fd, status);
//			unix_socket_reconn(sock);

//			usleep(1000*1000);
//		}
//	}
}

int main()
{
	unix_socket_t* sock = unix_socket_create(UNIX_SOCKET_CLIENT, 1, "test", "clnt1", on_recv, on_status);

	unix_socket_conn(sock, "test");
	int count = 0;
	char data[64] = {0};
	while(1)
	{
		sprintf(data, "data%d",count++);
		usleep(1000*1000);
		unix_socket_clnt_send(sock, data, strlen(data));
	}
	
	unix_socket_destory(sock);
	
	return 0;
}

