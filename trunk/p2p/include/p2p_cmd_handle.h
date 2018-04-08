#ifndef P2P_CMD_HANDLE_H
#define P2P_CMD_HANDLE_H

#include "p2p_conn_client.h"

//default function
int speak_data_send(unsigned char* data, unsigned int lenght);
int speak_data_handle(p2p_conn_clnt* clnt, void* data, int len,  P2pHead info);
int cmd_handle(p2p_conn_clnt* clnt, void* data, int len,  P2pHead info);
//default function 

//////////////////////////////////////////////////////////////////////////////////////////
int cmd_video_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_video_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_audio_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_audio_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_speak_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_speak_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_stream_ctrl_get(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_stream_ctrl_set(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_PTZ_ctrl(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_get_record_file_start(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_get_record_file_stop(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_get_record_file_lock(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_get_record_file_resend(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_play_record(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);
int cmd_stop_play_record(p2p_conn_clnt* clnt, unsigned char* cmd, unsigned int len, CmdRespInfo* respInfo);

//////////////////////////////////////////////////////////////////////////////////////////

int globle_vquality_init();
int globle_vquality_get();
int globle_vswitch_get();
int globle_aswitch_get();
#endif