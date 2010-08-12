/*
 * fimg/system.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE SYSTEM-DEVICE INTERFACE
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#include "common.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/ioctl.h>
#include <sys/mman.h>
#else
#define ioctl(args...)	(0)
#define mmap(args...)	((void *)0)
#define MAP_FAILED	0
#define munmap(args...)
#endif
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int fimgFileDesc = -1;
volatile void *fimgBase = NULL;

#define FIMG_SFR_SIZE 0x80000

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgDeviceOpen
 * SYNOPSIS:	This function opens the 3D device and initializes global variables
 * RETURNS:	 0, on success
 *		-errno, on error
 *****************************************************************************/
int fimgDeviceOpen(void)
{
	int fd;

	fd = open("/dev/s3c-g3d", O_RDWR, 0);
	if(fd < 0) {
		printf("fimg3D error: Couldn't open /dev/s3c-g3d (%s).\n", strerror(errno));
		return -errno;
	}

	fimgBase = mmap(NULL, FIMG_SFR_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if(fimgBase == MAP_FAILED) {
		printf("fimg3D error: Couldn't mmap /dev/s3c-g3d (%s).\n", strerror(errno));
		close(fd);
		return -errno;
	}

	printf("fimg3D: Opened /dev/s3c-g3d (%d).\n", fd);
	fimgFileDesc = fd;
	return 0;
}

/*****************************************************************************
 * FUNCTIONS:	fimgDeviceClose
 * SYNOPSIS:	This function closes the 3D device
 *****************************************************************************/
void fimgDeviceClose(void)
{
	if(fimgFileDesc < 0) {
		printf("fimg3D warning: Trying to close already closed device.\n");
		return;
	}

	munmap(fimgBase, FIMG_SFR_SIZE);
	close(fimgFileDesc);

	printf("fimg3D: Closed /dev/s3c-g3d (%d).", fimgFileDesc);

	fimgFileDesc = -1;
}

/* TODO: Implement rest of kernel driver functions */
