#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "hzk_bitmap.h"

int hzk_bitmap_create(char* str, int font, unsigned char fcolor, unsigned char bcolor, int rotate, BITMAP_INFO_T* info)
{
	char  hz[64];
	FILE* HZK = NULL;

	sprintf(hz , "./HZK%d" , font);
	HZK = fopen(hz , "rb");
	if( HZK == NULL)  
	{  
		printf("Can't Open %s\n" , hz);  
		return -1; 
	}

	int ii=0;
	int sum = font*font/8;
	int hz_count = strlen(str)/2;
	unsigned char* pData;
	if(info->format == BMP_PIXEL_FORMAT_RGB_888){
		pData = (unsigned char*)malloc(sizeof(RGB_T) * font * (font*hz_count));
	}
	else if(info->format == BMP_PIXEL_FORMAT_RGB_1555)
	{
		pData = (unsigned char*)malloc(sizeof(RGB1555_T) * font * (font*hz_count));
	}
	
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
		int i,j,k;
		//显示 
		for(i=0; i<font; i++) //16
		{ 
			for(j=0; j<font/8; j++) //2
			{ 
				for(k=0; k<8 ; k++) //8bits , 1 byte
				{ 
					int rtt_row;
					if(rotate == 1)
					{
						rtt_row = font-1-row;
					}
					else
					{
						rtt_row = row;
					}
					
					if( mat[i*font/8 + j] & (0x80>>k))
					{//测试为1的位则显示 
						//汉字颜色
						if(info->format == BMP_PIXEL_FORMAT_RGB_888)
						{
							RGB_T* pRGB = (RGB_T*)pData;
							pRGB[rtt_row*(font*hz_count) + col + ii*font].r = fcolor;//bmp中的行坐标倒序
							pRGB[rtt_row*(font*hz_count) + col + ii*font].g = fcolor;
							pRGB[rtt_row*(font*hz_count) + col + ii*font].b = fcolor;
						}
						else if(info->format == BMP_PIXEL_FORMAT_RGB_1555)
						{
							RGB1555_T* pRGB = (RGB1555_T*)pData;
							pRGB[rtt_row*(font*hz_count) + col + ii*font].r = fcolor&0x1f;//bmp中的行坐标倒序
							pRGB[rtt_row*(font*hz_count) + col + ii*font].g = fcolor&0x1f;
							pRGB[rtt_row*(font*hz_count) + col + ii*font].b = fcolor&0x1f;
							pRGB[rtt_row*(font*hz_count) + col + ii*font].a = 1;
						}
					} 
					else 
					{	//背景色
						int flag = 0;
						if(k < 7) 	//描左边
						{
							if(mat[i*font/8 + j] & (0x80>>(k+1)))
							{
								flag = 1;
							}
						}

						if(k > 1)	//描右边
						{
							if(mat[i*font/8 + j] & (0x80>>(k-1)))
							{
								flag = 1;
							}
						}

						if(i > 0)		//描下边
						{
							if(mat[(i-1)*font/8 + j] & (0x80>>k))
							{
								flag = 1;
							}
						}

						if(i < font-1)	//描上边
						{
							if(mat[(i+1)*font/8 + j] & (0x80>>k))
							{
								flag = 1;
							}
						}
						
						if(flag == 1)
						{
							if(info->format == BMP_PIXEL_FORMAT_RGB_888)
							{
								RGB_T* pRGB = (RGB_T*)pData;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].r = bcolor;//bmp中的行坐标倒序
								pRGB[rtt_row*(font*hz_count) + col + ii*font].g = bcolor;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].b = bcolor;
							}
							else if(info->format == BMP_PIXEL_FORMAT_RGB_1555)
							{
								RGB1555_T* pRGB = (RGB1555_T*)pData;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].r = 0x8a&0x1f;//bmp中的行坐标倒序
								pRGB[rtt_row*(font*hz_count) + col + ii*font].g = 0x8a&0x1f;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].b = 0x8a&0x1f;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].a = 1;
							}
						}
						else
						{
							if(info->format == BMP_PIXEL_FORMAT_RGB_888)
							{
								RGB_T* pRGB = (RGB_T*)pData;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].r = bcolor;//bmp中的行坐标倒序
								pRGB[rtt_row*(font*hz_count) + col + ii*font].g = bcolor;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].b = bcolor;
							}
							else if(info->format == BMP_PIXEL_FORMAT_RGB_1555)
							{
								RGB1555_T* pRGB = (RGB1555_T*)pData;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].r = bcolor&0x1f;//bmp中的行坐标倒序
								pRGB[rtt_row*(font*hz_count) + col + ii*font].g = bcolor&0x1f;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].b = bcolor&0x1f;
								pRGB[rtt_row*(font*hz_count) + col + ii*font].a = 0;
							}
						}
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

	info->pRGB = pData;
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
	int bmpsize;
	int bitcount;
	if(info.format == BMP_PIXEL_FORMAT_RGB_888){
		bmpsize = info.width*info.height*3; // 每个像素点3个字节
		bitcount = 24;
	}
	else if(info.format == BMP_PIXEL_FORMAT_RGB_1555)
	{
		bmpsize = info.width*info.height*2; // 每个像素点3个字节
		bitcount = 16;
	}

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
	bih.biBitCount = bitcount;
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

