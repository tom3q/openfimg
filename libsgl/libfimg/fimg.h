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

/** Flush vertex FIFO. */
#define FGHI_PIPELINE_FIFO	(1 << 0)
/** Flush host interface. */
#define FGHI_PIPELINE_HOSTIF	(1 << 1)
/** Flush vertex shader FIFO. */
#define FGHI_PIPELINE_HVF	(1 << 2)
/** Flush vertex cache. */
#define FGHI_PIPELINE_VCACHE	(1 << 3)
/** Flush vertex shader. */
#define FGHI_PIPELINE_VSHADER	(1 << 4)
/** Flush primitive engine. */
#define FGHI_PIPELINE_PRIM_ENG	(1 << 8)
/** Flush triangle setup engine. */
#define FGHI_PIPELINE_TRI_ENG	(1 << 9)
/** Flush rasterizer. */
#define FGHI_PIPELINE_RA_ENG	(1 << 10)
/** Flush pixel shader. */
#define FGHI_PIPELINE_PSHADER	(1 << 12)
/** Flush per-fragment unit. */
#define FGHI_PIPELINE_PER_FRAG	(1 << 16)
/** Flush color cache. */
#define FGHI_PIPELINE_CCACHE	(1 << 18)

/** Flush complete rendering pipeline without color cache. */
#define FGHI_PIPELINE_ALL ( \
	FGHI_PIPELINE_FIFO | FGHI_PIPELINE_HOSTIF | FGHI_PIPELINE_HVF | \
	FGHI_PIPELINE_VCACHE | FGHI_PIPELINE_VSHADER | FGHI_PIPELINE_PRIM_ENG |\
	FGHI_PIPELINE_TRI_ENG | FGHI_PIPELINE_RA_ENG | FGHI_PIPELINE_PSHADER | \
	FGHI_PIPELINE_PER_FRAG )

/* Functions */
uint32_t fimgGetPipelineStatus(fimgContext *ctx);
int fimgFlush(fimgContext *ctx);
int fimgSelectiveFlush(fimgContext *ctx, uint32_t mask);
int fimgInvalidateCache(fimgContext *ctx,
				unsigned int vtcclear, unsigned int tcclear);
int fimgFlushCache(fimgContext *ctx,
				unsigned int ccflush, unsigned int zcflush);
int fimgWaitForCacheFlush(fimgContext *ctx,
				unsigned int ccflush, unsigned int zcflush);
void fimgFinish(fimgContext *ctx);
void fimgSoftReset(fimgContext *ctx);
void fimgGetVersion(fimgContext *ctx, int *major, int *minor, int *rev);

/*
 * Host interface
 */

/** Number of attributes supported by libfimg. */
#define FIMG_ATTRIB_NUM		9

/* Type definitions */
/**
 * Calculates NUMCOMP value based on number of components.
 * @param i Number of components.
 * @return NUMCOMP value.
 */
#define FGHI_NUMCOMP(i)		((i) - 1)

/** Vertex attribute data types supported by FIMG-3DSE. */
typedef enum {
	FGHI_ATTRIB_DT_BYTE = 0,	/**< 8-bit signed. */
	FGHI_ATTRIB_DT_SHORT,		/**< 16-bit signed. */
	FGHI_ATTRIB_DT_INT,		/**< 32-bit signed. */
	FGHI_ATTRIB_DT_FIXED,		/**< 16.16 fixed point. */
	FGHI_ATTRIB_DT_UBYTE,		/**< 8-bit unsigned. */
	FGHI_ATTRIB_DT_USHORT,		/**< 16-bit unsigned. */
	FGHI_ATTRIB_DT_UINT,		/**< 32-bit unsigned. */
	FGHI_ATTRIB_DT_FLOAT,		/**< 1/8/23 floating point. */
	FGHI_ATTRIB_DT_NBYTE,		/**< 8-bit normalized signed. */
	FGHI_ATTRIB_DT_NSHORT,		/**< 16-bit normalized signed. */
	FGHI_ATTRIB_DT_NINT,		/**< 32-bit normalized signed. */
	FGHI_ATTRIB_DT_NFIXED,		/**< 16.16 normalized fixed point. */
	FGHI_ATTRIB_DT_NUBYTE,		/**< 8-bit normalized unsigned. */
	FGHI_ATTRIB_DT_NUSHORT,		/**< 16-bit normalized unsigned. */
	FGHI_ATTRIB_DT_NUINT,		/**< 32-bit normalized unsigned. */
	FGHI_ATTRIB_DT_HALF_FLOAT	/**< 1/5/10 floating point. */
} fimgHostDataType;

