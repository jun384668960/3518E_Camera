CROSS := arm-hisiv300-linux-
CC := $(CROSS)gcc
CFLAG := -Wno-write-strings -lpthread -lp2p -lm -L./lib \
		-I../common -I../utils -I./include \
		
BINS := p2p

SRCS := $(wildcard ./src/*.c)
SRCS += $(wildcard ../utils/*.c)
OBJS := $(wildcard ./*.o ./src/*.o ../utils/*.o)

all: $(BINS)

p2p:
	$(CC) p2p.c $(SRCS) -o p2p $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)