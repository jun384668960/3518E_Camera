#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "unix_socket.h"
#include "utils_log.h"

static void on_recv(void* sock, char* data, unsigned int len, int fd)
{
	LOGI_print("%d %s %d", fd, data, len);
	//unix_socket_send(data, len, fd);
	unix_socket_server_broadcast((unix_socket_t*)sock, data, len, fd);
}

static void on_status(void* sock, int fd, int status)
{
	LOGI_print("%d %d", fd, status);
}

int main()
{
	unix_socket_t* sock = unix_socket_create(UNIX_SOCKET_SERVER, 10, "test", "", on_recv, on_status);
	unix_socket_listen(sock);
	unix_socket_accept(sock);
	while(1)
	{
		usleep(1000*1000);
	}
	
	unix_socket_destory(sock);
	
	return 0;
}
