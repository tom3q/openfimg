/*
 * fimg/fimg_private.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (PRIVATE PART)
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

#ifndef _FIMG_PRIVATE_H_
#define _FIMG_PRIVATE_H_

/* Include public part */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "fimg.h"
#include <cutils/log.h>

#define TRACE(a)	LOGD(#a); a

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/*
 * Global block
 */

/* Type definitions */
typedef union {
	unsigned int val;
	struct {
		unsigned host_fifo	:1;
		unsigned hi		:1;
		unsigned hvf		:1;
		unsigned vc		:1;
		unsigned vs		:1;
		unsigned		:3;
		unsigned pe		:1;
		unsigned tse		:1;
		unsigned ra		:1;
		unsigned		:1;
		unsigned ps0		:1;
		unsigned		:3;
		unsigned pf0		:1;
		unsigned		:1;
		unsigned ccache0	:1;
		unsigned		:13;
	};
} fimgPipelineStatus;

typedef union {
	unsigned int val;
	struct {
		unsigned		:8;
		unsigned revision	:8;
		unsigned minor		:8;
		unsigned major		:8;
	};
} fimgVersion;

/*
 * Host interface
 */

typedef union {
	unsigned int val;
	struct {
		unsigned numoutattrib	:4;
		unsigned envc		:1;
		unsigned		:11;
		unsigned autoinc	:1;
		unsigned		:7;
		unsigned idxtype	:2;
		unsigned		:5;
		unsigned envb		:1;
	};
} fimgHInterface;

typedef union {
	unsigned int val;
	struct {
		unsigned range		:16;
		unsigned		:8;
		unsigned stride		:8;
	};
} fimgVtxBufAttrib;

typedef union {
	struct {
		unsigned srcx		:2;
		unsigned srcy		:2;
		unsigned srcz		:2;
		unsigned srcw		:2;
		unsigned numcomp	:2;
		unsigned		:2;
		unsigned dt		:4;
		unsigned		:15;
		unsigned lastattr	:1;
	};
	unsigned int val;
} fimgAttribute;

inline void fimgDrawArraysBufferedAutoinc(fimgContext *ctx,
		fimgArray *arrays, unsigned int first, unsigned int count);

/*
 * Primitive Engine
 */

typedef union {
	unsigned int val;
	struct {
		unsigned flatShadeSel	:9;
		unsigned flatShadeEn	:1;
		unsigned vsOut		:4;
		unsigned		:4;
		unsigned pointSize	:1;
		unsigned type		:8;
		unsigned		:5;
	};
} fimgVertexContext;

/*
 * Raster engine
 */

typedef union {
	unsigned int val;
	struct {
		unsigned face		:2;
		unsigned clockwise	:1;
		unsigned enable		:1;
		unsigned		:28;
	};
} fimgCullingControl;

typedef union {
	unsigned int val;
	struct {
		unsigned minval		:12;
		unsigned		:4;
		unsigned maxval		:12;
		unsigned		:4;
	};
} fimgClippingControl;

typedef union {
	unsigned int val;
	struct {
		struct {
			unsigned lod	:1;
			unsigned ddx	:1;
			unsigned ddy	:1;
		} coef[8];
		unsigned		:8;
	};
} fimgLODControl;

/*
 * Shaders
 */

#define FGSP_MAX_ATTRIBTBL_SIZE 12
#define BUILD_SHADER_VERSION(major, minor)	(0xFFFF0000 | (((minor)&0xFF)<<8) | ((major) & 0xFF))
#define VERTEX_SHADER_MAGIC 			(((('V')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define PIXEL_SHADER_MAGIC 			(((('P')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define SHADER_VERSION 				BUILD_SHADER_VERSION(8,0)
#define FGVS_ATTRIB(i)				(3 - ((i) % 4))
#define FGVS_ATTRIB_BANK(i)			(((i) / 4) & 3)

