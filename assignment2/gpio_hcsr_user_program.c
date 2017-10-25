#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <poll.h>
#include "hcsr_ioctl.h"


/* Structure for holding the measurement data */
typedef struct {
    int      distance;
    uint64_t timestamp;
}hcsr_measure_t;

int hcsr_set_pins(int fd, int trigger, int echo)
{
	int set_retval;
	SIOCTL_PINCONG Sioctl_pinconf;
	Sioctl_pinconf.trigger = trigger;
	Sioctl_pinconf.echo = echo;
	if((set_retval = ioctl(fd,HCSR_IOCTL_PINCONG,(unsigned long)&Sioctl_pinconf))!=0){
		printf("ioctl error %d\n", set_retval);}
	if(set_retval)
		printf("hcsr_dev_error: unable to perform hcsr_set_pins \n");
	return 0;
}

int hcsr_set_params(int fd, int m, int delta)
{
	int set_retval;
	SIOCTL_SETPARAM Sioctl_setparams;
	Sioctl_setparams.m = m;
	Sioctl_setparams.delta = delta;
	if((set_retval = ioctl(fd,HCSR_IOCTL_SETPARAM,(unsigned long)&Sioctl_setparams))!=0){
		printf("ioctl error %d\n", set_retval);}
	if(set_retval)
		printf("hcsr_dev_error: unable to perform hcsr_set_pins \n");
	return 0;
}

int main()
{
	int fd,m = 1;
	fd = open("/dev/HCSR_dev1",O_WRONLY); 
	if (fd < 0 ){
		printf("Can not open device  Hcsr_dev0 file\n");		
		return 0;
	}
	else{
		printf("hcsr_dev0 device is opened\n");
	}

	write(fd,&m,sizeof(int) );
	read(fd,&m,sizeof(int));
	return 0;
}
