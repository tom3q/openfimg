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

#define GLOBAL_OFFSET		0x0000

#define FGGB_PIPESTATE		GLOBAL_ADDR(0x00)
#define FGGB_CACHECTL		GLOBAL_ADDR(0x04)
#define FGGB_RST		GLOBAL_ADDR(0x08)
#define FGGB_VERSION		GLOBAL_ADDR(0x10)
#define FGGB_INTPENDING		GLOBAL_ADDR(0x40)
#define FGGB_INTMASK		GLOBAL_ADDR(0x44)
#define FGGB_PIPEMASK		GLOBAL_ADDR(0x48)
#define FGGB_PIPETGTSTATE	GLOBAL_ADDR(0x4c)
#define FGGB_PIPEINTSTATE	GLOBAL_ADDR(0x50)

#define GLOBAL_OFFS(reg)	(GLOBAL_OFFSET + (reg))
#define GLOBAL_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + GLOBAL_OFFS(reg)))

typedef union {
	unsigned int val;
	struct {
		unsigned		:19;
		unsigned vtcclear	:1;
		unsigned		:2;
		unsigned tcclear	:2;
		unsigned		:2;
		unsigned ccflush	:2;
		unsigned		:2;
		unsigned zcflush	:2;
	} bits;
} fimgCacheCtl;

static inline void fimgGlobalWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline unsigned int fimgGlobalRead(volatile unsigned int *reg)
{
	return *reg;
}

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgGetPipelineStatus
 * SYNOPSIS:	This function obtains status of the pipeline
 * RETURNS:	fimgPipelineStatus conaining pipeline status
 *****************************************************************************/
fimgPipelineStatus fimgGetPipelineStatus(void)
{
	fimgPipelineStatus stat;
	stat.val = fimgGlobalRead(FGGB_PIPESTATE);
	return stat;
}

/*****************************************************************************
 * FUNCTIONS:	fimgFlush
 * SYNOPSIS:	This function flushes the fimg3d pipeline
 * PARAMETERS:	[IN] pipelineFlags: Specified pipeline states are flushed
 * RETURNS:	 0, on success
 *		-1, on timeout
 *****************************************************************************/
int fimgFlush(/*fimgPipelineStatus pipelineFlags*/)
{
#if 0
	/* TODO: Finish interrupt support */
	return fimgWaitForFlushInterrupt(pipelineFlags);
#else
	unsigned int timeout = 1000000;

	while(fimgGlobalRead(FGGB_PIPESTATE)/* & pipelineFlags.val*/) {
		if(--timeout == 0)
			return -1;
		/* TODO: Check performance impact of sleeping */
		usleep(1);
	}

	return 0;
#endif
}

/*****************************************************************************
 * FUNCTIONS:	fimgClearCache
 * SYNOPSIS:	This function clears the caches
 * PARAMETERS:	[IN] clearFlags: Specified caches are cleared
 * RETURNS:	 0, on success
 *		-1, on timeout
 *****************************************************************************/
int fimgClearInvalidateCache(unsigned int vtcclear, unsigned int tcclear,
			unsigned int ccflush, unsigned int zcflush)
{
	unsigned int timeout = 1000000;
	fimgCacheCtl ctl;

	ctl.val = 0;
	ctl.bits.vtcclear = vtcclear;
	ctl.bits.tcclear = tcclear;
	ctl.bits.ccflush = ccflush;
	ctl.bits.zcflush = zcflush;

	fimgGlobalWrite(ctl.val, FGGB_CACHECTL); // start clearing the cache

	while(fimgGlobalRead(FGGB_CACHECTL) & ctl.val) {
		if(--timeout == 0)
			return -1;

		usleep(1);
	}

	return 0;
}

/*****************************************************************************
 * FUNCTIONS:	fimgSoftReset
 * SYNOPSIS:	This function resets FIMG-3DSE, but the SFR values are not affected
 *****************************************************************************/
void fimgSoftReset(void)
{
	fimgGlobalWrite(1, FGGB_RST);
	usleep(1);
	fimgGlobalWrite(0, FGGB_RST);
}

/*****************************************************************************
 * FUNCTIONS:	fglGetVersion
 * SYNOPSIS:	This function gets FIMG-3DSE version.
 * RETURNS:	Version of 3D hardware
 *****************************************************************************/
fimgVersion fimgGetVersion(void)
{
	fimgVersion ret;

	ret.val = fimgGlobalRead(FGGB_VERSION);
	return ret;
}

/*****************************************************************************
 * FUNCTIONS:	fimgGetInterrupt
 * SYNOPSIS:	This function returns current interrupt state.
 * RETURNS:	Non-zero if interrupt is active.
 *****************************************************************************/
unsigned int fimgGetInterrupt(void)
{
	return fimgGlobalRead(FGGB_INTPENDING) & 1;
}

/*****************************************************************************
 * FUNCTIONS:	fimgClearInterrupt
 * SYNOPSIS:	This function clears currunt interrupt
 *****************************************************************************/
void fimgClearInterrupt(void)
{
	fimgGlobalWrite(1, FGGB_INTPENDING);
}

/*****************************************************************************
 * FUNCTIONS:	fimgEnableInterrupt
 * SYNOPSIS:	This function enables the FIMG-3DSE interrupt
 *****************************************************************************/
void fimgEnableInterrupt(fimgContext *ctx)
{
	ctx->global.intEn = 1;
	fimgGlobalWrite(1, FGGB_INTMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgDisableInterrupt
 * SYNOPSIS:	This function disables the FIMG-3DSE interrupt
 *****************************************************************************/
void fimgDisableInterrupt(fimgContext *ctx)
{
	ctx->global.intEn = 0;
	fimgGlobalWrite(0, FGGB_INTMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetInterruptBlock
 * SYNOPSIS:	This function sets pipeline blocks to generate interrupt.
 * PARAMETERS:	[IN] pipeMask: Oring PIPESTATE_XXXX block of generating interrupt
 *****************************************************************************/
void fimgSetInterruptBlock(fimgContext *ctx, fimgPipelineStatus pipeMask)
{
	ctx->global.intMask = pipeMask.val;
	fimgGlobalWrite(pipeMask.val, FGGB_PIPEMASK);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetInterruptState
 * SYNOPSIS:	This function sets an interrupt generated state of each block
 * PARAMETERS:	[IN] status: each block state for interrupt to occur
 *****************************************************************************/
void fimgSetInterruptState(fimgContext *ctx, fimgPipelineStatus status)
{
	ctx->global.intTarget = status.val;
	fimgGlobalWrite(status.val, FGGB_PIPETGTSTATE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgGetInterruptState
 * SYNOPSIS:	This function returns the value of pipeline-state when interrupt
 *          	is to occur
 * RETURNS:	fimgPipelineStatus containing interrupt state
 *****************************************************************************/
fimgPipelineStatus fimgGetInterruptState(void)
{
	fimgPipelineStatus stat;
	stat.val = fimgGlobalRead(FGGB_PIPEINTSTATE);
	return stat;
}

void fimgCreateGlobalContext(fimgContext *ctx)
{
	// Nothing to initialize yet
}

void fimgRestoreGlobalState(fimgContext *ctx)
{
	fimgGlobalWrite(ctx->global.intTarget, FGGB_PIPETGTSTATE);
	fimgGlobalWrite(ctx->global.intMask, FGGB_PIPEMASK);
	fimgGlobalWrite(ctx->global.intEn, FGGB_INTMASK);
}