/* Type definitions */
typedef struct {
	unsigned int	Magic;
	unsigned int	Version;
	unsigned int	HeaderSize;
	unsigned int	fimgVersion;

	unsigned int	InstructSize;
	unsigned int	ConstFloatSize;
	unsigned int	ConstIntSize;
	unsigned int	ConstBoolSize;

	unsigned int	InTableSize;
	unsigned int	OutTableSize;
	unsigned int	UniformTableSize;
	unsigned int	SamTableSize;

	unsigned int	reserved[6];
} fimgShaderHeader;

typedef struct {
	int		validTableInfo;
	unsigned int	outAttribTableSize;
	unsigned int	inAttribTableSize;
	unsigned int	vsOutAttribTable[12];
	unsigned int	psInAttribTable[12];
} fimgShaderAttribTable;

/*
 * Per-fragment unit
 */

typedef union {
	unsigned int val;
	struct {
		unsigned min		:12;
		unsigned		:4;
		unsigned max		:12;
		unsigned		:3;
		unsigned enable		:1;
	};
} fimgScissorTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned mode		:4;
		unsigned value		:8;
		unsigned		:19;
	};
} fimgAlphaTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned mode		:3;
		unsigned ref		:8;
		unsigned mask		:8;
		unsigned		:3;
		unsigned sfail		:3;
		unsigned dpfail		:3;
		unsigned dppass		:3;
	};
} fimgStencilTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned mode		:3;
		unsigned		:28;
	};
} fimgDepthTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned csrcblendfunc	:4;
		unsigned asrcblendfunc	:4;
		unsigned cdstblendfunc	:4;
		unsigned adstblendfunc	:4;
		unsigned cblendequation	:3;
		unsigned ablendequation	:3;
		unsigned		:9;
	};
} fimgBlendControl;

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned color		:4;
		unsigned alpha		:4;
		unsigned		:23;
	};
} fimgLogOpControl;

typedef union {
	unsigned int val;
	struct {
		unsigned a		:1;
		unsigned b		:1;
		unsigned g		:1;
		unsigned r		:1;
		unsigned		:28;
	};
} fimgColorBufMask;

typedef union {
	unsigned int val;
	struct {
		unsigned depth		:1;
		unsigned		:15;
		unsigned frontmask	:8;
		unsigned backmask	:8;
	};
} fimgDepthBufMask;

typedef union {
	unsigned int val;
	struct {
		unsigned colormode	:3;
		unsigned dither		:1;
		unsigned alphaconst	:8;
		unsigned alphathreshold	:8;
		unsigned opaque		:1;
		unsigned		:11;
	};
} fimgFramebufferControl;

/*
 * Texturing
 */

typedef union {
	unsigned int val;
	struct {
		unsigned useMipmap	:2;
		unsigned minFilter	:1;
		unsigned magFilter	:1;
		unsigned texCoordSys	:1;
		unsigned		:1;
		unsigned pAddrMode	:2;
		unsigned vAddrMode	:2;
		unsigned uAddrMode	:2;
		unsigned textureFmt	:5;
		unsigned paletteFmt	:2;
		unsigned alphaFmt	:1;
		unsigned texExp		:1;
		unsigned clrKeyEn	:1;
		unsigned clrKeySel	:1;
		unsigned		:4;
		unsigned type		:2;
		unsigned		:3;
	};
} fimgTexControl;

typedef union {
	unsigned int val;
	struct {
		unsigned vsize		:4;
		unsigned usize		:4;
		unsigned vmod		:2;
		unsigned umod		:2;
		unsigned		:20;
	};
} fimgVtxTexControl;

struct _fimgTexture {
	fimgTexControl control;
	unsigned int uSize;
	unsigned int vSize;
	unsigned int pSize;
	unsigned int offset[FGTU_MAX_MIPMAP_LEVEL];
	unsigned int minLevel;
	unsigned int maxLevel;
	unsigned int baseAddr;
};

