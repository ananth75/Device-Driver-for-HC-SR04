/*
 * A sample program to show the binding of platform driver and device.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "device_platform_hcsr.h"


static struct HCSR_chip HCSR1_chip = {
		.name	= "DEVICE_NAME",
		.dev_no 	= 1,
		.plf_dev = {
			.name	= "DEVICE_NAME",
			.id	= -1,
		}
};
/*
static struct HCSR_chip HCSR2_chip = {
		.name	= "HCSR_2",
		.dev_no 	= 2,
		.plf_dev = {
			.name	= "HCSR_2",
			.id	= -1,
		}
};
*/

/**
 * register the device when module is initiated
 */

static int p_device_init(void)
{
	int ret = 0;
	
	

	/* Register the device */
	platform_device_register(&HCSR1_chip.plf_dev);
	
	printk(KERN_ALERT "Platform device 1 is registered in init \n");

	//platform_device_register(&HCSR2_chip.plf_dev);

	//printk(KERN_ALERT "Platform device 2 is registered in init \n");
	
	return ret;
}

static void p_device_exit(void)
{
    	platform_device_unregister(&HCSR1_chip.plf_dev);

	//platform_device_unregister(&HCSR2_chip.plf_dev);

	printk(KERN_ALERT "Goodbye, unregister the device\n");
}

module_init(p_device_init);
module_exit(p_device_exit);
MODULE_LICENSE("GPL");

