#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/slab.h>

#define HCSR_NUM_DEVICES     5


/* Structure for holding the measurement data */
typedef struct {
    int      distance;
    uint64_t timestamp;
}hcsr_measure_t;


/* FIFO Buffer to hold the mesaurement data per device structure */
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
    hcsr_buffer_t *m_data;
};

struct hcsr_dev_str *hcsr_dev_t[HCSR_NUM_DEVICES];


static int hcsr_open(struct inode *inode, struct file *file)
{
    pr_info("I have been awoken\n");
    return 0;
}

static int hcsr_close(struct inode *inodep, struct file *filp)
{
    pr_info("Sleepy time\n");
    return 0;
}

static ssize_t hcsr_write(struct file *file, const char *buf,
		       size_t len, loff_t *ppos)
{
    pr_info("Yummy - I just ate %d bytes\n", len);
    return 0; /* But we don't actually do anything with the data */
}

/*HCSR driver read function*/
static ssize_t hcsr_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    return 0; /* But we don't actually do anything with the data */
}

static const struct file_operations hcsr_fops = {
    .owner			= THIS_MODULE,
    .write			= hcsr_write,
    .open			= hcsr_open,
    .release		= hcsr_close,
    .read 		    = hcsr_read,
};


static int __init misc_init(void)
{
    int i,error;
    char dev_name[50];

    for (i = 0; i < HCSR_NUM_DEVICES; i++)
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
        hcsr_dev_t[i]->my_dev.minor = i + 1;
        sprintf(dev_name, "HCSR_%d", i);
        hcsr_dev_t[i]->my_dev.name = dev_name;
        hcsr_dev_t[i]->my_dev.fops = &hcsr_fops;

        /* Register the miscallaneous driver to be able to detect it as in the character devices block */
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
    for (i = 0 ; i < HCSR_NUM_DEVICES; i++) {
        kfree(hcsr_dev_t[i]->m_data);
        printk("FIFO Buffer Freed");
        misc_deregister(&hcsr_dev_t[i]->my_dev);
        printk("Misc driver de-initialized");
        kfree(hcsr_dev_t[i]);
        printk("Freed per-device structure");
    }
    
}

module_init(misc_init);
module_exit(misc_exit);

MODULE_DESCRIPTION("Simple Misc Driver");
MODULE_LICENSE("GPL");
