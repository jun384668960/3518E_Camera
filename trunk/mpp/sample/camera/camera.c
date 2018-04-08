#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "sample_comm.h"
#include "acodec.h"
#include "tlv320aic31.h"

#include "utils_log.h"
#include "stream_manager.h"
#include "stream_define.h"
#include "osd_convert.h"
#include "utils_common.h"

////////////////////////////////////////////////////////////////
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_NTSC;
HI_U32 g_u32BlkCnt = 4;

static shm_stream_t* g_handle = NULL;
static unsigned char g_frame[128*1024];

////////////////////////////////////////////////////////////////
static PAYLOAD_TYPE_E gs_enPayloadType = PT_G711A;

//static HI_BOOL gs_bMicIn = HI_FALSE;

static HI_BOOL gs_bAioReSample  = HI_FALSE;
static HI_BOOL gs_bUserGetMode  = HI_FALSE;
static HI_BOOL gs_bAoVolumeCtrl = HI_TRUE;
static AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
static AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
static HI_U32 u32AencPtNumPerFrm = 0;
/* 0: close, 1: open*/
static HI_U32 u32AiVqeType = 1;  
/* 0: close, 1: open*/
static HI_U32 u32AoVqeType = 1;  

static shm_stream_t* g_audiohandle = NULL;
////////////////////////////////////////////////////////////////

