#include <GroupsockHelper.hh>
#include "H264MainStream.hh"

#include <pthread.h>
#include "utils_log.h"
#include "H264VideoLiveDiscreteFramer.hh"
#include "nalu_utils.hh"
#include "stream_define.h"

H264LiveVideoSource* H264LiveVideoSource::createNew(UsageEnvironment& env,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame) 
{
    H264LiveVideoSource* videosource = new H264LiveVideoSource(env, preferredFrameSize, playTimePerFrame);
	return videosource;
}
H264LiveVideoSource::H264LiveVideoSource(UsageEnvironment& env,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame)
: FramedSource(env),fPreferredFrameSize(preferredFrameSize), fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0)
{
	fPresentationTime.tv_sec  = 0;
	fPresentationTime.tv_usec = 0;
	
	m_StreamList = shm_stream_create("rtsp_mainread", "mainstream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	if(m_StreamList == NULL)
	{
		LOGE_print("shm_stream_create Failed");
	}

	m_framer = NULL;
	m_front_nalu_type = 0;
	m_front_nalu_len = 0;
	m_ref = 0;

}
H264LiveVideoSource::~H264LiveVideoSource() 
{
	if(m_StreamList != NULL)
	{
		shm_stream_destory(m_StreamList);
	}
}

void H264LiveVideoSource::setFramer(H264VideoStreamFramer* framer)
{
	m_framer = framer;
}

void H264LiveVideoSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void H264LiveVideoSource::incomingDataHandler(H264LiveVideoSource* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void H264LiveVideoSource::incomingDataHandler1()
{	
	fFrameSize = 0;
	frame_info info;
	unsigned char* pData;
	unsigned int length;
	int ret = shm_stream_get(m_StreamList, &info, &pData, &length);
	if(ret == 0)
	{	
//		LOGI_print("pts:%llu length:%d info=>length:%d", info.pts, length, info.length);
		int nalu_type = 0;

		fFrameSize = info.length;
		if(fFrameSize > fMaxSize)
		{
			fNumTruncatedBytes = fFrameSize - fMaxSize;
			fFrameSize = fMaxSize;
		}
		else
		{
			fNumTruncatedBytes = 0;
		}

		unsigned char* data = pData;
		nalu_type = data[4] & 0x1f;

		if(nalu_type == 7)
		{
			if(m_front_nalu_type == 0)		//第一次取sps+pps+i -> sps
			{
				int nalu_len = get_annexb_nalu(data+m_front_nalu_len, fFrameSize-m_front_nalu_len);
				fFrameSize = nalu_len - 4;
				memcpy(fTo, data + m_front_nalu_len + 4, fFrameSize);
				m_front_nalu_len += nalu_len;
				m_front_nalu_type = 7;
			}
			else if(m_front_nalu_type == 7)	//第一次取sps+pps+i -> pps
			{
				int nalu_len = get_annexb_nalu(data+m_front_nalu_len, fFrameSize-m_front_nalu_len);
				fFrameSize = nalu_len - 4;
				memcpy(fTo, data + m_front_nalu_len + 4, fFrameSize);
				m_front_nalu_len += nalu_len;
				m_front_nalu_type = 8;
			}
			else if(m_front_nalu_type == 8)	//第一次取sps+pps+i -> 06
			{
				int nalu_len = get_annexb_nalu(data+m_front_nalu_len, fFrameSize-m_front_nalu_len);
				fFrameSize = nalu_len - 4;
				//memcpy(fTo, data + m_front_nalu_len + 4, fFrameSize);
				m_front_nalu_len += nalu_len;
				m_front_nalu_type = 6;
			}
			else if(m_front_nalu_type == 6)	//第一次取sps+pps+i -> i frame
			{
				fFrameSize = info.length - m_front_nalu_len - 4;
				memcpy(fTo, data + m_front_nalu_len + 4, fFrameSize);
				nalu_type = 5;
				m_front_nalu_type = 0;
				m_front_nalu_len = 0;
			}
		}
		else
		{
			fFrameSize = info.length - 4;
			memcpy(fTo, data + 4, fFrameSize);
		}

		if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) 
		{
			// This is the first frame, so use the current time:
			gettickcount(&fPresentationTime, NULL);
			m_ref = info.pts;
		} 
		else if(nalu_type == 1 || nalu_type == 5)
		{
			// Increment by the play time of the previous frame:
			unsigned uSeconds = fPresentationTime.tv_usec + (unsigned int)(info.pts - m_ref);
			fPresentationTime.tv_sec += uSeconds/1000000;
			fPresentationTime.tv_usec = uSeconds%1000000;
			m_ref = info.pts;
		}
		fDurationInMicroseconds = 1000000 / 30;
		
		if(nalu_type == 1 || nalu_type == 5)
		{
//			LOGI_print("pts:%llu length:%d info=>length:%d", info.pts, length, info.length);
			int cnt = shm_stream_remains(m_StreamList);
			LOGD_print("cnt:%d framer video pts:%llu fFrameSize:%d", cnt, info.pts, fFrameSize);
		}
		if(m_front_nalu_type == 6 || m_front_nalu_type == 9)	//skip 06 09
		{
			nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
					(TaskFunc*)incomingDataHandler, this);
		}
		else
		{
			shm_stream_post(m_StreamList);
			nextTask() = envir().taskScheduler().scheduleDelayedTask(0,(TaskFunc*)FramedSource::afterGetting, this);
		}
	}
	else
	{	
		nextTask() = envir().taskScheduler().scheduleDelayedTask(5*1000,
			(TaskFunc*)incomingDataHandler, this);
	}
}

