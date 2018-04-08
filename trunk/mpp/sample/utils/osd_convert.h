#ifndef OSD_CONVERT_H
#define OSD_CONVERT_H

#pragma pack(1)
typedef struct {
	unsigned short 	bfType; 	  //位图文件的类型，必须为BM 
	unsigned int	bfSize; 	  //文件大小，以字节为单位
	unsigned short 	bfReserved1; //位图文件保留字，必须为0 
	unsigned short 	bfReserved2; //位图文件保留字，必须为0 
	unsigned int	bfOffBits;  //位图文件头到数据的偏移量，以字节为单位
} BMPFILEHEADER_T;

typedef struct{
	unsigned int 	biSize;						//该结构大小，字节为单位
	unsigned int  	biWidth;					   //图形宽度以象素为单位
	unsigned int  	biHeight; 					//图形高度以象素为单位
	unsigned short 	biPlanes;			   //目标设备的级别，必须为1 
	unsigned short  biBitCount;			   //颜色深度，每个象素所需要的位数
	unsigned int  	biCompression;		 //位图的压缩类型
	unsigned int  	biSizeImage;				//位图的大小，以字节为单位
	unsigned int  	biXPelsPerMeter;		 //位图水平分辨率，每米像素数
	unsigned int  	biYPelsPerMeter;		 //位图垂直分辨率，每米像素数
	unsigned int  	biClrUsed;			//位图实际使用的颜色表中的颜色数
	unsigned int  	biClrImportant;		//位图显示过程中重要的颜色数
} BMPINFOHEADER_T;

typedef struct{
	unsigned short b:5;
	unsigned short g:5;
	unsigned short r:5;
	unsigned short a:1;
}RGB1555_T;

#pragma pack()

typedef struct{
	int width;
	int height;
	char* pRGB;
}BITMAP_INFO_T;

int hzk_bitmap_create(int b2feild, unsigned short font, int space, char *text, 
					  unsigned short fcolor, unsigned short bcolor, unsigned short edgecolor, BITMAP_INFO_T* info);
void hzk_bitmap_destory(BITMAP_INFO_T info);
int hzk_bitmap_save(char* name, BITMAP_INFO_T info);
	
int hzk_bitmap_charbit(unsigned char *in, unsigned short qu, unsigned short wei, unsigned short font);

int hzk_bitmap_draw32(int x_pos, unsigned char* bitmap, unsigned short fcolor
		, unsigned short bcolor, unsigned short edgecolor, unsigned char *bmp_buffer
		, unsigned short bmp_width, unsigned short font);
int hzk_bitmap_draw(int x_pos, unsigned char* bitmap, unsigned short fcolor
		, unsigned short bcolor, unsigned short edgecolor, unsigned char *bmp_buffer
		, unsigned short bmp_width, unsigned short font);
#endif