void COMM_VENC_UseStream(VENC_CHN VeChn, PAYLOAD_TYPE_E enType, VENC_STREAM_S *pstStream)
{
	//do some thing
	if(VeChn == 0)
	{
		LOGI_print("video %d %llu", VeChn, pstStream->pstPack[pstStream->u32PackCount-1].u64PTS);

		HI_S32 total_length = 0;
		HI_S32 i;
	    for (i = 0; i < pstStream->u32PackCount; i++)
	    {
	    	HI_U8* data = pstStream->pstPack[i].pu8Addr+pstStream->pstPack[i].u32Offset;
			HI_S32 len = pstStream->pstPack[i].u32Len-pstStream->pstPack[i].u32Offset;
			memcpy(&g_frame[total_length], data, len);
			total_length += len;
	    }
			
#if 0
		static FILE* h264 = NULL;
		if(h264 == NULL)
			h264 = fopen("./stream0.h264", "wb");
		if(h264 != NULL)
		{
			fwrite(g_frame, total_length, 1, h264);
			fflush(h264);
		}
#else
		if(g_handle == NULL)
			g_handle = shm_stream_create("write", "mainstream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_MAX_SIZE, SHM_STREAM_WRITE);

		if(g_handle != NULL)
		{
			frame_info info;
			info.type = enType;
			if(enType == PT_H264)
			{
				info.key = pstStream->pstPack[pstStream->u32PackCount-1].DataType.enH264EType;
			}
			info.pts = pstStream->pstPack[pstStream->u32PackCount-1].u64PTS;
			info.length = total_length;
			
			int ret = shm_stream_put(g_handle, info, g_frame, total_length);
			if(ret != 0)
			{
//				LOGE_print("shm_stream_put error");
			}
			else
			{
				LOGI_print("shm_stream_put video info.lenght:%d info.pts:%llu", info.length, info.pts);
			}
		}
#endif
	}
}


void COMM_AENC_UseStream(HI_S32 AeChn, AUDIO_STREAM_S *pstStream)
{
	LOGI_print("audio %d %llu %u", AeChn, pstStream->u64TimeStamp, pstStream->u32Len);
	//gs_enPayloadType ???????????
	if(AeChn == 0 && gs_enPayloadType == PT_G711A)
	{
#if 0
		static FILE* g711a = NULL;
		if(g711a == NULL)
			g711a = fopen("./stream0.g711a", "wb");
		if(h264 != NULL)
		{
			fwrite(g_frame, total_length, 1, g711a);
			fflush(g711a);
		}
#else
		if(g_audiohandle == NULL)
			g_audiohandle = shm_stream_create("write", "audiostream", STREAM_MAX_USER, STREAM_MAX_FRAMES, STREAM_MAX_SIZE, SHM_STREAM_WRITE);

		if(g_audiohandle != NULL)
		{
			frame_info info;
			info.type = gs_enPayloadType;
			info.key = 1;
			info.pts = pstStream->u64TimeStamp;
			info.length = pstStream->u32Len - 4;
			
			int ret = shm_stream_put(g_audiohandle, info, pstStream->pStream + 4, info.length);
			if(ret != 0)
			{
//				LOGE_print("shm_stream_put error");
			}
			else
			{
				LOGI_print("shm_stream_put audio info.lenght:%d info.pts:%llu", info.length, info.pts);
			}
		}
#endif
	}
}

HI_VOID *SAMPLE_RGN_UpdateBitmap(void *pData)
{
	RGN_HANDLE Handle;
	BITMAP_S stBitmap;
	HI_S32 s32Ret;
	char t_str[1024] = {0};
	
	while(1)
	{
		char time[1024] = {0}; 
		
		localtime_string_get(time);
		if(strcmp(t_str, time) != 0)
		{
			///////////////////////////////////////////////////////////////////////
			BITMAP_INFO_T info;
			hzk_bitmap_create(0, 32, 14, time, 
				0xFFFF,0xFC00,0x81FF, &info);

			Handle = 0;
			stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
			stBitmap.u32Width = info.width;
			stBitmap.u32Height = info.height;
			stBitmap.pData = info.pRGB;
			
			s32Ret = HI_MPI_RGN_SetBitMap(Handle,&stBitmap);
			if (HI_SUCCESS != s32Ret)
			{
				printf("HI_MPI_RGN_SetBitMap error:%d\n",s32Ret);
			}
			
			hzk_bitmap_destory(info);
			///////////////////////////////////////////////////////////////////////
			
			strcpy(t_str, time);
		}

		usleep(50*1000);
	}
	return HI_NULL;
}

HI_S32 SAMPLE_RGN_CreateOverlayForVenc(RGN_HANDLE Handle, HI_U32 u32Num)
{
    HI_S32 s32Ret;
    MPP_CHN_S stChn;
    RGN_ATTR_S stRgnAttr;
    RGN_CHN_ATTR_S stChnAttr;
    
    /* Add cover to vpss group */
    stChn.enModId  = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = 0;
        
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt       = PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width  = 600;//300
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = 300;
    stRgnAttr.unAttr.stOverlay.u32BgColor       = 0x000003e0;

    
    s32Ret = HI_MPI_RGN_Create(u32Num, &stRgnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("HI_MPI_RGN_Create failed! s32Ret: 0x%x.\n", s32Ret);
        return s32Ret;
    }

    stChnAttr.bShow  = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 48;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 48;
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha   = 0;//128;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha   = 128;//128;
    stChnAttr.unChnAttr.stOverlayChn.u32Layer     = u32Num;
    
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable  = HI_FALSE;

    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16*(u32Num%2+1);
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width  = 16*(u32Num%2+1);
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod     = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn    = HI_FALSE;
    
    s32Ret = HI_MPI_RGN_AttachToChn(0, &stChn, &stChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
		printf("HI_MPI_RGN_AttachToChn error:%d\n",s32Ret);
    }

    return HI_SUCCESS;
    
}

HI_S32 SAMPLE_RGN_DestroyRegion(RGN_HANDLE Handle, HI_U32 u32Num)
{
    HI_S32 s32Ret;    
        
    s32Ret = HI_MPI_RGN_Destroy(u32Num);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_RGN_Destroy failed! s32Ret: 0x%x.\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
    
}

/******************************************************************************
* function :  H.264@1080p@30fps+H.264@VGA@30fps


******************************************************************************/
HI_S32 SAMPLE_VENC_1080P_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[3]= {PT_H264, PT_H264,PT_H264};
    PIC_SIZE_E enSize[3] = {PIC_HD1080, PIC_VGA,PIC_QVGA};
	HI_U32 u32Profile = 0;
	
    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig = {0};
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
	
    HI_S32 s32ChnNum=0;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    char c;


    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    
	SAMPLE_COMM_VI_GetSizeBySensor(&enSize[0]);
    if (PIC_HD1080 == enSize[0])
    {
        enSize[1] = PIC_VGA;
		s32ChnNum = 1;
    }
    else if (PIC_HD720 == enSize[0])
    {
        enSize[1] = PIC_VGA;			
		enSize[2] = PIC_QVGA;
		s32ChnNum = 3;
    }
    else
    {
        printf("not support this sensor\n");
        return HI_FAILURE;
    }
#ifdef hi3518ev201
	s32ChnNum = 1;
#endif
	printf("s32ChnNum = %d\n",s32ChnNum);

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
	if(s32ChnNum >= 1)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[0].u32BlkCnt = g_u32BlkCnt;
	}
	if(s32ChnNum >= 2)
    {
	    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
	                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
	    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
	    stVbConf.astCommPool[1].u32BlkCnt =g_u32BlkCnt;
	}
	if(s32ChnNum >= 3)
    {
		u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
		stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
		stVbConf.astCommPool[2].u32BlkCnt = g_u32BlkCnt;
    }

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_1080P_CLASSIC_0;
    }

	/******************************************
     step 2.1: 
    ******************************************/
    SAMPLE_RGN_CreateOverlayForVenc(0, 0);

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    stViConfig.enWDRMode  = WDR_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
    
    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_1080P_CLASSIC_1;
    }
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    stVpssGrpAttr.u32MaxW = stSize.u32Width;
	    stVpssGrpAttr.u32MaxH = stSize.u32Height;
	    stVpssGrpAttr.bIeEn = HI_FALSE;
	    stVpssGrpAttr.bNrEn = HI_TRUE;
	    stVpssGrpAttr.bHistEn = HI_FALSE;
	    stVpssGrpAttr.bDciEn = HI_FALSE;
	    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_NODIE;
	    stVpssGrpAttr.enPixFmt = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		
	    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_2;
	    }

	    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Vi bind Vpss failed!\n");
	        goto END_VENC_1080P_CLASSIC_3;
	    }

		VpssChn = 0;
	    stVpssChnMode.enChnMode      = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble        = HI_FALSE;
	    stVpssChnMode.enPixelFormat  = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width       = stSize.u32Width;
	    stVpssChnMode.u32Height      = stSize.u32Height;
	    stVpssChnMode.enCompressMode = COMPRESS_MODE_SEG;
	    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}

	if(s32ChnNum >= 2)
	{
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[1], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	    VpssChn = 1;
	    stVpssChnMode.enChnMode       = VPSS_CHN_MODE_USER;
	    stVpssChnMode.bDouble         = HI_FALSE;
	    stVpssChnMode.enPixelFormat   = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	    stVpssChnMode.u32Width        = stSize.u32Width;
	    stVpssChnMode.u32Height       = stSize.u32Height;
	    stVpssChnMode.enCompressMode  = COMPRESS_MODE_SEG;
	    stVpssChnAttr.s32SrcFrameRate = -1;
	    stVpssChnAttr.s32DstFrameRate = -1;
	    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Enable vpss chn failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
	}
	

	if(s32ChnNum >= 3)
	{	
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[2], &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_VENC_1080P_CLASSIC_4;
	    }
		VpssChn = 2;
		stVpssChnMode.enChnMode 	= VPSS_CHN_MODE_USER;
		stVpssChnMode.bDouble		= HI_FALSE;
		stVpssChnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
		stVpssChnMode.u32Width		= stSize.u32Width;
		stVpssChnMode.u32Height 	= stSize.u32Height;
		stVpssChnMode.enCompressMode = COMPRESS_MODE_NONE;
		
		stVpssChnAttr.s32SrcFrameRate = -1;
		stVpssChnAttr.s32DstFrameRate = -1;
		
		s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("Enable vpss chn failed!\n");
			goto END_VENC_1080P_CLASSIC_4;
		}
	}
    /******************************************
     step 5: start stream venc
    ******************************************/
	enRcMode = SAMPLE_RC_VBR;
	
	/*** enSize[0] **/
	if(s32ChnNum >= 1)
	{
		VpssGrp = 0;
	    VpssChn = 0;
	    VencChn = 0;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[0],\
	                                   gs_enNorm, enSize[0], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	/*** enSize[1] **/
	if(s32ChnNum >= 2)
	{
		VpssChn = 1;
	    VencChn = 1;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[1], \
	                                    gs_enNorm, enSize[1], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}
	/*** enSize[2] **/
	if(s32ChnNum >= 3)
	{
	    VpssChn = 2;
	    VencChn = 2;
	    s32Ret = SAMPLE_COMM_VENC_Start(VencChn, enPayLoad[2], \
	                                    gs_enNorm, enSize[2], enRcMode,u32Profile);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }

	    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("Start Venc failed!\n");
	        goto END_VENC_1080P_CLASSIC_5;
	    }
	}

	pthread_t g_stOsdUpdateThread = 0;
	pthread_create(&g_stOsdUpdateThread, NULL, SAMPLE_RGN_UpdateBitmap, NULL);
    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum, COMM_VENC_UseStream);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_1080P_CLASSIC_5;
    }

	return HI_SUCCESS;

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();

	if(g_stOsdUpdateThread)
	{
		pthread_join(g_stOsdUpdateThread, NULL);
	}
	
