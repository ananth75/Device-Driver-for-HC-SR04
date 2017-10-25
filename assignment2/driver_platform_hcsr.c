/*
 * A program to show the binding of platform driver and device.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include "device_platform_hcsr.h"

#define SIZE_OF_BUF	1
int errno;
short int irq_num    = 0;
int errno;
uint64_t tsc1=0, tsc2=0;
int dist;

struct class *hcsr_class;
struct device *hcsr_device;
static LIST_HEAD(device_list);
static dev_t hcsr_dev;

static const struct platform_device_id P_id_table[] = {
         { "DEVICE_NAME", 0 },
        //{ "HCSR_2", 0 },
        // { "HCSR_3", 0 },
};

#if defined(__i386__)
static __inline__ unsigned long long get_tsc(void)
{
	unsigned long long int x;
 	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
 	return x;
}

#elif defined(__x86_64__)
static __inline__ unsigned long long get_tsc(void)
{
	unsigned hi, lo;
  	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  	return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

//Show functions
static ssize_t sampling_period_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
	return snprintf(buf, PAGE_SIZE, "%d\n", pphcsr_dev_str_obj->time);
}

static ssize_t samples_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%d\n", pphcsr_dev_str_obj->samples);
}

static ssize_t trigger_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%d\n", pphcsr_dev_str_obj->trigger);
}

static ssize_t echo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
    return snprintf(buf, PAGE_SIZE, "%d\n", pphcsr_dev_str_obj->echo);
}
/*
static ssize_t Enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
        return snprintf(buf, PAGE_SIZE, "%d\n", pphcsr_dev_str_obj->enable);
}
*/
static ssize_t distance_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
	printk("distance = %d\n", pphcsr_dev_str_obj->pshcsr_buffer_obj->pdata[pphcsr_dev_str_obj->pshcsr_buffer_obj->tail].distance);
	return snprintf(buf, PAGE_SIZE, "%d\n",pphcsr_dev_str_obj->pshcsr_buffer_obj->pdata[pphcsr_dev_str_obj->pshcsr_buffer_obj->tail].distance);
}

//Store functions
static ssize_t samples_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t count)
{
      phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
      sscanf(buf, "%d", &pphcsr_dev_str_obj->samples);
      printk("number of samples needed : %d\n",pphcsr_dev_str_obj->samples);
      return count;
}

static ssize_t sampling_period_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t count)
{
      phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
      sscanf(buf, "%d", &pphcsr_dev_str_obj->time);
      printk("Time between each trigger would be : %d\n",pphcsr_dev_str_obj->time);
      return count;
}

static ssize_t trigger_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);		
    sscanf(buf, "%d", &pphcsr_dev_str_obj->trigger);
	printk("Trigger pin has been stored\n");
	gpio_request(pphcsr_dev_str_obj->trigger, "sysfs");    							
   	gpio_direction_output(pphcsr_dev_str_obj->trigger, 0);
	printk("Trigger pin direction has been set as output\n");	
	return count;
}

static ssize_t echo_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf,
                                  size_t count)
{
	phcsr_dev_str pphcsr_dev_str_obj = dev_get_drvdata(dev);
    sscanf(buf, "%d", &pphcsr_dev_str_obj->echo);
	printk("Echo pin has been stored\n");
	gpio_request(pphcsr_dev_str_obj->echo, "sysfs");        
	gpio_direction_input(pphcsr_dev_str_obj->echo);
	printk("Echo pin direction has been set as input\n");	
	//r_int_config(pshcsr_dev_obj->echo, (void*)pshcsr_dev_obj);
	return count;
}
/*
static ssize_t enable_store(struct device *dev,  struct device_attribute *attr, const char *buf, size_t count)
{

}
*/

static DEVICE_ATTR(trigger,S_IRWXU, trigger_show, trigger_store);
static DEVICE_ATTR(echo,S_IRWXU, echo_show, echo_store);
static DEVICE_ATTR(samples,S_IRWXU,samples_show, samples_store);
static DEVICE_ATTR(sampling_period,S_IRWXU, sampling_period_show, sampling_period_store);
//static DEVICE_ATTR(Enable,S_IRWXU, Enable_show, Enable_store);
static DEVICE_ATTR(distance, S_IRWXU, distance_show,NULL);

