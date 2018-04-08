CROSS := arm-hisiv300-linux-
CC := $(CROSS)g++
CFLAG := -Wno-write-strings -lpthread -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment -L./libs \
		-I../utils -I./include \
		-I./live555/BasicUsageEnvironment/include \
		-I./live555/groupsock/include \
		-I./live555/liveMedia/include \
		-I./live555/UsageEnvironment/include
		
BINS := rtsp_server

SRCS := $(wildcard ./src/*.cpp)
SRCS += $(wildcard ../utils/*.c)
OBJS := $(wildcard ./*.o ./src/*.o ../utils/*.o)

all: $(BINS)

rtsp_server:
	$(CC) rtsp_server.c $(SRCS) -o rtsp_server $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)