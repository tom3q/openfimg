/*
 * fimg/global.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE GLOBAL BLOCK RELATED FUNCTIONS
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fimg_private.h"
#include <unistd.h>

/*
 * Global hardware
 */

#define FGGB_PIPESTATE		0x0000
#define FGGB_CACHECTL		0x0004
#define FGGB_RST		0x0008
#define FGGB_VERSION		0x0010
#define FGGB_INTPENDING		0x0040
#define FGGB_INTMASK		0x0044
#define FGGB_PIPEMASK		0x0048
#define FGGB_PIPETGTSTATE	0x004c
#define FGGB_PIPEINTSTATE	0x0050

typedef union {
	unsigned int val;
	struct {
		unsigned zcflush	:2;
		unsigned		:2;
		unsigned ccflush	:2;
		unsigned		:2;
		unsigned tcclear	:2;
		unsigned		:2;
		unsigned vtcclear	:1;
		unsigned		:19;
	};
} fimgCacheCtl;

/**
 * Obtains status of graphics pipeline.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @return Bit mask with set bits corresponding to busy parts of pipeline.
 */
uint32_t fimgGetPipelineStatus(fimgContext *ctx)
{
	return fimgRead(ctx, FGGB_PIPESTATE);
}

/**
 * Flushes complete graphics pipeline.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @return 0 on success, negative on timeout.
 */
int fimgFlush(fimgContext *ctx)
{
	/* Return if already flushed */
	if((fimgRead(ctx, FGGB_PIPESTATE) & FGHI_PIPELINE_ALL) == 0)
		return 0;

	/* Flush whole pipeline */
	return fimgWaitForFlush(ctx, FGHI_PIPELINE_ALL);
}

/**
 * Flushes selected parts of graphics pipeline.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @param mask Mask of pipeline parts to be flushed.
 * @return 0 on success, negative on error.
 */
int fimgSelectiveFlush(fimgContext *ctx, uint32_t mask)
{
	if (fimgRead(ctx, FGGB_PIPESTATE) & mask)
		return fimgWaitForFlush(ctx, mask);

	return 0;
}

/**
 * Invalidates selected texture caches.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @param vtcclear Vertex texture cache mask.
 * @param tcclear Texture cache mask.
 * @return 0 on success, negative on error.
 *
 */
int fimgInvalidateCache(fimgContext *ctx,
				unsigned int vtcclear, unsigned int tcclear)
{
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.vtcclear = vtcclear;
	ctl.tcclear = tcclear;

	fimgWrite(ctx, ctl.val, FGGB_CACHECTL); // start clearing the cache

	while(fimgRead(ctx, FGGB_CACHECTL) & ctl.val);

	return 0;
}

/**
 * Initiates a flush of selected fragment caches.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @param ccflush Color cache mask.
 * @param zcflush Z cache mask.
 * @return 0 on success, negative on error.
 */
int fimgFlushCache(fimgContext *ctx, unsigned int ccflush, unsigned int zcflush)
{
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.ccflush = ccflush;
	ctl.zcflush = zcflush;

	fimgWrite(ctx, ctl.val, FGGB_CACHECTL); // start clearing the cache

	return 0;
}

/**
 * Waits for selected fragment caches to flush.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @param ccflush Color cache mask.
 * @param zcflush Z cache mask.
 * @return 0 on success, negative on error.
 */
int fimgWaitForCacheFlush(fimgContext *ctx,
				unsigned int ccflush, unsigned int zcflush)
{
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.ccflush = ccflush;
	ctl.zcflush = zcflush;

	while(fimgRead(ctx, FGGB_CACHECTL) & ctl.val);

	return 0;
}

/**
 * Makes sure that any rendering that is in progress finishes and any data
 * in caches is saved to buffers in memory.
 * @param ctx Hardware context.
 */
void fimgFinish(fimgContext *ctx)
{
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushCache(ctx, 3, 3);
	fimgSelectiveFlush(ctx, FGHI_PIPELINE_CCACHE);
	fimgWaitForCacheFlush(ctx, 3, 3);
	fimgPutHardware(ctx);
}

/**
 * Resets graphics pipeline without affecting register values.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 */
void fimgSoftReset(fimgContext *ctx)
{
	fimgWrite(ctx, 1, FGGB_RST);
	usleep(1);
	fimgWrite(ctx, 0, FGGB_RST);
}

/**
 * Gets FIMG-3DSE version.
 * (Must be called with hardware lock.)
 * @param ctx Hardware context.
 * @param major Points where to store major version.
 * @param minor Points where to store minor version.
 * @param rev Points where to store revision number.
 */
void fimgGetVersion(fimgContext *ctx, int *major, int *minor, int *rev)
{
	fimgVersion version;

	version.val = fimgRead(ctx, FGGB_VERSION);

	if (major)
		*major = version.major;

	if (minor)
		*minor = version.minor;

	if (rev)
		*rev = version.revision;
}

/**
 * Initializes hardware context for global block.
 * @param ctx Hardware context.
 */
void fimgCreateGlobalContext(fimgContext *ctx)
{
	// Nothing to initialize
}

/**
 * Restores hardware context of global block.
 * @param ctx Hardware context.
 */
void fimgRestoreGlobalState(fimgContext *ctx)
{
	// Nothing to restore
}
