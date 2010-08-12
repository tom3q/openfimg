/*
 * fimg/primitive.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE PRIMITIVE ENGINE RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _PRIMITIVE_H_
#define _PRIMITIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * Primitive Engine
 */

/* Type definitions */
typedef enum {
	FGPE_TRIANGLES		= (1 << 7),
	FGPE_TRIANGLE_FAN	= (1 << 6),
	FGPE_TRIANGLE_STRIP	= (1 << 5),
	FGPE_LINES		= (1 << 4),
	FGPE_LINE_LOOP		= (1 << 3),
	FGPE_LINE_STRIP		= (1 << 2),
	FGPE_POINTS		= (1 << 1),
	FGPE_POINT_SPRITE	= (1 << 0)
} fimgPrimitiveType;

typedef union {
	unsigned int val;
	struct {
		unsigned intUse		:2;
		unsigned		:3;
		unsigned type		:8;
		unsigned pointSize	:1;
		unsigned		:4;
		unsigned vsOut		:4;
		unsigned flatShadeEn	:1;
		unsigned flatShadeSel	:9;
	} bits;
} fimgVertexContext;

/* Functions */
void fimgSetVertexContext(fimgPrimitiveType type,
			unsigned int count);
void fimgSetViewportParams(/*int bYFlip,*/
			   float x0,
			   float y0,
			   float px,
			   float py/*,
			   float H*/);
void fimgSetDepthRange(float n, float f);

#ifdef __cplusplus
}
#endif

#endif
