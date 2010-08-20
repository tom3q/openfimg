/*
 * fimg/fimg_private.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (PRIVATE PART)
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _FIMG_PRIVATE_H_
#define _FIMG_PRIVATE_H_

/* Include public part */
#include "fimg.h"

// extern volatile void *fimgBase;

/*
 * Host interface
 */

typedef struct {
	union {
		unsigned int val;
		struct {
			unsigned stride		:8;
			unsigned		:8;
			unsigned range		:16;
		} bits;
	} ctrl;
	unsigned int base;
} fimgVtxBufAttrib;

typedef union {
	struct {
		unsigned lastattr	:1;
		unsigned		:15;
		unsigned dt		:4;
		unsigned		:2;
		unsigned numcomp	:2;
		unsigned srcw		:2;
		unsigned srcz		:2;
		unsigned srcy		:2;
		unsigned srcx		:2;
	} bits;
	unsigned int val;
} fimgAttribute;

/*
 * Primitive Engine
 */

typedef union {
	unsigned int val;
	struct {
		unsigned intUse		:2;
		unsigned		:3;
		unsigned type		:8;
		unsigned pointSize	:1;
		unsigned		:4;
		unsigned vsOut		:4;
		unsigned flatShadeEn	:1;
		unsigned flatShadeSel	:9;
	} bits;
} fimgVertexContext;

/*
 * Raster engine
 */

typedef union {
	unsigned int val;
	struct {
		unsigned		:28;
		unsigned enable		:1;
		unsigned clockwise	:1;
		unsigned face		:2;
	} bits;
} fimgCullingControl;

typedef union {
	unsigned int val;
	struct {
		unsigned		:4;
		unsigned maxval		:12;
		unsigned		:4;
		unsigned minval		:12;
	} bits;
} fimgClippingControl;

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

typedef union {
	unsigned int val;
	struct {
		unsigned		:12;
		unsigned out		:4;
		unsigned		:12;
		unsigned in		:4;
	} bits;
} fimgShaderAttribNum;

/*
 * Texture engine
 */

/*
 * OpenGL 1.1 compatibility
 */

/*
 * Per-fragment unit
 */

typedef union {
	unsigned int val;
	struct {
		unsigned enable		:1;
		unsigned		:3;
		unsigned max		:12;
		unsigned		:4;
		unsigned min		:12;
	} bits;
} fimgScissorTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned		:19;
		unsigned value		:8;
		unsigned mode		:4;
		unsigned enable		:1;
	} bits;
} fimgAlphaTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned dppass		:3;
		unsigned dpfail		:3;
		unsigned sfail		:3;
		unsigned		:3;
		unsigned mask		:8;
		unsigned ref		:8;
		unsigned mode		:3;
		unsigned enable		:1;
	} bits;
} fimgStencilTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned		:28;
		unsigned mode		:3;
		unsigned enable		:1;
	} bits;
} fimgDepthTestData;

typedef union {
	unsigned int val;
	struct {
		unsigned		:9;
		unsigned ablendequation	:3;
		unsigned cblendequation	:3;
		unsigned adstblendfunc	:4;
		unsigned cdstblendfunc	:4;
		unsigned asrcblendfunc	:4;
		unsigned csrcblendfunc	:4;
		unsigned enable		:1;
	} bits;
} fimgBlendControl;

typedef union {
	unsigned int val;
	struct {
		unsigned		:23;
		unsigned alpha		:4;
		unsigned color		:4;
		unsigned enable		:1;
	} bits;
} fimgLogOpControl;

typedef union {
	unsigned int val;
	struct {
		unsigned		:28;
		unsigned a		:1;
		unsigned b		:1;
		unsigned g		:1;
		unsigned r		:1;
	} bits;
} fimgColorBufMask;

typedef union {
	unsigned int val;
	struct {
		unsigned backmask	:8;
		unsigned frontmask	:8;
		unsigned		:15;
		unsigned depth		:1;
	} bits;
} fimgDepthBufMask;

typedef union {
	unsigned int val;
	struct {
		unsigned		:11;
		unsigned opaque		:1;
		unsigned alphathreshold	:8;
		unsigned alphaconst	:8;
		unsigned dither		:1;
		unsigned colormode	:3;
	} bits;
} fimgFramebufferControl;

/*
 * OS support
 */

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
	fimgVtxBufAttrib bufAttrib[FIMG_ATTRIB_NUM];
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
	fimgClippingControl xClip;
	float pointWidth;
	float pointWidthMin;
	float pointWidthMax;
	unsigned int spriteCoordAttrib;
	float lineWidth;
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

struct _fimgContext {
	volatile char *base;
	int fd;
	int memfd;
	/* Individual contexts */
	fimgGlobalContext global;
	fimgHostContext host;
	fimgPrimitiveContext primitive;
	fimgRasterizerContext rasterizer;
	fimgFragmentContext fragment;
	/* Shared context */
	unsigned int numAttribs;
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

#endif /* _FIMG_PRIVATE_H_ */
