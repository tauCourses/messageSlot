#include "message_slot.h"    

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char *argv[] )  
{
	int file_desc, ret_val, index;
	if(argc != 1)
	{
		printf("Invalid number of arguments\n");
		exit(-1);
	}
	
	file_desc = open("/dev/"DEVICE_FILE_NAME, 0);
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
	
	char tempBuf[BUFFER_SIZE];
	ret_val = read(file_desc, tempBuf, BUFFER_SIZE);

	if (ret_val < 0) {
		printf ("ioctl_set_msg failed:%d\n", ret_val);
		close(file_desc); 
		exit(-1);
	}	 
	
	close(file_desc); 
	printf("%d\n",temoBuf);
	printf("and a status message\n"); //just kidding, please don't kill me :)
	return 0;
}