/** Structure describing layout of vertex attribute array. */
typedef struct {
	/** Pointer to vertex attribute data. */
	const void	*pointer;
	/** Stride of single vertex. */
	uint16_t	stride;
	/** Width of single vertex. */
	uint16_t	width;
} fimgArray;

/* Functions */
void fimgDrawArrays(fimgContext *ctx, unsigned int mode,
					fimgArray *arrays, unsigned int count);
void fimgDrawElementsUByteIdx(fimgContext *ctx, unsigned int mode,
		fimgArray *arrays, unsigned int count, const uint8_t *indices);
void fimgDrawElementsUShortIdx(fimgContext *ctx, unsigned int mode,
		fimgArray *arrays, unsigned int count, const uint16_t *indices);
void fimgSetAttribute(fimgContext *ctx,
		      unsigned int idx,
		      unsigned int type,
		      unsigned int numComp);
void fimgSetAttribCount(fimgContext *ctx, unsigned char count);

/*
 * Primitive Engine
 */

/* Type definitions */

/** Primitive types supported by FIMG-3DSE. */
typedef enum {
	FGPE_POINT_SPRITE = 0,	/**< Point sprites */
	FGPE_POINTS,		/**< Separate points */
	FGPE_LINE_STRIP,	/**< Line strips */
	FGPE_LINE_LOOP,		/**< Line loops */
	FGPE_LINES,		/**< Separate lines */
	FGPE_TRIANGLE_STRIP,	/**< Triangle strips */
	FGPE_TRIANGLE_FAN,	/**< Triangle fans */
	FGPE_TRIANGLES,		/**< Separate triangles */
	FGPE_PRIMITIVE_MAX,
} fimgPrimitiveType;

/* Functions */
void fimgSetVertexContext(fimgContext *ctx, unsigned int type);
void fimgSetShadingMode(fimgContext *ctx, int en, unsigned attrib);
void fimgSetViewportParams(fimgContext *ctx, float x0, float y0,
							float px, float py);
void fimgSetViewportBypass(fimgContext *ctx);
void fimgSetDepthRange(fimgContext *ctx, float n, float f);

/*
 * Raster engine
 */

/* Type definitions */

/** Polygon face for face culling. */
typedef enum {
	FGRA_BFCULL_FACE_BACK = 0,	/**< Cull back face */
	FGRA_BFCULL_FACE_FRONT,		/**< Cull front face */
	FGRA_BFCULL_FACE_BOTH = 3	/**< Cull both faces */
} fimgCullingFace;

/* Functions */
void fimgSetPixelSamplePos(fimgContext *ctx, int corner);
void fimgEnableDepthOffset(fimgContext *ctx, int enable);
void fimgSetDepthOffsetParam(fimgContext *ctx, float factor, float units);
void fimgSetFaceCullEnable(fimgContext *ctx, int enable);
void fimgSetFaceCullFace(fimgContext *ctx, unsigned int face);
void fimgSetFaceCullFront(fimgContext *ctx, int bCW);
void fimgSetYClip(fimgContext *ctx, unsigned int ymin, unsigned int ymax);
void fimgSetLODControl(fimgContext *ctx, unsigned int attrib,
					int lod, int ddx, int ddy);
void fimgSetXClip(fimgContext *ctx, unsigned int xmin, unsigned int xmax);
void fimgSetPointWidth(fimgContext *ctx, float pWidth);
void fimgSetMinimumPointWidth(fimgContext *ctx, float pWidthMin);
void fimgSetMaximumPointWidth(fimgContext *ctx, float pWidthMax);
void fimgSetCoordReplace(fimgContext *ctx, unsigned int coordReplaceNum);
void fimgSetLineWidth(fimgContext *ctx, float lWidth);

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

/** Max. mipmap level supported by FIMG-3DSE. */
#define FGTU_MAX_MIPMAP_LEVEL	11

/* Type definitions */

