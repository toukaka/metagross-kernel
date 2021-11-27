/*
 * file: gkos_char_device.c
 *
 * Desc: A simple device that
 *      echos a message when read,
 *      write method not implemented
 *
 *      This was made on top of 
 *      LDD and LKMPG examples 
 *      
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "gkos_char_device"

/*
 * Prototypes 
 */
int init_module(void);
void cleanup_module(void);
static int dev_open(struct inode *inode, struct file *);
static int dev_release(struct inode *inode, struct file *);
static ssize_t dev_read(struct file *fp, char *buf, size_t len, loff_t *off);
static ssize_t dev_write(struct file *, const char *buf, size_t len, 
                         loff_t *off);
static int dev_init(void);

/*
 * Our variables, @use_counter
 * will block concurrenty opens.
 * @buffer is the message and
 * @buffer_len the lenght of @buffer (duh)
 */
static char use_counter = 0;
static char buffer[] = "Hello character device world\n";
static int buffer_len = sizeof(buffer);

static dev_t dev;
static struct cdev *cdevp;


static struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = dev_read,
        .write = dev_write,
        .open = dev_open,
        .release = dev_release
};

/*
 * Any device specific initialization
 * goes here. Its called at bottom of init_module()
 */
static int dev_init(void)
{
        return 0;
}

/*
 * Called when device is opened
 */
static int dev_open(struct inode *inode, struct file *fp)
{
        if (use_counter)
                return -EBUSY;
        use_counter++;
        try_module_get(THIS_MODULE);
        return 0;
}

/* 
 * Called when device is released. The device is
 * released when there is no process using it.
 */
static int dev_release(struct inode *inode, struct file *fp)
{
        use_counter--;
        module_put(THIS_MODULE);
        return 0;
}


/*
 * @off controls
 * the "walk" through our buffer, is whith @off
 * that we say to user where is stoped.
 * @len is how much bytes to read. I almost ignore it.
 * I just check if is greater than 0.
 *
 * Called when device is read. 
 * This method will read one, and only one byte per call,
 * If @off is longer than my buffer size or len is not
 * greater than 0 it returns 0, otherwise I copy one byte
 * to user buffer and returns the bytes readed, so 1.
 */
static ssize_t dev_read(struct file *fp, char *buf, size_t len, loff_t *off)
{
        if (*off >= buffer_len || len <= 0)
                return 0;

        if (copy_to_user(buf, &buffer[*off], 1u))
                return -EFAULT;

        (*off)++;
        return 1;
}


/*
 * Not implemented at all
 */
static ssize_t dev_write(struct file *fp, const char *buf, size_t len, 
                         loff_t *off)
{
        return -ENOSYS;
}

/*
 * Called when module is load
 */
int init_module(void)
{
        int error;

        /* Alloc a device region */
        error = alloc_chrdev_region(&dev, 1, 1, DEVICE_NAME);
        if (error) 
                goto error_out;

        /* Registring */
        cdevp = cdev_alloc();
        if (!cdevp) 
                return -ENOMEM; 

        /* Init it! */
        cdev_init(cdevp, &fops); 

        /* Tell the kernel "hey, I'm exist" */
        error = cdev_add(cdevp, dev, 1);
        if (error < 0) 
                goto error_out;

        printk(KERN_INFO DEVICE_NAME " registred with major %d\n", MAJOR(dev));
        printk(KERN_INFO DEVICE_NAME " do: `mknod /dev/%s c %d %d' to create "
               "the device file\n", 
               DEVICE_NAME, MAJOR(dev), MINOR(dev)); 

        /* Device initialization isn't needed yet */
        if (dev_init())
                goto error_out;

        return 0;

error_out:
        return -EFAULT;
}

void cleanup_module(void)
{
        cdev_del(cdevp);
}

MODULE_LICENSE("GPL");
