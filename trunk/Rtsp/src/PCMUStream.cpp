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

#include "PCMUStream.hh"
#include "MPEG4GenericRTPSink.hh"
#include "GroupsockHelper.hh"
#include "utils_log.h"
#include "stream_define.h"

PCMUStream * PCMUStream::createNew(UsageEnvironment& env, int sub) 
{
    return new PCMUStream(env, sub, 0, 0, 0);
}

PCMUStream::PCMUStream(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration)
:FramedSource(env) 
{	
	fPresentationTime.tv_sec  = 0;
	fPresentationTime.tv_usec = 0;
	fuSecsPerFrame = (320/*samples-per-frame*/*1000000) / 8000/*samples-per-second*/;

	m_StreamList = shm_stream_create("rtsp_pcmread", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_MAX_SIZE, SHM_STREAM_READ);
	if(m_StreamList == NULL)
	{
		LOGE_print("shm_stream_create Failed");
	}

	m_ref = -1;
}

PCMUStream::~PCMUStream() 
{
	if(m_StreamList != NULL)
	{
		shm_stream_destory(m_StreamList);
	}
}

void PCMUStream::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void PCMUStream::incomingDataHandler(PCMUStream* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void PCMUStream::incomingDataHandler1()
{
	fFrameSize = 0;
	frame_info info;
	unsigned char* pData;
	unsigned int length;
	int ret = shm_stream_get(m_StreamList, &info, &pData, &length);
	
	if(ret == 0)
    {	
    	fFrameSize = info.pts;
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
		LOGD_print("framer audio pts:%u fFrameSize:%d", info.pts, fFrameSize);	
		fDurationInMicroseconds = fuSecsPerFrame;
		shm_stream_post(m_StreamList);
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0,(TaskFunc*)FramedSource::afterGetting, this);
    }
	else
	{	
		nextTask() = envir().taskScheduler().scheduleDelayedTask(2*1000,
			(TaskFunc*)incomingDataHandler,this);
	}
}

PCMUServerMediaSubsession*PCMUServerMediaSubsession::createNew(UsageEnvironment& env,int sub,Boolean reuseFirstSource) 
{
  	return new PCMUServerMediaSubsession(env, sub, reuseFirstSource);
}

PCMUServerMediaSubsession::PCMUServerMediaSubsession(UsageEnvironment& env, int sub, Boolean reuseFirstSource)
:OnDemandServerMediaSubsession(env, reuseFirstSource)
{
	this->sub = sub;
}

PCMUServerMediaSubsession::~PCMUServerMediaSubsession() 
{
}

FramedSource* PCMUServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
	estBitrate = 96; // kbps, estimate
	return PCMUStream::createNew(envir(), sub);
}

RTPSink* PCMUServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,unsigned char rtpPayloadTypeIfDynamic,FramedSource* inputSource)
{	
	LOGD_print("enc_type E1_AENC_G711A");
	return SimpleRTPSink::createNew(envir(), rtpGroupsock, 97, 8000, "audio", "PCMA", 1, False);//for test
}
