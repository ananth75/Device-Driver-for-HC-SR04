#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "hcsr_ioctl.h"

#define MAX_NUM_DEVICES     5

static int num_of_devices = 1;
module_param(num_of_devices,int,0);
static int my_device_monir_num = 0;
/* Structure for holding the measurement data */
typedef struct {
    int      distance;
    uint64_t timestamp;
}hcsr_measure_t;


/* FIFO Buffer to hold the measurement data per device structure */
typedef struct {
    hcsr_measure_t hcsr_measure[5];
    int head;
    int tail;
}hcsr_buffer_t;


/* Per device structure of the driver */
struct hcsr_dev_str{
    struct miscdevice my_dev;  
    int mode;
    int trigger;
    int echo;
    int time;
    hcsr_buffer_t *m_data;
};

struct hcsr_dev_str *hcsr_dev_t[MAX_NUM_DEVICES];

typedef struct hcsr_dev_str* phcsr_dev_str; 

static int hcsr_open(struct inode *inode, struct file *file)
{
    printk("I have been awoken up\n");
    return 0;
}

static int hcsr_close(struct inode *inodep, struct file *filp)
{
    printk("Sleepy time\n");
    return 0;
}

static ssize_t hcsr_write(struct file *file, const char *buf,
               size_t len, loff_t *ppos)
{
    printk("Yummy - I just ate %d bytes\n", len);
    return 0; /* But we don't actually do anything with the data */
}

/*HCSR driver read function*/
static ssize_t hcsr_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    printk("Echo : %d",hcsr_dev_t[0]->echo);
    printk("Trigger : %d",hcsr_dev_t[0]->trigger);
    return 0; /* But we don't actually do anything with the data */
}

static ssize_t hcsr_pinconfiguration(void *handle, int trigger, int echo)
{
    printk("Inside HCSR Pin Configuration\n");
    hcsr_dev_t[0]->echo = echo;
    hcsr_dev_t[0]->trigger = trigger;
    printk("Reading hcsr_dev_t[0]->echo%d\n",hcsr_dev_t[0]->echo);
    printk("Reading hcsr_dev_t[0]->trigger%d\n",hcsr_dev_t[0]->trigger);
    return 0;
}

static ssize_t hcsr_setparameters(void * handle, int mode, int time)
{
    printk("Inside HCSR Set Parameters\n");
    hcsr_dev_t[0]->mode = mode;
    hcsr_dev_t[0]->time = time;
    return 0;
}

static ssize_t hcsr_ioctl(struct file *file,unsigned int ioctl_num, unsigned long ioctl_param)
{
    phcsr_dev_str phcsr_dev = file->private_data;
    void *pIoctlBuf = NULL;
    my_device_monir_num = iminor(file->f_path.dentry->d_inode);

    printk("My device ioctl minor number = %d\n", my_device_monir_num);   
    
    if (!(pIoctlBuf = kmalloc(_IOC_SIZE(ioctl_num), GFP_KERNEL)))
    {
        printk("HCSR driver : Error while allocating IO buffer memory in IOCTL\n");
        return -ENOMEM;
    }

    if(copy_from_user(pIoctlBuf,(unsigned long *)(ioctl_param), _IOC_SIZE(ioctl_num)))
    {
        printk("HCSR driver : Error while copying data from userspace to IO buffer memory in IOCTL\n");
        kfree(pIoctlBuf);
        return -ENOMEM;
    }
    switch(ioctl_num)
    {
        case HCSR_IOCTL_PINCONG:
            hcsr_pinconfiguration( (void*)phcsr_dev,((PSIOCTL_PINCONG)pIoctlBuf)->trigger, ((PSIOCTL_PINCONG)pIoctlBuf)->echo);
            break;
        case HCSR_IOCTL_SETPARAM:
            hcsr_setparameters( (void*)phcsr_dev,((PSIOCTL_SETPARAM)pIoctlBuf)->mode, ((PSIOCTL_SETPARAM)pIoctlBuf)->time);
            break;
        kfree(pIoctlBuf);
        pIoctlBuf = NULL;
    }

    return 0;
}
static const struct file_operations hcsr_fops = {
    .owner          = THIS_MODULE,
    .write          = hcsr_write,
    .open           = hcsr_open,
    .release        = hcsr_close,
    .read           = hcsr_read,
    .unlocked_ioctl = hcsr_ioctl,
};


static int __init misc_init(void)
{
    int i,error;
    char dev_name[50];
    printk("Value of num_of_devices : %d",num_of_devices);
    for (i = 0; i < num_of_devices; i++)
    {   
        hcsr_dev_t[i] = kmalloc(sizeof(struct hcsr_dev_str), GFP_KERNEL);
        if(!hcsr_dev_t[i])
        {
            printk("BAD kmalloc in per device struct \n");
            return -ENOMEM;
        }

        memset(hcsr_dev_t[i], 0, sizeof(struct hcsr_dev_str));
        memset(dev_name, 0, 50 * sizeof(char));

        /* Set the minor numbers and device names of each of the per device structures */
        hcsr_dev_t[i]->my_dev.minor = i + 10;
        sprintf(dev_name, "HCSR_%d", i);
        hcsr_dev_t[i]->my_dev.name = dev_name;
        hcsr_dev_t[i]->my_dev.fops = &hcsr_fops;

        /* Register the miscellaneous driver to be able to detect it as in the character devices block */
        error = misc_register(&hcsr_dev_t[i]->my_dev);


        if (error) {
            pr_err("can't misc_register\n");
            return error;
        }

        /* Initialize the FIFO buffer in the per-device structure */
        hcsr_dev_t[i]->m_data = kmalloc(sizeof(hcsr_buffer_t), GFP_KERNEL);
        if(!hcsr_dev_t[i]->m_data){
            printk("BAD kmalloc in per device struct \n");
            return -ENOMEM;
        }

        hcsr_dev_t[i]->m_data->head = hcsr_dev_t[i]->m_data->tail = 0;

    }

}

static void __exit misc_exit(void)
{
    int i;
    for (i = 0 ; i < num_of_devices; i++) {
        kfree(hcsr_dev_t[i]->m_data);
        printk("FIFO Buffer Freed\n");
        misc_deregister(&hcsr_dev_t[i]->my_dev);
        printk("Misc driver de-initialized\n");
        kfree(hcsr_dev_t[i]);
        printk("Freed per-device structure\n");
    }
    
}

module_init(misc_init);
module_exit(misc_exit);

MODULE_DESCRIPTION("Simple Misc Driver");
MODULE_LICENSE("GPL");
