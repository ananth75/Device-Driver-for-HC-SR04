#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h> 
#include <linux/interrupt.h> 
#include <linux/time.h>
#include <linux/delay.h>
#include <asm/delay.h>
#include "hcsr_ioctl.h"

#define MAX_NUM_DEVICES     5
#define MINOR_NUM_BASE      10
#define BUFFER_SIZE         5
#define MAX_IO_COUNT        14
int count =0;
uint64_t tsc1=0, tsc2=0;
struct timeval time1, time2;
uint64_t dist_compute = 2352941;
static int num_of_devices = 1;
module_param(num_of_devices,int,0);
static int my_device_minor_num = 0;

/* Structure for holding the measurement data */
typedef struct {
    int      distance;
    uint64_t timestamp;
}hcsr_measure_t;

struct hcsr_pin_config_t {
    int gpio_pin;
    int enable;
};

struct hcsr_pin_mux_config_t {
    struct hcsr_pin_config_t pin_level_direction;
    struct hcsr_pin_config_t pin_value;
};

struct hcsr_pin_mux_config_t mux_lookup[MAX_IO_COUNT] = {
 {{32, 1 }, {11, 1}},
 {{28, 1}, {12, 1}},
 {{34, 1}, {13, 1}},
 {{16, 1}, {14, 1}},
 {{36, 1}, {6, 1}},
 {{18, 1}, {0, 1}},
 {{20, 1}, {1, 1}},
 {{0, 0}, {38, 1}},
 {{0, 0}, {40, 1}},
 {{22, 1}, {4, 1}},
 {{26, 1}, {10, 1}},
 {{24, 1}, {5, 1}},
 {{42, 1}, {15, 1}},
 {{30, 1}, {7, 1}},
};
/* FIFO Buffer to hold the measurement data per device structure */
typedef struct {
    hcsr_measure_t hcsr_measure[BUFFER_SIZE];
    int head;
    int tail;
    int count;
}hcsr_buffer_t;

/* Per device structure of the driver */
struct hcsr_dev_str{
    struct miscdevice my_dev;  
    int m;
    int delta;
    int trigger;
    int echo;
    hcsr_buffer_t *m_data;
    int measurement_ongoing;
    spinlock_t m_Lock;
    int count;
    int min;
    int max;
};


struct hcsr_dev_str *hcsr_dev_t[MAX_NUM_DEVICES];

typedef struct hcsr_dev_str* phcsr_dev_str;

#if 0
/*Time Stamp Counter code*/
#if defined(__i386__)
static __inline__ unsigned long long get_rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}
#elif defined(__x86_64__)
static __inline__ unsigned long long get_rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

#endif
/* Read the time stamp counter */

static inline __attribute__((no_instrument_function)) uint64_t get_rdtsc(void)

{

    uint32_t high, low;

    __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high));

    return (((uint64_t)high) << 32) | low;

}
int hcsr_pin_input_configuration(int IO_pin) {

    int level_shifter_pin = mux_lookup[IO_pin].pin_level_direction.gpio_pin;
    int gpio_pin = mux_lookup[IO_pin].pin_value.gpio_pin;

    if(mux_lookup[IO_pin].pin_level_direction.enable == 1) {
        gpio_request(level_shifter_pin, "sysfs");
        gpio_direction_output(level_shifter_pin, true);
        gpio_set_value_cansleep(level_shifter_pin, true);
    }
        gpio_request(gpio_pin, "sysfs");
        gpio_direction_input(gpio_pin);
        gpio_set_value_cansleep(gpio_pin, false);
        return gpio_pin;
}

int hcsr_pin_output_configuration(int IO_pin) {

    int level_shifter_pin = mux_lookup[IO_pin].pin_level_direction.gpio_pin;
    int gpio_pin = mux_lookup[IO_pin].pin_value.gpio_pin;

    if(mux_lookup[IO_pin].pin_level_direction.enable == 1) {
        gpio_request(level_shifter_pin, "sysfs");
        gpio_direction_output(level_shifter_pin, true);
        gpio_set_value_cansleep(level_shifter_pin, false);
    }
        gpio_request(gpio_pin, "sysfs");
        gpio_direction_output(gpio_pin, false);
        gpio_set_value_cansleep(gpio_pin, true);
        return gpio_pin;
}

