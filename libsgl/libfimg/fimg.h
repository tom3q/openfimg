/*
 * fimg/fimg.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (PUBLIC PART)
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

#ifndef _FIMG_H_
#define _FIMG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "config.h"

//=============================================================================

struct _fimgContext;
typedef struct _fimgContext fimgContext;

/*
 * Global block
 */

#define FGHI_PIPELINE_FIFO	(1 << 0)
#define FGHI_PIPELINE_HOSTIF	(1 << 1)
#define FGHI_PIPELINE_HVF	(1 << 2)
#define FGHI_PIPELINE_VCACHE	(1 << 3)
#define FGHI_PIPELINE_VSHADER	(1 << 4)
#define FGHI_PIPELINE_PRIM_ENG	(1 << 8)
#define FGHI_PIPELINE_TRI_ENG	(1 << 9)
#define FGHI_PIPELINE_RA_ENG	(1 << 10)
#define FGHI_PIPELINE_PSHADER	(1 << 12)
#define FGHI_PIPELINE_PER_FRAG	(1 << 16)
#define FGHI_PIPELINE_CCACHE	(1 << 18)

#define FGHI_PIPELINE_ALL ( \
	FGHI_PIPELINE_FIFO | FGHI_PIPELINE_HOSTIF | FGHI_PIPELINE_HVF | \
	FGHI_PIPELINE_VCACHE | FGHI_PIPELINE_VSHADER | FGHI_PIPELINE_PRIM_ENG |\
	FGHI_PIPELINE_TRI_ENG | FGHI_PIPELINE_RA_ENG | FGHI_PIPELINE_PSHADER | \
	FGHI_PIPELINE_PER_FRAG | FGHI_PIPELINE_CCACHE )

/* Functions */
uint32_t fimgGetPipelineStatus(fimgContext *ctx);
int fimgFlush(fimgContext *ctx);
int fimgSelectiveFlush(fimgContext *ctx, uint32_t mask);
int fimgInvalidateFlushCache(fimgContext *ctx,
			     unsigned int vtcclear, unsigned int tcclear,
			     unsigned int ccflush, unsigned int zcflush);
void fimgFinish(fimgContext *ctx);
void fimgSoftReset(fimgContext *ctx);
void fimgGetVersion(fimgContext *ctx, int *major, int *minor, int *rev);
unsigned int fimgGetInterrupt(fimgContext *ctx);
void fimgClearInterrupt(fimgContext *ctx);
void fimgEnableInterrupt(fimgContext *ctx);
void fimgDisableInterrupt(fimgContext *ctx);
void fimgSetInterruptBlock(fimgContext *ctx, uint32_t pipeMask);
void fimgSetInterruptState(fimgContext *ctx, uint32_t status);
uint32_t fimgGetInterruptState(fimgContext *ctx);

/*
 * Host interface
 */

#define FIMG_ATTRIB_NUM			8

/* Type definitions */
#define FGHI_NUMCOMP(i)		((i) - 1)

typedef enum {
	FGHI_ATTRIB_DT_BYTE = 0,
	FGHI_ATTRIB_DT_SHORT,
	FGHI_ATTRIB_DT_INT,
	FGHI_ATTRIB_DT_FIXED,
	FGHI_ATTRIB_DT_UBYTE,
	FGHI_ATTRIB_DT_USHORT,
	FGHI_ATTRIB_DT_UINT,
	FGHI_ATTRIB_DT_FLOAT,
	FGHI_ATTRIB_DT_NBYTE,
	FGHI_ATTRIB_DT_NSHORT,
	FGHI_ATTRIB_DT_NINT,
	FGHI_ATTRIB_DT_NFIXED,
	FGHI_ATTRIB_DT_NUBYTE,
	FGHI_ATTRIB_DT_NUSHORT,
	FGHI_ATTRIB_DT_NUINT,
	FGHI_ATTRIB_DT_HALF_FLOAT
} fimgHostDataType;

typedef struct {
	const void	*pointer;
	unsigned int	stride;
	unsigned int	width;
} fimgArray;

/* Functions */
void fimgDrawArrays(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
					unsigned int first, unsigned int count);
void fimgDrawElementsUByteIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint8_t *indices);
void fimgDrawElementsUShortIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint16_t *indices);
void fimgDrawArraysBuffered(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
					unsigned int first, unsigned int count);
void fimgDrawElementsBufferedUByteIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint8_t *indices);
void fimgDrawElementsBufferedUShortIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint16_t *indices);
void fimgSetAttribute(fimgContext *ctx,
		      unsigned int idx,
		      unsigned int type,
		      unsigned int numComp);
void fimgSetAttribCount(fimgContext *ctx, unsigned char count);

/*
 * Primitive Engine
 */

/* Type definitions */
typedef enum {
	FGPE_TRIANGLES		= (1 << 7),
	FGPE_TRIANGLE_FAN	= (1 << 6),
	FGPE_TRIANGLE_STRIP	= (1 << 5),
	FGPE_LINES		= (1 << 4),
	FGPE_LINE_LOOP		= (1 << 3),
	FGPE_LINE_STRIP		= (1 << 2),
	FGPE_POINTS		= (1 << 1),
	FGPE_POINT_SPRITE	= (1 << 0)
} fimgPrimitiveType;

/* Functions */
void fimgSetVertexContext(fimgContext *ctx, unsigned int type);
void fimgSetShadingMode(fimgContext *ctx, int en, unsigned attrib);
void fimgSetViewportParams(fimgContext *ctx, float x0, float y0,
							float px, float py);
void fimgSetDepthRange(fimgContext *ctx, float n, float f);

enum {
	FIMG_VIEWPORT_X,
	FIMG_VIEWPORT_Y,
	FIMG_VIEWPORT_W,
	FIMG_VIEWPORT_H,
	FIMG_DEPTH_RANGE_NEAR,
	FIMG_DEPTH_RANGE_FAR
};

float fimgGetPrimitiveStateF(fimgContext *ctx, unsigned int name);

/*
 * Raster engine
 */

/* Type definitions */
typedef enum {
	FGRA_BFCULL_FACE_BACK = 0,
	FGRA_BFCULL_FACE_FRONT,
	FGRA_BFCULL_FACE_BOTH = 3
} fimgCullingFace;

/* Functions */
void fimgSetPixelSamplePos(fimgContext *ctx, int corner);
void fimgEnableDepthOffset(fimgContext *ctx, int enable);
void fimgSetDepthOffsetParam(fimgContext *ctx, float factor, float units);
void fimgSetFaceCullEnable(fimgContext *ctx, int enable);
void fimgSetFaceCullFace(fimgContext *ctx, unsigned int face);
void fimgSetFaceCullFront(fimgContext *ctx, int bCW);
void fimgSetFaceCullControl(fimgContext *ctx, int bCW,fimgCullingFace face);
void fimgSetYClip(fimgContext *ctx, unsigned int ymin, unsigned int ymax);
void fimgSetLODControl(fimgContext *ctx, unsigned int attrib,
					int lod, int ddx, int ddy);
void fimgSetXClip(fimgContext *ctx, unsigned int xmin, unsigned int xmax);
void fimgSetPointWidth(fimgContext *ctx, float pWidth);
void fimgSetMinimumPointWidth(fimgContext *ctx, float pWidthMin);
void fimgSetMaximumPointWidth(fimgContext *ctx, float pWidthMax);
void fimgSetCoordReplace(fimgContext *ctx, unsigned int coordReplaceNum);
void fimgSetLineWidth(fimgContext *ctx, float lWidth);

enum {
	FIMG_CULL_FACE_EN,
	FIMG_DEPTH_OFFSET_EN,
	FIMG_POINT_SIZE,
	FIMG_LINE_WIDTH,
	FIMG_CULL_FACE_MODE,
	FIMG_FRONT_FACE,
	FIMG_DEPTH_OFFSET_FACTOR,
	FIMG_DEPTH_OFFSET_UNITS
};

float fimgGetRasterizerStateF(fimgContext *ctx, unsigned int name);
unsigned int fimgGetRasterizerState(fimgContext *ctx, unsigned int name);

/*
 * Shaders
 */

#ifndef FIMG_FIXED_PIPELINE
/* Vertex shader functions */
int fimgLoadVShader(fimgContext *ctx,
		    const unsigned int *pShaderCode, unsigned int numAttribs);
