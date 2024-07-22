#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/cred.h>

#define DEV_NAME "my_char_driver"
#define INITIAL_BUFFER_SIZE 512
#define IOCTL_SET_IO_MODE _IOW('a', 1, int*)
#define IOCTL_GET_LAST_OP_INFO _IOR('a', 2, struct last_op_info*)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Suvorov Dmitriy dim-suv@mail.ru");
MODULE_DESCRIPTION("A character driver with a shared ring buffer");

static int buffer_size = INITIAL_BUFFER_SIZE;
module_param(buffer_size, int, S_IRUGO);

struct ring_buffer {
    char* data;
    int write_p;
    int read_p;
    int size;
    struct semaphore sem;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
};

struct last_operation_info {
    unsigned long last_read_time;
    unsigned long last_write_time;
    pid_t read_proc_id;
    pid_t write_proc_id;
    kuid_t read_user_id;
    kuid_t write_user_id;
};

static struct ring_buffer* r_buf;
static struct last_operation_info rw_info;

static dev_t    dev_nums;
static struct   cdev my_char_dev;

static int      c_dev_open(struct inode*, struct file*);
static int      c_dev_release(struct inode*, struct file*);
static ssize_t  c_dev_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t  c_dev_write(struct file*, const char __user*, size_t, loff_t*);
static long int c_dev_ioctl(struct file*, unsigned int, unsigned long);

static struct file_operations fops = {
   .owner = THIS_MODULE,
   .open = c_dev_open,
   .read = c_dev_read,
   .write = c_dev_write,
   .unlocked_ioctl = c_dev_ioctl,
   .release = c_dev_release,
};

static int __init my_char_driver_init(void) {
    int success;

    printk(KERN_INFO "Initializing %s\n", DEV_NAME);

    // buffer initializing
    r_buf = kmalloc(sizeof(struct ring_buffer), GFP_KERNEL);
    if (!r_buf) {
        printk(KERN_ERR "Failed to allocate memory for the ring buffer\n");
        return -ENOMEM;
    }
    r_buf->data = kmalloc(buffer_size, GFP_KERNEL);
    if (!r_buf->data) {
        kfree(r_buf);
        printk(KERN_ERR "Failed to allocate memory for buffer data\n");
        return -ENOMEM;
    }
    r_buf->write_p = 0;
    r_buf->read_p = 0;
    r_buf->size = buffer_size;
    sema_init(&r_buf->sem, 1);
    init_waitqueue_head(&r_buf->read_queue);
    init_waitqueue_head(&r_buf->write_queue);

    // Try to get a major device number
    success = alloc_chrdev_region(&dev_nums, 0, 1, DEV_NAME);
    if (success < 0) {
        kfree(r_buf->data);
        kfree(r_buf);
        printk(KERN_ERR "Error registering major number for the device %s\n", DEV_NAME);
        return success;
    }
    printk(KERN_INFO "The major number <%d> and the minor number <%d> for the device '%s' has been successfully registered\n", 
           MAJOR(dev_nums), MINOR(dev_nums), DEV_NAME);

    // init and add to the system struct cdev
    cdev_init(&my_char_dev, &fops);
    my_char_dev.owner = THIS_MODULE;

    success = cdev_add(&my_char_dev, dev_nums, 1);
    if (success < 0) {
        unregister_chrdev_region(dev_nums, 1);
        kfree(r_buf->data);
        kfree(r_buf);
        printk(KERN_ERR "Failed to add character device\n");
        return success;
    }

    printk(KERN_INFO "The character device is successfully created\n");
    return 0;
}

static void __exit my_char_driver_exit(void) {
    cdev_del(&my_char_dev);
    unregister_chrdev_region(dev_nums, 1);
    kfree(r_buf->data);
    kfree(r_buf);
    printk(KERN_INFO "Goodbye from the %s!\n", DEV_NAME);
}

static int c_dev_open(struct inode* inode_p, struct file* file_p) {
    file_p->f_flags &= ~O_NONBLOCK; // Read and Write operations are initially blocking
    printk(KERN_INFO "Device '%s' has been opened\n", DEV_NAME);
    return 0;
}

static int c_dev_release(struct inode* inode_p, struct file* file_p) {
    printk(KERN_INFO "Device '%s' has been closed\n", DEV_NAME);
    return 0;
}

