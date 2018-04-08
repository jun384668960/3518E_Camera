#include <stdio.h>
#include <string.h>
#include "p2p_utils.h"

int private_head_dsize_enough(unsigned char* data, int len)
{
	if(data == NULL || len <= 0) return -1;
	
	P2pHead* head = (P2pHead*)data;
	
	if(head->size + sizeof(P2pHead) <= (unsigned int)len)
	{
		return head->size + sizeof(P2pHead);
	}
	else
	{
		return -1;
	}
}

int private_head_check(unsigned char* data, int len)
{
	if(data == NULL || len < (int)sizeof(P2pHead)) return -1;

	P2pHead* head = (P2pHead*)data;
	if(head->flag != 0x67736d80)
	{
		printf("PrivHeadCheck flag error\n");
		return -1;
	}

	if(head->size + sizeof(P2pHead) != (unsigned int)len )
	{
		printf("PrivHeadCheck size:%d len%d error\n",head->size,len);
		return -1;
	}

	return 0;

}

P2pHead private_head_format(int size, int msgType, int type, int protoType)
{
	P2pHead head;
	memset(&head, 0, sizeof(P2pHead));

	head.flag = 0x67736d80;
	head.size = size;
	head.type = type;
	head.protoType = protoType;
	head.msgType = msgType;

	return head;
}

