#CROSS := arm-hisiv300-linux-
CC := $(CROSS)gcc
CFLAG := -lpthread -I../
#BINS := stream_write stream_read1 stream_read2
BINS := unix_socket_svr unix_socket_clnt
#BINS := stream_write stream_read1 stream_read2 ipc_utils_test shm_utils_write shm_utils_read1 shm_utils_read2 ipc_utils_clnt ipc_utils_clnt2 
#BINS := ipc_utils_test shm_utils_write shm_utils_read1 shm_utils_read2 ipc_utils_clnt ipc_utils_clnt2 
SRCS := $(wildcard ../*.c)
OBJS := $(wildcard ../*.o ./*.o)

all: $(BINS)

ipc_utils_clnt:
	$(CC) ipc_utils_clnt.c ../ipc_utils.c ../utils_log.c -o ipc_utils_clnt $(CFLAG)

ipc_utils_clnt2:
	$(CC) ipc_utils_clnt2.c ../ipc_utils.c ../utils_log.c -o ipc_utils_clnt2 $(CFLAG)
	
ipc_utils_test:
	$(CC) ipc_utils_test.c ../ipc_utils.c ../utils_log.c -o ipc_utils_test $(CFLAG)

shm_utils_write:
	$(CC) shm_utils_write.c ../ipc_utils.c ../ipc_shm_utils.c ../utils_log.c -o shm_utils_write $(CFLAG)

shm_utils_read1:
	$(CC) shm_utils_read1.c ../ipc_utils.c ../ipc_shm_utils.c ../utils_log.c -o shm_utils_read1 $(CFLAG)

shm_utils_read2:
	$(CC) shm_utils_read2.c ../ipc_utils.c ../ipc_shm_utils.c ../utils_log.c -o shm_utils_read2 $(CFLAG)

stream_write:
	$(CC) stream_write.c $(SRCS) -o stream_write $(CFLAG)

stream_read1:
	$(CC) stream_read1.c $(SRCS) -o stream_read1 $(CFLAG)

stream_read2:
	$(CC) stream_read2.c $(SRCS) -o stream_read2 $(CFLAG)

unix_socket_server:
	$(CC) unix_socket_server.c $(SRCS) -o unix_socket_server $(CFLAG)

unix_socket_client:
	$(CC) unix_socket_client.c $(SRCS) -o unix_socket_client $(CFLAG)

unix_socket_svr:
	$(CC) unix_socket_svr.c $(SRCS) -o unix_socket_svr $(CFLAG)

unix_socket_clnt:
	$(CC) unix_socket_clnt.c $(SRCS) -o unix_socket_clnt $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)