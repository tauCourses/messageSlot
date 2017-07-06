#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>



/* The major device number. We can't rely on dynamic 
 * registration any more, because ioctls need to know 
 * it. */
#define MAJOR_NUM 245
#define NUM_OF_BUFFERS 4
#define BUFFER_SIZE 128


/* Set the message of the device driver */
#define IOCTL_SET_INDEX _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"

#define DEVICE_FILE_NAME "simple_char_dev"
#define SUCCESS 0


#endif
