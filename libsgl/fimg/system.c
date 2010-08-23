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
//static unsigned int refCount = 0;

#define FIMG_SFR_BASE 0x72000000
#define FIMG_SFR_SIZE 0x80000

/*****************************************************************************
 * FUNCTION:	fimgDeviceOpen
 * SYNOPSIS:	This function opens the 3D device and initializes global variables
 * RETURNS:	 0, on success
 *		-errno, on error
 *****************************************************************************/
int fimgDeviceOpen(void)
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

//	fimgEnterCriticalSection();

	LOGD("fimg3D: Opened /dev/s3c-g3d (%d) and /dev/mem (%d).", fimgFileDesc, fimgMemFileDesc);

	return 0;
}

/*****************************************************************************
 * FUNCTION:	fimgDeviceClose
 * SYNOPSIS:	This function closes the 3D device
 *****************************************************************************/
void fimgDeviceClose(void)
{
//	fimgExitCriticalSection();

	munmap((void *)fimgBase, FIMG_SFR_SIZE);
	close(fimgFileDesc);
	close(fimgMemFileDesc);

	LOGD("fimg3D: Closed /dev/s3c-g3d (%d) and /dev/mem (%d).", fimgFileDesc, fimgMemFileDesc);
}

/**
	Memory management
*/

#define S3C_3D_MEM_ALLOC	_IOWR('S', 310, struct s3c_3d_mem_alloc)
#define S3C_3D_MEM_FREE		_IOWR('S', 311, struct s3c_3d_mem_alloc)

struct s3c_3d_mem_alloc {
        int             size;
        unsigned int    vir_addr;
        unsigned int    phy_addr;
};

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

	if(mem.vir_addr)
		LOGD("Allocated %d bytes of memory. (0x%08x @ 0x%08x)", mem.size, mem.vir_addr, mem.phy_addr);
	else
		LOGE("Memory allocation of %d bytes failed", mem.size);

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

	if(!vaddr)
		return;

	mem.vir_addr = (unsigned int)vaddr;
	mem.phy_addr = paddr;
	mem.size = size;

	LOGD("Freed %d bytes of memory. (0x%08x @ 0x%08x)", mem.size, mem.vir_addr, mem.phy_addr);

	ioctl(fimgFileDesc, S3C_3D_MEM_FREE, &mem);
}

#define S3C_3D_CACHE_INVALID		_IOWR('S', 316, struct s3c_3d_mem_alloc)
#define S3C_3D_CACHE_CLEAN		_IOWR('S', 317, struct s3c_3d_mem_alloc)
#define S3C_3D_CACHE_CLEAN_INVALID	_IOWR('S', 318, struct s3c_3d_mem_alloc)
void fimgFlushBufferCache(void *vaddr, unsigned long size)
{
	struct s3c_3d_mem_alloc mem;

	if(!vaddr)
		return;

	mem.vir_addr = (unsigned int)vaddr;
	mem.size = size;

	LOGD("Flushed %d bytes of memory @ 0x%08x.)", mem.size, mem.vir_addr);

	ioctl(fimgFileDesc, S3C_3D_CACHE_INVALID, &mem);
}

void fimgClearBufferCache(void *vaddr, unsigned long size)
{
	struct s3c_3d_mem_alloc mem;

	if(!vaddr)
		return;

	mem.vir_addr = (unsigned int)vaddr;
	mem.size = size;

	LOGD("Invalidated %d bytes of memory @ 0x%08x.)", mem.size, mem.vir_addr);

	ioctl(fimgFileDesc, S3C_3D_CACHE_CLEAN, &mem);
}

void fimgClearFlushBufferCache(void *vaddr, unsigned long size)
{
	struct s3c_3d_mem_alloc mem;

	if(!vaddr)
		return;

	mem.vir_addr = (unsigned int)vaddr;
	mem.size = size;

	LOGD("Flushed and invalidated %d bytes of memory @ 0x%08x.)", mem.size, mem.vir_addr);

	ioctl(fimgFileDesc, S3C_3D_CACHE_CLEAN_INVALID, &mem);
}

/**
	Context management
*/

fimgContext *fimgCreateContext(void)
{
	fimgContext *ctx;
	int i;

	if((ctx = malloc(sizeof(*ctx))) == NULL)
		return NULL;

	memset(ctx, 0, sizeof(fimgContext));

	ctx->base = fimgBase;

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

/**
	Power management
*/

struct s3c_3d_pm_status
{
	unsigned int criticalSection;
	int powerStatus;
	int reserved;
//	int memStatus;
};

#define LOCK_CRITICAL_SECTION		1
#define UNLOCK_CRITICAL_SECTION		0
#define S3C_3D_CRITICAL_SECTION		_IOWR('S', 322, struct s3c_3d_pm_status)

int fimgEnterCriticalSection(void)
{
	struct s3c_3d_pm_status status_desc;

	status_desc.criticalSection = LOCK_CRITICAL_SECTION;
	status_desc.powerStatus = 0;
	status_desc.reserved = 0;

	if(ioctl(fimgFileDesc, S3C_3D_CRITICAL_SECTION, &status_desc)) {
		LOGE("Could not enter 3D critical section");
		return -1;
	}

	return 0;
}

int fimgExitCriticalSection(void)
{
	struct s3c_3d_pm_status status_desc;

	status_desc.criticalSection = UNLOCK_CRITICAL_SECTION;
	status_desc.powerStatus = 0;
	status_desc.reserved = 0;

	if(ioctl(fimgFileDesc, S3C_3D_CRITICAL_SECTION, &status_desc)) {
		LOGE("Could not exit 3D critical section");
		return -1;
	}

	return 0;
}

/* TODO: Implement rest of kernel driver functions */
