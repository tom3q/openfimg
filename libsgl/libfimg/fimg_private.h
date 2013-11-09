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
#include <stdint.h>
#include <drm/exynos_drm.h>
#include "platform.h"
#include "fimg.h"

#define TRACE(a)	LOGD(#a); a

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define NELEM(i)	(sizeof(i)/sizeof(*i))

/*
 * Host interface
 */

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

/*
 * Per-fragment unit
 */

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
	struct drm_exynos_g3d_texture hw;
	unsigned gem;
	unsigned flags;
};

/*
 * Hardware context
 */

void fimgCreateGlobalContext(fimgContext *ctx);

typedef struct __attribute__ ((__packed__)) {
	fimgAttribute attrib[FIMG_ATTRIB_NUM + 1];
	fimgVtxBufAttrib vbctrl[FIMG_ATTRIB_NUM + 1];
	unsigned int vbbase[FIMG_ATTRIB_NUM + 1];
} fimgHostContext;

void fimgCreateHostContext(fimgContext *ctx);

typedef struct __attribute__ ((__packed__)) {
	fimgVertexContext vctx;
	float ox;
	float oy;
	float halfPX;
	float halfPY;
	float halfDistance;
	float center;
} fimgPrimitiveContext;

void fimgCreatePrimitiveContext(fimgContext *ctx);

typedef struct __attribute__ ((__packed__)) {
	unsigned int samplePos;
	unsigned int dOffEn;
	float dOffFactor;
	float dOffUnits;
	fimgCullingControl cull;
	fimgClippingControl yClip;
	unsigned int lodGen;
	fimgClippingControl xClip;
	float pointWidth;
	float pointWidthMin;
	float pointWidthMax;
	unsigned int spriteCoordAttrib;
	float lineWidth;
} fimgRasterizerContext;

void fimgCreateRasterizerContext(fimgContext *ctx);

typedef struct __attribute__ ((__packed__)) {
	fimgAlphaTestData alpha;
	fimgStencilTestData stBack;
	unsigned int blendColor;
	fimgBlendControl blend;
	fimgLogOpControl logop;
	fimgColorBufMask mask;
} fimgFragmentContext;

typedef struct __attribute__ ((__packed__)) {
	fimgStencilTestData stFront;
	fimgDepthTestData depth;
	fimgDepthBufMask dbmask;
	fimgFramebufferControl fbctl;
	unsigned int depthAddr;
	unsigned int colorAddr;
	unsigned int bufWidth;
} fimgProtectedContext;

void fimgCreateFragmentContext(fimgContext *ctx);

#ifdef FIMG_FIXED_PIPELINE

