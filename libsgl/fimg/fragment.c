/*
* fimg/fragment.c
*
* SAMSUNG S3C6410 FIMG-3DSE PER FRAGMENT UNIT RELATED FUNCTIONS
*
* Copyrights:	2005 by Samsung Electronics Limited (original code)
*		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
*/

#include "fimg.h"

#define FRAGMENT_OFFSET		0x70000

#define FGPF_SCISSOR_X		FRAGMENT_ADDR(0x00)
#define FGPF_SCISSOR_Y		FRAGMENT_ADDR(0x04)
#define FGPF_ALPHAT		FRAGMENT_ADDR(0x08)
#define FGPF_FRONTST		FRAGMENT_ADDR(0x0c)
#define FGPF_BACKST		FRAGMENT_ADDR(0x10)
#define FGPF_DEPTHT		FRAGMENT_ADDR(0x14)
#define FGPF_CCLR		FRAGMENT_ADDR(0x18)
#define FGPF_BLEND		FRAGMENT_ADDR(0x1c)
#define FGPF_LOGOP		FRAGMENT_ADDR(0x20)
#define FGPF_CBMSK		FRAGMENT_ADDR(0x24)
#define FGPF_DBMSK		FRAGMENT_ADDR(0x28)
#define FGPF_FBCTL		FRAGMENT_ADDR(0x2c)
#define FGPF_DBADDR		FRAGMENT_ADDR(0x30)
#define FGPF_CBADDR		FRAGMENT_ADDR(0x34)
#define FGPF_FBW		FRAGMENT_ADDR(0x38)

#define FRAGMENT_OFFS(reg)	(FRAGMENT_OFFSET + (reg))
#define FRAGMENT_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + FRAGMENT_OFFS(reg)))

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

static inline void fimgFragmentWrite(unsigned int data, volatile unsigned int *addr)
{
	*addr = data;
}

static inline unsigned int fimgFragmentRead(volatile unsigned int *addr)
{
	return *addr;
}

/* TODO: Function inlining */