/* Pixel shader functions */
int fimgLoadPShader(fimgContext *ctx,
		    const unsigned int *pShaderCode, unsigned int numAttribs);
#endif

/*
 * Texture engine
 */

#define FGTU_MAX_MIPMAP_LEVEL	11

/* Type definitions */
enum {
	FGTU_TSTA_TYPE_2D = 1,
	FGTU_TSTA_TYPE_CUBE,
	FGTU_TSTA_TYPE_3D
};

enum {
	FGTU_TSTA_TEX_EXP_DUP = 0,
	FGTU_TSTA_TEX_EXP_ZERO
};

enum {
	FGTU_TSTA_AFORMAT_ARGB = 0,
	FGTU_TSTA_AFORMAT_RGBA
};

enum {
	FGTU_TSTA_PAL_TEX_FORMAT_1555 = 0,
	FGTU_TSTA_PAL_TEX_FORMAT_565,
	FGTU_TSTA_PAL_TEX_FORMAT_4444,
	FGTU_TSTA_PAL_TEX_FORMAT_8888
};

enum {
	FGTU_TSTA_TEXTURE_FORMAT_1555 = 0,
	FGTU_TSTA_TEXTURE_FORMAT_565,
	FGTU_TSTA_TEXTURE_FORMAT_4444,
	FGTU_TSTA_TEXTURE_FORMAT_DEPTHCOMP16,
	FGTU_TSTA_TEXTURE_FORMAT_88,
	FGTU_TSTA_TEXTURE_FORMAT_8,
	FGTU_TSTA_TEXTURE_FORMAT_8888,
	FGTU_TSTA_TEXTURE_FORMAT_1BPP,
	FGTU_TSTA_TEXTURE_FORMAT_2BPP,
	FGTU_TSTA_TEXTURE_FORMAT_4BPP,
	FGTU_TSTA_TEXTURE_FORMAT_8BPP,
	FGTU_TSTA_TEXTURE_FORMAT_S3TC,
	FGTU_TSTA_TEXTURE_FORMAT_Y1VY0U,
	FGTU_TSTA_TEXTURE_FORMAT_VY1UY0,
	FGTU_TSTA_TEXTURE_FORMAT_Y1UY0V,
	FGTU_TSTA_TEXTURE_FORMAT_UY1VY0
};

enum {
	FGTU_TSTA_ADDR_MODE_REPEAT = 0,
	FGTU_TSTA_ADDR_MODE_FLIP,
	FGTU_TSTA_ADDR_MODE_CLAMP
};

enum {
	FGTU_TSTA_TEX_COOR_PARAM = 0,
	FGTU_TSTA_TEX_COOR_NON_PARAM
};

enum {
	FGTU_TSTA_FILTER_NEAREST = 0,
	FGTU_TSTA_FILTER_LINEAR
};

enum {
	FGTU_TSTA_MIPMAP_DISABLED = 0,
	FGTU_TSTA_MIPMAP_NEAREST,
	FGTU_TSTA_MIPMAP_LINEAR
};

enum {
	FGTU_VTSTA_MOD_REPEAT = 0,
	FGTU_VTSTA_MOD_FLIP,
	FGTU_VTSTA_MOD_CLAMP
};

struct _fimgTexture;
typedef struct _fimgTexture fimgTexture;

/* Functions */
fimgTexture *fimgCreateTexture(void);
void fimgDestroyTexture(fimgTexture *texture);
void fimgInitTexture(fimgTexture *texture, unsigned int format,
			unsigned int maxLevel, unsigned long addr);
void fimgSetTexMipmapOffset(fimgTexture *texture, unsigned int level,
						unsigned int offset);
unsigned int fimgGetTexMipmapOffset(fimgTexture *texture, unsigned level);
void fimgSetupTexture(fimgContext *ctx, fimgTexture *texture, unsigned unit);
void fimgSetTexMipmapLevel(fimgTexture *texture, int level);
void fimgSetTexBaseAddr(fimgTexture *texture, unsigned int addr);
void fimgSetTex2DSize(fimgTexture *texture,
	unsigned int uSize, unsigned int vSize);
void fimgSetTex3DSize(fimgTexture *texture, unsigned int vSize,
				unsigned int uSize, unsigned int pSize);
void fimgSetTexUAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexVAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexPAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexMinFilter(fimgTexture *texture, unsigned mode);
void fimgSetTexMagFilter(fimgTexture *texture, unsigned mode);
void fimgSetTexMipmap(fimgTexture *texture, unsigned mode);
void fimgSetTexCoordSys(fimgTexture *texture, unsigned mode);

/*
 * OpenGL 1.1 compatibility
 */

#ifdef FIMG_FIXED_PIPELINE

#define FIMG_NUM_TEXTURE_UNITS	2

typedef enum {
	FGFP_MATRIX_TRANSFORM = 0,
	FGFP_MATRIX_LIGHTING,
	FGFP_MATRIX_TEXTURE
} fimgMatrix;
#define FGFP_MATRIX_TEXTURE(i)	(FGFP_MATRIX_TEXTURE + (i))

typedef enum {
	FGFP_TEXFUNC_REPLACE = 0,
	FGFP_TEXFUNC_MODULATE,
	FGFP_TEXFUNC_DECAL,
	FGFP_TEXFUNC_BLEND,
	FGFP_TEXFUNC_ADD,
	FGFP_TEXFUNC_COMBINE
} fimgTexFunc;

typedef enum {
	FGFP_COMBFUNC_REPLACE = 0,
	FGFP_COMBFUNC_MODULATE,
	FGFP_COMBFUNC_ADD,
	FGFP_COMBFUNC_ADD_SIGNED,
	FGFP_COMBFUNC_INTERPOLATE,
	FGFP_COMBFUNC_SUBTRACT,
	FGFP_COMBFUNC_DOT3_RGB,
	FGFP_COMBFUNC_DOT3_RGBA
} fimgCombFunc;

typedef enum {
	FGFP_COMBARG_TEX = 0,
	FGFP_COMBARG_CONST,
	FGFP_COMBARG_COL,
	FGFP_COMBARG_PREV
} fimgCombArgSrc;

typedef enum {
	FGFP_COMBARG_SRC_COLOR = 0,
	FGFP_COMBARG_ONE_MINUS_SRC_COLOR,
	FGFP_COMBARG_SRC_ALPHA,
	FGFP_COMBARG_ONE_MINUS_SRC_ALPHA
} fimgCombArgMod;

void fimgLoadMatrix(fimgContext *ctx, unsigned int matrix, const float *pData);
void fimgEnableTexture(fimgContext *ctx, unsigned int unit);
void fimgDisableTexture(fimgContext *ctx, unsigned int unit);
void fimgCompatLoadPixelShader(fimgContext *ctx);
void fimgCompatSetTextureEnable(fimgContext *ctx, unsigned unit, int enable);
void fimgCompatSetTextureFunc(fimgContext *ctx, unsigned unit, fimgTexFunc func);
void fimgCompatSetColorCombiner(fimgContext *ctx, unsigned unit,
							fimgCombFunc func);
void fimgCompatSetAlphaCombiner(fimgContext *ctx, unsigned unit,
							fimgCombFunc func);
void fimgCompatSetColorCombineArgSrc(fimgContext *ctx, unsigned unit,
					unsigned arg, fimgCombArgSrc src);
void fimgCompatSetColorCombineArgMod(fimgContext *ctx, unsigned unit,
					unsigned arg, fimgCombArgMod mod);
void fimgCompatSetAlphaCombineArgSrc(fimgContext *ctx, unsigned unit,
					unsigned arg, fimgCombArgSrc src);
void fimgCompatSetAlphaCombineArgMod(fimgContext *ctx, unsigned unit,
					unsigned arg, fimgCombArgMod src);
void fimgCompatSetColorScale(fimgContext *ctx, unsigned unit, float scale);
void fimgCompatSetAlphaScale(fimgContext *ctx, unsigned unit, float scale);
void fimgCompatSetEnvColor(fimgContext *ctx, unsigned unit,
					float r, float g, float b, float a);
void fimgCompatSetupTexture(fimgContext *ctx, fimgTexture *tex,
						uint32_t unit, int swap);

#endif

enum {
	FGFP_CLEAR_COLOR	= 1,
	FGFP_CLEAR_DEPTH	= 2,
	FGFP_CLEAR_STENCIL	= 4
};

