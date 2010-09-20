/*
* fimg/compat.c
*
* SAMSUNG S3C6410 FIMG-3DSE FIXED PIPELINE COMPATIBILITY FUNCTIONS
*
* Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
*/

#include "fimg_private.h"

#define FGVS_CFLOAT_START	0x14000
#define FGVS_CINT_START		0x18000
#define FGVS_CBOOL_START	0x18400

#define FGPS_CFLOAT_START	0x44000
#define FGPS_CINT_START		0x48000
#define FGPS_CBOOL_START	0x48400

/**
	Compatibility module for OpenGL ES 1.1 operation.

	Use functions from this file only when the fixed-pipeline shader is loaded.

	matrix	<=>	const float mapping:
	TRANSFORMATION	0-3
	MODELVIEW_INV	4-7
	TEXTURE(0)	8-11
	TEXTURE(1)	12-5
*/

/*****************************************************************************
 * FUNCTIONS:	fimgLoadMatrix
 * SYNOPSIS:	This function loads the specified matrix (4x4) into const float
 *		registers of vertex shader.
 * PARAMETERS:	[IN] matrix - which matrix to load (FGL_MATRIX_*)
 *		[IN] pData - pointer to matrix elements in column-major ordering
 *****************************************************************************/
void fimgLoadMatrix(fimgContext *ctx, unsigned int matrix, float *pfData)
{
	unsigned int i;
	unsigned int pReg = FGVS_CFLOAT_START + 64*matrix;
	unsigned int *pData = (unsigned int *)pfData;

	for(i = 0; i < 4; i++) {
		fimgWrite(ctx, *(pData++), pReg);
		pReg += 4;
		fimgWrite(ctx, *(pData++), pReg);
		pReg += 4;
		fimgWrite(ctx, *(pData++), pReg);
		pReg += 4;
		fimgWrite(ctx, *(pData++), pReg);
		pReg += 4;
	}
}

void fimgEnableTexture(fimgContext *ctx, unsigned int unit)
{
	unsigned int val;
	val = fimgRead(ctx, FGPS_CBOOL_START);
	val |= (1 << unit);
	fimgWrite(ctx, val, FGPS_CBOOL_START);
}

void fimgDisableTexture(fimgContext *ctx, unsigned int unit)
{
	unsigned int val;
	val = fimgRead(ctx, FGPS_CBOOL_START);
	val &= ~(1 << unit);
	fimgWrite(ctx, val, FGPS_CBOOL_START);
}
