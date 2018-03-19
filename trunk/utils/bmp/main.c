#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>

#include "charset_convert.h"
#include "hzk_bitmap.h"

#define size 32
#define sum (size*size/8)

//#pragma pack(1)
//typedef struct {
//	unsigned short 	bfType; 	  //位图文件的类型，必须为BM 
//	unsigned int	bfSize; 	  //文件大小，以字节为单位
//	unsigned short 	bfReserved1; //位图文件保留字，必须为0 
//	unsigned short 	bfReserved2; //位图文件保留字，必须为0 
//	unsigned int	bfOffBits;  //位图文件头到数据的偏移量，以字节为单位
//} BMPFILEHEADER_T;

//typedef struct{
//	unsigned int 	biSize;						//该结构大小，字节为单位
//	unsigned int  	biWidth;					   //图形宽度以象素为单位
//	unsigned int  	biHeight; 					//图形高度以象素为单位
//	unsigned short 	biPlanes;			   //目标设备的级别，必须为1 
//	unsigned short  biBitCount;			   //颜色深度，每个象素所需要的位数
//	unsigned int  	biCompression;		 //位图的压缩类型
//	unsigned int  	biSizeImage;				//位图的大小，以字节为单位
//	unsigned int  	biXPelsPerMeter;		 //位图水平分辨率，每米像素数
//	unsigned int  	biYPelsPerMeter;		 //位图垂直分辨率，每米像素数
//	unsigned int  	biClrUsed;			//位图实际使用的颜色表中的颜色数
//	unsigned int  	biClrImportant;		//位图显示过程中重要的颜色数
//} BMPINFOHEADER_T;

//typedef struct{
//	unsigned char b;
//	unsigned char g;
//	unsigned char r;
//}RGB_T;

//#pragma pack()

unsigned char* convert(char* HZ, int hz_count, RGB_T* pRGB);
void Snapshot(unsigned char* pData, int width, int height,  char * filename );

void local_time_get(char* tstr)
{
	char *wday[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	time_t timep;
	struct tm *p;
	
	time(&timep);
	p=localtime(&timep);
	
	sprintf(tstr,"%d/%02d/%02d %s %02d:%02d:%02d", (1900+p->tm_year),(1+p->tm_mon), p->tm_mday
		, wday[p->tm_wday],p->tm_hour, p->tm_min, p->tm_sec);
}

void file_time_get(char* tstr)
{
	time_t timep;
	struct tm *p;
	
	time(&timep);
	p=localtime(&timep);
	
	sprintf(tstr,"%d-%02d-%02d %02d-%02d-%02d", (1900+p->tm_year),(1+p->tm_mon), p->tm_mday
		,p->tm_hour, p->tm_min, p->tm_sec);
}

int main(int arg0, char* argv[])
{
	if(arg0 > 1)
		printf("%d %s\n", arg0, argv[1]);

	char t_str[1024] = {0};
	while(1)
	{
		usleep(300*1000);

		//文件格式要求UTF-8
		char h0[1024] = {0}; 

		local_time_get(h0);
		if(strcmp(t_str, h0) != 0)
		{
			char filename[1024] = {0};
			file_time_get(filename);
			sprintf(filename, "%s.bmp", filename);
			printf("%s\n", filename);
			printf("%s\n", h0);

			char hz[1024] = {0};
			size_t inLen = strlen(h0);
			size_t outLen = inLen * 3;
			char *pOut = (char *)malloc(sizeof(char) * outLen);
			memset(pOut, 0, sizeof(char) * outLen);
			
			charset_convert_UTF8_TO_GB2312(h0, (size_t)inLen, pOut, (size_t)inLen);
			charset_convert_GB2312_HALF_TO_FULL(hz, 1024, pOut, outLen);
			
			free(pOut);

			///////////////////////////////////////////////////////////////////////

			
			BITMAP_INFO_T info;
			hzk_bitmap_create(hz, size, 0xff, 0x00, &info);
			hzk_bitmap_save(filename, info);
			hzk_bitmap_destory(info);
			///////////////////////////////////////////////////////////////////////
			
			strcpy(t_str, h0);
		}
	}
	
	return 0;
}

unsigned char* convert(char* HZ, int hz_count, RGB_T* pRGB) 
{    
	char  hz[64];
	FILE* HZK = NULL;

	sprintf(hz , "HZK%d" , size);
	HZK = fopen(hz , "rb");
	if( HZK == NULL)  
		//fail to open file
	{  
		printf("Can't Open hzk%d\n" , size);  
		getchar(); 
		return NULL; 
	}

	int ii=0;
	for(ii=0; ii<hz_count; ii++)
	{
		unsigned char qh = HZ[ii*2+0] - 0xa0; //获得区码  
		unsigned char wh = HZ[ii*2+1] - 0xa0; //获得位码

		unsigned long offset = 0;
		unsigned char mat[size][size/8] = {0} ; //mat[sum][2]

		offset = (94*(qh-1)+(wh-1))*sum; //得到偏移位置
		fseek(HZK, offset, SEEK_SET); 
		fread(mat, sum , 1, HZK);

		int col=0;
		int row=0;
		//显示 
		for(int i=0; i<size; i++) //16
		{ 
			for(int j=0; j<size/8; j++) //2
			{ 
				for(int k=0; k<8 ; k++) //8bits , 1 byte
				{ 
					if( mat[i][j] & (0x80>>k))
					{//测试为1的位则显示 
						//汉字颜色
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].r = 0xff;//bmp中的行坐标倒序
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].g = 0xff;
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].b = 0xff;
					} 
					else 
					{	//背景色
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].r = 0x00;//bmp中的行坐标倒序
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].g = 0x00;
						pRGB[(size-1-row)*(size*hz_count) + col + ii*size].b = 0x00;
					} 
					col++;
				}
			}
			row++;
			col = 0;
		}//for
	}
	
	fclose(HZK); 
}

void Snapshot(unsigned char* pData, int width, int height,  char * filename )
{
	int bmpsize = width*height*3; // 每个像素点3个字节

	// 位图第一部分，文件信息
	BMPFILEHEADER_T bfh;
	bfh.bfType = 0x4d42;  //bm
	bfh.bfSize = bmpsize  // data size
		+ sizeof( BMPFILEHEADER_T ) // first section size
		+ sizeof( BMPINFOHEADER_T ) // second section size
		;
	bfh.bfReserved1 = 1; // reserved 
	bfh.bfReserved2 = 0; // reserved
	bfh.bfOffBits = bfh.bfSize - bmpsize;
	// 位图第二部分，数据信息
	BMPINFOHEADER_T bih;
	bih.biSize = sizeof(BMPINFOHEADER_T);
	bih.biWidth = width;
	bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = 0;
	bih.biSizeImage = bmpsize;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;       

	FILE * fp = fopen(filename,"wb");
	if( !fp ) return;

	fwrite( &bfh, 1, sizeof(BMPFILEHEADER_T), fp );
	fwrite( &bih, 1, sizeof(BMPINFOHEADER_T), fp );
	fwrite( pData, 1, bmpsize, fp );
	fclose( fp );
}