void fimgClear(fimgContext *ctx, uint32_t mode);
void fimgSetClearColor(fimgContext *ctx, float red, float green,
						float blue, float alpha);
void fimgSetClearDepth(fimgContext *ctx, float depth);
void fimgSetClearStencil(fimgContext *ctx, uint8_t stencil);

/*
 * Per-fragment unit
 */

/* Type definitions */
typedef enum {
	FGPF_TEST_MODE_NEVER = 0,
	FGPF_TEST_MODE_ALWAYS,
	FGPF_TEST_MODE_LESS,
	FGPF_TEST_MODE_LEQUAL,
	FGPF_TEST_MODE_EQUAL,
	FGPF_TEST_MODE_GREATER,
	FGPF_TEST_MODE_GEQUAL,
	FGPF_TEST_MODE_NOTEQUAL
} fimgTestMode;

/**
 *	WORKAROUND
 *	Inconsistent with fimgTestMode due to hardware bug
 */
typedef enum {
	FGPF_STENCIL_MODE_NEVER = 0,
	FGPF_STENCIL_MODE_ALWAYS,
	FGPF_STENCIL_MODE_GREATER,
	FGPF_STENCIL_MODE_GEQUAL,
	FGPF_STENCIL_MODE_EQUAL,
	FGPF_STENCIL_MODE_LESS,
	FGPF_STENCIL_MODE_LEQUAL,
	FGPF_STENCIL_MODE_NOTEQUAL
} fimgStencilMode;

typedef enum {
	FGPF_TEST_ACTION_KEEP = 0,
	FGPF_TEST_ACTION_ZERO,
	FGPF_TEST_ACTION_REPLACE,
	FGPF_TEST_ACTION_INCR,
	FGPF_TEST_ACTION_DECR,
	FGPF_TEST_ACTION_INVERT,
	FGPF_TEST_ACTION_INCR_WRAP,
	FGPF_TEST_ACTION_DECR_WRAP
} fimgTestAction;

typedef enum {
	FGPF_BLEND_EQUATION_ADD = 0,
	FGPF_BLEND_EQUATION_SUB,
	FGPF_BLEND_EQUATION_REVSUB,
	FGPF_BLEND_EQUATION_MIN,
	FGPF_BLEND_EQUATION_MAX
} fimgBlendEquation;

typedef enum {
	FGPF_BLEND_FUNC_ZERO = 0,
	FGPF_BLEND_FUNC_ONE,
	FGPF_BLEND_FUNC_SRC_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_SRC_COLOR,
	FGPF_BLEND_FUNC_DST_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_DST_COLOR,
	FGPF_BLEND_FUNC_SRC_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
	FGPF_BLEND_FUNC_DST_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA,
	FGPF_BLEND_FUNC_CONST_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_CONST_COLOR,
	FGPF_BLEND_FUNC_CONST_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_CONST_ALPHA,
	FGPF_BLEND_FUNC_SRC_ALPHA_SATURATE
} fimgBlendFunction;

typedef enum {
	FGPF_LOGOP_CLEAR = 0,
	FGPF_LOGOP_AND,
	FGPF_LOGOP_AND_REVERSE,
	FGPF_LOGOP_COPY,
	FGPF_LOGOP_AND_INVERTED,
	FGPF_LOGOP_NOOP,
	FGPF_LOGOP_XOR,
	FGPF_LOGOP_OR,
	FGPF_LOGOP_NOR,
	FGPF_LOGOP_EQUIV,
	FGPF_LOGOP_INVERT,
	FGPF_LOGOP_OR_REVERSE,
	FGPF_LOGOP_COPY_INVERTED,
	FGPF_LOGOP_OR_INVERTED,
	FGPF_LOGOP_NAND,
	FGPF_LOGOP_SET
} fimgLogicalOperation;

typedef enum {
	FGPF_COLOR_MODE_555 = 0,
	FGPF_COLOR_MODE_565,
	FGPF_COLOR_MODE_4444,
	FGPF_COLOR_MODE_1555,
	FGPF_COLOR_MODE_0888,
	FGPF_COLOR_MODE_8888
} fimgColorMode;

