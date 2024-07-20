#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>

#define DEV_NAME "my_char_driver"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Suvorov Dmitriy dim-suv@mail.ru");
MODULE_DESCRIPTION("A character driver with a shared ring buffer");

static dev_t   dev_nums;
static struct  cdev my_char_dev;

static int     c_dev_open(struct inode*, struct file*);
static int     c_dev_release(struct inode*, struct file*);
static ssize_t c_dev_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t c_dev_write(struct file*, const char __user*, size_t, loff_t*);
static int     c_dev_ioctl(struct inode*, struct file*, unsigned int, unsigned long);

static struct file_operations fops = {
   .owner = THIS_MODULE,
   .open = c_dev_open,
   .read = c_dev_read,
   .write = c_dev_write,
   .ioctl = c_dev_ioctl,
   .release = dev_release,
};

static int __init my_char_driver_init(void) {
    int success;

    printk(KERN_INFO "Initializing %s\n", DEV_NAME);

    // Try to get a major device number
    success = alloc_chrdev_region(&dev_nums, 0, 1, DEV_NAME);
    if (success  0) {
        printk(KERN_ERR "Error registering major number for the device %s\n", DEV_NAME);
        return success;
    }
    printk(KERN_INFO "The major number <%d> for the device '%s' has been successfully registered\n", DEV_NAME, MAJOR(dev_num));

    // init and add to the system struct cdev
    cdev_init(&my_char_dev, &fops);
    my_char_dev.owner = THIS_MODULE;

    success = cdev_add(&my_char_dev, dev_nums, 1);
    if (success < 0) {
        unregister_chrdev_region(dev_nums, 1);
        printk(KERN_ERR "Failed to add character device\n");
        return result;
    }

    printk(KERN_INFO "The character device is successfully created\n");
    return 0;
}

static void __exit my_char_driver_exit(void) {
    cdev_del(&my_char_dev);
    unregister_chrdev_region(dev_nums, 1);
    printk(KERN_INFO "Goodbye from the %s!\n", DEV_NAME);
}

static int dev_open(struct inode* inode_p, struct file* file_p) {
    printk(KERN_INFO "Device '%s' has been opened\n", DEV_NAME);
    return 0;
}

static int dev_release(struct inode* inode_p, struct file* file_p) {
    printk(KERN_INFO "Device '%s' has been closed\n", DEV_NAME);
    return 0;
}

static ssize_t dev_read(struct file* file_p, char* buffer, size_t len, loff_t* offset) {
    printk(KERN_INFO "Read operation\n");
    return 0;
}

static ssize_t dev_write(struct file* filep, const char* buffer, size_t len, loff_t* offset) {
    printk(KERN_INFO "Write operation\n");
    return len;
}

static int c_dev_ioctl(struct inode*, struct file*, unsigned int, unsigned long) {
    printk(KERN_INFO "IOCTL command\n");
    return 0;
}

module_init(my_char_driver_init);
module_exit(my_char_driver_exit);