static int hcsr_open(struct inode *inode, struct file *file)
{    
    pr_info("Device opened\n");
    const struct file_operations *new_fops = NULL;
	int dev_minor= iminor(inode);
	phcsr_dev_str pphcsr; 
	printk("minor number in open = %d\n", dev_minor);	
	list_for_each_entry(pphcsr, &device_list, device_entry)
	{
		if(pphcsr->dev_misc.minor == dev_minor)
		{
			new_fops = fops_get(pphcsr->dev_misc.fops);
			break;
		}
	}
	file->private_data = pphcsr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////

/*INTERRUPT HANDLER*/

///////////////////////////////////////////////////////////////////////////////////////////////

static irqreturn_t r_irq_handler(int irq, void *dev_id)
{
	phcsr_dev_str pphcsr_dev_str = (phcsr_dev_str)dev_id;	
	int check = gpio_get_value(pphcsr_dev_str->echo);
	printk("the gpio get value is %d", check);
	printk("INTERRUPT HANDLER HERE\n");	
	printk( "interrupt received (irq: %d)\n", irq);	
	{
		if (check ==0)
		{  
	        	printk("gpio pin is low\n");  
	 		tsc2=get_tsc();
			printk("TSC2 = %llu\n",tsc2);
			dist =  ((int)(tsc2-tsc1)/(139200)); //
			printk("the distance is %d",dist);					
			

			irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);			
		}
       		else
		{
			tsc1=get_tsc();
			printk("TSC1 = %llu\n",tsc1);
	    		printk("gpio pin is high\n");  
			irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);											
	
		}
	}
	return IRQ_HANDLED;  
}

////////////////////////////////////////////////////////////////////////////////////////////////

/*CONFIG*/

///////////////////////////////////////////////////////////////////////////////////////////////
void r_int_config(int int_gpio, void* handle) 
{	
	int return_val,irq_any_gpio;
	phcsr_dev_str pp = (phcsr_dev_str)handle;
	printk("Entered r_int_config");
	if ( (irq_any_gpio = gpio_to_irq(int_gpio)) < 0 ) 
	{
		printk("GPIO to IRQ mapping failure %s\n",GPIO_INT_NAME );
      		return;
   	}
   	printk(KERN_NOTICE "Mapped int %d\n", irq_any_gpio);
	return_val = request_irq(irq_any_gpio,(irq_handler_t ) r_irq_handler, IRQF_TRIGGER_RISING , GPIO_INT_NAME, (void*)pp);
	printk("return value of r_int_config is %d", return_val);
	if (return_val != 0)
   	{
     		printk("Irq Request failure\n");
		return;
   	}
   	return;
}

/* HCSR_WRITE function*/
ssize_t hcsr_write(struct file *file, const char *buf,
	size_t count, loff_t *ppos)
{
	phcsr_dev_str pphcsr_dev_str_obj= file->private_data;
	
	//gpio_request(pphcsr_dev_str_obj->trigger,"sysfs");

	//gpio_direction_output(pphcsr_dev_str_obj->trigger,0);
	printk("TRIGGER PIN: %d\n",pphcsr_dev_str_obj->trigger);
	
	//gpio_request(pphcsr_dev_str_obj->echo,"sysfs");
	//gpio_direction_input(pphcsr_dev_str_obj->echo);
	printk("ECHO PIN: %d\n",pphcsr_dev_str_obj->echo);
        
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 0);
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 1);
	udelay(10);
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 0);
    mdelay(200);
	r_int_config(pphcsr_dev_str_obj->echo, (void*)pphcsr_dev_str_obj);
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 0);
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 1);
	udelay(10);
	gpio_set_value_cansleep(pphcsr_dev_str_obj->trigger, 0);
	
	printk("End of driver write module\n");

        return 0;
}

