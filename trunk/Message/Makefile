CROSS := arm-hisiv300-linux-
CC := $(CROSS)gcc
CFLAG := -Wno-write-strings -lpthread -L./lib \
		-I../common -I../utils -I./include \
		
BINS := message_center

SRCS := $(wildcard ./src/*.c)
SRCS += $(wildcard ../utils/*.c)
OBJS := $(wildcard ./*.o ./src/*.o ../utils/*.o)

all: $(BINS)

message_center:
	$(CC) message_center.c $(SRCS) -o message_center $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)