/*
* fimg/compat.c
*
* SAMSUNG S3C6410 FIMG-3DSE FIXED PIPELINE COMPATIBILITY FUNCTIONS
*
* Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
*/

#include "fimg.h"

static inline void fimgVShaderWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

#define VSHADER_OFFSET		0x10000

#define FGVS_CFLOAT_START	VSHADER_ADDR(0x4000)
#define FGVS_CINT_START		VSHADER_ADDR(0x8000)
#define FGVS_CBOOL_START	VSHADER_ADDR(0x8400)

#define VSHADER_OFFS(reg)	(VSHADER_OFFSET + (reg))
#define VSHADER_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + VSHADER_OFFS(reg)))

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
void fimgLoadMatrix(unsigned int matrix, float *pfData)
{
	unsigned int i;
	volatile unsigned int *pReg = FGVS_CFLOAT_START + 4*matrix;
	unsigned int *pData = (unsigned int *)pfData;

	for(i = 0; i < 16; i++)
		fimgVShaderWrite(*(pData++), pReg++);
}
