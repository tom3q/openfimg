/*
* fimg/fragment.c
*
* SAMSUNG S3C6410 FIMG-3DSE PER FRAGMENT UNIT RELATED FUNCTIONS
*
* Copyrights:	2005 by Samsung Electronics Limited (original code)
*		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
*/

#include "fimg_private.h"

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
void fimgSetScissorParams(fimgContext *ctx,
			  unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin)
{
	ctx->fragment.scX.bits.max = xMax;
	ctx->fragment.scX.bits.min = xMin;
	fimgFragmentWrite(ctx->fragment.scX.val, FGPF_SCISSOR_X);

	ctx->fragment.scY.bits.max = yMax;
	ctx->fragment.scY.bits.min = yMin;
	fimgFragmentWrite(ctx->fragment.scY.val, FGPF_SCISSOR_Y);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetScissorParams
* SYNOPSIS:	This function specifies an arbitary screen-aligned rectangle
*		outside of which fragments will be discarded.
* PARAMETERS:	[IN] enable - non-zero to enable scissor test
*****************************************************************************/
void fimgSetScissorEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.scX.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.scX.val, FGPF_SCISSOR_X);
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
void fimgSetAlphaParams(fimgContext *ctx, unsigned int refAlpha,
			fimgTestMode mode)
{
	ctx->fragment.alpha.bits.value = refAlpha;
	ctx->fragment.alpha.bits.mode = mode;
	fimgFragmentWrite(ctx->fragment.alpha.val, FGPF_ALPHAT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetAlphaEnable
* SYNOPSIS:	This function discards a fragment depending on the outcome of a
*		comparison between the fragment's alpha value and a constant
*		reference value.
* PARAMETERS:	[IN] enable - non-zero to enable alpha test
*****************************************************************************/
void fimgSetAlphaEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.alpha.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.alpha.val, FGPF_ALPHAT);
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
void fimgSetFrontStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			     unsigned char ref, unsigned char mask)
{
	ctx->fragment.stFront.bits.mode = mode;
	ctx->fragment.stFront.bits.ref = ref;
	ctx->fragment.stFront.bits.mask = mask;
	fimgFragmentWrite(ctx->fragment.stFront.val, FGPF_FRONTST);
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
void fimgSetFrontStencilOp(fimgContext *ctx, fimgTestAction sfail,
			   fimgTestAction dpfail, fimgTestAction dppass)
{
	ctx->fragment.stFront.bits.sfail = sfail;
	ctx->fragment.stFront.bits.dpfail = dpfail;
	ctx->fragment.stFront.bits.dppass = dppass;
	fimgFragmentWrite(ctx->fragment.stFront.val, FGPF_FRONTST);
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
void fimgSetBackStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			    unsigned char ref, unsigned char mask)
{
	ctx->fragment.stBack.bits.mode = mode;
	ctx->fragment.stBack.bits.ref = ref;
	ctx->fragment.stBack.bits.mask = mask;
	fimgFragmentWrite(ctx->fragment.stBack.val, FGPF_BACKST);
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
void fimgSetBackStencilOp(fimgContext *ctx, fimgTestAction sfail,
			  fimgTestAction dpfail, fimgTestAction dppass)
{
	ctx->fragment.stBack.bits.sfail = sfail;
	ctx->fragment.stBack.bits.dpfail = dpfail;
	ctx->fragment.stBack.bits.dppass = dppass;
	fimgFragmentWrite(ctx->fragment.stBack.val, FGPF_BACKST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetStencilEnable
* SYNOPSIS:	The stencil test conditionally discards a fragment based on the
*		outcome of a comparison between the value in the stencil buffer
*		and a reference value.
* PARAMETERS:	[IN] enable - non-zero to enable stencil test
*****************************************************************************/
void fimgSetStencilEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.stFront.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.stFront.val, FGPF_FRONTST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthParams
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] mode: Specifies the depth-comparison function
*****************************************************************************/
void fimgSetDepthParams(fimgContext *ctx, fimgTestMode mode)
{
	ctx->fragment.depth.bits.mode = mode;
	fimgFragmentWrite(ctx->fragment.depth.val, FGPF_DEPTHT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthEnable
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] enable: if non-zero, depth test is enabled
*****************************************************************************/
void fimgSetDepthEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.depth.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.depth.val, FGPF_DEPTHT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendEquation
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
* PARAMETERS:	[in] alpha - alpha blend equation
*		[in] color - color blend equation
*****************************************************************************/
void fimgSetBlendEquation(fimgContext *ctx,
			  fimgBlendEquation alpha, fimgBlendEquation color)
{
	ctx->fragment.blend.bits.ablendequation = alpha;
	ctx->fragment.blend.bits.cblendequation = color;
	fimgFragmentWrite(ctx->fragment.blend.val, FGPF_BLEND);
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
void fimgSetBlendFunc(fimgContext *ctx,
		      fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor)
{
	ctx->fragment.blend.bits.asrcblendfunc = srcAlpha;
	ctx->fragment.blend.bits.csrcblendfunc = srcColor;
	ctx->fragment.blend.bits.adstblendfunc = dstAlpha;
	ctx->fragment.blend.bits.cdstblendfunc = dstColor;
	fimgFragmentWrite(ctx->fragment.blend.val, FGPF_BLEND);
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
void fimgSetBlendFuncRGB565(fimgContext *ctx,
			    fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
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

	fimgSetBlendFunc(ctx, srcAlpha, srcColor, dstAlpha, dstColor);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendEnable
* SYNOPSIS:	In RGB mode, pixels can be drawn using a function that blends
*		the incoming (source) RGBA values with the RGBA values that are
*		already in the framebuffer (the destination values).
* PARAMETERS:	[in] enable - non-zero to enable blending
*****************************************************************************/
void fimgSetBlendEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.blend.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.blend.val, FGPF_BLEND);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetBlendColor
* SYNOPSIS:	This function set constant blend color.
*          	This value can be used in blending.
* PARAMETERS:	[IN] blendColor - RGBA-order 32bit color
*****************************************************************************/
void fimgSetBlendColor(fimgContext *ctx, unsigned int blendColor)
{
	ctx->fragment.blendColor = blendColor;
	fimgFragmentWrite(blendColor, FGPF_CCLR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDitherEnable
* SYNOPSIS:	This function controls image dithering.
* PARAMETERS:	[IN] enale - non-zero to enable dithering
*****************************************************************************/
void fimgSetDitherEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.fbctl.bits.dither = !!enable;
	fimgFragmentWrite(ctx->fragment.fbctl.val, FGPF_FBCTL);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetLogicalOpParams
* SYNOPSIS:	A logical operation can be applied the fragment and value stored
*		at the corresponding location in the framebuffer; the result
*		replaces the current framebuffer value.
* PARAMETERS:	[in] alpha - logical operation on alpha data
*		[in[ color - logical operation on color data
*****************************************************************************/
void fimgSetLogicalOpParams(fimgContext *ctx, fimgLogicalOperation alpha,
			    fimgLogicalOperation color)
{
	ctx->fragment.logop.bits.alpha = alpha;
	ctx->fragment.logop.bits.color = color;
	fimgFragmentWrite(ctx->fragment.logop.val, FGPF_LOGOP);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetLogicalOpEnable
* SYNOPSIS:	A logical operation can be applied the fragment and value stored
*          	at the corresponding location in the framebuffer; the result
*          	replaces the current framebuffer value.
* PARAMETERS:	[IN] enable: if non-zero, logical operation is enabled
*****************************************************************************/
void fimgSetLogicalOpEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.logop.bits.enable = !!enable;
	fimgFragmentWrite(ctx->fragment.logop.val, FGPF_LOGOP);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetColorBufWriteMask
* SYNOPSIS:	enable and disable writing of frame(color) buffer color components
* PARAMETERS:	[IN] r - whether red can or cannot be written into the frame buffer.
*		[IN] g - whether green can or cannot be written into the frame buffer.
*		[IN] b - whether blue can or cannot be written into the frame buffer.
*		[IN] a - whether alpha can or cannot be written into the frame buffer.
*****************************************************************************/
void fimgSetColorBufWriteMask(fimgContext *ctx, int r, int g, int b, int a)
{
	ctx->fragment.mask.bits.r = r;
	ctx->fragment.mask.bits.g = g;
	ctx->fragment.mask.bits.b = b;
	ctx->fragment.mask.bits.a = a;
	fimgFragmentWrite(ctx->fragment.mask.val, FGPF_CBMSK);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetStencilBufWriteMask
* SYNOPSIS:	control the front and/or back writing of individual bits
*		in the stencil buffer.
* PARAMETERS:	[IN] back - 0 - update front mask, non-zero - update back mask
*		[IN] mask - A bit mask to enable and disable writing of individual
*                    bits in the stencil buffer.
*****************************************************************************/
void fimgSetStencilBufWriteMask(fimgContext *ctx, int back, unsigned int mask)
{
	if(!back)
		ctx->fragment.dbmask.bits.frontmask = (~mask) & 0xff;
	else
		ctx->fragment.dbmask.bits.backmask = (~mask) & 0xff;
	fimgFragmentWrite(ctx->fragment.dbmask.val, FGPF_DBMSK);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetZBufWriteMask
* SYNOPSIS:	enables or disables writing into the depth buffer.
* PARAMETERS:	[IN] enable - specifies whether the depth buffer is enabled
*		     for writing.
*****************************************************************************/
void fimgSetZBufWriteMask(fimgContext *ctx, int enable)
{
	ctx->fragment.dbmask.bits.depth = !enable;
	fimgFragmentWrite(ctx->fragment.dbmask.val, FGPF_DBMSK);
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
void fimgSetFrameBufParams(fimgContext *ctx,
			   int opaqueAlpha, unsigned int thresholdAlpha,
			   unsigned int constAlpha, fimgColorMode format)
{
	ctx->fragment.fbctl.bits.opaque = !!opaqueAlpha;
	ctx->fragment.fbctl.bits.alphathreshold = thresholdAlpha;
	ctx->fragment.fbctl.bits.alphaconst = constAlpha;
	ctx->fragment.fbctl.bits.colormode = format;
	fimgFragmentWrite(ctx->fragment.fbctl.val, FGPF_FBCTL);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetZBufBaseAddr
* SYNOPSIS:	Depth and Stencil buffer base address
* PARAMETERS:	[IN] addr - specifies the value used for stencil/depth buffer
*		     address.
*****************************************************************************/
void fimgSetZBufBaseAddr(fimgContext *ctx, unsigned int addr)
{
	ctx->fragment.depthAddr = addr;
	fimgFragmentWrite(addr, FGPF_DBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetColorBufBaseAddr
* SYNOPSIS:	color buffer base address
* PARAMETERS:	[IN] addr - specifies the value used for frame buffer address.
*****************************************************************************/
void fimgSetColorBufBaseAddr(fimgContext *ctx, unsigned int addr)
{
	ctx->fragment.colorAddr = addr;
	fimgFragmentWrite(addr, FGPF_CBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrameBufWidth
* SYNOPSIS:	frame buffer width
* PARAMETERS:	[IN] width - specifies the value used for frame buffer width.
*****************************************************************************/
void fimgSetFrameBufWidth(fimgContext *ctx, unsigned int width)
{
	ctx->fragment.bufWidth = width;
	fimgFragmentWrite(width, FGPF_FBW);
}

void fimgRestoreFragmentState(fimgContext *ctx)
{
	fimgFragmentWrite(ctx->fragment.scY.val, FGPF_SCISSOR_Y);
	fimgFragmentWrite(ctx->fragment.scX.val, FGPF_SCISSOR_X);
	fimgFragmentWrite(ctx->fragment.alpha.val, FGPF_ALPHAT);
	fimgFragmentWrite(ctx->fragment.stBack.val, FGPF_BACKST);
	fimgFragmentWrite(ctx->fragment.stFront.val, FGPF_FRONTST);
	fimgFragmentWrite(ctx->fragment.depth.val, FGPF_DEPTHT);
	fimgFragmentWrite(ctx->fragment.blend.val, FGPF_BLEND);
	fimgFragmentWrite(ctx->fragment.blendColor, FGPF_CCLR);
	fimgFragmentWrite(ctx->fragment.fbctl.val, FGPF_FBCTL);
	fimgFragmentWrite(ctx->fragment.logop.val, FGPF_LOGOP);
	fimgFragmentWrite(ctx->fragment.mask.val, FGPF_CBMSK);
	fimgFragmentWrite(ctx->fragment.dbmask.val, FGPF_DBMSK);
	fimgFragmentWrite(ctx->fragment.depthAddr, FGPF_DBADDR);
	fimgFragmentWrite(ctx->fragment.colorAddr, FGPF_CBADDR);
	fimgFragmentWrite(ctx->fragment.bufWidth, FGPF_FBW);
}