static int hcsr_close(struct inode *inodep, struct file *file)
{
	//int ret;
	phcsr_dev_str pshcsr_dev_obj = file->private_data;
	printk("Entered the close module");
	
	//ret =r_int_release((void*)pshcsr_dev_obj);
	//if(ret !=0)
		//printk("failed to free interrupt\n");
	gpio_set_value_cansleep(pshcsr_dev_obj->trigger, 0);
	gpio_set_value_cansleep(pshcsr_dev_obj->echo, 0);
	gpio_free(pshcsr_dev_obj->echo); 
    gpio_free(pshcsr_dev_obj->trigger); 
	gpio_set_value_cansleep(pshcsr_dev_obj->samples, 0);
	gpio_set_value_cansleep(pshcsr_dev_obj->time, 0);
	gpio_free(pshcsr_dev_obj->samples); 
    gpio_free(pshcsr_dev_obj->time); 
	printk("freed\n");
	return 0; 
	pr_info("Device closed\n");
   	return 0;
}

static const struct file_operations hcsr_fops = {
    .owner			= THIS_MODULE,
    //.read                       = hcsr_read,
    .write			= hcsr_write,
    .open			= hcsr_open,
    .release		        = hcsr_close,
    //.unlocked_ioctl = hcsr_ioctl,
    //.llseek 		       = no_llseek,
};

/*Initialise the device*/
phcsr_dev_str init_device(void* Pchip_handle)
{
	int ret;
	//struct HCSR_chip *hcsrchip;
	struct HCSR_chip *hcsrchip = (struct HCSR_chip*)Pchip_handle;	
	phcsr_dev_str phcsr_dev_str_obj = kmalloc(sizeof(struct HCSR_DEV_OBJ), GFP_KERNEL);
	phcsr_dev_str_obj->phcsrchip = hcsrchip;
	memset(phcsr_dev_str_obj, 0, sizeof (struct HCSR_DEV_OBJ));

	phcsr_dev_str_obj->dev_misc.minor = MISC_DYNAMIC_MINOR,
	phcsr_dev_str_obj->dev_misc.name = "HCSR_1",
	phcsr_dev_str_obj->dev_misc.fops = &hcsr_fops;
	ret = misc_register(&phcsr_dev_str_obj->dev_misc);
	phcsr_dev_str_obj->pshcsr_buffer_obj = kmalloc(sizeof(struct shcsr_buffer), GFP_KERNEL);
	phcsr_dev_str_obj->pshcsr_buffer_obj->head = 0;
	phcsr_dev_str_obj->pshcsr_buffer_obj->tail = 0;
	
    pr_info("Successfully REGISTERED misc driver HCSR\n");	
	if (hcsrchip->dev_no ==1)
	{
		phcsr_dev_str_obj->dev_name = DEVICE_NAME1;
		printk("Device name is %s\n",phcsr_dev_str_obj->dev_name);
		phcsr_dev_str_obj->dev_misc.name = DEVICE_NAME1;
		printk("\nDevice name is %s\n",phcsr_dev_str_obj->dev_misc.name);
	}
/*	
	if (pdevice->dev_no ==2)
	{
		phcsr_dev_str_obj->dev_name = DEVICE_NAME2;
		phcsr_dev_str_obj->dev_misc.name = DEVICE_NAME2;
	}
*/		
	printk("inside probe  minor = %d", phcsr_dev_str_obj->dev_misc.minor);

    if (!(phcsr_dev_str_obj->pshcsr_buffer_obj->pdata = kmalloc(sizeof(hcsr_measure_t)* SIZE_OF_BUF, GFP_KERNEL)))
    {
        printk("Bad Kmalloc\n");            
    }
	printk("inside probe  minor = %d", phcsr_dev_str_obj->dev_misc.minor);
	printk("endof init\n");
	return phcsr_dev_str_obj;
	
}
//////////////////////////////////////////////////////////////////////////
/*Probe function*/
static int P_driver_probe(struct platform_device *dev_found)
{
	int rval, ret;	
	
	struct HCSR_chip *hcsrchip;
	phcsr_dev_str phcsr_dev_str_obj = kmalloc(sizeof(struct HCSR_DEV_OBJ), GFP_KERNEL);
	//struct PHCSR_chip hcsrchip;

	hcsrchip = container_of(dev_found, struct HCSR_chip, plf_dev);
	printk("Entered the probe function\n");
	printk(KERN_ALERT "Found the device -- %s  %d \n", hcsrchip->name, hcsrchip->dev_no);
	
	hcsr_class = class_create(THIS_MODULE, CLASS_NAME);
	printk( "Created a class name %s\n", CLASS_NAME);
    if (IS_ERR(hcsr_class))
	{
                printk( " cant create class %s\n", CLASS_NAME);
                goto class_err;

    }
	if(!(phcsr_dev_str_obj = init_device((void *)hcsrchip)))
	{
		printk("device initialisation failed\n");
	}
	
	printk( "Returned after initialization\n");
	INIT_LIST_HEAD(&phcsr_dev_str_obj->device_entry);
	printk( "About to create a device\n");
	list_add(&phcsr_dev_str_obj->device_entry, &device_list );
	printk( "About to create a device after adding things in the list\n");
	/* device */
	//printk("");
	printk( "Device to be created is named as %s \n",phcsr_dev_str_obj->dev_name); 
        hcsr_device = device_create(hcsr_class, NULL, hcsr_dev, phcsr_dev_str_obj, phcsr_dev_str_obj->dev_name);
	printk( "Created a device named %s \n",phcsr_dev_str_obj->dev_name);        
	if (IS_ERR(hcsr_device)) {
                
                printk( " cant create device %s\n", phcsr_dev_str_obj->dev_name);
                goto device_err;
        }

	printk("device create\n");	
	ret = device_create_file(hcsr_device, &dev_attr_trigger);
        if (ret < 0) {
                printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_trigger.attr.name);
        }
	
	ret = device_create_file(hcsr_device, &dev_attr_echo);
        if (ret < 0) {
               printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_echo.attr.name);
        }

	rval = device_create_file(hcsr_device, &dev_attr_samples);
        if (rval < 0) {
               printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_samples.attr.name);
        }

	rval = device_create_file(hcsr_device, &dev_attr_sampling_period);
        if (rval < 0) {
       printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_sampling_period.attr.name);
        }
