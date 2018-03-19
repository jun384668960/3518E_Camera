#ifndef HZK_BITMAP_H
#define HZK_BITMAP_H

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
	unsigned char b;
	unsigned char g;
	unsigned char r;
}RGB_T;
#pragma pack()

typedef enum{
	HZK_FONT_16 = 16,
	HZK_FONT_24 = 24,
	HZK_FONT_32 = 32,
}HZK_FONT_SIZE_E;

typedef struct{
	int width;
	int height;
	RGB_T* pRGB;
}BITMAP_INFO_T;

//public
int hzk_bitmap_create(char* str, int font, unsigned char fcolor, unsigned char bcolor, BITMAP_INFO_T* info);
int hzk_bitmap_destory(BITMAP_INFO_T info);
int hzk_bitmap_save(char* name, BITMAP_INFO_T info);

//private

#endif
