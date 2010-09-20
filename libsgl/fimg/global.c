/*
 * fimg/global.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE GLOBAL BLOCK RELATED FUNCTIONS
 *
 * Copyrights:	2005 by Samsung Electronics Limited (original code)
 *		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
 */

#include "fimg_private.h"
#include <unistd.h>

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
	} bits;
} fimgCacheCtl;

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgGetPipelineStatus
 * SYNOPSIS:	This function obtains status of the pipeline
 * RETURNS:	fimgPipelineStatus conaining pipeline status
 *****************************************************************************/
fimgPipelineStatus fimgGetPipelineStatus(fimgContext *ctx)
{
	fimgPipelineStatus stat;
	stat.val = fimgRead(ctx, FGGB_PIPESTATE);
	return stat;
}

/*****************************************************************************
 * FUNCTIONS:	fimgFlush
 * SYNOPSIS:	This function flushes the fimg3d pipeline
 * PARAMETERS:	[IN] pipelineFlags: Specified pipeline states are flushed
 * RETURNS:	 0, on success
 *		-1, on timeout
 *****************************************************************************/
int fimgFlush(fimgContext *ctx/*, fimgPipelineStatus pipelineFlags*/)
{
	/* Return if already flushed */
	if(fimgRead(ctx, FGGB_PIPESTATE) == 0)
		return 0;

	/* Flush whole pipeline (TODO: Allow selective flushing) */
	return fimgWaitForFlush(ctx, 0xffffffff);
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
	unsigned int timeout = 1000000;
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.bits.vtcclear = vtcclear;
	ctl.bits.tcclear = tcclear;
	ctl.bits.ccflush = ccflush;
	ctl.bits.zcflush = zcflush;

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
fimgVersion fimgGetVersion(fimgContext *ctx)
{
	fimgVersion ret;

	ret.val = fimgRead(ctx, FGGB_VERSION);
	return ret;
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
void fimgSetInterruptBlock(fimgContext *ctx, fimgPipelineStatus pipeMask)
{
	ctx->global.intMask = pipeMask.val;
	fimgWrite(ctx, pipeMask.val, FGGB_PIPEMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetInterruptState
 * SYNOPSIS:	This function sets an interrupt generated state of each block
 * PARAMETERS:	[IN] status: each block state for interrupt to occur
 *****************************************************************************/
void fimgSetInterruptState(fimgContext *ctx, fimgPipelineStatus status)
{
	ctx->global.intTarget = status.val;
	fimgWrite(ctx, status.val, FGGB_PIPETGTSTATE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgGetInterruptState
 * SYNOPSIS:	This function returns the value of pipeline-state when interrupt
 *          	is to occur
 * RETURNS:	fimgPipelineStatus containing interrupt state
 *****************************************************************************/
fimgPipelineStatus fimgGetInterruptState(fimgContext *ctx)
{
	fimgPipelineStatus stat;
	stat.val = fimgRead(ctx, FGGB_PIPEINTSTATE);
	return stat;
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
