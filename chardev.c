/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level 
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

/* ***** Example w/ minimal error handling - for ease of reading ***** */

//Our custom definitions of IOCTL operations
#include "chardev.h"

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");

struct chardev_info{
    spinlock_t lock;
};

static int dev_open_flag = 0; /* used to prevent concurent access into the same device */
static struct chardev_info device_info;

static char Message[BUF_LEN]; /* The message the device will give when asked */

static int encryption_flag = 0; /*Do we need to encrypt?*/

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    printk("device_open(%p)\n", file);

    /* 
     * We don't want to talk to two processes at the same time 
     */
    spin_lock_irqsave(&device_info.lock, flags);
    if (dev_open_flag){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    dev_open_flag++;
    spin_unlock_irqrestore(&device_info.lock, flags); 

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    printk("device_release(%p,%p)\n", inode, file);

    /* ready for our next caller */
    spin_lock_irqsave(&device_info.lock, flags);
    dev_open_flag--;
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

/* a process which has already opened 
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
               char __user * buffer, /* buffer to be filled with data */
               size_t length,  /* length of the buffer     */
               loff_t * offset)
{
    /* read doesnt really do anything (for now) */
    printk("device_read(%p,%d) - operation not supported yet (last written - %s)\n", file, length, Message);

    return -EINVAL; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t
device_write(struct file *file,
         const char __user * buffer, size_t length, loff_t * offset)
{
  int i;
  printk("device_write(%p,%d)\n", file, length);
  for (i = 0; i < length && i < BUF_LEN; i++)
  {
    get_user(Message[i], buffer + i);
		if(1 == encryption_flag)
      Message[i] += 1;
  }
 
  /* return the number of input characters used */
  return i;
}

//----------------------------------------------------------------------------
static
long device_ioctl( //struct inode*  inode,
                  struct file*   file,
                  unsigned int   ioctl_num,/* The number of the ioctl */
                  unsigned long  ioctl_param) /* The parameter to it */
{
  /* Switch according to the ioctl called */
  if(IOCTL_SET_ENC == ioctl_num) 
	{
    /* Get the parameter given to ioctl by the process */
		printk("chardev, ioctl: setting encryption flag to %ld\n", ioctl_param);
    encryption_flag = ioctl_param;
  }

  return SUCCESS;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */

struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl= device_ioctl,
    .open = device_open,
    .release = device_release,  
};

/* Called when module is loaded. 
 * Initialize the module - Register the character device */
static int __init simple_init(void)
{
		unsigned int rc = 0;
    /* init dev struct*/
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);    

    /* Register a character device. Get newly assigned major num */
    rc = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /* 
     * Negative values signify an error 
     */
    if (rc < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", MAJOR_NUM);
        return -1;
    }

    printk("Registeration is a success. The major device number is %d.\n", MAJOR_NUM);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and rmmod when you're done\n");

    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit simple_cleanup(void)
{
    /* 
     * Unregister the device 
     * should always succeed (didnt used to in older kernel versions)
     */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);
