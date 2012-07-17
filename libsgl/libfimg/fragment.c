/*
 * fimg/fragment.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE PER FRAGMENT UNIT RELATED FUNCTIONS
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

/**
 * Specifies parameters of scissor test.
 * Scissor test, if enabled, discards fragments outside the specified rectangle.
 * @see fimgSetScissorEnable
 * @param ctx Hardware context.
 * @param xMax Fragments with greater or equal x will be discarded.
 * @param xMin Fragments with less x will be discarded.
 * @param yMax Fragments with greater or equal y will be discarded.
 * @param yMin Fragments with less y will be discarded.
 */
void fimgSetScissorParams(fimgContext *ctx,
			  unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin)
{
	if (ctx->flipY) {
		unsigned int tmp = ctx->fbHeight - yMax;
		yMax = ctx->fbHeight - yMin;
		yMin = tmp;
	}

	ctx->fragment.scY.max = yMax;
	ctx->fragment.scY.min = yMin;
	fimgQueue(ctx, ctx->fragment.scY.val, FGPF_SCISSOR_Y);

	ctx->fragment.scX.max = xMax;
	ctx->fragment.scX.min = xMin;
	fimgQueue(ctx, ctx->fragment.scX.val, FGPF_SCISSOR_X);
}

/**
 * Controls enable state of scissor test.
 * Scissor test, if enabled, discards fragments outside the specified rectangle.
 * @see fimgSetScissorParams
 * @param ctx Hardware context.
 * @param enable Non-zero to enable scissor test.
 */
void fimgSetScissorEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.scX.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.scX.val, FGPF_SCISSOR_X);
}

/**
 * Specifies parameters of alpha test.
 * Alpha test, if enabled, discards a fragment depending on result of
 * comparison between alpha value of processed fragment and configured
 * reference value.
 * @see fimgSetAlphaEnable
 * @param ctx Hardware context.
 * @param refAlpha The reference value to which incoming alpha values
 * are compared.
 * @param mode The alpha comparison function.
 */
void fimgSetAlphaParams(fimgContext *ctx, unsigned int refAlpha,
			fimgTestMode mode)
{
	ctx->fragment.alpha.value = refAlpha;
	ctx->fragment.alpha.mode = mode;
	fimgQueue(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
}

/**
 * Controls enable state of alpha test.
 * Alpha test, if enabled, discards a fragment depending on result of
 * comparison between alpha value of processed fragment and configured
 * reference value.
 * @see fimgSetAlphaParams
 * @param ctx Hardware context.
 * @param enable Non-zero to enable alpha test.
 */
void fimgSetAlphaEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.alpha.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
}

/**
 * Specifies parameters of front stencil test.
 * Stencil test, if enabled, conditionally discards fragments based on result
 * of comparison between the value in the stencil buffer and reference value.
 * In addition the value in stencil buffer is modified according to configured
 * operation and test result.
 * @see fimgSetFrontStencilOp
 * @see fimgSetBackStencilFunc
 * @see fimgSetBackStencilOp
 * @see fimgSetStencilEnable
 * @param ctx Hardware context.
 * @param mode Test comparison function.
 * @param ref Reference value.
 * @param mask Test mask.
 */
void fimgSetFrontStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			     unsigned char ref, unsigned char mask)
{
	ctx->fragment.stFront.mode = mode;
	ctx->fragment.stFront.ref = ref;
	ctx->fragment.stFront.mask = mask;
	fimgQueue(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
}

/**
 * Specifies operations on stencil buffer after front stencil test.
 * Stencil test, if enabled, conditionally discards fragments based on result
 * of comparison between the value in the stencil buffer and reference value.
 * In addition the value in stencil buffer is modified according to configured
 * operation and test result.
 * @see fimgSetFrontStencilFunc
 * @see fimgSetBackStencilFunc
 * @see fimgSetBackStencilOp
 * @see fimgSetStencilEnable
 * @param ctx Hardware context.
 * @param sfail Action to take if stencil test fails.
 * @param dpfail Action to take if depth buffer test fails.
 * @param dppass Action to take if depth buffer test passes.
 */
void fimgSetFrontStencilOp(fimgContext *ctx, fimgTestAction sfail,
			   fimgTestAction dpfail, fimgTestAction dppass)
{
	ctx->fragment.stFront.sfail = sfail;
	ctx->fragment.stFront.dpfail = dpfail;
	ctx->fragment.stFront.dppass = dppass;
	fimgQueue(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
}

/**
 * Specifies parameters of back stencil test.
 * Stencil test, if enabled, conditionally discards fragments based on result
 * of comparison between the value in the stencil buffer and reference value.
 * In addition the value in stencil buffer is modified according to configured
 * operation and test result.
 * @see fimgSetBackStencilOp
 * @see fimgSetFrontStencilFunc
 * @see fimgSetFrontStencilOp
 * @see fimgSetStencilEnable
 * @param ctx Hardware context.
 * @param mode Test comparison function.
 * @param ref Reference value.
 * @param mask Test mask.
 */
void fimgSetBackStencilFunc(fimgContext *ctx, fimgStencilMode mode,
			    unsigned char ref, unsigned char mask)
{
	ctx->fragment.stBack.mode = mode;
	ctx->fragment.stBack.ref = ref;
	ctx->fragment.stBack.mask = mask;
	fimgQueue(ctx, ctx->fragment.stBack.val, FGPF_BACKST);
}

/**
 * Specifies operations on stencil buffer after back stencil test.
 * Stencil test, if enabled, conditionally discards fragments based on result
 * of comparison between the value in the stencil buffer and reference value.
 * In addition the value in stencil buffer is modified according to configured
 * operation and test result.
 * @see fimgSetBackStencilFunc
 * @see fimgSetFrontStencilFunc
 * @see fimgSetFrontStencilOp
 * @see fimgSetStencilEnable
 * @param ctx Hardware context.
 * @param sfail Action to take if stencil test fails.
 * @param dpfail Action to take if depth buffer test fails.
 * @param dppass Action to take if depth buffer test passes.
 */
void fimgSetBackStencilOp(fimgContext *ctx, fimgTestAction sfail,
			  fimgTestAction dpfail, fimgTestAction dppass)
{
	ctx->fragment.stBack.sfail = sfail;
	ctx->fragment.stBack.dpfail = dpfail;
	ctx->fragment.stBack.dppass = dppass;
	fimgQueue(ctx, ctx->fragment.stBack.val, FGPF_BACKST);
}

/**
 * Controls enable state of stencil test.
 * Stencil test, if enabled, conditionally discards fragments based on result
 * of comparison between the value in the stencil buffer and reference value.
 * In addition the value in stencil buffer is modified according to configured
 * operation and test result.
 * @see fimgSetFrontStencilFunc
 * @see fimgSetFrontStencilOp
 * @see fimgSetBackStencilFunc
 * @see fimgSetBackStencilFunc
 * @param ctx Hardware context.
 * @param enable Non-zero to enable stencil test.
 */
void fimgSetStencilEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.stFront.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
}

/**
 * Specifies test function of depth test.
 * Depth test, if enabled, conditionally discards fragments based on comparision
 * between depth value of processed fragment and value stored in depth buffer.
 * @see fimgSetDepthEnable
 * @param ctx Hardware context.
 * @param mode Depth comparison function.
 */
void fimgSetDepthParams(fimgContext *ctx, fimgTestMode mode)
{
	ctx->fragment.depth.mode = mode;
	fimgQueue(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
}

/**
 * Controls enable state of depth test.
 * Depth test, if enabled, conditionally discards fragments based on comparision
 * between depth value of processed fragment and value stored in depth buffer.
 * @see fimgSetDepthParams
 * @param ctx Hardware context.
 * @param enable If non-zero, depth test is enabled.
 */
void fimgSetDepthEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.depth.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
}

/**
 * Specifies blend equation for pixel blending.
 * Blending, if enabled, combines color value of processed fragment with value
 * already stored in framebuffer, according to configured blend equation,
 * function and constant.
 * @see fimgSetBlendEnable
 * @see fimgSetBlendFunc
 * @see fimgSetBlendFuncNoAlpha
 * @see fimgSetBlendColor
 * @param ctx Hardware context.
 * @param alpha Alpha blend equation.
 * @param color Color blend equation.
 */
void fimgSetBlendEquation(fimgContext *ctx,
			  fimgBlendEquation alpha, fimgBlendEquation color)
{
	ctx->fragment.blend.ablendequation = alpha;
	ctx->fragment.blend.cblendequation = color;
	fimgQueue(ctx, ctx->fragment.blend.val, FGPF_BLEND);
}

/**
 * Specifies blend function for pixel blending.
 * Blending, if enabled, combines color value of processed fragment with value
 * already stored in framebuffer, according to configured blend equation,
 * function and constant.
 * @see fimgSetBlendEnable
 * @see fimgSetBlendEquation
 * @see fimgSetBlendFuncNoAlpha
 * @see fimgSetBlendColor
 * @param ctx Hardware context.
 * @param srcAlpha Source alpha blend function.
 * @param srcColor Source color blend function.
 * @param dstAlpha Destination alpha blend function.
 * @param dstColor Destination color blend function.
 */
void fimgSetBlendFunc(fimgContext *ctx,
		      fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor)
{
	ctx->fragment.blend.asrcblendfunc = srcAlpha;
	ctx->fragment.blend.csrcblendfunc = srcColor;
	ctx->fragment.blend.adstblendfunc = dstAlpha;
	ctx->fragment.blend.cdstblendfunc = dstColor;
	fimgQueue(ctx, ctx->fragment.blend.val, FGPF_BLEND);
}

/**
 * Specifies blend function for pixel blending, for pixel formats without alpha.
 * Blending, if enabled, combines color value of processed fragment with value
 * already stored in framebuffer, according to configured blend equation,
 * function and constant.
 * @see fimgSetBlendEnable
 * @see fimgSetBlendEquation
 * @see fimgSetBlendFunc
 * @see fimgSetBlendColor
 * @param ctx Hardware context.
 * @param srcAlpha Source alpha blend function.
 * @param srcColor Source color blend function.
 * @param dstAlpha Destination alpha blend function.
 * @param dstColor Destination color blend function.
 */
void fimgSetBlendFuncNoAlpha(fimgContext *ctx,
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

/**
 * Controls enable state of blending.
 * Blending, if enabled, combines color value of processed fragment with value
 * already stored in framebuffer, according to configured blend equation,
 * function and constant.
 * @see fimgSetBlendEquation
 * @see fimgSetBlendFunc
 * @see fimgSetBlendFuncNoAlpha
 * @see fimgSetBlendColor
 * @param ctx Hardware context.
 * @param enable Non-zero to enable blending.
 */
void fimgSetBlendEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.blend.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.blend.val, FGPF_BLEND);
}