H264LiveVideoServerMediaSubsession* H264LiveVideoServerMediaSubsession::createNew(UsageEnvironment& env, Boolean reuseFirstSource ) 
{
	return new H264LiveVideoServerMediaSubsession(env, reuseFirstSource);
}
 
static void checkForAuxSDPLine(void* clientData) 
{
 	H264LiveVideoServerMediaSubsession* subsess = (H264LiveVideoServerMediaSubsession*)clientData;
   	subsess->checkForAuxSDPLine1();
}
 
void H264LiveVideoServerMediaSubsession::checkForAuxSDPLine1() 
{
	char const* dasl;
    if (fAuxSDPLine != NULL) 
	{
      // Signal the event loop that we're done:
     	setDoneFlag();
   	} 
	else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) 
	{
      	fAuxSDPLine = strDup(dasl);
     	fDummyRTPSink = NULL;
      // Signal the event loop that we're done:
     	setDoneFlag();
   	} 
	else 
	{
     // try again after a brief delay:
     	int uSecsToDelay = 100000; // 100 ms
    	nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,(TaskFunc*)checkForAuxSDPLine, this);
  	}
}

void H264LiveVideoServerMediaSubsession::afterPlayingDummy1() 
{
   // Unschedule any pending 'checking' task:
 	envir().taskScheduler().unscheduleDelayedTask(nextTask());
   // Signal the event loop that we're done:
   	setDoneFlag();
}
 
 void H264LiveVideoServerMediaSubsession::startStream(unsigned clientSessionId,
						 void* streamToken,
						 TaskFunc* rtcpRRHandler,
						 void* rtcpRRHandlerClientData,
						 unsigned short& rtpSeqNum,
						 unsigned& rtpTimestamp,
						 ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						 void* serverRequestAlternativeByteHandlerClientData) {
 
	//demand idr
//	GOS_SDK_VENC_RequestIFrame(0, 0, 1);
	 
	OnDemandServerMediaSubsession::startStream(clientSessionId,streamToken,rtcpRRHandler,rtcpRRHandlerClientData,rtpSeqNum,rtpTimestamp,serverRequestAlternativeByteHandler,
	           serverRequestAlternativeByteHandlerClientData);
 }

H264LiveVideoServerMediaSubsession::H264LiveVideoServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource ) :OnDemandServerMediaSubsession(env, reuseFirstSource),fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL) 
{
}
H264LiveVideoServerMediaSubsession::~H264LiveVideoServerMediaSubsession() 
{
	delete[] fAuxSDPLine;
}
static void afterPlayingDummy(void* clientData) 
{
  
	H264LiveVideoServerMediaSubsession* subsess = (H264LiveVideoServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}
 
char const* H264LiveVideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
	if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)
    if (fDummyRTPSink == NULL) 
	{ // we're not already setting it up for another, concurrent stream
      // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
     // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
      // and we need to start reading data from our file until this changes.
    	fDummyRTPSink = rtpSink;
      // Start reading the file:
      	fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
      // Check whether the sink's 'auxSDPLine()' is ready:
     	checkForAuxSDPLine(this);
   }
   envir().taskScheduler().doEventLoop(&fDoneFlag);
   return fAuxSDPLine;
}
FramedSource* H264LiveVideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
	estBitrate = 1500;

    H264LiveVideoSource *buffsource = H264LiveVideoSource::createNew(envir());
    if (buffsource == NULL) return NULL;

	H264VideoLiveDiscreteFramer* videoSource = H264VideoLiveDiscreteFramer::createNew(envir(), (FramedSource*)buffsource);
	return videoSource;
}

RTPSink* H264LiveVideoServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,FramedSource* /*inputSource*/) 
{
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
