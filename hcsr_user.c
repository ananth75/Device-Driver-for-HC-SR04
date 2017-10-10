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
	int fd,num_of_drivers,d,delta,i=0,trigger[50],echo[50],m;
	char file_name[50],yes_no;
	//printf("Enter the number of drivers you've installed using insmod\n");
	//scanf("%d",&num_of_drivers);
	/*printf("Enter the Value of m\n");
	scanf("%d",&m);
	printf("Enter the Value of delta\n");
	scanf("%d",&delta);
	SIOCTL_PINCONG pinconf;
	while(1)
	{
		printf("Enter the Trigger Pin for HCSR_%d driver",i);
		scanf("%d",&trigger[i]);
		printf("Enter the Echo Pin for HCSR_%d driver",i);
		scanf("%d",&echo[i]);
	        sprintf(file_name, "/dev/HCSR_%d", i);
		fd[i] = open(file_name,O_WRONLY);
		printf("Do you have another driver to enter Trigger and echo pins? y/n \n");	
		scanf("%c",&yes_no);
		if(yes_no == 'y')
			i++;
		else if(yes_no == 'n')
			break;
		
	} */


	fd = open("/dev/HCSR_0",O_WRONLY); 
	if (fd < 0 ){
		printf("Can not open device  hcsr0 file\n");		
		return 0;
	}
	else{
		printf("hcsr1 device is opened\n");
	}
	hcsr_set_pins(fd, 0, 1);
	write(fd,&m,sizeof(int) );
//	read(fd,&pinconf,sizeof(SIOCTL_PINCONG));
	return 0;
}
