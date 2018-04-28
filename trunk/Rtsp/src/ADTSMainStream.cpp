/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an AAC audio file in ADTS format
// Implementation

#include "ADTSMainStream.hh"
#include "MPEG4GenericRTPSink.hh"
#include "GroupsockHelper.hh"
#include "utils_log.h"
#include "nalu_utils.hh"
#include "stream_define.h"

#define AUDIOSTREAM 2

struct timeval audiotimespace;
static unsigned const samplingFrequencyTable[16] = 
{
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

ADTSMainSource * ADTSMainSource::createNew(UsageEnvironment& env, int sub) 
{
    return new ADTSMainSource(env, sub, 1, 11, 1);
	//return new ADTSMainSource(env, fid, profile, sampling_frequency_index, channel_configuration);
}

ADTSMainSource::ADTSMainSource(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration): FramedSource(env) 
{
	fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
	fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
	fuSecsPerFrame
	  = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;
	
	// Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
	unsigned char audioSpecificConfig[2];
	u_int8_t const audioObjectType = profile + 1;
	audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
	audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

	m_StreamList = shm_stream_create("rtsp_pcmread", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_VIDEO_MAX_SIZE, SHM_STREAM_READ);
	if(m_StreamList == NULL)
	{
		LOGE_print("shm_stream_create Failed");
	}

	m_ref = -1;
}

ADTSMainSource::~ADTSMainSource() 
{
	if(m_StreamList != NULL)
	{
		shm_stream_destory(m_StreamList);
	}
}


void ADTSMainSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void ADTSMainSource::incomingDataHandler(ADTSMainSource* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void ADTSMainSource::incomingDataHandler1()
{
	fFrameSize = 0;
	frame_info info;
	unsigned char* pData;
	unsigned int length;
	int ret = shm_stream_get(m_StreamList, &info, &pData, &length);
	
	if(ret == 0)
	{	
		fFrameSize = info.pts - 7;//skip adts header

		if(fFrameSize > fMaxSize)
		{
			fNumTruncatedBytes = fFrameSize - fMaxSize;
			fFrameSize = fMaxSize;
		}
		else
		{
			fNumTruncatedBytes = 0;
		}
		memcpy(fTo,pData, fFrameSize);

		if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) 
		{
			// This is the first frame, so use the current time:
			gettickcount(&fPresentationTime, NULL);
			m_ref = info.pts;
		} 
		else 
		{
			// Increment by the play time of the previous frame:
			unsigned uSeconds = fPresentationTime.tv_usec + (info.pts - m_ref);
			fPresentationTime.tv_sec += uSeconds/1000000;
			fPresentationTime.tv_usec = uSeconds%1000000;
			m_ref = info.pts;
		}
		shm_stream_post(m_StreamList);
		LOGT_print("framer audio pts:%u fFrameSize:%d", info.pts, fFrameSize);
		fDurationInMicroseconds = fuSecsPerFrame;
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0,(TaskFunc*)FramedSource::afterGetting, this);
		
	}
	else
	{	
		nextTask() = envir().taskScheduler().scheduleDelayedTask(2*1000,
											(TaskFunc*)incomingDataHandler, this);
	}

}

void ADTSMainServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {
  StreamState* streamState = (StreamState*)streamToken;
  Destinations* destinations
    = (Destinations*)(fDestinationsHashTable->Lookup((char const*)clientSessionId));
  if (streamState != NULL) {
    streamState->startPlaying(destinations, clientSessionId,
			      rtcpRRHandler, rtcpRRHandlerClientData,
			      serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);
    RTPSink* rtpSink = streamState->rtpSink(); // alias
    if (rtpSink != NULL) {
      rtpSeqNum = rtpSink->currentSeqNo();
      rtpTimestamp = rtpSink->presetNextTimestamp();
    }
  }
}

ADTSMainServerMediaSubsession* ADTSMainServerMediaSubsession::createNew(UsageEnvironment& env, int sub,Boolean reuseFirstSource) 
{
  	return new ADTSMainServerMediaSubsession(env, sub, reuseFirstSource);
}

ADTSMainServerMediaSubsession::ADTSMainServerMediaSubsession(UsageEnvironment& env, int sub, Boolean reuseFirstSource)
:OnDemandServerMediaSubsession(env, reuseFirstSource)
{
	this->sub = sub;
}

ADTSMainServerMediaSubsession::~ADTSMainServerMediaSubsession() 
{
}

FramedSource* ADTSMainServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
	estBitrate = 96; // kbps, estimate
	return ADTSMainSource::createNew(envir(), sub);
}

RTPSink* ADTSMainServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
		   						unsigned char rtpPayloadTypeIfDynamic,
		   						FramedSource* inputSource)
{	
	ADTSMainSource* adtsSource = (ADTSMainSource*)inputSource;
	return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					  rtpPayloadTypeIfDynamic,
					  adtsSource->samplingFrequency(),
					  "audio", "AAC-hbr", adtsSource->configStr(),
					  adtsSource->numChannels());
}