/*****************************************************************************
* FUNCTIONS:	fimgSetScissorParams
* SYNOPSIS:	This function specifies an arbitary screen-aligned rectangle
*		outside of which fragments will be discarded.
* PARAMETERS:	[IN] xMax: the maximum x coordinates of scissor box.
*		[IN] xMin: the minimum x coordinates of scissor box.
*		[IN] yMax: the maximum y coordiantes of scissor box.
*		[IN] yMin: the minimum y coordinates of scissor box.
*****************************************************************************/
void fimgSetScissorParams(unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin)
{
	fimgScissorTestData scX, scY;

	scX.val = fimgFragmentRead(FGPF_SCISSOR_X);
	scX.bits.max = xMax;
	scX.bits.min = xMin;
	fimgFragmentWrite(scX.val, FGPF_SCISSOR_X);

	scY.val = 0;
	scY.bits.max = yMax;
	scY.bits.min = yMin;
	fimgFragmentWrite(scY.val, FGPF_SCISSOR_Y);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetScissorParams
* SYNOPSIS:	This function specifies an arbitary screen-aligned rectangle
*		outside of which fragments will be discarded.
* PARAMETERS:	[IN] enable - non-zero to enable scissor test
*****************************************************************************/
void fimgSetScissorEnable(int enable)
{
	fimgScissorTestData scX;

	scX.val = fimgFragmentRead(FGPF_SCISSOR_X);
	scX.bits.enable = !!enable;
	fimgFragmentWrite(scX.val, FGPF_SCISSOR_X);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetAlphaParams
* SYNOPSIS:	This function discards a fragment depending on the outcome of a
*          	comparison between the fragment's alpha value and a constant
*          	reference value.
* PARAMETERS:	[IN] refAlpha: The reference value to which incoming alpha values
*                    are compared. This value is clamped to the range 8bit value.
*		[IN] mode: The alpha comparison function.
*****************************************************************************/
void fimgSetAlphaParams(unsigned int refAlpha, fimgTestMode mode)
{
	fimgAlphaTestData alpha;

	alpha.val = fimgFragmentRead(FGPF_ALPHAT);
	alpha.bits.value = refAlpha;
	alpha.bits.mode = mode;
	fimgFragmentWrite(alpha.val, FGPF_ALPHAT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetAlphaEnable
* SYNOPSIS:	This function discards a fragment depending on the outcome of a
*		comparison between the fragment's alpha value and a constant
*		reference value.
* PARAMETERS:	[IN] enable - non-zero to enable alpha test
*****************************************************************************/
void fimgSetAlphaEnable(int enable)
{
	fimgAlphaTestData alpha;

	alpha.val = fimgFragmentRead(FGPF_ALPHAT);
	alpha.bits.enable = !!enable;
	fimgFragmentWrite(alpha.val, FGPF_ALPHAT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrontStencilFunc
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] mode - test comparison function
*		[IN] ref - reference value
*		[IN] mask - test mask
*****************************************************************************/
void fimgSetFrontStencilFunc(fimgStencilMode mode, unsigned char ref, unsigned char mask)
{
	fimgStencilTestData stencil;

	stencil.val = fimgFragmentRead(FGPF_FRONTST);
	stencil.bits.mode = mode;
	stencil.bits.ref = ref;
	stencil.bits.mask = mask;
	fimgFragmentWrite(stencil.val, FGPF_FRONTST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrontStencilOp
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] sfail - action to take if stencil test fails
*		[IN] dpfail - action to take if depth buffer test fails
*		[IN] dppass - action to take if depth buffer test passes
*****************************************************************************/
void fimgSetFrontStencilOp(fimgTestAction sfail, fimgTestAction dpfail, fimgTestAction dppass)
{
	fimgStencilTestData stencil;

	stencil.val = fimgFragmentRead(FGPF_FRONTST);
	stencil.bits.sfail = sfail;
	stencil.bits.dpfail = dpfail;
	stencil.bits.dppass = dppass;
	fimgFragmentWrite(stencil.val, FGPF_FRONTST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBackStencilFunc
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] mode - test comparison function
*		[IN] ref - reference value
*		[IN] mask - test mask
*****************************************************************************/
void fimgSetBackStencilFunc(fimgStencilMode mode, unsigned char ref, unsigned char mask)
{
	fimgStencilTestData stencil;

	stencil.val = fimgFragmentRead(FGPF_BACKST);
	stencil.bits.mode = mode;
	stencil.bits.ref = ref;
	stencil.bits.mask = mask;
	fimgFragmentWrite(stencil.val, FGPF_BACKST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBackStencilOp
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] sfail - action to take if stencil test fails
*		[IN] dpfail - action to take if depth buffer test fails
*		[IN] dppass - action to take if depth buffer test passes
*****************************************************************************/
void fimgSetBackStencilOp(fimgTestAction sfail, fimgTestAction dpfail, fimgTestAction dppass)
{
	fimgStencilTestData stencil;

	stencil.val = fimgFragmentRead(FGPF_BACKST);
	stencil.bits.sfail = sfail;
	stencil.bits.dpfail = dpfail;
	stencil.bits.dppass = dppass;
	fimgFragmentWrite(stencil.val, FGPF_BACKST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetStencilEnable
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] enable - non-zero to enable stencil test
*****************************************************************************/
void fimgSetStencilEnable(int enable)
{
	fimgStencilTestData stencil;

	stencil.val = fimgFragmentRead(FGPF_FRONTST);
	stencil.bits.enable = !!enable;
	fimgFragmentWrite(stencil.val, FGPF_FRONTST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthParams
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] mode: Specifies the depth-comparison function
*****************************************************************************/
void fimgSetDepthParams(fimgTestMode mode)
{
	fimgDepthTestData depth;

	depth.val = fimgFragmentRead(FGPF_DEPTHT);
	depth.bits.mode = mode;
	fimgFragmentWrite(depth.val, FGPF_DEPTHT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthEnable
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] enable: if non-zero, depth test is enabled
*****************************************************************************/
void fimgSetDepthEnable(int enable)
{
	fimgDepthTestData depth;

	depth.val = fimgFragmentRead(FGPF_DEPTHT);
	depth.bits.enable = !!enable;
	fimgFragmentWrite(depth.val, FGPF_DEPTHT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendEquation
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
* PARAMETERS:	[in] alpha - alpha blend equation
*		[in] color - color blend equation
*****************************************************************************/
void fimgSetBlendEquation(fimgBlendEquation alpha, fimgBlendEquation color)
{
	fimgBlendControl blend;

	blend.val = fimgFragmentRead(FGPF_BLEND);
	blend.bits.ablendequation = alpha;
	blend.bits.cblendequation = color;
	fimgFragmentWrite(blend.val, FGPF_BLEND);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendFunc
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
* PARAMETERS:	[in] srcAlpha - source alpha blend function
*		[in] srcColor - source color blend function
*		[in] dstAlpha - destination alpha blend function
*		[in] dstColor - destination color blend function
*****************************************************************************/
void fimgSetBlendFunc(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor)
{
	fimgBlendControl blend;

	blend.val = fimgFragmentRead(FGPF_BLEND);
	blend.bits.asrcblendfunc = srcAlpha;
	blend.bits.csrcblendfunc = srcColor;
	blend.bits.adstblendfunc = dstAlpha;
	blend.bits.cdstblendfunc = dstColor;
	fimgFragmentWrite(blend.val, FGPF_BLEND);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendFuncRGB565
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
*		(A variant with workaround for RGB565 mode.)
* PARAMETERS:	[IN] srcAlpha - source alpha blend function
*		[IN] srcColor - source color blend function
*		[IN] dstAlpha - destination alpha blend function
*		[IN] dstColor - destination color blend function
*****************************************************************************/
void fimgSetBlendFuncRGB565(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
			    fimgBlendFunction dstAlpha, fimgBlendFunction dstColor)
{
	switch(srcColor) {
	case FGPF_BLEND_FUNC_DST_ALPHA:
		srcColor = FGPF_BLEND_FUNC_ONE;
		break;
	case FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA:
		srcColor = FGPF_BLEND_FUNC_ZERO;
		break;
	case FGPF_BLEND_FUNC_SRC_ALPHA_SATURATE:
		srcColor = FGPF_BLEND_FUNC_ONE;
		break;
	default:
		break;
	}

	switch(dstColor) {
	case FGPF_BLEND_FUNC_DST_ALPHA:
		dstColor = FGPF_BLEND_FUNC_ONE;
		break;
	case FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA:
		dstColor = FGPF_BLEND_FUNC_ZERO;
		break;
	default:
		break;
	}

	fimgSetBlendFunc(srcAlpha, srcColor, dstAlpha, dstColor);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendEnable
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
* PARAMETERS:	[in] enable - non-zero to enable blending
*****************************************************************************/
void fimgSetBlendEnable(int enable)
{
	fimgBlendControl blend;

	blend.val = fimgFragmentRead(FGPF_BLEND);
	blend.bits.enable = !!enable;
	fimgFragmentWrite(blend.val, FGPF_BLEND);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendColor
* SYNOPSIS:	This function set constant blend color.
*          	This value can be used in blending.
* PARAMETERS:	[IN] blendColor - RGBA-order 32bit color
*****************************************************************************/
void fimgSetBlendColor(unsigned int blendColor)
{
	fimgFragmentWrite(blendColor, FGPF_CCLR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDitherEnable
* SYNOPSIS:	This function controls image dithering.
* PARAMETERS:	[IN] enale - non-zero to enable dithering
*****************************************************************************/
void fimgSetDitherEnable(int enable)
{
	fimgFramebufferControl fbctl;

	fbctl.val = fimgFragmentRead(FGPF_FBCTL);
	fbctl.bits.dither = !!enable;
	fimgFragmentWrite(fbctl.val, FGPF_FBCTL);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetLogicalOpParams
* SYNOPSIS:	A logical operation can be applied the fragment and value stored
*		at the corresponding location in the framebuffer; the result
*		replaces the current framebuffer value.
* PARAMETERS:	[in] alpha - logical operation on alpha data
*		[in[ color - logical operation on color data
*****************************************************************************/
void fimgSetLogicalOpParams(fimgLogicalOperation alpha, fimgLogicalOperation color)
{
	fimgLogOpControl logop;

	logop.val = fimgFragmentRead(FGPF_LOGOP);
	logop.bits.alpha = alpha;
	logop.bits.color = color;
	fimgFragmentWrite(logop.val, FGPF_LOGOP);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetLogicalOpEnable
* SYNOPSIS:	A logical operation can be applied the fragment and value stored
*          	at the corresponding location in the framebuffer; the result
*          	replaces the current framebuffer value.
* PARAMETERS:	[IN] enable: if non-zero, logical operation is enabled
*****************************************************************************/
void fimgSetLogicalOpEnable(int enable)
{
	fimgLogOpControl logop;

	logop.val = fimgFragmentRead(FGPF_LOGOP);
	logop.bits.enable = !!enable;
	fimgFragmentWrite(logop.val, FGPF_LOGOP);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetColorBufWriteMask
* SYNOPSIS:	enable and disable writing of frame(color) buffer color components
* PARAMETERS:	[IN] r - whether red can or cannot be written into the frame buffer.
*		[IN] g - whether green can or cannot be written into the frame buffer.
*		[IN] b - whether blue can or cannot be written into the frame buffer.
*		[IN] a - whether alpha can or cannot be written into the frame buffer.
*****************************************************************************/
void fimgSetColorBufWriteMask(int r, int g, int b, int a)
{
	fimgColorBufMask mask;

	mask.val = 0;
	mask.bits.r = r;
	mask.bits.g = g;
	mask.bits.b = b;
	mask.bits.a = a;
	fimgFragmentWrite(mask.val, FGPF_CBMSK);
}

#if TARGET_FIMG_VERSION != _FIMG3DSE_VER_1_2
/*****************************************************************************
* FUNCTIONS:	fimgSetStencilBufWriteMask
* SYNOPSIS:	control the front and/or back writing of individual bits
*		in the stencil buffer.
* PARAMETERS:	[IN] back - 0 - update front mask, non-zero - update back mask
*		[IN] mask - A bit mask to enable and disable writing of individual
*                    bits in the stencil buffer.
*****************************************************************************/
void fimgSetStencilBufWriteMask(int back, unsigned int mask)
{
	fimgDepthBufMask dbmask;

	dbmask.val = fimgFragmentRead(FGPF_DBMSK);
	if(!back)
		dbmask.bits.frontmask = (~mask) & 0xff;
	else
		dbmask.bits.backmask = (~mask) & 0xff;
	fimgFragmentWrite(dbmask.val, FGPF_DBMSK);
}
#endif

/*****************************************************************************
* FUNCTIONS:	fimgSetZBufWriteMask
* SYNOPSIS:	enables or disables writing into the depth buffer.
* PARAMETERS:	[IN] enable - specifies whether the depth buffer is enabled
*		     for writing.
*****************************************************************************/
void fimgSetZBufWriteMask(int enable)
{
	fimgDepthBufMask dbmask;

	dbmask.val = fimgFragmentRead(FGPF_DBMSK);
	dbmask.bits.depth = !enable;
	fimgFragmentWrite(dbmask.val, FGPF_DBMSK);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrameBufParams
* SYNOPSIS:	specifies the value used for frame buffer control.
* PARAMETERS:	[IN] opaqueAlpha - after alpha blending, the alpha value is
*		     forced to opaque.
*		[IN] thresholdAlpha - specifies an alpha value in the frame buffer
*		     ARGB1555 format.
*		[IN] constAlpha - specifies constant alpha value in the frame
*		     buffer ARGB0888 format.
*		[IN] format - specifies the format used for the frame buffer.
*****************************************************************************/
void fimgSetFrameBufParams(int opaqueAlpha, unsigned int thresholdAlpha,
			   unsigned int constAlpha, fimgColorMode format)
{
	fimgFramebufferControl fbctl;

	fbctl.val = fimgFragmentRead(FGPF_FBCTL);
	fbctl.bits.opaque = !!opaqueAlpha;
	fbctl.bits.alphathreshold = thresholdAlpha;
	fbctl.bits.alphaconst = constAlpha;
	fbctl.bits.colormode = format;
	fimgFragmentWrite(fbctl.val, FGPF_FBCTL);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetZBufBaseAddr
* SYNOPSIS:	Depth and Stencil buffer base address
* PARAMETERS:	[IN] addr - specifies the value used for stencil/depth buffer
*		     address.
*****************************************************************************/
void fimgSetZBufBaseAddr(unsigned int addr)
{
	fimgFragmentWrite(addr, FGPF_DBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetColorBufBaseAddr
* SYNOPSIS:	color buffer base address
* PARAMETERS:	[IN] addr - specifies the value used for frame buffer address.
*****************************************************************************/
void fimgSetColorBufBaseAddr(unsigned int addr)
{
	fimgFragmentWrite(addr, FGPF_CBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrameBufWidth
* SYNOPSIS:	frame buffer width
* PARAMETERS:	[IN] width - specifies the value used for frame buffer width.
*****************************************************************************/
void fimgSetFrameBufWidth(unsigned int width)
{
	fimgFragmentWrite(width, FGPF_FBW);
}
