/*
* fimg/compat.c
*
* SAMSUNG S3C6410 FIMG-3DSE FIXED PIPELINE COMPATIBILITY FUNCTIONS
*
* Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
*/

#include "fimg_private.h"

#define VSHADER_OFFSET		0x10000

#define FGVS_CFLOAT_START	0x14000
#define FGVS_CINT_START		0x18000
#define FGVS_CBOOL_START	0x18400

/**
	Compatibility module for OpenGL ES 1.1 operation.

	Use functions from this file only when the fixed-pipeline shader is loaded.

	matrix	<=>	const float mapping:
	PROJECTION	0-3
	MODELVIEW	4-7
	MODELVIEW_INV	8-11
	TEXTURE(0)	12-15
	TEXTURE(1)	16-19
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
		fimgWrite(ctx, pData[0],  pReg +  0);
		fimgWrite(ctx, pData[4],  pReg +  4);
		fimgWrite(ctx, pData[8],  pReg +  8);
		fimgWrite(ctx, pData[12], pReg + 12);
		pReg += 16;
		pData++;
	}
}