/**
 * Sets constant blend color.
 * Blending, if enabled, combines color value of processed fragment with value
 * already stored in framebuffer, according to configured blend equation,
 * function and constant.
 * @see fimgSetBlendEquation
 * @see fimgSetBlendFunc
 * @see fimgSetBlendFuncNoAlpha
 * @see fimgSetBlendEnable
 * @param ctx Hardware context.
 * @param blendColor Blend color in RGBA8888 format.
 */
void fimgSetBlendColor(fimgContext *ctx, unsigned int blendColor)
{
	ctx->fragment.blendColor = blendColor;
	fimgQueue(ctx, blendColor, FGPF_CCLR);
}

/**
 * Controls enable state image dithering.
 * @param ctx Hardware context.
 * @param enable Non-zero to enable dithering.
 */
void fimgSetDitherEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.fbctl.dither = !!enable;
	fimgQueue(ctx, ctx->fragment.fbctl.val, FGPF_FBCTL);
}

/**
 * Specifies parameters of logical operation on fragment values.
 * A logical operation can be applied to processed fragment and value stored
 * at the corresponding location in the framebuffer. The result is then
 * written to the framebuffer.
 * @see fimgSetLogicalOpEnable
 * @param ctx Hardware context.
 * @param alpha Logical operation on alpha data.
 * @param color Logical operation on color data.
 */
