#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <time.h>

/* Create a client endpoint and connect to a server.   Returns fd if all OK, <0 on error. */
int unix_socket_conn(const char *servername)
{
	int fd;
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)    /* create a UNIX domain stream socket */
	{
		return(-1);
	}
	int len, rval;
	struct sockaddr_un un;
	memset(&un, 0, sizeof(un));            /* fill socket address structure with our address */
	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "scktmp%05d", getpid());
	len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
	unlink(un.sun_path);               /* in case it already exists */
	if (bind(fd, (struct sockaddr *)&un, len) < 0)
	{
		rval = -2;
	}
	else
	{
		/* fill socket address structure with server's address */
		memset(&un, 0, sizeof(un));
		un.sun_family = AF_UNIX;
		strcpy(un.sun_path, servername);
		len = offsetof(struct sockaddr_un, sun_path) + strlen(servername);
		if (connect(fd, (struct sockaddr *)&un, len) < 0)
		{
			rval = -4;
		}
		else
		{
			return (fd);
		}
	}
	int err;
	err = errno;
	close(fd);
	errno = err;
	return rval;
}

void unix_socket_close(int fd)
{
	close(fd);
}


int main(void)
{
	srand((int)time(0));
	int connfd;
	connfd = unix_socket_conn("foo.sock");
	if (connfd < 0)
	{
		printf("Error[%d] when connecting...", errno);
		return 0;
	}
	printf("Begin to recv/send...\n");
	int i, n, size;
	char rvbuf[4096];
	for (i = 0; i < 10; i++)
	{
		/*
		 //=========½ÓÊÕ=====================
		 size = recv(connfd, rvbuf, 800, 0);   //MSG_DONTWAIT
		 if(size>=0)
		 {
		 printf("Recieved Data[%d]:%c...%c\n",size,rvbuf[0],rvbuf[size-1]);
		 }
		 if(size==-1)
		 {
		 printf("Error[%d] when recieving Data.\n",errno);
		 break;
		 }
		 if(size < 800) break;
		 */
		//=========·¢ËÍ======================
		memset(rvbuf, 'a', 2048);
		rvbuf[2047] = 'b';
		size = send(connfd, rvbuf, 2048, 0);
		if (size >= 0)
		{
			printf("Data[%d] Sended:%c.\n", size, rvbuf[0]);
		}
		if (size == -1)
		{
			printf("Error[%d] when Sending Data:%s.\n", errno, strerror(errno));
			break;
		}
		sleep(1);
	}
	unix_socket_close(connfd);
	printf("Client exited.\n");
}

