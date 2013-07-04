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

#include <sys/ioctl.h>
#include "fimg_private.h"

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
	ctx->hw.fragment.alpha.value = refAlpha;
	ctx->hw.fragment.alpha.mode = mode;
	fimgQueue(ctx, ctx->hw.fragment.alpha.val, FGPF_ALPHAT);
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
	ctx->hw.fragment.alpha.enable = !!enable;
	fimgQueue(ctx, ctx->hw.fragment.alpha.val, FGPF_ALPHAT);
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
	ctx->hw.prot.stFront.mode = mode;
	ctx->hw.prot.stFront.ref = ref;
	ctx->hw.prot.stFront.mask = mask;
	fimgQueue(ctx, ctx->hw.prot.stFront.val, FGPF_FRONTST);
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
	ctx->hw.prot.stFront.sfail = sfail;
	ctx->hw.prot.stFront.dpfail = dpfail;
	ctx->hw.prot.stFront.dppass = dppass;
	fimgQueue(ctx, ctx->hw.prot.stFront.val, FGPF_FRONTST);
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
	ctx->hw.fragment.stBack.mode = mode;
	ctx->hw.fragment.stBack.ref = ref;
	ctx->hw.fragment.stBack.mask = mask;
	fimgQueue(ctx, ctx->hw.fragment.stBack.val, FGPF_BACKST);
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
	ctx->hw.fragment.stBack.sfail = sfail;
	ctx->hw.fragment.stBack.dpfail = dpfail;
	ctx->hw.fragment.stBack.dppass = dppass;
	fimgQueue(ctx, ctx->hw.fragment.stBack.val, FGPF_BACKST);
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
	ctx->hw.prot.stFront.enable = !!enable;
	fimgQueue(ctx, ctx->hw.prot.stFront.val, FGPF_FRONTST);
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
	ctx->hw.prot.depth.mode = mode;
	fimgQueue(ctx, ctx->hw.prot.depth.val, FGPF_DEPTHT);
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
	ctx->hw.prot.depth.enable = !!enable;
	fimgQueue(ctx, ctx->hw.prot.depth.val, FGPF_DEPTHT);
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
	ctx->hw.fragment.blend.ablendequation = alpha;
	ctx->hw.fragment.blend.cblendequation = color;
	fimgQueue(ctx, ctx->hw.fragment.blend.val, FGPF_BLEND);
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
	ctx->hw.fragment.blend.asrcblendfunc = srcAlpha;
	ctx->hw.fragment.blend.csrcblendfunc = srcColor;
	ctx->hw.fragment.blend.adstblendfunc = dstAlpha;
	ctx->hw.fragment.blend.cdstblendfunc = dstColor;
	fimgQueue(ctx, ctx->hw.fragment.blend.val, FGPF_BLEND);
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
	ctx->hw.fragment.blend.enable = !!enable;
	fimgQueue(ctx, ctx->hw.fragment.blend.val, FGPF_BLEND);
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
	ctx->hw.fragment.blendColor = blendColor;
	fimgQueue(ctx, blendColor, FGPF_CCLR);
}

/**
 * Controls enable state image dithering.
 * @param ctx Hardware context.
 * @param enable Non-zero to enable dithering.
 */
void fimgSetDitherEnable(fimgContext *ctx, int enable)
{
	ctx->hw.prot.fbctl.dither = !!enable;
	fimgQueue(ctx, ctx->hw.prot.fbctl.val, FGPF_FBCTL);
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
	ctx->hw.fragment.logop.alpha = alpha;
	ctx->hw.fragment.logop.color = color;
	fimgQueue(ctx, ctx->hw.fragment.logop.val, FGPF_LOGOP);
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
	ctx->hw.fragment.logop.enable = !!enable;
	fimgQueue(ctx, ctx->hw.fragment.logop.val, FGPF_LOGOP);
}

/**
 * Controls which color components are written to color buffer.
 * @param ctx Hardware context.
 * @param mask Bit mask with set bits corresponding to disabled components.
 */
void fimgSetColorBufWriteMask(fimgContext *ctx, unsigned int mask)
{
	ctx->hw.fragment.mask.val = mask & 0xf;
	fimgQueue(ctx, ctx->hw.fragment.mask.val, FGPF_CBMSK);
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
		ctx->hw.prot.dbmask.frontmask = ~mask;
	else
		ctx->hw.prot.dbmask.backmask = ~mask;
	fimgQueue(ctx, ctx->hw.prot.dbmask.val, FGPF_DBMSK);
}

/**
 * Controls whether writing to depth buffer is allowed.
 * @param ctx Hardware context.
 * @param enable Non-zero to enable writing to depth buffer.
 */
void fimgSetZBufWriteMask(fimgContext *ctx, int enable)
{
	ctx->hw.prot.dbmask.depth = !enable;
	fimgQueue(ctx, ctx->hw.prot.dbmask.val, FGPF_DBMSK);
}

/**
 * Initializes hardware context of per-fragment block.
 * @param ctx Hardware context.
 */
void fimgCreateFragmentContext(fimgContext *ctx)
{
	ctx->hw.fragment.alpha.mode = FGPF_TEST_MODE_ALWAYS;
	ctx->hw.prot.depth.mode = FGPF_TEST_MODE_LESS;
	ctx->hw.prot.stFront.mode = FGPF_STENCIL_MODE_ALWAYS;
	ctx->hw.prot.stFront.mask = 0xff;
	ctx->hw.fragment.stBack.mode = FGPF_STENCIL_MODE_ALWAYS;
	ctx->hw.fragment.stBack.mask = 0xff;
	ctx->hw.fragment.blend.asrcblendfunc = FGPF_BLEND_FUNC_ONE;
	ctx->hw.fragment.blend.csrcblendfunc = FGPF_BLEND_FUNC_ONE;
	ctx->hw.prot.fbctl.dither = 1;
}

/**
 * Controls framebuffer parameters such as pixel format, component swap, etc.
 * @param ctx Hardware context.
 * @param fb Structure containing framebuffer parameters.
 */
void fimgSetFramebuffer(fimgContext *ctx, fimgFramebuffer *fb)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	struct drm_exynos_g3d_framebuffer g3d_fb;
	int ret;

	ctx->hw.prot.fbctl.opaque = 0;
	ctx->hw.prot.fbctl.alphathreshold = 0;
	ctx->hw.prot.fbctl.alphaconst = 255;
	ctx->hw.prot.fbctl.colormode = fb->format;
#ifdef FIMG_FIXED_PIPELINE
	FGFP_BITFIELD_SET(ctx->compat.psState.ps,
			PS_SWAP, !!(fb->flags & FGPF_COLOR_MODE_BGR));
#endif
	ctx->fbFlags = fb->flags;
	ctx->fbHeight = fb->height;
	ctx->flipY = fb->flipY;
	fimgQueue(ctx, ctx->hw.prot.fbctl.val, FGPF_FBCTL);

	g3d_fb.fbctl = ctx->hw.prot.fbctl.val;
	g3d_fb.coffset = fb->coffset;
	g3d_fb.doffset = fb->zoffset;
	g3d_fb.width = fb->width;

	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_FRAMEBUFFER_SETUP;
	req.framebuffer.flags = fb->flags;
	req.framebuffer.chandle = fb->chandle;
	req.framebuffer.zhandle = fb->zhandle;
	req.length = sizeof(g3d_fb);
	req.data = &g3d_fb;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_FRAMEBUFFER_SETUP failed (%d)", ret);
}