/*
 * Hardware context
 */

typedef struct {
	unsigned int intEn;
	unsigned int intMask;
	unsigned int intTarget;
} fimgGlobalContext;

void fimgCreateGlobalContext(fimgContext *ctx);
void fimgRestoreGlobalState(fimgContext *ctx);

typedef struct {
	fimgAttribute attrib[FIMG_ATTRIB_NUM];
	fimgVtxBufAttrib vbctrl[FIMG_ATTRIB_NUM];
	unsigned int vbbase[FIMG_ATTRIB_NUM];
	fimgHInterface control;
	unsigned int indexOffset;
} fimgHostContext;

void fimgCreateHostContext(fimgContext *ctx);
void fimgRestoreHostState(fimgContext *ctx);

typedef struct {
	fimgVertexContext vctx;
	float ox;
	float oy;
	float halfPX;
	float halfPY;
	float halfDistance;
	float center;
} fimgPrimitiveContext;

void fimgCreatePrimitiveContext(fimgContext *ctx);
void fimgRestorePrimitiveState(fimgContext *ctx);

typedef struct {
	unsigned int samplePos;
	unsigned int dOffEn;
	float dOffFactor;
	float dOffUnits;
	fimgCullingControl cull;
	fimgClippingControl yClip;
	float pointWidth;
	float pointWidthMin;
	float pointWidthMax;
	unsigned int spriteCoordAttrib;
	float lineWidth;
	fimgLODControl lodGen;
	fimgClippingControl xClip;
} fimgRasterizerContext;

void fimgCreateRasterizerContext(fimgContext *ctx);
void fimgRestoreRasterizerState(fimgContext *ctx);

typedef struct {
	fimgScissorTestData scY;
	fimgScissorTestData scX;
	fimgAlphaTestData alpha;
	fimgStencilTestData stBack;
	fimgStencilTestData stFront;
	fimgDepthTestData depth;
	fimgBlendControl blend;
	unsigned int blendColor;
	fimgFramebufferControl fbctl;
	fimgLogOpControl logop;
	fimgColorBufMask mask;
	fimgDepthBufMask dbmask;
	unsigned int depthAddr;
	unsigned int colorAddr;
	unsigned int bufWidth;
} fimgFragmentContext;

void fimgCreateFragmentContext(fimgContext *ctx);
void fimgRestoreFragmentState(fimgContext *ctx);

#ifdef FIMG_FIXED_PIPELINE

typedef struct {
	fimgCombArgSrc src;
	fimgCombArgMod mod;
} fimgCombArg;

typedef struct {
	fimgCombFunc func;
	fimgCombArg arg[3];
} fimgCombiner;

typedef struct {
	int enabled;
	int dirty;
	fimgTexFunc func;
	fimgCombiner combc;
	fimgCombiner comba;
	float env[4];
	float scale[4];
	fimgTexture *texture;
	int swap;
} fimgTextureCompat;

typedef struct {
	int vsDirty;
	uint32_t vshaderEnd;
	int psDirty;
	uint32_t pshaderEnd;
	fimgTextureCompat texture[FIMG_NUM_TEXTURE_UNITS];
	int matrixDirty[2 + FIMG_NUM_TEXTURE_UNITS];
	const float *matrix[2 + FIMG_NUM_TEXTURE_UNITS];
	/* More to come */
} fimgCompatContext;

void fimgCreateCompatContext(fimgContext *ctx);
void fimgRestoreCompatState(fimgContext *ctx);
void fimgCompatFlush(fimgContext *ctx);

#endif

typedef struct {
	float color[4];
	float depth;
	uint8_t stencil;
} fimgClearContext;

