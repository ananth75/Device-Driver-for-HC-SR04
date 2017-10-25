#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h> 
#define GPIO_INT_NAME "gpio_int"
#define CLASS_NAME "HCSR"
#define DRIVER_NAME "HCSR_of_driver"
#define DEVICE_NAME1 "HCSR_1"
#define DEVICE_NAME2 "HCSR_2"
#define MAX_IO_COUNT 14

#ifndef __SAMPLE_PLATFORM_H__
#define __SAMPLE_PLATFORM_H__

struct HCSR_chip {
		char 	*name;
		int		dev_no;
		struct platform_device 	plf_dev;
}*PHCSR_chip;

/*Measurement*/
typedef struct {
	int distance;	
	uint64_t TSC;
}hcsr_measure_t;

/*FIFO buffer structure*/
typedef struct shcsr_buffer {
     hcsr_measure_t *pdata;
     int size;     
     int head;
     int tail;
} pshcsr_buffer;

/*per device structure*/
struct HCSR_DEV_OBJ 
{
	struct miscdevice dev_misc;
	char *dev_name;             							
	int state;
	int samples;
	int time;
	pshcsr_buffer *pshcsr_buffer_obj;
	int dev_num;
	int thread_state;
	int trigger;
	int echo;
	spinlock_t m_Lock;
	int ongoing;
	int distance[100];
	int index;
	//int enable;
	struct HCSR_chip *phcsrchip;
	struct list_head device_entry;
	struct task_struct *task;
	struct HCSR_DEV_OBJ *next;
	uint64_t tsc1 ;
	uint64_t tsc2 ;
} *PPSHCSR_DEV_OBJ;

//typedef struct HCSR_DEV_OBJ SHCSR_DEV_OBJ;
typedef struct HCSR_DEV_OBJ* phcsr_dev_str;
typedef struct HCSR_DEV_OBJ hcsr_dev_str;
#endif /* __GPIO_FUNC_H__ */