static irqreturn_t gpio_irq_handler(int irq, void *pdev_str)
{
    printk( "Interrupt received from irq: %d\n", irq);
    phcsr_dev_str  phcsr_dev_t = (struct hcsr_dev_str*)pdev_str;
    unsigned long flags;
    int dist,curr_dist,count,max,min;   
    int curr_val = gpio_get_value(phcsr_dev_t->echo);
    //count++;    
    
    if (irq == gpio_to_irq(phcsr_dev_t->echo))
    {
	do_gettimeofday(&time1);
	
        if(curr_val == 1)
        {
            tsc1=get_rdtsc();
            do_gettimeofday(&time1);
            printk("Value of Echo after Interrupt is 1\n");  
            irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);                                           //change the pin to falling edge
        }
        else if (curr_val == 0)
        {  
            printk("Value of Echo after Interrupt is 0\n");  
            tsc2=   get_rdtsc();
            do_gettimeofday(&time2);
            unsigned long long xx = (time2.tv_sec - time1.tv_sec) * 1000000 + (time2.tv_usec - time1.tv_usec);
            
            printk("Value of this is tv secs %lld\n", time2.tv_sec - time1.tv_sec);
            printk("Value of time in Seconds %lld\n", xx);
            do_div(xx, 1000000);

            printk("Value of time in Seconds %lld\n", xx);
            dist = (int)(xx * 17000);

            //dist =  ((int)(tsc2-tsc1)/(139200));                                // processor_freq * 58 = 2400 * 58= 139200 
            printk("Value of distance : %d\n",dist);
            printk("Timstamp diffs %llu\n", tsc2-tsc1);
            printk("Timestamp computed dist %llu\n", div64_u64((tsc2-tsc1), dist_compute)); 
            printk("Value of distance : %d\n",dist);

            {
                spin_lock_irqsave(&phcsr_dev_t->m_Lock, flags );
                if(dist > max)
                {
                    max = dist;
                }
                else if(dist < min)
                {
                    min = dist;
                }
                curr_dist = phcsr_dev_t->m_data->hcsr_measure[phcsr_dev_t->m_data->tail].distance;
                phcsr_dev_t->m_data->hcsr_measure[phcsr_dev_t->m_data->tail].timestamp = tsc2;
                count = phcsr_dev_t->count;
                curr_dist = (curr_dist * count + dist)/(count+1);
                phcsr_dev_t->count = count+1;
                phcsr_dev_t->m_data->hcsr_measure[phcsr_dev_t->m_data->tail].distance = dist;
                phcsr_dev_t->m_data->tail = (phcsr_dev_t->m_data->tail + 1) % BUFFER_SIZE;
                if(phcsr_dev_t->m_data->tail != phcsr_dev_t->m_data->head)
                {
                    phcsr_dev_t->m_data->count++;
                }
                //phcsr_dev_t->ongoing = 0;

                spin_unlock_irqrestore(&phcsr_dev_t->m_Lock, flags);
            }
            /*if(phcsr_dev_t->state == 1)  
            {
                up(&phcsr_dev_t->lock);
                printk("releasing the semaphore\n");

                phcsr_dev_t->state =0;
            }*/
            irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);                //change the pin to rising edge
        }
    }
    return IRQ_HANDLED;  
}

static int hcsr_open(struct inode *inode, struct file *file)
{
    printk("I have been awoken up\n");

    int minor_num = iminor(inode);
    int i = 0;
    for(i = 0; i < num_of_devices; i++)
    {
        if (hcsr_dev_t[i]->my_dev.minor == minor_num)
        {
            file->private_data = hcsr_dev_t[i];
            break;
        }
    }   
    return 0;
}

static int hcsr_close(struct inode *inodep, struct file *filp)
{
    printk("Sleepy time\n");
    struct hcsr_dev_str *dev_str = filp->private_data;
    free_irq(gpio_to_irq(dev_str->echo), dev_str);
    gpio_set_value_cansleep(dev_str->echo, 0);
    gpio_set_value_cansleep(dev_str->trigger, 0);
    gpio_free(dev_str->echo); //free all the pins
    gpio_free(dev_str->trigger); //free all the pins

    printk("freed");
    return 0;
}

static ssize_t hcsr_write(struct file *file, const char *buf,
               size_t len, loff_t *ppos)
{
    int minor_num = iminor(file->f_path.dentry->d_inode), dev_num,trig,echo,m,i=0,user_in,min,max,dist,delta;
    dev_num = minor_num - MINOR_NUM_BASE ;
    printk("Yummy - I just ate %d bytes\n", len);
    trig = hcsr_dev_t[dev_num]->trigger;
    echo = hcsr_dev_t[dev_num]->echo;
    m = hcsr_dev_t[dev_num]->m;
    delta = hcsr_dev_t[dev_num]->delta;
    memset(&user_in, 0, sizeof(int));
    if (copy_from_user(&user_in, buf, len))
        return -EFAULT;
    if(hcsr_dev_t[dev_num]->measurement_ongoing==1)
    {
        printk("There is an ongoing measurement\n");
        return -EINVAL;
    }
    else
    {
        printk("New measurement started\n");
        while(i<m)
        {
            hcsr_dev_t[dev_num]->measurement_ongoing=1;
            gpio_set_value_cansleep(trig, 0);
            gpio_set_value_cansleep(trig, 1);
            udelay(10);
            gpio_set_value_cansleep(trig, 0);                                    //make the trigger pin low
            mdelay(delta);                                                      //in the user space get the frequecy convert it into time for kenel space usage
            i++;
        }
        hcsr_dev_t[dev_num]->count = 0;
        min = hcsr_dev_t[dev_num]->min;
        max = hcsr_dev_t[dev_num]->max;
        dist = hcsr_dev_t[dev_num]->m_data->hcsr_measure[hcsr_dev_t[dev_num]->m_data->tail].distance;
        dist = dist*(m+2)-min - max;
        dist = (int)dist /m ; 
        hcsr_dev_t[dev_num]->m_data->hcsr_measure[hcsr_dev_t[dev_num]->m_data->tail].distance = dist;
    hcsr_dev_t[dev_num]->measurement_ongoing = 0;
    }
    return 0; /* But we don't actually do anything with the data */
}

