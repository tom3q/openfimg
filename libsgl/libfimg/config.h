/*
 * fimg/config.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (CONFIGURATION HEADER)
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

#ifndef _FIMG_CONFIG_H_
#define _FIMG_CONFIG_H_

/* Workaround for rasterizer bug. */
#define FIMG_INTERPOLATION_WORKAROUND

/* Enable buffered geometry transfer */
#define FIMG_USE_VERTEX_BUFFER

/* Use vertex buffer 0 stride for constant attributes */
#define FIMG_USE_STRIDE_0_CONSTANTS

/* Use busy waiting instead of interrupts for FIFO transfers */
//#define FIMG_FIFO_BUSY_WAIT

/* Use fixed pipeline emulation */
#define FIMG_FIXED_PIPELINE

/* Flip the Y axis */
#define FIMG_COORD_FLIP_Y

/* Dump register state before sending draw request (for debugging) */
//#define FIMG_DUMP_STATE_BEFORE_DRAW

/* Enable clipper workaround */
//#define FIMG_CLIPPER_WORKAROUND

#endif /* _FIMG_CONFIG_H_ */