/** Texture type. */
enum {
	FGTU_TSTA_TYPE_2D = 1,	/**< 2D texture */
	FGTU_TSTA_TYPE_CUBE,	/**< Cube map texture */
	FGTU_TSTA_TYPE_3D	/**< 3D texture */
};

/** Texture color expansion mode. */
enum {
	FGTU_TSTA_TEX_EXP_DUP = 0,	/**< Duplicate LSB. */
	FGTU_TSTA_TEX_EXP_ZERO		/**< Expand with zeroes. */
};

/** Texture alpha component position. */
enum {
	FGTU_TSTA_AFORMAT_ARGB = 0,	/**< Alpha component in MSB/MSb. */
	FGTU_TSTA_AFORMAT_RGBA		/**< Alpha component in LSB/LSb. */
};

/** Texture palette format. */
enum {
	FGTU_TSTA_PAL_TEX_FORMAT_1555 = 0,
	FGTU_TSTA_PAL_TEX_FORMAT_565,
	FGTU_TSTA_PAL_TEX_FORMAT_4444,
	FGTU_TSTA_PAL_TEX_FORMAT_8888
};

/** Texture pixel format. */
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

/** Texture addressing mode. */
enum {
	FGTU_TSTA_ADDR_MODE_REPEAT = 0,
	FGTU_TSTA_ADDR_MODE_FLIP,
	FGTU_TSTA_ADDR_MODE_CLAMP
};

/** Texture coordinate calculation mode. */
enum {
	FGTU_TSTA_TEX_COOR_PARAM = 0,	/**< Parametric (STRQ) */
	FGTU_TSTA_TEX_COOR_NON_PARAM	/**< Non-parametric (direct) */
};

/** Texture filtering mode. */
enum {
	FGTU_TSTA_FILTER_NEAREST = 0,
	FGTU_TSTA_FILTER_LINEAR
};

/** Texture mipmap mode. */
enum {
	FGTU_TSTA_MIPMAP_DISABLED = 0,
	FGTU_TSTA_MIPMAP_NEAREST,
	FGTU_TSTA_MIPMAP_LINEAR
};

/** Vertex texture addressing mode. */
enum {
	FGTU_VTSTA_MOD_REPEAT = 0,
	FGTU_VTSTA_MOD_FLIP,
	FGTU_VTSTA_MOD_CLAMP
};

/** Texture component order. */
enum {
	FGTU_TEX_RGBA	= (1 << 0),
	FGTU_TEX_BGR	= (1 << 1)
};

struct _fimgTexture;
typedef struct _fimgTexture fimgTexture;

/* Functions */
fimgTexture *fimgCreateTexture(void);
void fimgDestroyTexture(fimgTexture *texture);
void fimgInitTexture(fimgTexture *texture, unsigned int flags,
				unsigned int format, unsigned long addr);
void fimgSetTexMipmapOffset(fimgTexture *texture, unsigned int level,
						unsigned int offset);
unsigned int fimgGetTexMipmapOffset(fimgTexture *texture, unsigned level);
void fimgSetupTexture(fimgContext *ctx, fimgTexture *texture, unsigned unit);
void fimgSetTexMipmapLevel(fimgTexture *texture, int level);
void fimgSetTexBaseAddr(fimgTexture *texture, unsigned int addr);
void fimgSetTex2DSize(fimgTexture *texture,
		unsigned int uSize, unsigned int vSize, unsigned int maxLevel);
void fimgSetTex3DSize(fimgTexture *texture, unsigned int vSize,
				unsigned int uSize, unsigned int pSize);
void fimgSetTexUAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexVAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexPAddrMode(fimgTexture *texture, unsigned mode);
void fimgSetTexMinFilter(fimgTexture *texture, unsigned mode);
void fimgSetTexMagFilter(fimgTexture *texture, unsigned mode);
void fimgSetTexMipmap(fimgTexture *texture, unsigned mode);
void fimgSetTexCoordSys(fimgTexture *texture, unsigned mode);
void fimgInvalidateTextureCache(fimgContext *ctx);

/*
 * OpenGL 1.1 compatibility
 */

#ifdef FIMG_FIXED_PIPELINE

#define FIMG_NUM_TEXTURE_UNITS	2

