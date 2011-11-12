/**
 * libsgl/glesPixel.cpp
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
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

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

GL_API void GL_APIENTRY glPixelStorei (GLenum pname, GLint param)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_UNPACK_ALIGNMENT:
		switch (param) {
		case 1:
		case 2:
		case 4:
		case 8:
			ctx->unpackAlignment = param;
			break;
		default:
			setError(GL_INVALID_VALUE);
		}
		break;
	case GL_PACK_ALIGNMENT:
		switch (param) {
		case 1:
		case 2:
		case 4:
		case 8:
			ctx->packAlignment = param;
			break;
		default:
			setError(GL_INVALID_VALUE);
		}
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

/**
	Reading pixels
*/

static inline void unpackPixel555(uint8_t *dst, uint16_t src)
{
	dst[0] = (src & 0x7c00) >> 7;
	dst[1] = (src & 0x03e0) >> 2;
	dst[2] = (src & 0x001f) << 3;
	dst[3] = 0xff;
}

static void convertToUByteRGBA555(FGLContext *ctx, uint8_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const uint16_t *src = (const uint16_t *)ctx->surface.draw->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*ctx->surface.stride;
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(ctx->surface.height - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel555(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
		dst -= dstStride;
	} while (--height);
}

static inline void unpackPixel565(uint8_t *dst, uint16_t src)
{
	dst[0] = (src & 0xf800) >> 8;
	dst[1] = (src & 0x07e0) >> 3;
	dst[2] = (src & 0x001f) << 3;
	dst[3] = 0xff;
}

static void convertToUByteRGBA565(FGLContext *ctx, uint8_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const uint16_t *src = (const uint16_t *)ctx->surface.draw->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*ctx->surface.stride;
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(ctx->surface.height - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel565(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
		dst -= dstStride;
	} while (--height);
}

static inline void unpackPixel4444(uint8_t *dst, uint16_t src)
{
	dst[0] = (src & 0x0f00) >> 4;
	dst[1] = (src & 0x00f0) >> 0;
	dst[2] = (src & 0x000f) << 4;
	dst[3] = (src & 0xf000) >> 8;
}

static void convertToUByteRGBA4444(FGLContext *ctx, uint8_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const uint16_t *src = (const uint16_t *)ctx->surface.draw->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*ctx->surface.stride;
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(ctx->surface.height - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel4444(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
		dst -= dstStride;
	} while (--height);
}

static inline void unpackPixel1555(uint8_t *dst, uint16_t src)
{
	dst[0] = (src & 0x7c00) >> 7;
	dst[1] = (src & 0x03e0) >> 2;
	dst[2] = (src & 0x001f) << 3;
	dst[3] = (src & 0x8000) ? 0xff : 0x00;
}

static void convertToUByteRGBA1555(FGLContext *ctx, uint8_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const uint16_t *src = (const uint16_t *)ctx->surface.draw->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*ctx->surface.stride;
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(ctx->surface.height - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel1555(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
		dst -= dstStride;
	} while (--height);
}

static inline void unpackPixel8888(uint32_t *dst, uint32_t src)
{
	uint8_t r, g, b, a;

	b = src & 0xff;
	src >>= 8;
	g = src & 0xff;
	src >>= 8;
	r = src & 0xff;
	src >>= 8;
	a = src;

	*dst = (a << 24) | (b << 16) | (g << 8) | r;
}

static void convertToUByteBGRA8888(FGLContext *ctx, uint32_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const uint32_t *src = (const uint32_t *)ctx->surface.draw->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = ((4*width + alignment - 1) & ~(alignment - 1)) / 4;
	unsigned srcStride = ctx->surface.stride;
	unsigned xOffset = x;
	unsigned yOffset = srcStride*(ctx->surface.height - y - height);
	unsigned srcPad = srcStride - width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		unsigned w = width;
		uint32_t *line = dst;
		do {
			unpackPixel8888(line++, *src++);
		} while (--w);
		src += srcPad;
		dst -= dstStride;
	} while (--height);
}

static void fallbackCopy(uint8_t *dst, const uint8_t *src, unsigned len)
{
	while (len--)
		*dst++ = *src++;
}

static void burstCopy16(void *dst, const void *src, unsigned len)
{
	const uint8_t *s = (const uint8_t *)src;
	uint8_t *d = (uint8_t *)dst;
	unsigned srcAlign = (4 - (unsigned)src % 4) % 4;
	unsigned dstAlign = (4 - (unsigned)dst % 4) % 4;

	if (srcAlign != dstAlign)
		return fallbackCopy(d, s, len);

	if (srcAlign >= len)
		return fallbackCopy(d, s, len);

	if (srcAlign) {
		fallbackCopy(d, s, srcAlign);
		s += srcAlign;
		d += srcAlign;
		len -= srcAlign;
	}

	while (len >= 16) {
		asm(	"ldmia %0!, {r0-r3}\n"
			"stmia %1!, {r0-r3}\n"
			: "=r"(s), "=r"(d)
			: "0"(s), "1"(d)
			: "r0", "r1", "r2", "r3");
		len -= 16;
	}

	if (len)
		fallbackCopy(d, s, len);
}

GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y,
				GLsizei width, GLsizei height, GLenum format,
				GLenum type, GLvoid *pixels)
{
	FGLContext *ctx = getContext();
	FGLSurface *draw = ctx->surface.draw;

	if (!draw || !draw->vaddr) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (width <= 0 || height <= 0 || x < 0 || y < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if (x >= ctx->surface.width || y >= ctx->surface.height)
		// Nothing to copy
		return;

	draw->flush();

	unsigned srcBpp = fglColorConfigs[ctx->surface.format].pixelSize;
	unsigned srcStride = srcBpp * ctx->surface.stride;
	unsigned alignment = ctx->packAlignment;

	if (format == fglColorConfigs[ctx->surface.format].readFormat
		&& type == fglColorConfigs[ctx->surface.format].readType)
	{
		// No format conversion needed
		unsigned yOffset = (ctx->surface.height - y - 1) * srcStride;
		const uint8_t *src = (const uint8_t *)draw->vaddr + yOffset;

		unsigned dstStride = (srcBpp*width + alignment - 1) & ~(alignment - 1);
		uint8_t *dst = (uint8_t *)pixels;

		if (y + height > ctx->surface.height)
			height = ctx->surface.height - y;

		if (!x && width == ctx->surface.width) {
			// Copy whole lines line-by-line
			if (srcStride < 32) {
				do {
					fallbackCopy(dst, src, srcStride);
					dst += dstStride;
					src -= srcStride;
				} while (--height);
			} else {
				do {
					burstCopy16(dst, src, srcStride);
					dst += dstStride;
					src -= srcStride;
				} while (--height);
			}

			// We are done
			return;
		}

		// Copy line parts line-by-line
		if (x + width > ctx->surface.width)
			width = ctx->surface.width - x;

		unsigned xOffset = srcBpp * x;
		unsigned srcWidth = srcBpp * width;

		if (srcWidth < 32) {
			do {
				fallbackCopy(dst, src + xOffset, srcWidth);
				dst += dstStride;
				src -= srcStride;
			} while (--height);
		} else {
			do {
				burstCopy16(dst, src + xOffset, srcWidth);
				dst += dstStride;
				src -= srcStride;
			} while (--height);
		}

		// We are done
		return;
	}

	if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
		// Convert to GL_RGBA and GL_UNSIGNED_BYTE
		switch (ctx->surface.format) {
		case FGPF_COLOR_MODE_555:
			convertToUByteRGBA555(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGPF_COLOR_MODE_565:
			convertToUByteRGBA565(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGPF_COLOR_MODE_4444:
			convertToUByteRGBA4444(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGPF_COLOR_MODE_1555:
			convertToUByteRGBA1555(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		/* BGRX8888 -> RGBX8888 */
		case FGPF_COLOR_MODE_0888:
		case FGPF_COLOR_MODE_8888:
			convertToUByteBGRA8888(ctx, (uint32_t *)pixels,
							x, y, width, height);
			break;
		}
		// We are done
		return;
	}

	setError(GL_INVALID_ENUM);
}

/**
	Clearing buffers
*/

static void *fillSingle16(void *buf, uint16_t val, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;

	do {
		*(buf16++) = val;
	} while (--cnt);

	return buf16;
}

static void *fillSingle16masked(void *buf, uint16_t val,
						uint16_t mask, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint16_t tmp;

	do {
		tmp = *buf16 & mask;
		tmp |= val;
		*(buf16++) = tmp;
	} while (--cnt);

	return buf16;
}

static void *fillSingle32(void *buf, uint32_t val, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;

	do {
		*(buf32++) = val;
	} while (--cnt);

	return buf32;
}

static void *fillBurst32(void *buf, uint32_t val, size_t cnt)
{
	asm volatile (
		"mov r0, %1\n\t"
		"mov r1, %1\n\t"
		"mov r2, %1\n\t"
		"mov r3, %1\n\t"
		"1:\n\t"
		"stmia %0!, {r0-r3}\n\t"
		"subs %2, %2, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(cnt)
		: "r0", "r1", "r2", "r3"
	);

	return buf;
}

static void *fillSingle32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t tmp;

	do {
		tmp = *buf32 & mask;
		tmp |= val;
		*(buf32++) = tmp;
	} while (--cnt);

	return buf32;
}

static void *fillBurst32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	asm volatile (
		"mov r0, %1\n\t"
		"mov r1, %1\n\t"
		"mov r2, %1\n\t"
		"mov r3, %1\n\t"
		"1:\n\t"
		"ldmia %0, {r0-r3}\n\t"
		"and r0, r0, %2\n\t"
		"and r1, r1, %2\n\t"
		"and r2, r2, %2\n\t"
		"and r3, r3, %2\n\t"
		"orr r0, r0, %1\n\t"
		"orr r1, r1, %1\n\t"
		"orr r2, r2, %1\n\t"
		"orr r3, r3, %1\n\t"
		"stmia %0!, {r0-r3}\n\t"
		"subs %3, %3, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(mask), "r"(cnt)
		: "r0", "r1", "r2", "r3"
	);

	return buf;
}

static void fill32(void *buf, uint32_t val, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t align = (intptr_t)buf32 % 16;

	if (align) {
		align = 16 - align;
		align /= 4;
		if (align >= cnt) {
			fillSingle32(buf32, val, cnt);
			return;
		}
		buf32 = (uint32_t *)fillSingle32(buf32, val, align);
		cnt -= align;
	}

	if(cnt / 4)
		buf32 = (uint32_t *)fillBurst32(buf32, val, cnt / 4);

	if(cnt % 4)
		fillSingle32(buf32, val, cnt % 4);
}

static void fill32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t align = (intptr_t)buf32 % 16;

	val &= mask;
	mask = ~mask;

	if (align) {
		align = 16 - align;
		align /= 4;
		if (align >= cnt) {
			fillSingle32masked(buf32, val, mask, cnt);
			return;
		}
		buf32 = (uint32_t *)fillSingle32masked(buf32, val, mask, align);
		cnt -= align;
	}

	if(cnt / 4)
		buf32 = (uint32_t *)fillBurst32masked(buf32, val, mask, cnt / 4);

	if(cnt % 4)
		fillSingle32masked(buf32, val, mask, cnt % 4);
}

static void fill16(void *buf, uint16_t val, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint32_t align = (intptr_t)buf16 % 16;

	if (align) {
		align = 16 - align;
		align /= 2;
		if (align >= cnt) {
			fillSingle16(buf16, val, cnt);
			return;
		}
		buf16 = (uint16_t *)fillSingle16(buf16, val, align);
		cnt -= align;
	}

	if(cnt / 8)
		buf16 = (uint16_t *)fillBurst32(buf16, (val << 16) | val, cnt / 8);

	if(cnt % 8)
		fillSingle16(buf16, val, cnt % 8);
}

static void fill16masked(void *buf, uint16_t val, uint16_t mask, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint32_t align = (intptr_t)buf16 % 16;

	val &= mask;
	mask = ~mask;

	if (align) {
		align = 16 - align;
		align /= 2;
		if (align >= cnt) {
			fillSingle16masked(buf16, val, mask, cnt);
			return;
		}
		buf16 = (uint16_t *)fillSingle16masked(buf16, val, mask, align);
		cnt -= align;
	}

	if(cnt / 8)
		buf16 = (uint16_t *)fillBurst32masked(buf16, (val << 16) | val,
						(mask << 16) | mask, cnt / 8);

	if(cnt % 8)
		fillSingle16masked(buf16, val, mask, cnt % 8);
}

static uint32_t getFillColor(FGLContext *ctx,
						uint32_t *mask, bool *is32bpp)
{
	uint8_t r, g, b, a;
	uint8_t rMask, gMask, bMask, aMask;
	uint32_t val = 0;
	uint32_t mval = 0;
	const FGLColorConfigDesc *configDesc;

	configDesc = fglGetColorConfigDesc(ctx->surface.format);
	if (!configDesc)
		return 0;

	r	= ubyteFromClampf(ctx->clear.red);
	rMask	= 0xff;
	r	>>= (8 - configDesc->red);
	rMask	>>= (8 - configDesc->red);

	g	= ubyteFromClampf(ctx->clear.green);
	gMask	= 0xff;
	g	>>= (8 - configDesc->green);
	gMask	>>= (8 - configDesc->green);

	b	= ubyteFromClampf(ctx->clear.blue);
	bMask	= 0xff;
	b	>>= (8 - configDesc->blue);
	bMask	>>= (8 - configDesc->blue);

	if (configDesc->opaque)
		a = 0xff;
	else
		a = ubyteFromClampf(ctx->clear.alpha);
	aMask	= 0xff;
	a	>>= (8 - configDesc->alpha);
	aMask	>>= (8 - configDesc->alpha);

	val |= r << configDesc->redPos;
	val |= g << configDesc->greenPos;
	val |= b << configDesc->bluePos;
	val |= a << configDesc->alphaPos;

	if (!ctx->perFragment.mask.red)
		mval |= rMask << configDesc->redPos;
	if (!ctx->perFragment.mask.green)
		mval |= gMask << configDesc->greenPos;
	if (!ctx->perFragment.mask.blue)
		mval |= bMask << configDesc->bluePos;
	if (!ctx->perFragment.mask.alpha)
		mval |= aMask << configDesc->alphaPos;

	*is32bpp = (configDesc->pixelSize == 4);
	*mask = mval;

	return val;
}

static inline uint32_t getFillDepth(FGLContext *ctx,
					uint32_t *mask, GLbitfield mode)
{
	uint32_t mval = 0xffffffff;
	uint32_t val = 0;

	if (mode & GL_DEPTH_BUFFER_BIT && ctx->perFragment.mask.depth) {
		mval &= ~0x00ffffff;
		val |= (uint32_t)(ctx->clear.depth * 0x00ffffff);
	}

	if (mode & GL_STENCIL_BUFFER_BIT) {
		mval &= ~ctx->perFragment.mask.stencil << 24;
		val |= ctx->clear.stencil << 24;
	}

	*mask = mval;
	return val;
}

static void fglClear(FGLContext *ctx, GLbitfield mode)
{
	FUNCTION_TRACER;
	FGLSurface *draw = ctx->surface.draw;
	uint32_t stride = ctx->surface.stride;
	bool lineByLine = false;
	int32_t l, b, t, w, h;

	l = 0;
	b = 0;
	w = ctx->surface.width;
	h = ctx->surface.height;
	if (ctx->perFragment.scissor.enabled) {
		if (ctx->perFragment.scissor.left > 0)
			l = ctx->perFragment.scissor.left;
		if (ctx->perFragment.scissor.bottom > 0)
			b = ctx->perFragment.scissor.bottom;
		if (ctx->perFragment.scissor.left + ctx->perFragment.scissor.width <= ctx->surface.width)
			w = ctx->perFragment.scissor.left + ctx->perFragment.scissor.width - l;
		else
			w = ctx->surface.width - l;
		if (ctx->perFragment.scissor.bottom + ctx->perFragment.scissor.height <= ctx->surface.height)
			h = ctx->perFragment.scissor.bottom + ctx->perFragment.scissor.height - b;
		else
			h = ctx->surface.height - b;
	}
	t = ctx->surface.height - b - h;

	if (!h || !w)
		return;

	//lineByLine |= (l > 0);
	lineByLine |= (w < ctx->surface.width);
	//lineByLine &= (ctx->perFragment.scissor.enabled == GL_TRUE);

	if (mode & GL_COLOR_BUFFER_BIT) {
		bool is32bpp = false;
		uint32_t mask = 0;
		uint32_t color = getFillColor(ctx, &mask, &is32bpp);

		if (lineByLine) {
			if (!is32bpp) {
				uint16_t *buf16 = (uint16_t *)draw->vaddr;
				buf16 += t * stride;
				if (mask & 0xffff) {
					do {
						uint16_t *line = buf16 + l;
						fill16masked(line, color, ~mask, w);
						buf16 += stride;
					} while (--h);
				} else {
					do {
						uint16_t *line = buf16 + l;
						fill16(line, color, w);
						buf16 += stride;
					} while (--h);
				}
			} else {
				uint32_t *buf32 = (uint32_t *)draw->vaddr;
				buf32 += t * stride;
				if (mask) {
					do {
						uint32_t *line = buf32 + l;
						fill32masked(line, color, ~mask, w);
						buf32 += stride;
					} while (--h);
				} else {
					do {
						uint32_t *line = buf32 + l;
						fill32(line, color, w);
						buf32 += stride;
					} while (--h);
				}
			}
		} else {
			if (!is32bpp) {
				uint16_t *buf16 = (uint16_t *)draw->vaddr;
				buf16 += t * stride;
				if (mask & 0xffff)
					fill16masked(buf16, color, ~mask, stride*h);
				else
					fill16(buf16, color, stride*h);
			} else {
				uint32_t *buf32 = (uint32_t *)draw->vaddr;
				buf32 += t * stride;
				if (mask)
					fill32masked(buf32, color, ~mask, stride*h);
				else
					fill32(buf32, color, stride*h);
			}
		}

		draw->flush();
	}

	if (mode & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
		FGLSurface *depth = ctx->surface.depth;
		if (!ctx->surface.depthFormat)
			return;

		uint32_t mask;
		uint32_t val = getFillDepth(ctx, &mask, mode);

		if (lineByLine) {
			uint32_t *buf32 = (uint32_t *)depth->vaddr;
			buf32 += t * stride;
			if (mask) {
				do {
					uint32_t *line = buf32 + l;
					fill32masked(line, val, ~mask, w);
					buf32 += stride;
				} while (--h);
			} else {
				do {
					uint32_t *line = buf32 + l;
					fill32(line, val, w);
					buf32 += stride;
				} while (--h);
			}
		} else {
			uint32_t *buf32 = (uint32_t *)depth->vaddr;
			buf32 += t * stride;
			if (mask)
				fill32masked(buf32, val, ~mask, stride*h);
			else
				fill32(buf32, val, stride*h);
		}

		depth->flush();
	}
}

#define FGL_CLEAR_MASK \
	(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)

//#define FGL_HARDWARE_CLEAR

GL_API void GL_APIENTRY glClear (GLbitfield mask)
{
	FGLContext *ctx = getContext();

	if ((mask & FGL_CLEAR_MASK) == 0)
		return;

#ifdef FGL_HARDWARE_CLEAR
	uint32_t mode = 0;

	if (mask & GL_COLOR_BUFFER_BIT)
		mode |= FGFP_CLEAR_COLOR;

	if (mask & GL_DEPTH_BUFFER_BIT)
		mode |= FGFP_CLEAR_DEPTH;

	if (mask & GL_STENCIL_BUFFER_BIT)
		mode |= FGFP_CLEAR_STENCIL;

	/* Clear the buffers in hardware */
	fimgClear(ctx->fimg, mode);
#else
	/* Make sure the hardware isn't rendering */
	glFlush();
	/* Clear the buffers in software */
	fglClear(ctx, mask);
#endif
}

GL_API void GL_APIENTRY glClearColor (GLclampf red, GLclampf green,
						GLclampf blue, GLclampf alpha)
{
	FGLContext *ctx = getContext();

	ctx->clear.red = clampFloat(red);
	ctx->clear.green = clampFloat(green);
	ctx->clear.blue = clampFloat(blue);
	ctx->clear.alpha = clampFloat(alpha);

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearColor(ctx->fimg, clampFloat(red), clampFloat(green),
				clampFloat(blue), clampFloat(alpha));
#endif
}

GL_API void GL_APIENTRY glClearColorx (GLclampx red, GLclampx green,
						GLclampx blue, GLclampx alpha)
{
	glClearColor(floatFromFixed(red), floatFromFixed(green),
				floatFromFixed(blue), floatFromFixed(alpha));
}

GL_API void GL_APIENTRY glClearDepthf (GLclampf depth)
{
	FGLContext *ctx = getContext();

	ctx->clear.depth = clampFloat(depth);

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearDepth(ctx->fimg, clampFloat(depth));
#endif
}

GL_API void GL_APIENTRY glClearDepthx (GLclampx depth)
{
	glClearDepthf(floatFromFixed(depth));
}

GL_API void GL_APIENTRY glClearStencil (GLint s)
{
	FGLContext *ctx = getContext();

	ctx->clear.stencil = s;

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearStencil(ctx->fimg, s);
#endif
}
