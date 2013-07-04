/*
 * fimg/primitive.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE PRIMITIVE ENGINE RELATED FUNCTIONS
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

/**
 * Configures primitive engine for processing selected primitive type.
 * @param ctx Hardware context.
 * @param type Primitive type.
 */
void fimgSetVertexContext(fimgContext *ctx, unsigned int type)
{
	ctx->hw.primitive.vctx.type = 1 << type; // See fimgPrimitiveType enum
#ifdef FIMG_INTERPOLATION_WORKAROUND
	ctx->hw.primitive.vctx.vsOut = FIMG_ATTRIB_NUM - 1; // WORKAROUND
#else
	ctx->hw.primitive.vctx.vsOut = ctx->numAttribs - 1; // Without position
#endif

	fimgQueue(ctx, ctx->hw.primitive.vctx.val, FGPE_VERTEX_CONTEXT);
}

/**
 * Specifies shading mode for selected vertex attribute.
 * @param ctx Hardware context.
 * @param en Non-zero to enable flat shading.
 * @param attrib Attribute index.
 */
void fimgSetShadingMode(fimgContext *ctx, int en, unsigned attrib)
{
	ctx->hw.primitive.vctx.flatShadeEn  = !!en;
	ctx->hw.primitive.vctx.flatShadeSel = (!!en << attrib);
}

/**
 * Configures viewport parameters.
 * @param ctx Hardware context.
 * @param x0 X coordinate of viewport origin.
 * @param y0 Y coordinate of viewport origin.
 * @param px Viewport width.
 * @param py Viewport height.
 */
void fimgSetViewportParams(fimgContext *ctx, float x0, float y0, float px, float py)
{
	// local variable declaration
	float half_px = px * 0.5f;
	float half_py;

	// ox: x-coordinate of viewport center
	float ox = x0 + half_px;
	// oy: y-coordindate of viewport center
	float oy;

	if (ctx->flipY) {
		half_py = -py * 0.5f;
		oy = (ctx->fbHeight - y0) + half_py;
	} else {
		half_py = py * 0.5f;
		oy = y0 + half_py;
	}

	ctx->hw.primitive.ox = ox;
	ctx->hw.primitive.oy = oy;
	ctx->hw.primitive.halfPX = half_px;
	ctx->hw.primitive.halfPY = half_py;

	fimgQueueF(ctx, ox, FGPE_VIEWPORT_OX);
	fimgQueueF(ctx, oy, FGPE_VIEWPORT_OY);
	fimgQueueF(ctx, half_px, FGPE_VIEWPORT_HALF_PX);
	fimgQueueF(ctx, half_py, FGPE_VIEWPORT_HALF_PY);
}

/**
 * Configures viewport parameters to bypass viewport transformation.
 * @param ctx Hardware context.
 */
void fimgSetViewportBypass(fimgContext *ctx)
{
	ctx->hw.primitive.ox = 0.0f;
	ctx->hw.primitive.oy = ctx->fbHeight;
	ctx->hw.primitive.halfPX = 1.0f;
	ctx->hw.primitive.halfPY = -1.0f;

	fimgQueueF(ctx, 0.0f, FGPE_VIEWPORT_OX);
	fimgQueueF(ctx, ctx->fbHeight, FGPE_VIEWPORT_OY);
	fimgQueueF(ctx, 1.0f, FGPE_VIEWPORT_HALF_PX);
	fimgQueueF(ctx, -1.0f, FGPE_VIEWPORT_HALF_PY);

	ctx->hw.primitive.halfDistance = 1.0f;
	ctx->hw.primitive.center = 0.0f;

	fimgQueueF(ctx, 1.0f, FGPE_DEPTHRANGE_HALF_F_SUB_N);
	fimgQueueF(ctx, 0.0f, FGPE_DEPTHRANGE_HALF_F_ADD_N);
}

/**
 * Configures depth range of viewport transformation.
 * @param ctx Hardware context.
 * @param n Near depth value, from <0, 1> range.
 * @param f Far depth value, from <0, 1> range.
 */
void fimgSetDepthRange(fimgContext *ctx, float n, float f)
{
	float half_distance = (f - n) * 0.5f;
	float center = (f + n) * 0.5f;

	ctx->hw.primitive.halfDistance = half_distance;
	ctx->hw.primitive.center = center;

	fimgQueueF(ctx, half_distance, FGPE_DEPTHRANGE_HALF_F_SUB_N);
	fimgQueueF(ctx, center, FGPE_DEPTHRANGE_HALF_F_ADD_N);
}

/**
 * Initializes hardware context of primitive engine.
 * @param ctx Hardware context.
 */
void fimgCreatePrimitiveContext(fimgContext *ctx)
{
	ctx->hw.primitive.halfDistance = 0.5f;
	ctx->hw.primitive.center = 0.5f;
}
