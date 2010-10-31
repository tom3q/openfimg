/*
 * fimg/system.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE SYSTEM-DEVICE INTERFACE
 *
 * Copyrights:	2010 by Tomasz Figa < tomasz.figa at gmail.com >
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <cutils/log.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/android_pmem.h>

#include "fimg_private.h"
#include "s3c_g3d.h"

#define FIMG_SFR_SIZE 0x80000

/*****************************************************************************
 * FUNCTION:	fimgDeviceOpen
 * SYNOPSIS:	This function opens and maps the G3D device.
 * RETURNS:	 0, on success
 *		-errno, on error
 *****************************************************************************/
int fimgDeviceOpen(fimgContext *ctx)
{
	ctx->fd = open("/dev/s3c-g3d", O_RDWR | O_SYNC, 0);
	if(ctx->fd < 0) {
		LOGE("Couldn't open /dev/s3c-g3d (%s).", strerror(errno));
		return -errno;
	}

	ctx->base = mmap(NULL, FIMG_SFR_SIZE, PROT_WRITE | PROT_READ,
					MAP_SHARED, ctx->fd, 0);
	if(ctx->base == MAP_FAILED) {
		LOGE("Couldn't mmap FIMG registers (%s).", strerror(errno));
		close(ctx->fd);
		return -errno;
	}

	LOGD("Opened /dev/s3c-g3d (%d).", ctx->fd);

	return 0;
}

/*****************************************************************************
 * FUNCTION:	fimgDeviceClose
 * SYNOPSIS:	This function closes the 3D device
 *****************************************************************************/
void fimgDeviceClose(fimgContext *ctx)
{
	munmap((void *)ctx->base, FIMG_SFR_SIZE);
	close(ctx->fd);

	LOGD("fimg3D: Closed /dev/s3c-g3d (%d).", ctx->fd);
}

/**
	Context management
*/

/*****************************************************************************
 * FUNCTION:	fimgCreateContext
 * SYNOPSIS:	This function creates a device context
 * RETURNS:	created context or NULL on error
 *****************************************************************************/
fimgContext *fimgCreateContext(void)
{
	fimgContext *ctx;
	uint32_t *queue;

	if ((ctx = malloc(sizeof(*ctx))) == NULL)
		return NULL;

	if ((queue = malloc(2*FIMG_MAX_QUEUE_LEN*sizeof(uint32_t))) == NULL) {
		free(ctx);
		return NULL;
	}

	memset(ctx, 0, sizeof(fimgContext));

	if(fimgDeviceOpen(ctx)) {
		free(queue);
		free(ctx);
		return NULL;
	}

	fimgCreateGlobalContext(ctx);
	fimgCreateHostContext(ctx);
	fimgCreatePrimitiveContext(ctx);
	fimgCreateRasterizerContext(ctx);
	fimgCreateFragmentContext(ctx);
#ifdef FIMG_FIXED_PIPELINE
	fimgCreateCompatContext(ctx);
#endif

	ctx->queue = queue;
	ctx->queue[0] = 0;
	ctx->queueStart = queue;

	return ctx;
}

/*****************************************************************************
 * FUNCTION:	fimgDestroyContext
 * SYNOPSIS:	This function destroys a device context
 *****************************************************************************/
void fimgDestroyContext(fimgContext *ctx)
{
	fimgDeviceClose(ctx);
	free(ctx->queueStart);
	free(ctx);
}

/*****************************************************************************
 * FUNCTION:	fimgRestoreContext
 * SYNOPSIS:	This function restores a device context to hardware registers
 *****************************************************************************/
void fimgRestoreContext(fimgContext *ctx)
{
//	fprintf(stderr, "fimg: Restoring global state\n"); fflush(stderr);
	fimgRestoreGlobalState(ctx);
//	fprintf(stderr, "fimg: Restoring host state\n"); fflush(stderr);
	fimgRestoreHostState(ctx);
//	fprintf(stderr, "fimg: Restoring primitive state\n"); fflush(stderr);
	fimgRestorePrimitiveState(ctx);
//	fprintf(stderr, "fimg: Restoring rasterizer state\n"); fflush(stderr);
	fimgRestoreRasterizerState(ctx);
//	fprintf(stderr, "fimg: Restoring fragment state\n"); fflush(stderr);
	fimgRestoreFragmentState(ctx);
#ifdef FIMG_FIXED_PIPELINE
//	fprintf(stderr, "fimg: Restoring compat state\n"); fflush(stderr);
	fimgRestoreCompatState(ctx);
#endif

	ctx->queue = ctx->queueStart;
	ctx->queue[0] = 0;
	ctx->queueLen = 0;
}

/**
	Power management
*/

/*****************************************************************************
 * FUNCTION:	fimgAcquireHardwareLock
 * SYNOPSIS:	This function claims the hardware for exclusive use
 * RETURNS:	0 on success,
 *		positive value if the context needs to be restored,
 *		negative value on error
 *****************************************************************************/
int fimgAcquireHardwareLock(fimgContext *ctx)
{
	int ret;

	if((ret = ioctl(ctx->fd, S3C_G3D_LOCK, 0)) < 0) {
		LOGE("Could not acquire the hardware lock");
		return -1;
	}

	return ret;
}

/*****************************************************************************
 * FUNCTION:	fimgReleaseHardwareLock
 * SYNOPSIS:	This function ends exclusive use of the hardware
 * RETURNS:	0 on success,
 *		negative value on error
 *****************************************************************************/
int fimgReleaseHardwareLock(fimgContext *ctx)
{
	if(ioctl(ctx->fd, S3C_G3D_UNLOCK, 0)) {
		LOGE("Could not release the hardware lock");
		return -1;
	}

	return 0;
}

/*****************************************************************************
 * FUNCTION:	fimgWaitForFlush
 * SYNOPSIS:	This function waits for the hardware to flush the pipeline
 * RETURNS:	0 on success,
 *		negative value on error
 * ARGUMENTS:	target - requested pipeline stages to be flushed
 *****************************************************************************/
int fimgWaitForFlush(fimgContext *ctx, uint32_t target)
{
	if(ioctl(ctx->fd, S3C_G3D_FLUSH, target)) {
		LOGE("Could not flush the hardware pipeline");
		return -1;
	}

	return 0;
}