static ssize_t c_dev_read(struct file* file_p, char __user* buffer, size_t len, loff_t* offset) {
    size_t bytes_read = 0;

    if (down_interruptible(&r_buf->sem))
        return -ERESTARTSYS;

    while (r_buf->write_p == r_buf->read_p) { /* while nothing to read */
        up(&r_buf->sem);

        // Nonblocking reading
        if (file_p->f_flags & O_NONBLOCK)
            return -EAGAIN;

        // Blocking reading
        if (wait_event_interruptible(r_buf->read_queue, (r_buf->write_p != r_buf->read_p)))
            return -ERESTARTSYS;

        if (down_interruptible(&r_buf->sem))
            return -ERESTARTSYS;
    }

    /* there is some data to read */
    if (r_buf->write_p > r_buf->read_p)
        bytes_read = min(len, (size_t)(r_buf->write_p - r_buf->read_p));
    else
        bytes_read = min(len, (size_t)(r_buf->size - r_buf->read_p));

    if (copy_to_user(buffer, r_buf->data + r_buf->read_p, bytes_read)) {
        up(&r_buf->sem);
        return -EFAULT;
    }

    r_buf->read_p += bytes_read;
    if (r_buf->read_p == r_buf->size)
        r_buf->read_p = 0;

    // if it is possible to read the data from the buffer first
    if (r_buf->read_p == 0 && len - bytes_read > 0) {
        int rest = min(len - bytes_read, (size_t)(r_buf->write_p));
        if (copy_to_user(buffer + bytes_read, r_buf->data, rest)) {
            up(&r_buf->sem);
            return -EFAULT;
        }
        bytes_read += rest;
    }

    rw_info.last_read_time = jiffies;
    rw_info.read_proc_id = current->pid;
    rw_info.read_user_id = current_uid();

    up(&r_buf->sem);

    // waking up the writers
    wake_up_interruptible(&r_buf->write_queue);

    printk(KERN_INFO "read_op: %zu bytes were read\n", len);
    return bytes_read;
}

static ssize_t c_dev_write(struct file* file_p, const char __user* buffer, size_t len, loff_t* offset) {
    size_t bytes_written = 0;

    if (down_interruptible(&r_buf->sem))
        return -ERESTARTSYS;

    while ((r_buf->write_p + 1) % r_buf->size == r_buf->read_p) { // no place to write
        up(&r_buf->sem);

        // Nonblocking writing
        if (file_p->f_flags & O_NONBLOCK)
            return -EAGAIN;

        // Blocking writing
        if (wait_event_interruptible(r_buf->write_queue, ((r_buf->write_p + 1) % r_buf->size != r_buf->read_p)))
            return -ERESTARTSYS;

        if (down_interruptible(&r_buf->sem))
            return -ERESTARTSYS;
    }

    // There is some place to write
    if (r_buf->write_p < r_buf->read_p)
        bytes_written = min(len, (size_t)(r_buf->read_p - r_buf->write_p - 1));
    else
        if (r_buf->read_p == 0)
            bytes_written = min(len, (size_t)(r_buf->size - r_buf->write_p - 1));
        else
            bytes_written = min(len, (size_t)(r_buf->size - r_buf->write_p));

    if (copy_from_user(r_buf->data + r_buf->write_p, buffer, bytes_written)) {
        up(&r_buf->sem);
        return -EFAULT;
    }

    // update the writer's index and, if necessary, go to the beginning of the buffer
    r_buf->write_p = (r_buf->write_p + bytes_written) % r_buf->size;

    if (r_buf->write_p == 0 && len - bytes_written > 0 && r_buf->read_p > 1) {
        int rest = min(len - bytes_written, (size_t)(r_buf->read_p - 1));
        if (copy_from_user(r_buf->data, buffer + bytes_written, rest)) {
            up(&r_buf->sem);
            return -EFAULT;
        }
        bytes_written += rest;
    }

    rw_info.last_write_time = jiffies;
    rw_info.write_proc_id = current->pid;
    rw_info.write_user_id = current_uid();

    up(&r_buf->sem);

    // waking up the readers
    wake_up_interruptible(&r_buf->read_queue);

    printk(KERN_INFO "write_op: %zu bytes were written\n", len);
    return bytes_written;
}

static long int c_dev_ioctl(struct file* file_p, unsigned int cmd, unsigned long arg) {
    int mode;
    struct last_operation_info local_info;

    switch(cmd) {
        case IOCTL_SET_IO_MODE:
            if (copy_from_user(&mode, (int*)arg, sizeof(mode))) {
                return -EFAULT;
            }

            if (mode == 0) {
                file_p->f_flags &= ~O_NONBLOCK; // Disable nonblocking mode
            }
            else {
                file_p->f_flags |= O_NONBLOCK;  // Enable nonblocking mode
            }
            break;
        case IOCTL_GET_LAST_OP_INFO:
            local_info = rw_info;
            if (copy_to_user((struct last_op_info*)arg, &local_info, sizeof(local_info))) {
                return -EFAULT;
            }
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

module_init(my_char_driver_init);
module_exit(my_char_driver_exit);
