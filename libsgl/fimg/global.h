/*
 * fimg/global.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE GLOBAL BLOCK RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * Global block
 */

/* Type definitions */
typedef union {
	unsigned int val;
	struct {
		unsigned		:13;
		unsigned ccache0	:1;
		unsigned		:1;
		unsigned pf0		:1;
		unsigned		:3;
		unsigned ps0		:1;
		unsigned		:1;
		unsigned ra		:1;
		unsigned tse		:1;
		unsigned pe		:1;
		unsigned		:3;
		unsigned vs		:1;
		unsigned vc		:1;
		unsigned hvf		:1;
		unsigned hi		:1;
		unsigned host_fifo	:1;
	} bits;
} fimgPipelineStatus;

typedef union {
	unsigned int val;
	struct {
		unsigned major		:8;
		unsigned minor		:8;
		unsigned revision	:8;
		unsigned		:8;
	} bits;
} fimgVersion;

/* Functions */
fimgPipelineStatus fimgGetPipelineStatus(void);
int fimgFlush(fimgPipelineStatus pipelineFlags);
int fimgClearCache(unsigned int clearFlags);
void fimgSoftReset(void);
fimgVersion fimgGetVersion(void);
unsigned int fimgGetInterrupt(void);
void fimgClearInterrupt(void);
void fimgEnableInterrupt(void);
void fimgDisableInterrupt(void);
void fimgSetInterruptBlock(fimgPipelineStatus pipeMask);
void fimgSetInterruptState(fimgPipelineStatus status);
fimgPipelineStatus fimgGetInterruptState(void);

#ifdef __cplusplus
}
#endif

#endif
