/*
 * fimg/system.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE SYSTEM-DEVICE INTERFACE
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#include "fimg_private.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/log.h>

static int fimgFileDesc = -1;
static int fimgMemFileDesc = -1;
static volatile void *fimgBase = NULL;
static unsigned int refCount = 0;

#define FIMG_SFR_BASE 0x72000000
#define FIMG_SFR_SIZE 0x80000

/*****************************************************************************/
#define G3D_IOCTL_MAGIC		'S'
#define S3C_3D_MEM_ALLOC	_IOWR(G3D_IOCTL_MAGIC, 310, struct s3c_3d_mem_alloc)
#define S3C_3D_MEM_FREE		_IOWR(G3D_IOCTL_MAGIC, 311, struct s3c_3d_mem_alloc)

struct s3c_3d_mem_alloc {
        int             size;
        unsigned int    vir_addr;
        unsigned int    phy_addr;
};
/*****************************************************************************/

/*****************************************************************************
 * FUNCTION:	fimgDeviceOpen
 * SYNOPSIS:	This function opens the 3D device and initializes global variables
 * RETURNS:	 0, on success
 *		-errno, on error
 *****************************************************************************/
static int fimgDeviceOpen(void)
{
	fimgFileDesc = open("/dev/s3c-g3d", O_RDWR, 0);
	if(fimgFileDesc < 0) {
		LOGE("Couldn't open /dev/s3c-g3d (%s).", strerror(errno));
		return -errno;
	}

	fimgMemFileDesc = open("/dev/mem", O_RDWR | O_SYNC, 0);
	if(fimgMemFileDesc < 0) {
		LOGE("Couldn't open /dev/mem (%s).", strerror(errno));
		close(fimgFileDesc);
		return -errno;
	}

	fimgBase = mmap(NULL, FIMG_SFR_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fimgMemFileDesc, FIMG_SFR_BASE);
	if(fimgBase == MAP_FAILED) {
		LOGE("Couldn't mmap FIMG registers (%s).", strerror(errno));
		close(fimgFileDesc);
		close(fimgMemFileDesc);
		return -errno;
	}

	LOGD("fimg3D: Opened /dev/s3c-g3d (%d) and /dev/mem (%d).", fimgFileDesc, fimgMemFileDesc);

	return 0;
}

/*****************************************************************************
 * FUNCTION:	fimgDeviceClose
 * SYNOPSIS:	This function closes the 3D device
 *****************************************************************************/
static void fimgDeviceClose(void)
{
	munmap((void *)fimgBase, FIMG_SFR_SIZE);
	close(fimgFileDesc);
	close(fimgMemFileDesc);

	LOGD("fimg3D: Closed /dev/s3c-g3d (%d) and /dev/mem (%d).", fimgFileDesc, fimgMemFileDesc);
}

/*****************************************************************************
 * FUNCTION:	fimgAllocMemory
 * SYNOPSIS:	This function allocates a block of 3D memory
 * PARAMETERS:	[in]  size - requested block size
 * 		[out] paddr - physical address
 * RETURNS:	virtual address of allocated block
 *****************************************************************************/
void *fimgAllocMemory(unsigned long *size, unsigned long *paddr)
{
	struct s3c_3d_mem_alloc mem;

	mem.size = *size;
	
	ioctl(fimgFileDesc, S3C_3D_MEM_ALLOC, &mem);

	LOGD("Allocated %d bytes of memory. (0x%08x @ 0x%08x)", mem.size, mem.vir_addr, mem.phy_addr);

	*paddr = mem.phy_addr;
	*size = mem.size;
	return (void *)mem.vir_addr;
}

/*****************************************************************************
 * FUNCTION:	fimgFreeMemory
 * SYNOPSIS:	This function frees allocated 3D memory block
 * PARAMETERS:	[in] vaddr - virtual address
 *		[in] paddr - physical address
 *		[in] size - size
 *****************************************************************************/
void fimgFreeMemory(void *vaddr, unsigned long paddr, unsigned long size)
{
	struct s3c_3d_mem_alloc mem;

	mem.vir_addr = (unsigned int)vaddr;
	mem.phy_addr = paddr;
	mem.size = size;

	LOGD("Freed %d bytes of memory. (0x%08x @ 0x%08x)", mem.size, mem.vir_addr, mem.phy_addr);

	ioctl(fimgFileDesc, S3C_3D_MEM_FREE, &mem);
}

fimgContext *fimgCreateContext(void)
{
	fimgContext *ctx;
	int i;

	if((ctx = malloc(sizeof(*ctx))) == NULL)
		return NULL;

	memset(ctx, 0, sizeof(fimgContext));

	if(!refCount && fimgDeviceOpen()) {
		free(ctx);
		return NULL;
	}

	ctx->base = fimgBase;
	refCount++;

	fimgCreateGlobalContext(ctx);
	fimgCreateHostContext(ctx);
	fimgCreatePrimitiveContext(ctx);
	fimgCreateRasterizerContext(ctx);
	fimgCreateFragmentContext(ctx);

	ctx->numAttribs = 0;

	return ctx;
}

void fimgDestroyContext(fimgContext *ctx)
{
	refCount--;

	if(!refCount)
		fimgDeviceClose();

	free(ctx);
}

void fimgRestoreContext(fimgContext *ctx)
{
	fprintf(stderr, "fimg: Restoring global state\n"); fflush(stderr);
	fimgRestoreGlobalState(ctx);
	fprintf(stderr, "fimg: Restoring host state\n"); fflush(stderr);
	fimgRestoreHostState(ctx);
	fprintf(stderr, "fimg: Restoring primitive state\n"); fflush(stderr);
	fimgRestorePrimitiveState(ctx);
	fprintf(stderr, "fimg: Restoring rasterizer state\n"); fflush(stderr);
	fimgRestoreRasterizerState(ctx);
	fprintf(stderr, "fimg: Restoring fragment state\n"); fflush(stderr);
	fimgRestoreFragmentState(ctx);
}

/* TODO: Implement rest of kernel driver functions */
