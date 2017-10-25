#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
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


#ifndef __SAMPLE_PLATFORM_H__
#define __SAMPLE_PLATFORM_H__

struct HCSR_chip {
		char 			*name;
		int			dev_no;
		struct platform_device 	plf_dev;
}*PHCSR_chip;

/*Timestamp*/
typedef struct {
	uint64_t TSC;
	int distance;
}hcsr_measure_t;

/*FIFO buffer structure*/
typedef struct fifo {
     hcsr_measure_t *ptym;
     int head;
     int tail;
     int size;
} fifo_t;

/*per device structure*/
struct HCSR_DEV_OBJ 
{
	struct miscdevice dev_misc;
	char *dev_name;             							
	int state;
	int samples;
	int time;
	int dev_num;
	struct fifo *pfifo_t;
	int thread_state;
	int trigger_pin;
	int echo_pin;
	spinlock_t my_lock;
	int ongoing;
	int distance[100];
	int index;
	//int enable;
	struct HCSR_chip *phcsrchip;
	struct list_head device_entry;
	struct task_struct *task;
	struct HCSR_DEV_OBJ *next;
} *PPSHCSR_DEV_OBJ;

//typedef struct HCSR_DEV_OBJ SHCSR_DEV_OBJ;
typedef struct HCSR_DEV_OBJ* phcsr_dev_str;
typedef struct HCSR_DEV_OBJ hcsr_dev_str;
#endif /* __GPIO_FUNC_H__ */


