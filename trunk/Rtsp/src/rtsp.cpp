#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "H264MainStream.hh"
#include "H264SubStream.hh"
#include "ADTSMainStream.hh"
#include "PCMUStream.hh"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "rtsp.h"
#include "utils_log.h"
#ifdef __cplusplus
extern "C"{
#endif

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,char const* streamName, char const* inputFileName); // fwd

static pthread_t MianStreamTh 	= 0;
static char fgMianStreamExit 	= 0;
Boolean reuseFirstSource 		= True;

void *vMainStreamPorc(void *pro)
{
	UsageEnvironment* env;
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
	UserAuthenticationDatabase* authDB = NULL;

#if RTSP_ACCESS_CONTROL
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord(rtsp_user[0].name, rtsp_user[0].passwd); // replace these with real strings
#endif

    RTSPServer* rtspServer = RTSPServer::createNew(*env, 554, authDB);
	if (rtspServer == NULL) 
	{
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		RTSPExit();
		return NULL;
	}
	
    char const* descriptionString = "Session streamed by IP-camer";
	char streamName[32]={'\0'};
	sprintf(streamName,"stream0");
	char const* inputFileName = "goscam";

	char streamName1[32]={'\0'};
	sprintf(streamName1,"stream1");
	LOGD_print("streamName:%s", streamName);

	ServerMediaSession* msms = ServerMediaSession::createNew(*env, streamName, streamName,descriptionString);
//	ServerMediaSession* msms1 = ServerMediaSession::createNew(*env, streamName1, streamName1,descriptionString);

	OutPacketBuffer::maxSize = 500000;

		msms->addSubsession(H264LiveVideoServerMediaSubsession::createNew(*env, reuseFirstSource));
//		msms1->addSubsession(H264SubLiveVideoServerMediaSubsession::createNew(*env, reuseFirstSource));

//	if(1)
//	{
		msms->addSubsession(PCMUServerMediaSubsession::createNew(*env, 0, reuseFirstSource));  
//		msms1->addSubsession(PCMUServerMediaSubsession::createNew(*env, 0, reuseFirstSource));	
//	}
//	else
//	{
//		msms->addSubsession(ADTSMainServerMediaSubsession::createNew(*env, 0, reuseFirstSource));  
//		msms1->addSubsession(ADTSMainServerMediaSubsession::createNew(*env, 0, reuseFirstSource));	
//	}

	rtspServer->addServerMediaSession(msms);
//	rtspServer->addServerMediaSession(msms1);
 	announceStream(rtspServer, msms, streamName, inputFileName);
//	announceStream(rtspServer, msms1, streamName1, inputFileName);
	
	#ifdef RTSP_OVER_HTTP
	if (rtspServer->setUpTunnelingOverHTTP(8080)) 
		*env << "We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling\n";
	else 
	    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
	#endif
	
	env->taskScheduler().doEventLoop(&fgMianStreamExit); // does not return
	return NULL; // only to prevent compiler warning
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,char const* streamName, char const* inputFileName) 
{
	char* url = rtspServer->rtspURL(sms);
  	UsageEnvironment& env = rtspServer->envir();
  	env << "Play this stream using the URL \"" << url << "\"\n";
  	delete[] url;
}

void RTSPInit()
{
	LOGD_print("RTSPInit");
	if(MianStreamTh == 0)
	{
		int err = pthread_create(&MianStreamTh, NULL, vMainStreamPorc,NULL);
		if(err != 0)
		{
			fprintf(stderr, "%s\n", strerror(err));
			return;
		}
		fgMianStreamExit =  0;
	}

	LOGD_print("RTSPInit done");
}

void RTSPExit()
{
	int err = 0;
	fgMianStreamExit =  1;
	if((err = pthread_cancel(MianStreamTh)) != 0)
	{
		fprintf(stderr, "%s\n", strerror(err));
		return;
	}
	if(MianStreamTh != 0)
	{
		pthread_join(MianStreamTh, NULL);
		MianStreamTh = 0;
	}
}

#ifdef __cplusplus
}
#endif

