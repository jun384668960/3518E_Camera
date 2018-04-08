/******************************************************************************

                  版权所有 (C), 2012-2022, Goscam

 ******************************************************************************
  文 件 名   : 
  版 本 号   : 初稿
  作    者   : chenjx
  生成日期   : 2014年10月10日
  最近修改   :
  功能描述   : 
  函数列表   :
  修改历史   :
  1.日    期   : 2014年6月5日
    作    者   : chenjx
    修改内容   : 创建文件

******************************************************************************/


#ifndef _ADTS_MAIN_STREAM_HH
#define _ADTS_MAIN_STREAM_HH
#include "FramedSource.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "stream_manager.h"

class ADTSMainSource: public FramedSource {
public:
	static ADTSMainSource* createNew(UsageEnvironment& env, int sub);
	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }
	// returns the 'AudioSpecificConfig' for this stream (in ASCII form)

private:
	ADTSMainSource(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration);
	// called only by createNew()
	virtual ~ADTSMainSource();

	static void incomingDataHandler(ADTSMainSource* source);
	void incomingDataHandler1();
	
private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

private:
	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	unsigned fuSecsPerFrame;
	char fConfigStr[5];
	shm_stream_t* 		   m_StreamList;;
	unsigned int uSecond;
	unsigned long long 	   m_ref;
};

class ADTSMainServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static ADTSMainServerMediaSubsession*createNew(UsageEnvironment& env, int sub, Boolean reuseFirstSource);
protected:
	ADTSMainServerMediaSubsession(UsageEnvironment& env, int sub, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~ADTSMainServerMediaSubsession();

	virtual void startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData);

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
               unsigned char rtpPayloadTypeIfDynamic,
		       FramedSource* inputSource);
	int sub;
};

#endif