struct _fimgContext {
	volatile char *base;
	int fd;
	/* Individual contexts */
	fimgGlobalContext global;
	fimgHostContext host;
	fimgPrimitiveContext primitive;
	fimgRasterizerContext rasterizer;
	fimgFragmentContext fragment;
#ifdef FIMG_FIXED_PIPELINE
	fimgCompatContext compat;
#endif
	fimgClearContext clear;
	/* Shared context */
	unsigned int numAttribs;
	unsigned int fbHeight;
	/* Register queue */
	unsigned int *queueStart;
	unsigned int *queue;
	unsigned int queueLen;
};

/* Registry accessors */
static inline void fimgWrite(fimgContext *ctx, unsigned int data, unsigned int addr)
{
	volatile unsigned int *reg = (volatile unsigned int *)((volatile char *)ctx->base + addr);
	*reg = data;
}

static inline unsigned int fimgRead(fimgContext *ctx, unsigned int addr)
{
	volatile unsigned int *reg = (volatile unsigned int *)((volatile char *)ctx->base + addr);
	return *reg;
}

static inline void fimgWriteF(fimgContext *ctx, float data, unsigned int addr)
{
	volatile float *reg = (volatile float *)((volatile char *)ctx->base + addr);
	*reg = data;
}

static inline float fimgReadF(fimgContext *ctx, unsigned int addr)
{
	volatile float *reg = (volatile float *)((volatile char *)ctx->base + addr);
	return *reg;
}

/* Register queue */
#define FIMG_MAX_QUEUE_LEN	64

static inline void fimgQueueFlush(fimgContext *ctx)
{
	unsigned int cnt;
	unsigned int *ptr;

	if (!ctx->queueLen)
		return;

	/* Above the maximum length it's more effective to restore the whole
	 * context than just the changed registers */
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN) {
		fimgRestoreContext(ctx);
		return;
	}

	cnt = ctx->queueLen;
	ptr = ctx->queueStart;

	while (cnt--) {
		fimgWrite(ctx, ptr[1], ptr[0]);
		ptr += 2;
	}

	ctx->queueLen = 0;
	ctx->queue = ctx->queueStart;
	ctx->queue[0] = 0;
}

static inline void fimgQueue(fimgContext *ctx, unsigned int data, unsigned int addr)
{
	if (ctx->queue[0] == addr) {
		ctx->queue[1] = data;
		return;
	}

	/* Above the maximum length it's more effective to restore the whole
	 * context than just the changed registers */
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN)
		return;

	ctx->queue += 2;
	ctx->queueLen++;
	ctx->queue[0] = addr;
	ctx->queue[1] = data;
}

static inline void fimgQueueF(fimgContext *ctx, float data, unsigned int addr)
{
	if (ctx->queue[0] == addr) {
		((float *)ctx->queue)[1] = data;
		return;
	}

	/* Above the maximum length it's more effective to restore the whole
	 * context than just the changed registers */
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN)
		return;

	ctx->queue += 2;
	ctx->queueLen++;
	ctx->queue[0] = addr;
	((float *)ctx->queue)[1] = data;
}

/* Hardware context */

static inline void fimgGetHardware(fimgContext *ctx)
{
	int ret;

	if(unlikely((ret = fimgAcquireHardwareLock(ctx)) != 0)) {
		if(likely(ret > 0)) {
			//fimgFlush(ctx);
			fimgRestoreContext(ctx);
			//fimgInvalidateFlushCache(ctx, 1, 1, 0, 0);
			return;
		} else {
			fprintf(stderr, "FIMG: Could not acquire hardware lock");
			exit(EBUSY);
		}
	}
}

static inline void fimgFlushContext(fimgContext *ctx)
{
	fimgQueueFlush(ctx);
#ifdef FIMG_FIXED_PIPELINE
	fimgCompatFlush(ctx);
#endif
}

static inline void fimgPutHardware(fimgContext *ctx)
{
	fimgReleaseHardwareLock(ctx);
}

extern void fimgDumpState(fimgContext *ctx, unsigned mode, unsigned count, const char *func);

#endif /* _FIMG_PRIVATE_H_ */
