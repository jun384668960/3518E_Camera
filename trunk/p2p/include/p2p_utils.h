#ifndef P2P_UTILS_H
#define P2P_UTILS_H

#include "p2p_define.h"

P2pHead private_head_format(int size, int msgType, int type, int protoType);
int private_head_check(unsigned char* data, int len);
int private_head_dsize_enough(unsigned char* data, int len);

#endif
