/*
 * fimg/shaders.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE SHADERS RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _SHADERS_H_
#define _SHADERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * OpenGL 1.1 compatibility
 */

void fimgLoadMatrix(unsigned int matrix, float *pData);

#ifdef __cplusplus
}
#endif

#endif
