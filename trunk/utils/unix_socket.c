#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/un.h>
#include <errno.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "unix_socket.h"
#include "utils_log.h"

unix_socket_t* unix_socket_create(UNIX_SOCKET_MODE mode, int clnts, char* servername, char* clntname, recv_callback on_recv, status_callback on_status)
{
	unix_socket_t* sock = (unix_socket_t*)malloc(sizeof(unix_socket_t));

	int ret;
	if(mode == UNIX_SOCKET_SERVER)
	{
		sock->listenfd = -1;
		sock->max_clnt = clnts;
		ret = unix_socket_server_open(sock, servername);
		if(ret != 0)
		{
			free(sock);
			LOGE_print("mode:%d unix_socket_server_open error");
			return NULL;
		}
		sock->cur_clnts = 0;
		sock->clnt_array = (int*)malloc(sizeof(int)*clnts);
		memset(sock->clnt_array, 0x0, sizeof(int)*clnts);
	}
	else
	{
		sock->connfd = -1;
		ret = unix_socket_client_open(sock, clntname);
		if(ret != 0)
		{
			free(sock);
			LOGE_print("mode:%d unix_socket_client_open error");
			return NULL;
		}
		sock->clnt_array = NULL;
		snprintf(sock->clnt, 64, "%s", clntname);
	}

	snprintf(sock->server, 64, "%s", servername);
	sock->max_buf_size = 1024*2;
	sock->buffer = (char*)malloc(sock->max_buf_size);
	sock->mode = mode;
	sock->on_recv = on_recv;
	sock->on_status = on_status;
	sock->status = -1;
	sock->recv_tid = 0;
	sock->recv_stop = 0;
	
	return sock;
}

void unix_socket_destory(unix_socket_t* sock)
{
	int i;

	if(sock->recv_tid != 0)
	{
		sock->recv_stop = 1;
		pthread_join(sock->recv_tid, NULL);
	}
	
	for (i = 0; i < sock->max_clnt; i++)
	{
		if (sock->clnt_array[i] != 0)
		{
			close(sock->clnt_array[i]);
		}
	}
	
	if(sock->buffer != NULL)
		free(sock->buffer);
	if(sock->clnt_array != NULL)
		free(sock->clnt_array);

	if(sock->listenfd != -1) close(sock->listenfd);
	if(sock->connfd != -1) close(sock->connfd);
	
	if(sock != NULL)
		free(sock);
	
	sock = NULL;
}

int unix_socket_listen(unix_socket_t* sock)
{
	if (listen(sock->listenfd, sock->max_clnt) < 0)
	{
		LOGE_print("listen error");
		return -1;
	}
	else
	{
		sock->status = 1;
		return 0;
	}
}

int unix_socket_accept(unix_socket_t* sock)
{
	if(sock->recv_tid != 0)
		return 0;
	
	sock->recv_stop = 0;
	if(pthread_create(&sock->recv_tid, NULL, unix_socket_server_recv, sock) != 0)
	{
		LOGE_print("pthread_create unix_socket_server_recv errno:%d", errno);
		return -1;
	}
	
	return 0;
}

int unix_socket_reconn(unix_socket_t* sock)
{
	int ret;
	ret = unix_socket_client_open(sock, sock->clnt);
	if(ret != 0)
	{
		return ret;
	}
	ret = unix_socket_conn(sock, sock->server);
	if(ret != 0)
	{
		return ret;
	}
	
	return 0;
}

int unix_socket_conn(unix_socket_t* sock, const char *servername)
{
	struct sockaddr_un un;
	
	/* fill socket address structure with server's address */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, servername);
	
	int len = offsetof(struct sockaddr_un, sun_path) + strlen(servername);
	if (connect(sock->connfd, (struct sockaddr *)&un, len) < 0)
	{
//		LOGE_print("connect error");
		return -1;
	}
	else
	{
		sock->status = 1;
		sock->recv_stop = 0;
		if(sock->recv_tid != 0) return 0;
		if(pthread_create(&sock->recv_tid, NULL, unix_socket_client_recv, sock) != 0)
		{
			LOGE_print("pthread_create unix_socket_server_recv errno:%d", errno);
			return -1;
		}
		
		return 0;
	}
}

int unix_socket_clnt_send(unix_socket_t* sock, char* data, unsigned int length)
{
	if(sock == NULL || data == NULL || length == 0)
		return -1;
	
	int size = send(sock->connfd, data, length, 0);
	
	return size;
}

int unix_socket_send(char* data, unsigned int length, int fd)
{	
	if(data == NULL || length == 0)
		return -1;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);

	int size = send(fd, data, length, 0);
	
	return size;
}

int unix_socket_server_open(unix_socket_t* sock, char* servername)
{
	int fd;
	struct sockaddr_un un;
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		LOGE_print("socket error");
		return -1;
	}
	
	int len, rval;
	unlink(servername); 			  /* in case it already exists */
	memset(&un, 0, sizeof(un));
	un.sun_family = AF_UNIX;
	strcpy(un.sun_path, servername);
	
	len = offsetof(struct sockaddr_un, sun_path) + strlen(servername);
	/* bind the name to the descriptor */
	if (bind(fd, (struct sockaddr *)&un, len) < 0)
	{
		LOGE_print("bind error");
		rval = -2;
	}
	else
	{
		sock->status = 0;
		sock->listenfd = fd;
		return 0;
	}
	
	close(fd);
	return rval;
}