void fimgSetLogicalOpParams(fimgContext *ctx, fimgLogicalOperation alpha,
			    fimgLogicalOperation color)
{
	ctx->fragment.logop.alpha = alpha;
	ctx->fragment.logop.color = color;
	fimgQueue(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
}

/**
 * Controls enable state of logical operation block.
 * A logical operation can be applied to processed fragment and value stored
 * at the corresponding location in the framebuffer. The result is then
 * written to the framebuffer.
 * @see fimgSetLogicalOpParams
 * @param ctx Hardware context.
 * @param enable If non-zero, logical operation is enabled.
 */
void fimgSetLogicalOpEnable(fimgContext *ctx, int enable)
{
	ctx->fragment.logop.enable = !!enable;
	fimgQueue(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
}

/**
 * Controls which color components are written to color buffer.
 * @param ctx Hardware context.
 * @param mask Bit mask with set bits corresponding to disabled components.
 */
void fimgSetColorBufWriteMask(fimgContext *ctx, unsigned int mask)
{
	ctx->fragment.mask.val = mask & 0xf;
	fimgQueue(ctx, ctx->fragment.mask.val, FGPF_CBMSK);
}

/**
 * Controls which bits are written to stencil buffer.
 * @param ctx Hardware context.
 * @param back Non-zero to set mask of back stencil, front otherwise.
 * @param mask Bit mask with set bits corresponding to writable bits.
 */
void fimgSetStencilBufWriteMask(fimgContext *ctx, int back, unsigned char mask)
{
	if(!back)
		ctx->fragment.dbmask.frontmask = ~mask;
	else
		ctx->fragment.dbmask.backmask = ~mask;
	fimgQueue(ctx, ctx->fragment.dbmask.val, FGPF_DBMSK);
}

/**
 * Controls whether writing to depth buffer is allowed.
 * @param ctx Hardware context.
 * @param enable Non-zero to enable writing to depth buffer.
 */
void fimgSetZBufWriteMask(fimgContext *ctx, int enable)
{
	ctx->fragment.dbmask.depth = !enable;
	fimgQueue(ctx, ctx->fragment.dbmask.val, FGPF_DBMSK);
}

/**
 * Controls framebuffer parameters such as pixel format, component swap, etc.
 * @param ctx Hardware context.
 * @param flags Extra flags altering framebuffer operation.
 * @param format Pixel format.
 */
void fimgSetFrameBufParams(fimgContext *ctx,
				unsigned int flags, unsigned int format)
{
	ctx->fragment.fbctl.opaque = 0;
	ctx->fragment.fbctl.alphathreshold = 0;
	ctx->fragment.fbctl.alphaconst = 255;
	ctx->fragment.fbctl.colormode = format;
#ifdef FIMG_FIXED_PIPELINE
	FGFP_BITFIELD_SET(ctx->compat.psState.ps,
			PS_SWAP, !!(flags & FGPF_COLOR_MODE_BGR));
#endif
	ctx->fbFlags = flags;
	fimgQueue(ctx, ctx->fragment.fbctl.val, FGPF_FBCTL);
}

/**
 * Sets depth/stencil buffer base address.
 * @param ctx Hardware context.
 * @param addr Physical address of depth/stencil buffer.
 */
void fimgSetZBufBaseAddr(fimgContext *ctx, unsigned int addr)
{
	ctx->fragment.depthAddr = addr;
	fimgQueue(ctx, addr, FGPF_DBADDR);
}

/**
 * Sets color buffer base address.
 * @param ctx Hardware context.
 * @param addr Physical address of color buffer.
 */
void fimgSetColorBufBaseAddr(fimgContext *ctx, unsigned int addr)
{
	ctx->fragment.colorAddr = addr;
	fimgQueue(ctx, addr, FGPF_CBADDR);
}

/**
 * Sets framebuffer width, height and Y-axis flip.
 * @param ctx Hardware context.
 * @param width Framebuffer width.
 * @param height Framebuffer height.
 * @param flipY Non-zero to enable Y-axis flip.
 */
void fimgSetFrameBufSize(fimgContext *ctx,
			unsigned int width, unsigned int height, int flipY)
{
	ctx->fragment.bufWidth = width;
	ctx->fbHeight = height;
	ctx->flipY = flipY;
	fimgQueue(ctx, width, FGPF_FBW);
}

/**
 * Initializes hardware context of per-fragment block.
 * @param ctx Hardware context.
 */
void fimgCreateFragmentContext(fimgContext *ctx)
{
	ctx->fragment.alpha.mode = FGPF_TEST_MODE_ALWAYS;
	ctx->fragment.depth.mode = FGPF_TEST_MODE_LESS;
	ctx->fragment.stFront.mode = FGPF_STENCIL_MODE_ALWAYS;
	ctx->fragment.stFront.mask = 0xff;
	ctx->fragment.stBack.mode = FGPF_STENCIL_MODE_ALWAYS;
	ctx->fragment.stBack.mask = 0xff;
	ctx->fragment.blend.asrcblendfunc = FGPF_BLEND_FUNC_ONE;
	ctx->fragment.blend.csrcblendfunc = FGPF_BLEND_FUNC_ONE;
	ctx->fragment.fbctl.dither = 1;
}

/**
 * Restores hardware context of per-fragment block.
 * @param ctx Hardware context.
 */
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
