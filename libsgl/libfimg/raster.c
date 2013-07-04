/*
 * fimg/raster.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE RASTER ENGINE RELATED FUNCTIONS
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

#define FGRA_COORDREPLACE_VAL(i)	(1 << (i))

/**
 * Sets pixel sampling position.
 * @param ctx Hardware context.
 * @param corner Non-zero to sample at top-left pixel corner, center otherwise.
 */
void fimgSetPixelSamplePos(fimgContext *ctx, int corner)
{
	ctx->hw.rasterizer.samplePos = !!corner;
	fimgQueue(ctx, !!corner, FGRA_PIX_SAMP);
}

/**
 * Controls enable state of depth offset.
 * @see fimgSetDepthOffsetParam
 * @param ctx Hardware context.
 * @param enable Non-zero to enable depth offset.
 */
void fimgEnableDepthOffset(fimgContext *ctx, int enable)
{
	ctx->hw.rasterizer.dOffEn = !!enable;
	fimgQueue(ctx, !!enable, FGRA_D_OFF_EN);
}

/**
 * Sets depth offset parameters.
 * @see fimgSetDepthOffset
 * @param ctx Hardware context.
 * @param factor Depth offset factor.
 * @param units Depth offset units.
 */
void fimgSetDepthOffsetParam(fimgContext *ctx, float factor, float units)
{
	ctx->hw.rasterizer.dOffFactor = factor;
	ctx->hw.rasterizer.dOffUnits = units;
	fimgQueueF(ctx, factor, FGRA_D_OFF_FACTOR);
	fimgQueueF(ctx, units, FGRA_D_OFF_UNITS);
}

/**
 * Controls face culling enable state.
 * @see fimgSetFaceCullFace
 * @see fimgSetFaceCullFront
 * @param ctx Hardware context.
 * @param enable Non-zero to enable.
 */
void fimgSetFaceCullEnable(fimgContext *ctx, int enable)
{
	ctx->hw.rasterizer.cull.enable = !!enable;
	fimgQueue(ctx, ctx->hw.rasterizer.cull.val, FGRA_BFCULL);
}

/**
 * Selects which faces to cull.
 * @see fimgCullingFace
 * @see fimgSetFaceCullEnable
 * @see fimgSetFaceCullFront
 * @param ctx Hardware context.
 * @param face Faces to cull (see fimgCullingFace enum).
 */
void fimgSetFaceCullFace(fimgContext *ctx, unsigned int face)
{
	ctx->hw.rasterizer.cull.face = face;
	fimgQueue(ctx, ctx->hw.rasterizer.cull.val, FGRA_BFCULL);
}

/**
 * Selects front face direction.
 * @see fimgSetFaceCullEnable
 * @see fimgSetFaceCullFace
 * @param ctx Hardware context.
 * @param bCW Non-zero to assume clockwise faces as front, counter-clockwise
 * otherwise.
 */
void fimgSetFaceCullFront(fimgContext *ctx, int bCW)
{
	ctx->hw.rasterizer.cull.clockwise = !!bCW;
	fimgQueue(ctx, ctx->hw.rasterizer.cull.val, FGRA_BFCULL);
}

/**
 * Configures Y coordinate range for early fragment clipping.
 * @param ctx Hardware context.
 * @param ymin Fragments with less Y coordinate will be clipped.
 * @param ymax Fragments with greater or equal Y coordinate will be clipped.
 */
void fimgSetYClip(fimgContext *ctx, unsigned int ymin, unsigned int ymax)
{
	if (ctx->flipY) {
		unsigned int tmp = ctx->fbHeight - ymax;
		ymax = ctx->fbHeight - ymin;
		ymin = tmp;
	}

	ctx->hw.rasterizer.yClip.maxval = ymax;
	ctx->hw.rasterizer.yClip.minval = ymin;

	fimgQueue(ctx, ctx->hw.rasterizer.yClip.val, FGRA_YCLIP);
}

