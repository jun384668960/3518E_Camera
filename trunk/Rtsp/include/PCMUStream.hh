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


#ifndef _PCMU_STREAM_HH
#define _PCMU_STREAM_HH
#include "FramedSource.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "SimpleRTPSink.hh"
#include "stream_manager.h"

class PCMUStream: public FramedSource {
public:
	static PCMUStream* createNew(UsageEnvironment& env, int sub);
	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }
	// returns the 'AudioSpecificConfig' for this stream (in ASCII form)

private:
	PCMUStream(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration);
	// called only by createNew()
	virtual ~PCMUStream();

	static void incomingDataHandler(PCMUStream* source);
	void incomingDataHandler1();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

private:
	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	unsigned fuSecsPerFrame;
	char fConfigStr[5];

	shm_stream_t* 		   m_StreamList;
	unsigned int m_ref;
};



class PCMUServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static PCMUServerMediaSubsession* createNew(UsageEnvironment& env, int sub, Boolean reuseFirstSource);
protected:
	PCMUServerMediaSubsession(UsageEnvironment& env, int sub, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~ PCMUServerMediaSubsession();

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                		unsigned char rtpPayloadTypeIfDynamic,
				       	FramedSource* inputSource);
	int sub;
};

#endif
