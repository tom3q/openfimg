/* g2d/s3c_g2d.h
 *
 * Copyright (c) 2010 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * Samsung S3C G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _S3C_G2D_H_
#define _S3C_G2D_H_

extern "C" {
////////////////////////////////////////////////////////////////////////////////

#include <linux/ioctl.h>

#define G2D_IOCTL_MAGIC			'G'

/*
 * S3C_G2D_BITBLT
 * Start hardware bitblt operation.
 * Argument:	a pointer to struct s3c_g2d_req with operation parameters
 * Returns:	  0 on success,
 *		< 0, on error
 */
#define S3C_G2D_BITBLT			_IOW(G2D_IOCTL_MAGIC, 0, struct s3c_g2d_req)

/*
 * S3C_G2D_FILLRECT
 * Start hardware fillrect operation.
 * Argument:	a pointer to struct s3c_g2d_fillrect with operation parameters
 * Returns:	  0 on success,
 *		< 0, on error
 */
#define S3C_G2D_FILLRECT		_IOW(G2D_IOCTL_MAGIC, 8, struct s3c_g2d_fillrect)

/*
 * S3C_G2D_SET_TRANSFORM
 * Set requested image transformation.
 * Argument:	one of G2D_ROT_* values
 */
#define S3C_G2D_SET_TRANSFORM		_IO(G2D_IOCTL_MAGIC, 1)
enum
{
	G2D_ROT_0	= 1 << 0,
	G2D_ROT_90	= 1 << 1,
	G2D_ROT_180	= 1 << 2,
	G2D_ROT_270	= 1 << 3,
	G2D_ROT_FLIP_X	= 1 << 4,
	G2D_ROT_FLIP_Y	= 1 << 5
};

/*
 * S3C_G2D_SET_ALPHA_VAL
 * Set requested plane alpha value.
 * Argument:	a value from <0, ALPHA_VALUE_MAX> range
 */
#define S3C_G2D_SET_ALPHA_VAL		_IO(G2D_IOCTL_MAGIC, 2)
#define ALPHA_VALUE_MAX			255

/*
 * S3C_G2D_SET_RASTER_OP
 * Set requested raster operation.
 * Argument:	an 8-bit value defining the operation
 */
#define S3C_G2D_SET_RASTER_OP		_IO(G2D_IOCTL_MAGIC, 3)
#define G2D_ROP_SRC_ONLY		(0xf0)
#define G2D_ROP_3RD_OPRND_ONLY		(0xaa)
#define G2D_ROP_DST_ONLY		(0xcc)
#define G2D_ROP_SRC_OR_DST		(0xfc)
#define G2D_ROP_SRC_OR_3RD_OPRND	(0xfa)
#define G2D_ROP_SRC_AND_DST		(0xc0)
#define G2D_ROP_SRC_AND_3RD_OPRND	(0xa0)
#define G2D_ROP_SRC_XOR_3RD_OPRND	(0x5a)
#define G2D_ROP_DST_OR_3RD_OPRND	(0xee)

/*
 * S3C_G2D_SET_BLENDING
 * Set requested alpha blending mode.
 * Argument:	one of G2D_*_ALPHA values
 */
#define S3C_G2D_SET_BLENDING		_IO(G2D_IOCTL_MAGIC, 4)
enum
{
	G2D_NO_ALPHA = 0,
	G2D_PLANE_ALPHA,
	G2D_PIXEL_ALPHA // with fallback to plane alpha
};

/* Maximum values for the hardware */
#define G2D_MAX_WIDTH			(2048)
#define G2D_MAX_HEIGHT			(2048)

/* Image data */
struct s3c_g2d_image
{
	uint32_t	base;	// image base address (NULL to use fd)
	int	        fd;	// image file descriptor (for PMEM)
	uint32_t	offs;	// buffer offset
	uint32_t	w;	// image full width
	uint32_t	h;	// image full height
	uint32_t	l;	// x coordinate of left edge
	uint32_t	t;	// y coordinate of top edge
	uint32_t	r;	// x coordinage of right edge
	uint32_t	b;	// y coordinate of bottom edge
	uint32_t	fmt;	// color format
};

/* Supported formats for struct s3c_g2d_image fmt field */
enum
{
	G2D_RGB16 = 0,
	G2D_RGBA16,
	G2D_ARGB16,
	G2D_RGBA32,
	G2D_ARGB32,
	G2D_XRGB32,
	G2D_RGBX32
};

/* Bitblt request */
struct s3c_g2d_req
{
	struct s3c_g2d_image src; // source image
	struct s3c_g2d_image dst; // destination image
};

/* Fillrect request */
struct s3c_g2d_fillrect
{
	struct s3c_g2d_image dst;
	uint32_t color;
	uint8_t alpha;
};

////////////////////////////////////////////////////////////////////////////////
}

#endif
