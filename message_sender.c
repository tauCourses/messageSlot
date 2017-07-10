#include "message_slot.h"    

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main( int argc, char *argv[] )  
{
	int file_desc, ret_val, index;
	if(argc < 3)
	{
		printf("Invalid number of arguments\n");
		exit(-1);
	}
	if(argc == 3)
		file_desc = open("/dev/"DEVICE_FILE_NAME, O_WRONLY);
	else
		file_desc = open(argv[3], O_WRONLY);
	
	if (file_desc < 0) 
	{
		printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}
	
	index = atoi(argv[1]); 
	ret_val = ioctl(file_desc, IOCTL_SET_INDEX, index);
	if (ret_val < 0) 
	{
		 printf ("ioctl set index failed:%d\n", ret_val);
		 close(file_desc); 
		 exit(-1);
	}	 
	
	ret_val = write(file_desc, argv[2], strlen(argv[2]));
	if (ret_val < 0) {
		printf("write to file failed:%d - %s\n", ret_val, strerror(errno));
		close(file_desc); 
		exit(-1);
	}	 
	
	close(file_desc); 
	printf("%d chars from the message:\n%s\nwrote to channel %d\n",ret_val,argv[2],index); 
	return 0;
}