int unix_socket_client_open(unix_socket_t* sock, char* clntname)
{
	int fd;
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)    /* create a UNIX domain stream socket */
	{
		LOGE_print("socket error");
		return(-1);
	}
	
	int len, rval;
	struct sockaddr_un un;
	memset(&un, 0, sizeof(un));            	/* fill socket address structure with our address */
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "%s", clntname);
	
	len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
	unlink(un.sun_path);               		/* in case it already exists */
	
	if (bind(fd, (struct sockaddr *)&un, len) < 0)
	{
		LOGE_print("bind error");
		close(fd);
		return -1;
	}
	sock->status = 0;
	if(sock->connfd != -1) 
		close(sock->connfd);
	sock->connfd = fd;
	
	return 0;
}


void* unix_socket_server_recv(void* arg)
{
	unix_socket_t* sock = (unix_socket_t*)arg;
	int i;
	struct timeval tv;
	int maxsock;
	
	while(sock->recv_stop == 0)
	{
		// 初始化文件描述符集合 initialize file descriptor set
		FD_ZERO(&sock->fdsr);
		// 把Sock_fd加入到文件描述符集合
		FD_SET(sock->listenfd, &sock->fdsr);
		// 超时设置30秒
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		int max = sock->listenfd;
		// 把活动的socket的句柄加入到文件描述符集合中
		for (i = 0; i < sock->max_clnt; i++)
		{
			if (sock->clnt_array[i] != 0)
			{
				LOGW_print("==================== %d", sock->clnt_array[i]);
				FD_SET(sock->clnt_array[i], &sock->fdsr);
				
				if(sock->clnt_array[i] > max) 
					max = sock->clnt_array[i];
			}
		}
		maxsock = max;

		int ret = select(maxsock + 1, &sock->fdsr, NULL, NULL, &tv);
		if (ret < 0)
		{
			LOGE_print("select error ret:%d", ret);
			break;
		}
		else if (ret == 0)
		{
			LOGI_print("timeout");
			continue;
		}

		if (FD_ISSET(sock->listenfd, &sock->fdsr))
		{
			int clnt;
			struct sockaddr_un un;
			struct stat statbuf;
			int len = sizeof(un);
			if ((clnt = accept(sock->listenfd, (struct sockaddr *)&un, (socklen_t*)&len)) < 0)
			{
				LOGE_print("accept error");
			}
			/* obtain the client's uid from its calling address */
			len -= offsetof(struct sockaddr_un, sun_path);	/* len of pathname */
			un.sun_path[len] = 0; /* null terminate */
			if (stat(un.sun_path, &statbuf) >= 0)
			{
				if (S_ISSOCK(statbuf.st_mode))
				{
					unlink(un.sun_path);	   /* we're done with pathname now */
					//加入
					FD_SET(clnt, &sock->fdsr);
					maxsock = clnt;
					for (i = 0; i < sock->max_clnt; i++)
					{
						if (sock->clnt_array[i] == 0)
						{
							sock->clnt_array[i] = clnt;
							break;
						}
					}
				}
				else
				{
					LOGE_print("S_ISSOCK error");
					close(clnt);
				}
			}
			else
			{
				LOGE_print("stat error");
				close(clnt);
			}
		}
		else
		{
			for (i = 0; i < sock->max_clnt; i++)
			{
				if (FD_ISSET(sock->clnt_array[i], &sock->fdsr))
				{
					//接收数据
					ret = recv(sock->clnt_array[i], sock->buffer, sock->max_buf_size, 0);
					if (ret <= 0) //接收数据出错
					{		
						FD_CLR(sock->clnt_array[i], &sock->fdsr);
						close(sock->clnt_array[i]);
						if(sock->on_status != NULL)
							(sock->on_status)(sock, sock->clnt_array[i], ret);
						sock->clnt_array[i] = 0;
						LOGE_print("client[%d] close error:%d", i, ret);
					}
					else
					{
						if(sock->on_recv != NULL)
							(sock->on_recv)(sock, sock->buffer, ret, sock->clnt_array[i]);
					}
				}
			}
		}
	}

	return NULL;
}

void* unix_socket_client_recv(void* arg)
{
	unix_socket_t* sock = (unix_socket_t*)arg;
	int i;
	struct timeval tv;
	
	while(sock->recv_stop == 0)
	{
		int maxsock = sock->connfd;
		if(sock->status != 1)
		{
			unix_socket_reconn(sock);
			usleep(100*1000);
			continue;
		}
		
		FD_ZERO(&sock->fdsr);
		if(sock->connfd != -1)
			FD_SET(sock->connfd, &sock->fdsr);
		
		tv.tv_sec = 30;
		tv.tv_usec = 0;
		int ret = select(maxsock + 1, &sock->fdsr, NULL, NULL, &tv);
		if (ret < 0)
		{
			LOGE_print("select error");
			break;
		}
		else if (ret == 0)
		{
			LOGI_print("timeout");
			continue;
		}
		if (FD_ISSET(sock->connfd, &sock->fdsr))
		{
			//接收数据
			ret = recv(sock->connfd, sock->buffer, sock->max_buf_size, 0);
			if (ret == 0) //接收数据出错
			{		
				FD_CLR(sock->connfd, &sock->fdsr);
				sock->status = -1;
				sock->connfd = -1;
				LOGE_print("client[%d] close error:%d", i, errno);
				if(sock->on_status != NULL)
					(sock->on_status)(sock, sock->connfd, sock->status);
			}
			else if(ret < 0)
			{

			}
			else
			{
				if(sock->on_recv != NULL)
					(sock->on_recv)(sock, sock->buffer, ret, sock->connfd);
			}
		}
	}
	
	return NULL;
}