/*HCSR driver read function*/
static ssize_t hcsr_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    printk("Echo : %d",hcsr_dev_t[0]->echo);
    printk("Trigger : %d",hcsr_dev_t[0]->trigger);
    return 0; /* But we don't actually do anything with the data */
}

void config_interrupt(int gpio_pin, void* dev_num_i) 
{
    int dev_num = (int *)dev_num_i;
    short int irq_gpio;
    if((irq_gpio = gpio_to_irq(gpio_pin)) < 0) 
    {
        printk("Failure in while mapping GPIO and irq %s\n","sensor_irq" );
            return;
    }
    printk(KERN_NOTICE "Mapped int %d\n", irq_gpio);
    if (request_irq(irq_gpio,(irq_handler_t ) gpio_irq_handler, IRQF_TRIGGER_RISING, "sensor_irq", (void*)hcsr_dev_t[dev_num])) 
    {
        printk("Failed in IRQ request\n");
        return;
    }
    return;
}

int check_trigger(int trigger)
{
    int  i;
    int pins[22]={0,1,4,5,6,7,10,11,12,13,14,15,38,40,48,50,52,54,56,58,61,62};
    for(i =0 ; i< 22 ; i++)
    {
        if(pins[i] == trigger)
        {   
            return 0;
        }
    }
    return 1;
}

int check_echo(int echo)
{
    int i;    
    int pins[19]={0,1,4,5,6,7,10,11,12,13,14,15,48,50,52,54,56,58,62};
     for(i =0 ; i< 19 ; i++)
    {
        if(pins[i] == echo)
        {       
            return 0;
        }
    }
    return 1;
}

static ssize_t hcsr_pinconfiguration(void *handle, int trigger, int echo, int minor_num)
{
    int dev_num = minor_num-MINOR_NUM_BASE;
    printk("Inside HCSR Pin Configuration\n");
    echo = hcsr_pin_input_configuration(echo);
    trigger = hcsr_pin_output_configuration(trigger);
    if(check_trigger(echo)== 0 && check_echo(trigger) == 0)
    {
        hcsr_dev_t[dev_num]->echo = echo;
        hcsr_dev_t[dev_num]->trigger = trigger; 
        config_interrupt(echo, (void*)dev_num);
    }
    else
    {
        printk("Enter valid Trigger and Echo pins");
        return 1;
    }
    printk("Reading hcsr_dev_t[minor_num - MINOR_NUM_BASE]->echo%d\n",hcsr_dev_t[dev_num]->echo);
    printk("Reading hcsr_dev_t[minor_num - MINOR_NUM_BASE]->trigger%d\n",hcsr_dev_t[dev_num]->trigger);
    return 0;
}

static ssize_t hcsr_setparameters(void * handle, int m, int delta, int minor_num)
{
    int dev_num = minor_num-MINOR_NUM_BASE;
    printk("Inside HCSR Set Parameters\n");
    hcsr_dev_t[dev_num]->m = m;
    hcsr_dev_t[dev_num]->delta = delta;
    return 0;
}

static ssize_t hcsr_ioctl(struct file *file,unsigned int ioctl_num, unsigned long ioctl_param)
{
    phcsr_dev_str phcsr_dev = file->private_data;
    void *pIoctlBuf = NULL;
    my_device_minor_num = iminor(file->f_path.dentry->d_inode);

    printk("My device ioctl minor number = %d\n", my_device_minor_num);   
    
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
            hcsr_pinconfiguration((void*)phcsr_dev,((PSIOCTL_PINCONG)pIoctlBuf)->trigger, ((PSIOCTL_PINCONG)pIoctlBuf)->echo,my_device_minor_num);
            break;
        case HCSR_IOCTL_SETPARAM:
            hcsr_setparameters( (void*)phcsr_dev,((PSIOCTL_SETPARAM)pIoctlBuf)->m, ((PSIOCTL_SETPARAM)pIoctlBuf)->delta,my_device_minor_num);
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
        hcsr_dev_t[i]->my_dev.minor = i + MINOR_NUM_BASE;
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
        hcsr_dev_t[i]->m_data->head = 0;
        hcsr_dev_t[i]->m_data->tail = 0;
        hcsr_dev_t[i]->m_data->count =0;
        hcsr_dev_t[i]->measurement_ongoing = 0;
        hcsr_dev_t[i]->count = 0;
        hcsr_dev_t[i]->min = INT_MAX;
        hcsr_dev_t[i]->max = 0;
        memset(&time1, 0, sizeof(struct timeval));
        memset(&time2, 0, sizeof(struct timeval));
    }
    return 0;
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
