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

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgGetPipelineStatus
 * SYNOPSIS:	This function obtains status of the pipeline
 * RETURNS:	fimgPipelineStatus conaining pipeline status
 *****************************************************************************/
uint32_t fimgGetPipelineStatus(fimgContext *ctx)
{
	return fimgRead(ctx, FGGB_PIPESTATE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgFlush
 * SYNOPSIS:	This function flushes the fimg3d pipeline
 * PARAMETERS:	[IN] pipelineFlags: Specified pipeline states are flushed
 * RETURNS:	 0, on success
 *		-1, on timeout
 *****************************************************************************/
int fimgFlush(fimgContext *ctx)
{
	/* Return if already flushed */
	if(fimgRead(ctx, FGGB_PIPESTATE) == 0)
		return 0;

	/* Flush whole pipeline */
	return fimgWaitForFlush(ctx, FGHI_PIPELINE_ALL);
}

int fimgSelectiveFlush(fimgContext *ctx, uint32_t mask)
{
	if (fimgRead(ctx, FGGB_PIPESTATE) & mask)
		return fimgWaitForFlush(ctx, mask);

	return 0;
}

/*****************************************************************************
 * FUNCTIONS:	fimgClearCache
 * SYNOPSIS:	This function clears the caches
 * PARAMETERS:	[IN] clearFlags: Specified caches are cleared
 * RETURNS:	 0, on success
 *		-1, on timeout
 *****************************************************************************/
int fimgInvalidateFlushCache(fimgContext *ctx,
			     unsigned int vtcclear, unsigned int tcclear,
			     unsigned int ccflush, unsigned int zcflush)
{
#if 0
	unsigned int timeout = 1000000;
#endif
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.vtcclear = vtcclear;
	ctl.tcclear = tcclear;
	ctl.ccflush = ccflush;
	ctl.zcflush = zcflush;

	fimgWrite(ctx, ctl.val, FGGB_CACHECTL); // start clearing the cache

	while(fimgRead(ctx, FGGB_CACHECTL) & ctl.val) {
#if 0
		if(--timeout == 0)
			return -1;

		usleep(1);
#endif
	}

	return 0;
}

void fimgFinish(fimgContext *ctx)
{
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgInvalidateFlushCache(ctx, 0, 0, 1, 1);
	fimgPutHardware(ctx);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSoftReset
 * SYNOPSIS:	This function resets FIMG-3DSE, but the SFR values are not affected
 *****************************************************************************/
void fimgSoftReset(fimgContext *ctx)
{
	fimgWrite(ctx, 1, FGGB_RST);
	usleep(1);
	fimgWrite(ctx, 0, FGGB_RST);
}

/*****************************************************************************
 * FUNCTIONS:	fglGetVersion
 * SYNOPSIS:	This function gets FIMG-3DSE version.
 * RETURNS:	Version of 3D hardware
 *****************************************************************************/
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

/*****************************************************************************
 * FUNCTIONS:	fimgGetInterrupt
 * SYNOPSIS:	This function returns current interrupt state.
 * RETURNS:	Non-zero if interrupt is active.
 *****************************************************************************/
unsigned int fimgGetInterrupt(fimgContext *ctx)
{
	return fimgRead(ctx, FGGB_INTPENDING) & 1;
}

/*****************************************************************************
 * FUNCTIONS:	fimgClearInterrupt
 * SYNOPSIS:	This function clears currunt interrupt
 *****************************************************************************/
void fimgClearInterrupt(fimgContext *ctx)
{
	fimgWrite(ctx, 1, FGGB_INTPENDING);
}

/*****************************************************************************
 * FUNCTIONS:	fimgEnableInterrupt
 * SYNOPSIS:	This function enables the FIMG-3DSE interrupt
 *****************************************************************************/
void fimgEnableInterrupt(fimgContext *ctx)
{
	ctx->global.intEn = 1;
	fimgWrite(ctx, 1, FGGB_INTMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgDisableInterrupt
 * SYNOPSIS:	This function disables the FIMG-3DSE interrupt
 *****************************************************************************/
void fimgDisableInterrupt(fimgContext *ctx)
{
	ctx->global.intEn = 0;
	fimgWrite(ctx, 0, FGGB_INTMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetInterruptBlock
 * SYNOPSIS:	This function sets pipeline blocks to generate interrupt.
 * PARAMETERS:	[IN] pipeMask: Oring PIPESTATE_XXXX block of generating interrupt
 *****************************************************************************/
void fimgSetInterruptBlock(fimgContext *ctx, uint32_t pipeMask)
{
	ctx->global.intMask = pipeMask;
	fimgWrite(ctx, pipeMask, FGGB_PIPEMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetInterruptState
 * SYNOPSIS:	This function sets an interrupt generated state of each block
 * PARAMETERS:	[IN] status: each block state for interrupt to occur
 *****************************************************************************/
void fimgSetInterruptState(fimgContext *ctx, uint32_t status)
{
	ctx->global.intTarget = status;
	fimgWrite(ctx, status, FGGB_PIPETGTSTATE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgGetInterruptState
 * SYNOPSIS:	This function returns the value of pipeline-state when interrupt
 *          	is to occur
 * RETURNS:	fimgPipelineStatus containing interrupt state
 *****************************************************************************/
uint32_t fimgGetInterruptState(fimgContext *ctx)
{
	return fimgRead(ctx, FGGB_PIPEINTSTATE);
}

void fimgCreateGlobalContext(fimgContext *ctx)
{
	// Nothing to initialize yet
}

void fimgRestoreGlobalState(fimgContext *ctx)
{
	//fimgWrite(ctx, 0, FGGB_INTMASK);
	//fimgWrite(ctx, 0, FGGB_PIPEMASK);
	//fimgWrite(ctx, 0, FGGB_INTPENDING);
	//fimgWrite(ctx, ctx->global.intTarget, FGGB_PIPETGTSTATE);
	//fimgWrite(ctx, ctx->global.intMask, FGGB_PIPEMASK);
	//fimgWrite(ctx, ctx->global.intEn, FGGB_INTMASK);
}
