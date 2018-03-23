CC := gcc
CFLAG := -lpthread -I../
BINS := stream_write stream_read1 stream_read2 ipc_utils_test shm_utils_write shm_utils_read1 shm_utils_read2 ipc_utils_clnt ipc_utils_clnt2 
#ipc_utils_test shm_utils_write shm_utils_read1 shm_utils_read2 ipc_utils_clnt ipc_utils_clnt2 
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
	$(CC) -g3 shm_utils_read1.c ../ipc_utils.c ../ipc_shm_utils.c ../utils_log.c -o shm_utils_read1 $(CFLAG)

shm_utils_read2:
	$(CC) shm_utils_read2.c ../ipc_utils.c ../ipc_shm_utils.c ../utils_log.c -o shm_utils_read2 $(CFLAG)

stream_write:
	$(CC) stream_write.c $(SRCS) -o stream_write $(CFLAG)

stream_read1:
	$(CC) stream_read1.c $(SRCS) -o stream_read1 $(CFLAG)

stream_read2:
	$(CC) stream_read2.c $(SRCS) -o stream_read2 $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)