#if 0
/**
 * Configures generation of LOD coefficients for selected attribute.
 * @param ctx Hardware context.
 * @param attrib Attribute index.
 * @param lod Non-zero to generate LOD coefficient.
 * @param ddx Non-zero to generate DDX coefficient.
 * @param ddy Non-zero to generate DDY coefficient.
 */
void fimgSetLODControl(fimgContext *ctx, unsigned int attrib,
						int lod, int ddx, int ddy)
{
	ctx->hw.rasterizer.lodGen.coef[attrib].lod = lod;
	ctx->hw.rasterizer.lodGen.coef[attrib].ddx = ddx;
	ctx->hw.rasterizer.lodGen.coef[attrib].ddy = ddy;

	fimgQueue(ctx, ctx->hw.rasterizer.lodGen.val, FGRA_LODCTL);
}
#endif

/**
 * Configures X coordinate range for early fragment clipping.
 * @param ctx Hardware context.
 * @param xmin Fragments with less X coordinate will be clipped.
 * @param xmax Fragments with greater or equal X coordinate will be clipped.
 */
void fimgSetXClip(fimgContext *ctx, unsigned int xmin, unsigned int xmax)
{
	ctx->hw.rasterizer.xClip.maxval = xmax;
	ctx->hw.rasterizer.xClip.minval = xmin;

	fimgQueue(ctx, ctx->hw.rasterizer.xClip.val, FGRA_XCLIP);
}

/**
 * Sets point width for point rendering.
 * @param ctx Hardware context.
 * @param pWidth Point width.
 */
void fimgSetPointWidth(fimgContext *ctx, float pWidth)
{
	ctx->hw.rasterizer.pointWidth = pWidth;
	fimgQueueF(ctx, pWidth, FGRA_PWIDTH);
}

/**
 * Sets minimum point width used to clamp dynamic point width.
 * @param ctx Hardware context.
 * @param pWidthMin Minimum point width.
 */
void fimgSetMinimumPointWidth(fimgContext *ctx, float pWidthMin)
{
	ctx->hw.rasterizer.pointWidthMin = pWidthMin;
	fimgQueueF(ctx, pWidthMin, FGRA_PSIZE_MIN);
}

/**
 * Sets maximum point width used to clamp dynamic point width.
 * @param ctx Hardware context.
 * @param pWidthMax Maximum point width.
 */
void fimgSetMaximumPointWidth(fimgContext *ctx, float pWidthMax)
{
	ctx->hw.rasterizer.pointWidthMax = pWidthMax;
	fimgQueueF(ctx, pWidthMax, FGRA_PSIZE_MAX);
}

/**
 * Selects attribute used as texture coordinate in point sprite mode.
 * @param ctx Hardware context.
 * @param coordReplaceNum Attribute index.
 */
void fimgSetCoordReplace(fimgContext *ctx, unsigned int coordReplaceNum)
{
	ctx->hw.rasterizer.spriteCoordAttrib = FGRA_COORDREPLACE_VAL(coordReplaceNum);
	fimgQueue(ctx, FGRA_COORDREPLACE_VAL(coordReplaceNum), FGRA_COORDREPLACE);
}

/**
 * Sets line width used for rendering lines.
 * @param ctx Hardware context.
 * @param lWidth Line width.
 */
void fimgSetLineWidth(fimgContext *ctx, float lWidth)
{
	ctx->hw.rasterizer.lineWidth = lWidth;
	fimgQueueF(ctx, lWidth, FGRA_LWIDTH);
}

/**
 * Initializes hardware context of rasterizer block.
 * @param ctx Hardware context.
 */
void fimgCreateRasterizerContext(fimgContext *ctx)
{
	ctx->hw.rasterizer.pointWidth = 1.0f;
	ctx->hw.rasterizer.pointWidthMin = 1.0f;
	ctx->hw.rasterizer.pointWidthMax = 2048.0f;
	ctx->hw.rasterizer.lineWidth = 1.0f;
}
