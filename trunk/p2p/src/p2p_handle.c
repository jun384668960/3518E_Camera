#include "p2p_handle.h"
#include "p2p_turn.h"
#include "tcp_turn.h"
#include "tcp_push.h"
#include <string.h>

int p2p_handle_uid_type(char* uid)
{
	int type;

	if(strlen(uid) != 15)
		return HANDLE_TYPE_NONE;
	if(uid[0] != 'A' && uid[0] != 'Z')
		return HANDLE_TYPE_NONE;
	
	//A99762001001001
	if(uid[5] == '3')
	{
		type = HANDLE_TYPE_TCP;
	}
	else if(uid[5] == '2')
	{
		type = HANDLE_TYPE_P2P;
	}
	else
	{
		type = HANDLE_TYPE_NONE;
	}
	return type;
}

p2p_handle* p2p_handle_create(char* uid)
{
	//根据uid判断能理解 p2p || tcp
	int type = p2p_handle_uid_type(uid);

	//创建不同的句柄
	if(type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_create();
	}
	else if(type == HANDLE_TYPE_P2P)
	{	
		return p2p_trans_create();
	}
	
	return NULL;
}

void p2p_handle_destory(p2p_handle* handle)
{
	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		tcp_turn_destory((tcp_turn*)handle);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		p2p_trans_destory((p2p_trans*)handle);
	}
}

int p2p_handle_init(p2p_handle* handle, char* dispathce_server, char* server, unsigned short port, char* uid, char* pwd, int tcp)
{
	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_init((tcp_turn*)handle, dispathce_server, server, port, uid);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		return p2p_trans_init((p2p_trans*)handle, dispathce_server, server, port, uid, pwd, tcp);
	}

	return -1;
}

int p2p_handle_uninit(p2p_handle* handle)
{
	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_uninit((tcp_turn*)handle);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		return p2p_trans_uninit((p2p_trans*)handle);
	}

	return -1;
}

int p2p_handle_write(p2p_handle* handle, unsigned char* data, unsigned int len, int type, unsigned int time_stamp, char is_key)
{
	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_av_write((tcp_turn*)handle, data, len, type, time_stamp, is_key);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		return p2p_trans_write((p2p_trans*)handle, data, len, type, time_stamp, is_key);
	}
	
	return -1;
}

int p2p_handle_av_swtich(p2p_handle* handle, int type)
{
	if(handle == NULL) return -1;

	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_av_swtich(type);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		return p2p_trans_av_swtich(type);
	}

	return -1;
}

int p2p_handle_param_set(p2p_handle* handle, int type, char* val)
{
	if(handle->m_type == HANDLE_TYPE_TCP)
	{
		return tcp_turn_param_set(type, val);
	}
	else if(handle->m_type == HANDLE_TYPE_P2P)
	{
		return p2p_trans_param_set(type, val);
	}

	return -1;
}



