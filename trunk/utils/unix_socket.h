#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

#include <sys/select.h>
#include <pthread.h>

typedef enum
{
	UNIX_SOCKET_SERVER,
	UNIX_SOCKET_CLIENT,
}UNIX_SOCKET_MODE;

typedef void (*recv_callback)(void* sock, char* data, unsigned int len, int fd);
typedef void (*status_callback)(void* sock, int fd, int status);

typedef struct unix_socket_st
{
	recv_callback	on_recv;
	status_callback   on_status;
	UNIX_SOCKET_MODE		mode;
	int		status;
	char	server[64];
	char	clnt[64];
	
	int 	listenfd;
	int 	max_clnt;
	int     cur_clnts;
	int*	clnt_array;
	fd_set 	fdsr;;		//	监控文件描述符集合
	int 	maxsock;	//	监控文件描述符集合中最大的文件号
	int		timeout_ms;	//	select 超时时长 ms
	char*	buffer;
	int 	max_buf_size;
	pthread_t	recv_tid;
	int			recv_stop;
	
	int		connfd;
}unix_socket_t;

//public
unix_socket_t* unix_socket_create(UNIX_SOCKET_MODE mode, int clnts, char* servername, char* clntname, recv_callback on_recv, status_callback on_status);
void unix_socket_destory(unix_socket_t* sock);

int unix_socket_listen(unix_socket_t* sock);
int unix_socket_accept(unix_socket_t* sock);

int unix_socket_reconn(unix_socket_t* sock);
int unix_socket_conn(unix_socket_t* sock, const char *servername);
int unix_socket_clnt_send(unix_socket_t* sock, char* data, unsigned int length);
int unix_socket_send(char* data, unsigned int length, int fd);
int unix_socket_server_broadcast(unix_socket_t* sock, char* data, unsigned int length, int fd);

//private
int unix_socket_server_open(unix_socket_t* sock, char* servername);
int unix_socket_client_open(unix_socket_t* sock, char* clntname);
void* unix_socket_server_recv(void* arg);
void* unix_socket_client_recv(void* arg);

#endif