/** Transformation matrices */
typedef enum {
	FGFP_MATRIX_TRANSFORM = 0,
	FGFP_MATRIX_LIGHTING,
	FGFP_MATRIX_TEXTURE
} fimgMatrix;
/**
 * Returns index of texture coordinate matrix of given texture unit.
 * @param i Texture unit index.
 * @return Index of matrix.
 */
#define FGFP_MATRIX_TEXTURE(i)	(FGFP_MATRIX_TEXTURE + (i))

/** Texturing functions. */
typedef enum {
	FGFP_TEXFUNC_NONE = 0,
	FGFP_TEXFUNC_REPLACE,
	FGFP_TEXFUNC_MODULATE,
	FGFP_TEXFUNC_DECAL,
	FGFP_TEXFUNC_BLEND,
	FGFP_TEXFUNC_ADD,
	FGFP_TEXFUNC_COMBINE
} fimgTexFunc;

/** Texture combiner modes. */
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

/** Texture combiner argument source. */
typedef enum {
	FGFP_COMBARG_TEX = 0,
	FGFP_COMBARG_CONST,
	FGFP_COMBARG_COL,
	FGFP_COMBARG_PREV
} fimgCombArgSrc;

/** Texture combiner argument mode. */
typedef enum {
	FGFP_COMBARG_SRC_COLOR = 0,
	FGFP_COMBARG_ONE_MINUS_SRC_COLOR,
	FGFP_COMBARG_SRC_ALPHA,
	FGFP_COMBARG_ONE_MINUS_SRC_ALPHA
} fimgCombArgMod;

void fimgLoadMatrix(fimgContext *ctx, uint32_t matrix, const float *pData);
void fimgCompatSetTextureFunc(fimgContext *ctx, uint32_t unit, fimgTexFunc func);
void fimgCompatSetColorCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func);
void fimgCompatSetAlphaCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func);
void fimgCompatSetColorCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src);
void fimgCompatSetColorCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod mod);
void fimgCompatSetAlphaCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src);
void fimgCompatSetAlphaCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod src);
void fimgCompatSetColorScale(fimgContext *ctx, uint32_t unit, float scale);
void fimgCompatSetAlphaScale(fimgContext *ctx, uint32_t unit, float scale);
void fimgCompatSetEnvColor(fimgContext *ctx, uint32_t unit,
					float r, float g, float b, float a);
void fimgCompatSetupTexture(fimgContext *ctx, fimgTexture *tex, uint32_t unit);

#endif

/*
 * Per-fragment unit
 */

/* Type definitions */

/** Test modes. */
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

/*
 *	WORKAROUND
 *	Inconsistent with fimgTestMode due to hardware bug
 */

/** Stencil test modes. */
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

/** Test actions. */
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

/** Blend equations. */
typedef enum {
	FGPF_BLEND_EQUATION_ADD = 0,
	FGPF_BLEND_EQUATION_SUB,
	FGPF_BLEND_EQUATION_REVSUB,
	FGPF_BLEND_EQUATION_MIN,
	FGPF_BLEND_EQUATION_MAX
} fimgBlendEquation;

/** Blend functions. */
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

/** Pixel logical operations. */
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

/** Framebuffer color modes. */
typedef enum {
	FGPF_COLOR_MODE_555 = 0,
	FGPF_COLOR_MODE_565,
	FGPF_COLOR_MODE_4444,
	FGPF_COLOR_MODE_1555,
	FGPF_COLOR_MODE_0888,
	FGPF_COLOR_MODE_8888
} fimgColorMode;

/** Special framebuffer flags. */
enum {
	FGPF_COLOR_MODE_BGR = (1 << 1)	/**< Component order is BGR */
};

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
void fimgSetColorBufWriteMask(fimgContext *ctx, unsigned int mask);
void fimgSetStencilBufWriteMask(fimgContext *ctx, int back, unsigned char mask);
void fimgSetZBufWriteMask(fimgContext *ctx, int enable);
void fimgSetFrameBufParams(fimgContext *ctx,
				unsigned int flags, unsigned int format);
void fimgSetZBufBaseAddr(fimgContext *ctx, unsigned int addr);
void fimgSetColorBufBaseAddr(fimgContext *ctx, unsigned int addr);
void fimgSetFrameBufSize(fimgContext *ctx,
			unsigned int width, unsigned int height, int flipY);

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
