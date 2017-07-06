#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel  yayyyy :) :) :)*/
#undef MODULE
#define MODULE     /* Not a permanent part, though. (don't be silly, we can't write permanent part yet)*/

#define UNDEFINED -1
//Our custom definitions of IOCTL operations
#include "message_slot.h"

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");


struct message_slot{
	ino_t file_ino;
	int index;
	char buffers[NUM_OF_BUFFERS][BUFFER_SIZE];
	struct message_slot* next;
};

static struct message_slot *root = NULL;

static struct message_slot* get_file_message_slot(ino_t file_ino)
{
	struct message_slot *slot = root;
	while(slot != NULL)
	{
		if(slot->file_ino == file_ino)
			return slot;
		slot = slot->next;
	}
	return NULL;
}

static int create_message_slot(ino_t file_ino)
{
	struct message_slot *slot = (struct message_slot*) kmalloc(sizeof(struct message_slot));
	if(slot == NULL)
	{
		printk("failed to malloc new message slot\n");
		return -1;
	}
	slot->file_ino = file_ino;
	slot->index = UNDEFINED; 
	slot->next = root;
	root = slot;
	return SUCCESS;
}

static int delete_message_slot(struct message_slot *slot)
{
	if(root == slot)
		root = slot->next;
	else
	{
		struct message_slot *temp = root;
		while(temp->next != slot)
		{
			if(temp->next == NULL)
			{
				printk("message slot not found!\n");
				return -1;
			}
			temp = temp->next;
		}
		temp->next = temp->next->next;
	}
	
	kfree(slot);
	return SUCCESS;
}

/***************** char device functions *********************/
/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    printk("device_open(%p)\n", file);

	struct message_slot *slot =  get_file_message_slot(file->f_inode->i_ino);
	if(slot == NULL)
	{
		if(create_message_slot(file->f_inode->i_ino) != SUCCESS)
			return -1;
	}

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    printk("device_release(%p,%p)\n", inode, file);

	/*struct message_slot *slot =  get_file_message_slot(file->f_inode->i_ino);
	if(slot == NULL)
	{
		printk("unknown file\n");
		return -1;
	}
	
	return delete_message_slot(slot);*/
	return SUCCESS;
}

static ssize_t device_read(struct file *file, char __user * buffer, size_t length, loff_t * offset)
{
	int i;
	if(buffer == NULL)
	{
		printk("buffer is NULL\n");
		return -1;
	}
	struct message_slot *slot =  get_file_message_slot(file->f_inode->i_ino);
	if(slot == NULL)
	{
		printk("couldn't find file\n");
		return -1;
	}
	if(slot->index == UNDEFINED)
	{
		printk("message_slot index never initialized\n");
		return -1;
	}
	for (i = 0; i < length && i < BUFFER_SIZE; i++)
		put_user(slot->buffers[slot->index][i], buffer + i);
	
    return i; // invalid argument error
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, 
	const char __user * buffer, size_t length, loff_t * offset)
{
	int i;
	printk("device_write(%p,%d)\n", file, length);
	if(buffer == NULL)
	{
		printk("buffer is NULL\n");
		return -1;
	}
	struct message_slot *slot =  get_file_message_slot(file->f_inode->i_ino);
	if(slot == NULL)
	{
		printk("couldn't find file\n");
		return -1;
	}
	if(slot->index == UNDEFINED)
	{
		printk("message_slot index never initialized\n");
		return -1;
	}
	
	for (i = 0; i < length && i < BUFFER_SIZE; i++)
		get_user(slot->buffers[slot->index][i], buffer + i);
	for(int j=i; j < BUFFER_SIZE; j++)
		slot->buffers[slot->index][i] = 0;
	
	return i;
}

//----------------------------------------------------------------------------
static long device_ioctl(struct file*   file, unsigned int ioctl_num, unsigned long  ioctl_param) 
{
	struct message_slot *slot =  get_file_message_slot(file->f_inode->i_ino);
	if(slot == NULL)
	{
		printk("couldn't find file\n");
		return -1;
	}
	if(ioctl_param<0 || ioctl_param >= NUM_OF_BUFFERS)
	{
		printk("invalid index %d\n",ioctl_param);
		return -1;
	}
	
	if(IOCTL_SET_INDEX == ioctl_num) 
	{
	
		printk("message slot ioctl: set index to %ld\n", ioctl_param);
		slot->index = ioctl_param;
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
	while(root != NULL) //free all the data 
		delete_message_slot(root);
    /* 
     * Unregister the device 
     * should always succeed (didnt used to in older kernel versions)
     */
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}


module_init(simple_init);
module_exit(simple_cleanup);