#define FGFP_BITFIELD_GET(reg, name)		\
	(((reg) & FGFP_ ## name ## _MASK) >> FGFP_ ## name ## _SHIFT)
#define FGFP_BITFIELD_GET_IDX(reg, name, idx)		\
	(((reg) & FGFP_ ## name ## _MASK((idx))) >> FGFP_ ## name ## _SHIFT((idx)))

#define FGFP_BITFIELD_SET(reg, name, val)	do {\
		reg &= ~(FGFP_ ## name ## _MASK); \
		reg |= (val) << FGFP_ ## name ## _SHIFT; \
	} while (0);
#define FGFP_BITFIELD_SET_IDX(reg, name, idx, val)	do {\
		reg &= ~(FGFP_ ## name ## _MASK((idx))); \
		reg |= (val) << FGFP_ ## name ## _SHIFT((idx)); \
	} while (0);

#define FGFP_TEX_MODE_SHIFT		(0)
#define FGFP_TEX_MODE_MASK		(0x7 << 0)
#define FGFP_TEX_SWAP_SHIFT		(3)
#define FGFP_TEX_SWAP_MASK		(0x1 << 3)
#define FGFP_TEX_COMBC_SRC_SHIFT(i)	(4 + 4*(i))
#define FGFP_TEX_COMBC_SRC_MASK(i)	(0x3 << (4 + 4*(i)))
#define FGFP_TEX_COMBC_MOD_SHIFT(i)	(6 + 4*(i))
#define FGFP_TEX_COMBC_MOD_MASK(i)	(0x3 << (6 + 4*(i)))
#define FGFP_TEX_COMBC_FUNC_SHIFT	(16)
#define FGFP_TEX_COMBC_FUNC_MASK	(0x7 << 16)
#define FGFP_TEX_COMBA_SRC_SHIFT(i)	(19 + 3*(i))
#define FGFP_TEX_COMBA_SRC_MASK(i)	(0x3 << (19 + 3*(i)))
#define FGFP_TEX_COMBA_MOD_SHIFT(i)	(21 + 3*(i))
#define FGFP_TEX_COMBA_MOD_MASK(i)	(0x1 << (21 + 3*(i)))
#define FGFP_TEX_COMBA_FUNC_SHIFT	(28)
#define FGFP_TEX_COMBA_FUNC_MASK	(0x7 << 28)
#define FGFP_PS_SWAP_SHIFT		(0)
#define FGFP_PS_SWAP_MASK		(0x1 << 0)
#define FGFP_PS_INVALID_SHIFT		(31)
#define FGFP_PS_INVALID_MASK		(0x1 << 31)

typedef union _fimgPixelShaderState {
	uint32_t val[FIMG_NUM_TEXTURE_UNITS + 1];
	struct {
		uint32_t tex[FIMG_NUM_TEXTURE_UNITS];
		uint32_t ps;
	};
} fimgPixelShaderState;

#define FGFP_VS_TEX_EN_SHIFT(i)		(i)
#define FGFP_VS_TEX_EN_MASK(i)		(0x1 << (i))
#define FGFP_VS_INVALID_SHIFT		(31)
#define FGFP_VS_INVALID_MASK		(0x1 << 31)

typedef union _fimgVertexShaderState {
	uint32_t val[1];
	struct {
		uint32_t vs;
	};
} fimgVertexShaderState;

typedef struct {
	int dirty;
	float env[4];
	float scale[4];
	fimgTexture *texture;
} fimgTextureCompat;

typedef struct fimgPixelShaderProgram {
	uint32_t instrCount;
	fimgPixelShaderState state;
} fimgPixelShaderProgram;

typedef struct fimgVertexShaderProgram {
	uint32_t instrCount;
	fimgVertexShaderState state;
} fimgVertexShaderProgram;

#define VS_CACHE_SIZE	4
#define PS_CACHE_SIZE	8

typedef struct {
	uint32_t		*vshaderBuf;
	int			vshaderLoaded;
	uint32_t		curVsNum;
	uint32_t		vsEvictCounter;
	fimgVertexShaderState	vsState;
	fimgVertexShaderProgram	vertexShaders[VS_CACHE_SIZE];
#ifdef FIMG_SHADER_CACHE_STATS
	uint8_t			vsMisses;
	uint8_t			vsSameHits;
	uint8_t			vsCacheHits;
	uint8_t			vsStatsCounter;
#endif

	uint32_t		*pshaderBuf;
	int			pshaderLoaded;
	uint32_t		curPsNum;
	uint32_t		psEvictCounter;
	uint32_t		psMask[FIMG_NUM_TEXTURE_UNITS + 1];
	fimgPixelShaderState	psState;
	fimgPixelShaderProgram	pixelShaders[PS_CACHE_SIZE];
#ifdef FIMG_SHADER_CACHE_STATS
	uint8_t			psMisses;
	uint8_t			psSameHits;
	uint8_t			psCacheHits;
	uint8_t			psStatsCounter;
#endif

	fimgTextureCompat	texture[FIMG_NUM_TEXTURE_UNITS];

	int			matrixDirty[2 + FIMG_NUM_TEXTURE_UNITS];
	const float		*matrix[2 + FIMG_NUM_TEXTURE_UNITS];
} fimgCompatContext;

void fimgCreateCompatContext(fimgContext *ctx);
void fimgCompatFlush(fimgContext *ctx);

#endif

struct _fimgContext {
	int fd;

	struct __attribute__ ((__packed__)) fimgHWContext {
		/* Individual contexts */
		fimgProtectedContext prot;
		fimgHostContext host;
		fimgPrimitiveContext primitive;
		fimgRasterizerContext rasterizer;
		fimgFragmentContext fragment;
	} hw;

#ifdef FIMG_FIXED_PIPELINE
	fimgCompatContext compat;
#endif

	/* Shared context */
	unsigned int invalTexCache;
	unsigned int numAttribs;
	unsigned int fbHeight;
	unsigned int fbFlags;
	int flipY;
	/* Register queue */
	struct g3d_state_entry *queueStart;
	struct g3d_state_entry *queue;
	unsigned int queueLen;
	/* Lock state */
	unsigned int locked;
	/* Vertex data */
	uint8_t *vertexData;
	size_t vertexDataSize;
};

/* Hardware context */

#define FIMG_MAX_QUEUE_LEN	64

static inline void fimgQueue(fimgContext *ctx,
				unsigned int data, enum g3d_register addr)
{
	if (ctx->queue->reg == addr) {
		ctx->queue->val = data;
		return;
	}

	/* Above the maximum length it's more effective to restore the whole
	 * context than just the changed registers */
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN)
		return;

	++ctx->queue;
	++ctx->queueLen;
	ctx->queue->reg = addr;
	ctx->queue->val = data;
}

static inline uint32_t getRawFloat(float val)
{
	union {
		float val;
		uint32_t raw;
	} tmp;

	tmp.val = val;
	return tmp.raw;
}

static inline void fimgQueueF(fimgContext *ctx,
			      float data, enum g3d_register addr)
{
	fimgQueue(ctx, getRawFloat(data), addr);
}

extern void fimgQueueFlush(fimgContext *ctx);

extern void fimgDumpState(fimgContext *ctx, unsigned mode, unsigned count, const char *func);

#endif /* _FIMG_PRIVATE_H_ */
