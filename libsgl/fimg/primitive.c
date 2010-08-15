/*
 * fimg/primitive.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE PRIMITIVE ENGINE RELATED FUNCTIONS
 *
 * Copyrights:	2005 by Samsung Electronics Limited (original code)
 *		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
 */

#include "fimg_private.h"

#define PRIMITIVE_OFFSET		0x30000

#define FGPE_VERTEX_CONTEXT		PRIMITIVE_ADDR(0x00)
#define FGPE_VIEWPORT_OX		PRIMITIVE_ADDR(0x04)
#define FGPE_VIEWPORT_OY		PRIMITIVE_ADDR(0x08)
#define FGPE_VIEWPORT_HALF_PX		PRIMITIVE_ADDR(0x0c)
#define FGPE_VIEWPORT_HALF_PY		PRIMITIVE_ADDR(0x10)
#define FGPE_DEPTHRANGE_HALF_F_SUB_N	PRIMITIVE_ADDR(0x14)
#define FGPE_DEPTHRANGE_HALF_F_ADD_N	PRIMITIVE_ADDR(0x18)

#define PRIMITIVE_OFFS(reg)	(PRIMITIVE_OFFSET + (reg))
#define PRIMITIVE_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + PRIMITIVE_OFFS(reg)))

static inline void fimgPrimitiveWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline void fimgPrimitiveWriteF(float data, volatile unsigned int *reg)
{
	*((volatile float *)reg) = data;
}

static inline unsigned int fimgPrimitiveRead(volatile unsigned int *reg)
{
	return *reg;
}

static inline float fimgPrimitiveReadF(volatile unsigned int *reg)
{
	return *((volatile float *)reg);
}

/*
 * Functions
 * TODO: Function inlining
 */
/*****************************************************************************
 * FUNCTIONS:	fimgSetVertexContext
 * SYNOPSIS:	This function specifies the type of primitive to be drawn
 *		and the number of input attributes
 * PARAMETERS:	[IN] pVtx: the pointer of FGL_Vertex strucutre
 *****************************************************************************/
void fimgSetVertexContext(fimgContext *ctx, unsigned int type, unsigned int count)
{
	ctx->primitive.vctx.bits.type = type; // See fimgPrimitiveType enum
	ctx->primitive.vctx.bits.vsOut = count - 1; // Without position

	ctx->numAttribs = count;

	fimgPrimitiveWrite(ctx->primitive.vctx.val, FGPE_VERTEX_CONTEXT);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetViewportParams
 * SYNOPSIS:	This function specifies the viewport parameters.
 * PARAMETERS:	[IN] bYFlip: Not zero if you want y-flipped window coordindate
 *		     zero if otherwise
 *		[IN] x0, y0: origin of viewport in window coordindate system
 *		[IN] px, py: width and height of viewport in terms of pixel
 *		[IN] H: height of window in terms of pixel
 *****************************************************************************/
void fimgSetViewportParams(fimgContext *ctx, float x0, float y0, float px, float py)
{
	// local variable declaration
	float half_px = px * 0.5f;
	float half_py = py * 0.5f;

	// ox: x-coordinate of viewport center
	float ox = x0 + half_px;
	// oy: y-coordindate of viewport center
	float oy = y0 + half_py;

	ctx->primitive.ox = ox;
	ctx->primitive.oy = oy;
	ctx->primitive.halfPX = half_px;
	ctx->primitive.halfPY = half_py;

	fimgPrimitiveWriteF(ox, FGPE_VIEWPORT_OX);
	fimgPrimitiveWriteF(oy, FGPE_VIEWPORT_OY);
	fimgPrimitiveWriteF(half_px, FGPE_VIEWPORT_HALF_PX);
	fimgPrimitiveWriteF(half_py, FGPE_VIEWPORT_HALF_PY);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetDepthRange
 * SYNOPSIS:	This function defines an encoding for z-coordinate that's performed
 *		during the viewport transformation.
 * PARAMETERS:	[IN] n: near value ( n should be in [0, 1])
 *		[IN] f: far value (f should be in [0, 1])
 *****************************************************************************/
void fimgSetDepthRange(fimgContext *ctx, float n, float f)
{
	float half_distance = (f - n) * 0.5f;
	float center = (f + n) * 0.5f;

	ctx->primitive.halfDistance = half_distance;
	ctx->primitive.center = center;

	fimgPrimitiveWriteF(half_distance, FGPE_DEPTHRANGE_HALF_F_SUB_N);
	fimgPrimitiveWriteF(center, FGPE_DEPTHRANGE_HALF_F_ADD_N);
}

void fimgRestorePrimitiveState(fimgContext *ctx)
{
	fimgPrimitiveWrite(ctx->primitive.vctx.val, FGPE_VERTEX_CONTEXT);
	fimgPrimitiveWriteF(ctx->primitive.ox, FGPE_VIEWPORT_OX);
	fimgPrimitiveWriteF(ctx->primitive.oy, FGPE_VIEWPORT_OY);
	fimgPrimitiveWriteF(ctx->primitive.halfPX, FGPE_VIEWPORT_HALF_PX);
	fimgPrimitiveWriteF(ctx->primitive.halfPY, FGPE_VIEWPORT_HALF_PY);
	fimgPrimitiveWriteF(ctx->primitive.halfDistance, FGPE_DEPTHRANGE_HALF_F_SUB_N);
	fimgPrimitiveWriteF(ctx->primitive.center, FGPE_DEPTHRANGE_HALF_F_ADD_N);
}
