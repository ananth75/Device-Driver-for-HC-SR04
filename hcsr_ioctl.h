#include <linux/ioctl.h>

#define MAJOR_NUMBER 				'10'

#define	HCSR_IOCTL_PINCONG			_IOWR(MAJOR_NUMBER, 0x01, struct _IOCTL_SETPIN)
#define HCSR_IOCTL_SETPARAM			_IOWR(MAJOR_NUMBER, 0x02, struct _IOCTL_SETMODE)


typedef struct _IOCTL_SETPIN
{
	int trigger;
	int echo;
}SIOCTL_PINCONG, *PSIOCTL_PINCONG;

typedef struct _IOCTL_SETMODE
{
	int m;
	int delta;
}SIOCTL_SETPARAM, *PSIOCTL_SETPARAM;
