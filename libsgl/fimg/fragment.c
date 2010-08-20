/*
* fimg/fragment.c
*
* SAMSUNG S3C6410 FIMG-3DSE PER FRAGMENT UNIT RELATED FUNCTIONS
*
* Copyrights:	2005 by Samsung Electronics Limited (original code)
*		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
*/

#include "fimg_private.h"

#define FGPF_SCISSOR_X		(0x70000)
#define FGPF_SCISSOR_Y		(0x70004)
#define FGPF_ALPHAT		(0x70008)
#define FGPF_FRONTST		(0x7000c)
#define FGPF_BACKST		(0x70010)
#define FGPF_DEPTHT		(0x70014)
#define FGPF_CCLR		(0x70018)
#define FGPF_BLEND		(0x7001c)
#define FGPF_LOGOP		(0x70020)
#define FGPF_CBMSK		(0x70024)
#define FGPF_DBMSK		(0x70028)
#define FGPF_FBCTL		(0x7002c)
#define FGPF_DBADDR		(0x70030)
#define FGPF_CBADDR		(0x70034)
#define FGPF_FBW		(0x70038)

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
	fimgWrite(ctx, ctx->fragment.scX.val, FGPF_SCISSOR_X);

	ctx->fragment.scY.bits.max = yMax;
	ctx->fragment.scY.bits.min = yMin;
	fimgWrite(ctx, ctx->fragment.scY.val, FGPF_SCISSOR_Y);
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
	fimgWrite(ctx, ctx->fragment.scX.val, FGPF_SCISSOR_X);
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
	fimgWrite(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
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
	fimgWrite(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
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
	fimgWrite(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
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
	fimgWrite(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
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
	fimgWrite(ctx, ctx->fragment.stBack.val, FGPF_BACKST);
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
	fimgWrite(ctx, ctx->fragment.stBack.val, FGPF_BACKST);
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
	fimgWrite(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthParams
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] mode: Specifies the depth-comparison function
*****************************************************************************/
void fimgSetDepthParams(fimgContext *ctx, fimgTestMode mode)
{
	ctx->fragment.depth.bits.mode = mode;
	fimgWrite(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDepthEnable
* SYNOPSIS:	This function specifies the value used for depth-buffer comparisons.
* PARAMETERS:	[IN] enable: if non-zero, depth test is enabled
*****************************************************************************/
void fimgSetDepthEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.depth.bits.enable = !!enable;
	fimgWrite(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
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
	fimgWrite(ctx, ctx->fragment.blend.val, FGPF_BLEND);
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
	fimgWrite(ctx, ctx->fragment.blend.val, FGPF_BLEND);
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
	fimgWrite(ctx, ctx->fragment.blend.val, FGPF_BLEND);
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
	fimgWrite(ctx, blendColor, FGPF_CCLR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetDitherEnable
* SYNOPSIS:	This function controls image dithering.
* PARAMETERS:	[IN] enale - non-zero to enable dithering
*****************************************************************************/
void fimgSetDitherEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.fbctl.bits.dither = !!enable;
	fimgWrite(ctx, ctx->fragment.fbctl.val, FGPF_FBCTL);
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
	fimgWrite(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
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
	fimgWrite(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
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
	fimgWrite(ctx, ctx->fragment.mask.val, FGPF_CBMSK);
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
	fimgWrite(ctx, ctx->fragment.dbmask.val, FGPF_DBMSK);
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
	fimgWrite(ctx, ctx->fragment.dbmask.val, FGPF_DBMSK);
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
	fimgWrite(ctx, ctx->fragment.fbctl.val, FGPF_FBCTL);
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
	fimgWrite(ctx, addr, FGPF_DBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetColorBufBaseAddr
* SYNOPSIS:	color buffer base address
* PARAMETERS:	[IN] addr - specifies the value used for frame buffer address.
*****************************************************************************/
void fimgSetColorBufBaseAddr(fimgContext *ctx, unsigned int addr)
{
	ctx->fragment.colorAddr = addr;
	fimgWrite(ctx, addr, FGPF_CBADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetFrameBufWidth
* SYNOPSIS:	frame buffer width
* PARAMETERS:	[IN] width - specifies the value used for frame buffer width.
*****************************************************************************/
void fimgSetFrameBufWidth(fimgContext *ctx, unsigned int width)
{
	ctx->fragment.bufWidth = width;
	fimgWrite(ctx, width, FGPF_FBW);
}

void fimgCreateFragmentContext(fimgContext *ctx)
{
	// Nothing to initialize yet
}

void fimgRestoreFragmentState(fimgContext *ctx)
{
	fimgWrite(ctx, ctx->fragment.scY.val, FGPF_SCISSOR_Y);
	fimgWrite(ctx, ctx->fragment.scX.val, FGPF_SCISSOR_X);
	fimgWrite(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
	fimgWrite(ctx, ctx->fragment.stBack.val, FGPF_BACKST);
	fimgWrite(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
	fimgWrite(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
	fimgWrite(ctx, ctx->fragment.blend.val, FGPF_BLEND);
	fimgWrite(ctx, ctx->fragment.blendColor, FGPF_CCLR);
	fimgWrite(ctx, ctx->fragment.fbctl.val, FGPF_FBCTL);
	fimgWrite(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
	fimgWrite(ctx, ctx->fragment.mask.val, FGPF_CBMSK);
	fimgWrite(ctx, ctx->fragment.dbmask.val, FGPF_DBMSK);
	fimgWrite(ctx, ctx->fragment.depthAddr, FGPF_DBADDR);
	fimgWrite(ctx, ctx->fragment.colorAddr, FGPF_CBADDR);
	fimgWrite(ctx, ctx->fragment.bufWidth, FGPF_FBW);
}
