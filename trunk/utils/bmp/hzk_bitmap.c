#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "hzk_bitmap.h"

int hzk_bitmap_create(char* str, int font, unsigned char fcolor, unsigned char bcolor, BITMAP_INFO_T* info)
{
	char  hz[64];
	FILE* HZK = NULL;

	sprintf(hz , "HZK%d" , font);
	HZK = fopen(hz , "rb");
	if( HZK == NULL)  
	{  
		printf("Can't Open hzk%d\n" , font);  
		return -1; 
	}

	int ii=0;
	int sum = font*font/8;
	int hz_count = strlen(str)/2;
	
	RGB_T* pRGB = (RGB_T*)malloc(sizeof(RGB_T) * font * (font*hz_count));
	
	for(ii=0; ii<hz_count; ii++)
	{
		unsigned char qh = str[ii*2+0] - 0xa0; //获得区码  
		unsigned char wh = str[ii*2+1] - 0xa0; //获得位码

		unsigned long offset = 0;
		unsigned char* mat = (unsigned char*)malloc(sizeof(char) * font * font/8);
		memset(mat, 0x0, font * font/8);

		offset = (94*(qh-1)+(wh-1))*sum; //得到偏移位置
		fseek(HZK, offset, SEEK_SET); 
		fread(mat, sum , 1, HZK);

		int col=0;
		int row=0;
		//显示 
		for(int i=0; i<font; i++) //16
		{ 
			for(int j=0; j<font/8; j++) //2
			{ 
				for(int k=0; k<8 ; k++) //8bits , 1 byte
				{ 
					if( mat[i*font/8 + j] & (0x80>>k))
					{//测试为1的位则显示 
						//汉字颜色
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].r = fcolor;//bmp中的行坐标倒序
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].g = fcolor;
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].b = fcolor;
					} 
					else 
					{	//背景色
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].r = bcolor;//bmp中的行坐标倒序
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].g = bcolor;
						pRGB[(font-1-row)*(font*hz_count) + col + ii*font].b = bcolor;
					} 
					col++;
				}
			}
			row++;
			col = 0;
		}//for

		free(mat);
	}
	
	fclose(HZK); 

	info->pRGB = pRGB;
	info->width = font*hz_count;
	info->height = font;

	return 0;
}

int hzk_bitmap_destory(BITMAP_INFO_T info)
{
	if(info.pRGB != NULL)
	{
		free(info.pRGB);
		info.pRGB = NULL;
	}

	return 0;
}

int hzk_bitmap_save(char* name, BITMAP_INFO_T info)
{
	int bmpsize = info.width*info.height*3; // 每个像素点3个字节

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
	bih.biWidth = info.width;
	bih.biHeight = info.height;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = 0;
	bih.biSizeImage = bmpsize;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;       

	FILE * fp = fopen(name,"wb");
	if( !fp ) return -1;

	fwrite( &bfh, 1, sizeof(BMPFILEHEADER_T), fp );
	fwrite( &bih, 1, sizeof(BMPINFOHEADER_T), fp );
	fwrite( (unsigned char*)info.pRGB, 1, bmpsize, fp );
	fclose( fp );

	return 0;
}

