#include <stdio.h>
#include <unistd.h>

#include "rtsp.h"
#include "utils_log.h"

int main(int argc, char* argv[])
{
	RTSPInit();

	while(1)
	{
		usleep(1000*1000);
	}
	
	return 0;
}