END_VENC_1080P_CLASSIC_5:
	
    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;   
		    VencChn = 2;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 2:
			VpssChn = 1;   
		    VencChn = 1;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
		case 1:
			VpssChn = 0;  
		    VencChn = 0;
		    SAMPLE_COMM_VENC_UnBindVpss(VencChn, VpssGrp, VpssChn);
		    SAMPLE_COMM_VENC_Stop(VencChn);
			break;
			
	}
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
	
END_VENC_1080P_CLASSIC_4:	//vpss stop

    VpssGrp = 0;
	switch(s32ChnNum)
	{
		case 3:
			VpssChn = 2;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 2:
			VpssChn = 1;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		case 1:
			VpssChn = 0;
			SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
		break;
	
	}

END_VENC_1080P_CLASSIC_3:    //vpss stop       
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_1080P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_1080P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_1080P_CLASSIC_0:	//system exit

	SAMPLE_RGN_DestroyRegion(0, 0);
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}

/******************************************************************************
* function : Ai -> Aenc -> file
*                       -> Adec -> Ao
******************************************************************************/
HI_S32 SAMPLE_AUDIO_AiAenc(HI_VOID)
{
    HI_S32 i, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
    AI_CHN      AiChn;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
    AO_CHN      AoChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
	HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AENC_CHN    AeChn;
    HI_BOOL     bSendAdec = HI_FALSE;
    FILE        *pfd = NULL;
    AIO_ATTR_S stAioAttr;

#ifdef HI_ACODEC_TYPE_TLV320AIC31
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 1;
#else
    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = SAMPLE_AUDIO_PTNUMPERFRM;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
#endif
    gs_bAioReSample = HI_FALSE;
    enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;
	u32AencPtNumPerFrm = stAioAttr.u32PtNumPerFrm;
		
    /********************************************
      step 1: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (HI_SUCCESS != s32Ret)
    {
    	LOGE_print("SAMPLE_COMM_AUDIO_CfgAcodec error:%d", s32Ret);
        return HI_FAILURE;
    }
	
    /********************************************
      step 2: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt; 
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE_print("SAMPLE_COMM_AUDIO_StartAi error:%d", s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = 1;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, u32AencPtNumPerFrm, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE_print("SAMPLE_COMM_AUDIO_StartAenc error:%d", s32Ret);
        return HI_FAILURE;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAiAenc(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE_print("SAMPLE_COMM_AUDIO_CreatTrdAiAenc AiDev:%d AiChn:%d AeChn:%d error:%d", AiDev, AiChn, AeChn, s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE_print("SAMPLE_COMM_AUDIO_AencBindAi AiDev:%d AiChn:%d AeChn:%d error:%d", AiDev, AiChn, AeChn, s32Ret);
                return s32Ret;
            }
        }
        LOGI_print("Ai(%d,%d) bind to AencChn:%d ok!",AiDev , AiChn, AeChn);
    }

    /********************************************
      step 5: start Adec & Ao. ( if you want )
    ********************************************/
	s32Ret = SAMPLE_COMM_AUDIO_CreatTrdAencUse(AeChn, AdChn, COMM_AENC_UseStream);
	if (s32Ret != HI_SUCCESS)
    {
        LOGE_print("SAMPLE_COMM_AUDIO_AencBindAi AeChn:%d AdChn:%d error:%d", AeChn, AdChn, s32Ret);
        return HI_FAILURE;
    }

	return HI_SUCCESS;

    /********************************************
      step 6: exit the process
    ********************************************/
    if (HI_TRUE == bSendAdec)
    {
        s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAencAdec(AdChn);
        if (s32Ret != HI_SUCCESS)
        {
            LOGE_print("SAMPLE_COMM_AUDIO_DestoryTrdAencAdec error:%d", s32Ret);
            return HI_FAILURE;
        }
    }
    
    for (i=0; i<s32AencChnCnt; i++)
    {
        AeChn = i;
        AiChn = i;

        if (HI_TRUE == gs_bUserGetMode)
        {
            s32Ret = SAMPLE_COMM_AUDIO_DestoryTrdAi(AiDev, AiChn);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE_print("SAMPLE_COMM_AUDIO_DestoryTrdAi error:%d", s32Ret);
                return HI_FAILURE;
            }
        }
        else
        {        
            s32Ret = SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE_print("SAMPLE_COMM_AUDIO_AencUnbindAi error:%d", s32Ret);
                return HI_FAILURE;
            }
        }
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE_print("SAMPLE_COMM_AUDIO_AencUnbindAi s32AencChnCnt:%d error:%d", s32AencChnCnt, s32Ret);
        return HI_FAILURE;
    }
    
    s32Ret = SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE_print("SAMPLE_COMM_AUDIO_StopAi AiDev:%d s32AiChnCnt:%d error:%d", AiDev, s32AiChnCnt, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

int main()
{
	SAMPLE_VENC_1080P_CLASSIC();
	SAMPLE_AUDIO_AiAenc();
	
	while(1)
	{
		usleep(1000*10);
	}

	return 0;
}