/* Functions */
void fimgSetScissorParams(fimgContext *ctx,
			  unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin);
void fimgSetScissorEnable(fimgContext *ctx, int enable);
void fimgSetAlphaParams(fimgContext *ctx, unsigned int refAlpha,
			fimgTestMode mode);
void fimgSetAlphaEnable(fimgContext *ctx, int enable);
void fimgSetFrontStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			     unsigned char ref, unsigned char mask);
void fimgSetFrontStencilOp(fimgContext *ctx, fimgTestAction sfail,
			   fimgTestAction dpfail, fimgTestAction dppass);
void fimgSetBackStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			    unsigned char ref, unsigned char mask);
void fimgSetBackStencilOp(fimgContext *ctx, fimgTestAction sfail,
			  fimgTestAction dpfail, fimgTestAction dppass);
void fimgSetStencilEnable(fimgContext *ctx, int enable);
void fimgSetDepthParams(fimgContext *ctx, fimgTestMode mode);
void fimgSetDepthEnable(fimgContext *ctx, int enable);
void fimgSetBlendEquation(fimgContext *ctx,
			  fimgBlendEquation alpha, fimgBlendEquation color);
void fimgSetBlendFunc(fimgContext *ctx,
		      fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendFuncNoAlpha(fimgContext *ctx,
			    fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
			    fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendEnable(fimgContext *ctx, int enable);
void fimgSetBlendColor(fimgContext *ctx, unsigned int blendColor);
void fimgSetDitherEnable(fimgContext *ctx, int enable);
void fimgSetLogicalOpParams(fimgContext *ctx, fimgLogicalOperation alpha,
			    fimgLogicalOperation color);
void fimgSetLogicalOpEnable(fimgContext *ctx, int enable);
void fimgSetColorBufWriteMask(fimgContext *ctx, int r, int g, int b, int a);
void fimgSetStencilBufWriteMask(fimgContext *ctx, int back, unsigned char mask);
void fimgSetZBufWriteMask(fimgContext *ctx, int enable);
void fimgSetFrameBufParams(fimgContext *ctx,
			   int opaqueAlpha, unsigned int thresholdAlpha,
			   unsigned int constAlpha, fimgColorMode format);
void fimgSetZBufBaseAddr(fimgContext *ctx, unsigned int addr);
void fimgSetColorBufBaseAddr(fimgContext *ctx, unsigned int addr);
void fimgSetFrameBufSize(fimgContext *ctx,
				unsigned int width, unsigned int height);

enum {
	FIMG_SCISSOR_TEST,
	FIMG_ALPHA_TEST,
	FIMG_STENCIL_TEST,
	FIMG_DEPTH_TEST,
	FIMG_BLEND,
	FIMG_DITHER,
	FIMG_COLOR_LOGIC_OP,
	FIMG_FRONT_STENCIL_FUNC,
	FIMG_FRONT_STENCIL_MASK,
	FIMG_FRONT_STENCIL_REF,
	FIMG_FRONT_STENCIL_SFAIL,
	FIMG_FRONT_STENCIL_DPFAIL,
	FIMG_FRONT_STENCIL_DPPASS,
	FIMG_BACK_STENCIL_FUNC,
	FIMG_BACK_STENCIL_MASK,
	FIMG_BACK_STENCIL_REF,
	FIMG_BACK_STENCIL_SFAIL,
	FIMG_BACK_STENCIL_DPFAIL,
	FIMG_BACK_STENCIL_DPPASS,
	FIMG_DEPTH_FUNC
};

unsigned int fimgGetFragmentState(fimgContext *ctx, unsigned int op);

/*
 * OS support
 */

fimgContext *fimgCreateContext(void);
void fimgDestroyContext(fimgContext *ctx);
void fimgRestoreContext(fimgContext *ctx);
int fimgAcquireHardwareLock(fimgContext *ctx);
int fimgReleaseHardwareLock(fimgContext *ctx);
int fimgDeviceOpen(fimgContext *ctx);
void fimgDeviceClose(fimgContext *ctx);
int fimgWaitForFlush(fimgContext *ctx, uint32_t target);

//=============================================================================

#ifdef __cplusplus
}
#endif

#endif /* _FIMG_H_ */
