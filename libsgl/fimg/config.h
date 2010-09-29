/*
 * fimg/config.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (CONFIGURATION HEADER)
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _FIMG_CONFIG_H_
#define _FIMG_CONFIG_H_

/* Workaround for rasterizer bug. Use 2 for alternative drawing if 1 fails. */
#define FIMG_INTERPOLATION_WORKAROUND	1

/* Workaround for clipper bug. */
//#define FIMG_CLIPPER_WORKAROUND	1

/* Enable buffered geometry transfer */
#define FIMG_USE_VERTEX_BUFFER

/* Use vertex buffer 0 stride for constant attributes */
#define FIMG_USE_STRIDE_0_CONSTANTS

/* Use busy waiting instead of interrupts for FIFO transfers */
//#define FIMG_FIFO_BUSY_WAIT

#endif /* _FIMG_CONFIG_H_ */
