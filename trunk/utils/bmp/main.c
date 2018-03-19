#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "charset_convert.h"
#include "hzk_bitmap.h"

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
			hzk_bitmap_create(hz, HZK_FONT_16, 0xff, 0x00, &info);
			hzk_bitmap_save(filename, info);
			hzk_bitmap_destory(info);
			///////////////////////////////////////////////////////////////////////
			
			strcpy(t_str, h0);
		}
	}
	
	return 0;
}

