/*
 * libsgl/fglpixelformat.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
 * 		2011 by Jordi Santiago Provencio < >
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

#ifndef _FGLPIXELFORMAT_H_
#define _FGLPIXELFORMAT_H_

#include <GLES/gl.h>
#include <GLES/glext.h>

/** Indices of color components inside a multi-component value. */
enum FGLColorComponent {
	FGL_COMP_RED = 0,
	FGL_COMP_GREEN,
	FGL_COMP_BLUE,
	FGL_COMP_ALPHA,

	FGL_COMP_LUM = 0,

	FGL_COMP_DEPTH = 0,

	FGL_COMP_IDX = 0,

	FGL_COMP_C0 = 0,
	FGL_COMP_C1 = 1,
	FGL_COMP_LUT = 2,

	FGL_COMP_Y0 = 0,
	FGL_COMP_U = 1,
	FGL_COMP_Y1 = 2,
	FGL_COMP_V = 3
};

/** Pixel formats supported inside OpenFIMG. */
enum FGLPixelFormatEnum {
	/* Unspecified format */
	FGL_PIXFMT_NONE = 0,
	/* Renderable formats */
	FGL_PIXFMT_XRGB1555,
	FGL_PIXFMT_RGB565,
	FGL_PIXFMT_ARGB4444,
	FGL_PIXFMT_ARGB1555,
	FGL_PIXFMT_XRGB8888,
	FGL_PIXFMT_ARGB8888,
	FGL_PIXFMT_XBGR8888,
	FGL_PIXFMT_ABGR8888,
	/* Non-renderable formats */
	FGL_PIXFMT_RGBA4444,
	FGL_PIXFMT_RGBA5551,
	FGL_PIXFMT_DEPTH16,
	FGL_PIXFMT_AL88,
	FGL_PIXFMT_L8,
	/* Compressed formats */
	FGL_PIXFMT_1BPP,
	FGL_PIXFMT_2BPP,
	FGL_PIXFMT_4BPP,
	FGL_PIXFMT_8BPP,
	FGL_PIXFMT_S3TC,
	/* YUV formats */
	FGL_PIXFMT_Y1VY0U,
	FGL_PIXFMT_VY1UY0,
	FGL_PIXFMT_Y1UY0V,
	FGL_PIXFMT_UY1VY0,
};

/**
 * Flags defining special characteristics of pixel format.
 * Defined to be equal to respective FGPF_ and FGTU_ flags in fimg.h
 */
enum FGLPixelFormatFlags {
	/** Alpha component is located in least significant bit. */
	FGL_PIX_ALPHA_LSB	= (1 << 0),
	/** Color component order is BGR (as opposed to RGB). */
	FGL_PIX_BGR		= (1 << 1),
	/** Alpha component must be set to opaque value. */
	FGL_PIX_OPAQUE		= (1 << 2)
};

/** A class holding information about pixel format */
struct FGLPixelFormat {
	struct {
		/** Index of first bit of the component. */
		uint8_t pos;
		/** Bits used by the component. */
		uint8_t size;
	} comp[4]; /**< List of color components. (See ::FGLColorComponent) */
	/** Read format as specified by OpenGL ES specification. */
	GLenum readFormat;
	/** Read type as specified by OpenGL ES specification. */
	GLenum readType;
	/** Pixel size in bytes. */
	uint32_t pixelSize;
	/** Texture format for libfimg. */
	uint32_t texFormat;
	/** Framebuffer format for libfimg. */
	uint32_t pixFormat;
	/** Special characteristics. (See ::FGLPixelFormatFlags.) */
	uint32_t flags;

	/**
	 * Gets a pointer to FGLPixelFormat object describing pixel format
	 * with given ID.
	 * @param format ID of pixel format.
	 * @return Pointer to FGLPixelFormat object describing the format.
	 */
	static const FGLPixelFormat *get(unsigned int format);

private:
	static const FGLPixelFormat table[];
};

#endif /* _FGLPIXELFORMAT_H_ */