/*
	rval = device_create_file(hcsr_device, &dev_attr_Enable);
        if (rval < 0) {
      printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_Enable.attr.name);
        }
*/
        rval = device_create_file(hcsr_device, &dev_attr_distance);
        if (rval < 0) {
   	 printk(" cant create device attribute %s %s\n", 
                       phcsr_dev_str_obj->dev_name, dev_attr_distance.attr.name);
        }
printk("probe done\n");

	return 0;
class_err:
        class_unregister(hcsr_class);
        class_destroy(hcsr_class);
device_err:
        device_destroy(hcsr_class, hcsr_dev);
		return -EFAULT;			
};


static int P_driver_remove(struct platform_device *pdev)
{
	phcsr_dev_str ptempcontext;	
	printk("enter remove of the probe\n");

	list_for_each_entry(ptempcontext, &device_list, device_entry)
	{
	    printk("remove a list\n");
		if(ptempcontext->dev_name == pdev->name)
		{
		    //--usDeviceNo;
		list_del(&ptempcontext->device_entry);
	    device_destroy(hcsr_class, ptempcontext->dev_misc.minor);
		kfree(ptempcontext->pshcsr_buffer_obj->pdata);
   	    printk("freed pdata\n");
	    kfree(ptempcontext->pshcsr_buffer_obj);
   	    printk("freed buf\n");
	    misc_deregister(&ptempcontext->dev_misc);
   	    printk("freed misc\n");
	    kfree(ptempcontext);
   	    printk("freed obj\n");
			//break;
		}
	}
	return 0;
};

static struct platform_driver P_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= P_driver_probe,
	.remove		= P_driver_remove,
	.id_table	= P_id_table,
};

module_platform_driver(P_driver);
MODULE_LICENSE("GPL v2");

