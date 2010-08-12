/*
 * fimg/raster.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE RASTER ENGINE RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _RASTER_H_
#define _RASTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * Raster engine
 */

/* Type definitions */
typedef enum {
	FGRA_DEPTH_OFFSET_FACTOR,
	FGRA_DEPTH_OFFSET_UNITS,
	FGRA_DEPTH_OFFSET_R
} fimgDepthOffsetParam;

typedef enum {
	FGRA_BFCULL_FACE_BACK = 0,
	FGRA_BFCULL_FACE_FRONT,
	FGRA_BFCULL_FACE_BOTH = 3
} fimgCullingFace;

typedef union {
	unsigned int val;
	struct {
		unsigned		:8;
		struct {
			unsigned ddy	:1;
			unsigned ddx	:1;
			unsigned lod	:1;
		} coef[8];
	} bits;
} fimgLODControl;

/* Functions */
void fimgSetPixelSamplePos(int corner);
void fimgEnableDepthOffset(int enable);
int fimgSetDepthOffsetParam(fimgDepthOffsetParam param,
			    unsigned int value);
int fimgSetDepthOffsetParamF(fimgDepthOffsetParam param,
			     float value);
void fimgSetFaceCullControl(int enable,
			    int bCW,
			    fimgCullingFace face);
void fimgSetYClip(unsigned int ymin, unsigned int ymax);
void fimgSetLODControl(fimgLODControl ctl);
void fimgSetXClip(unsigned int xmin, unsigned int xmax);
void fimgSetPointWidth(float pWidth);
void fimgSetMinimumPointWidth(float pWidthMin);
void fimgSetMaximumPointWidth(float pWidthMax);
void fimgSetCoordReplace(unsigned int coordReplaceNum);
void fimgSetLineWidth(float lWidth);

#ifdef __cplusplus
}
#endif

#endif
