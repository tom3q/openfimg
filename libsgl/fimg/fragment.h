/*
 * fimg/fragment.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE PER-FRAGMENT UNIT RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _FRAGMENT_H_
#define _FRAGMENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

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
void fimgSetScissorParams(unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin);
void fimgSetScissorEnable(int enable);
void fimgSetAlphaParams(unsigned int refAlpha, fimgTestMode mode);
void fimgSetAlphaEnable(int enable);
void fimgSetFrontStencilFunc(fimgStencilMode mode, unsigned char ref,
			     unsigned char mask);
void fimgSetFrontStencilOp(fimgTestAction sfail, fimgTestAction dpfail,
			   fimgTestAction dppass);
void fimgSetBackStencilFunc(fimgStencilMode mode, unsigned char ref,
			    unsigned char mask);
void fimgSetBackStencilOp(fimgTestAction sfail, fimgTestAction dpfail,
			  fimgTestAction dppass);
void fimgSetStencilEnable(int enable);
void fimgSetDepthParams(fimgTestMode mode);
void fimgSetDepthEnable(int enable);
void fimgSetBlendEquation(fimgBlendEquation alpha, fimgBlendEquation color);
void fimgSetBlendFunc(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendFuncRGB565(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
			    fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendEnable(int enable);
void fimgSetBlendColor(unsigned int blendColor);
void fimgSetDitherEnable(int enable);
void fimgSetLogicalOpParams(fimgLogicalOperation alpha, fimgLogicalOperation color);
void fimgSetLogicalOpEnable(int enable);
void fimgSetColorBufWriteMask(int r, int g, int b, int a);
#if TARGET_FIMG_VERSION != _FIMG3DSE_VER_1_2
void fimgSetStencilBufWriteMask(int back, unsigned int mask);
#endif
void fimgSetZBufWriteMask(int enable);
void fimgSetFrameBufParams(int opaqueAlpha, unsigned int thresholdAlpha,
			   unsigned int constAlpha, fimgColorMode format);
void fimgSetZBufBaseAddr(unsigned int addr);
void fimgSetColorBufBaseAddr(unsigned int addr);
void fimgSetFrameBufWidth(unsigned int width);

/* Inline functions */

#ifdef __cplusplus
}
#endif

#endif
