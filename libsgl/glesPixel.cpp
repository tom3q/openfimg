/*
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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

/*
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
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	const uint16_t *src = (const uint16_t *)fba->surface->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*fb->getWidth();
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(fb->getHeight() - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		dst -= dstStride;
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel555(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
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
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	const uint16_t *src = (const uint16_t *)fba->surface->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*fb->getWidth();
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(fb->getHeight() - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		dst -= dstStride;
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel565(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
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
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	const uint16_t *src = (const uint16_t *)fba->surface->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*fb->getWidth();
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(fb->getHeight() - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		dst -= dstStride;
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel4444(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
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
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	const uint16_t *src = (const uint16_t *)fba->surface->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = (4*width + alignment - 1) & ~(alignment - 1);
	unsigned srcStride = 2*fb->getWidth();
	unsigned xOffset = 2*x;
	unsigned yOffset = srcStride*(fb->getHeight() - y - height);
	unsigned srcPad = srcStride - 2*width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		dst -= dstStride;
		unsigned w = width;
		uint8_t *line = dst;
		do {
			unpackPixel1555(line, *src++);
			line += 4;
		} while (--w);
		src += srcPad;
	} while (--height);
}

static inline void unpackPixel8888(uint32_t *dst, uint32_t src)
{
	register uint32_t rgba;

	rgba = src & 0xff00ff00;
	src = (src >> 16) | (src << 16);
	rgba |= src & 0x00ff00ff;

	*dst = rgba;
}

static void convertToUByteBGRA8888(FGLContext *ctx, uint32_t *dst,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	const uint32_t *src = (const uint32_t *)fba->surface->vaddr;
	unsigned alignment = ctx->packAlignment;
	unsigned dstStride = ((4*width + alignment - 1) & ~(alignment - 1)) / 4;
	unsigned srcStride = fb->getWidth();
	unsigned xOffset = x;
	unsigned yOffset = srcStride*(fb->getHeight() - y - height);
	unsigned srcPad = srcStride - width;

	src += yOffset + xOffset;
	dst += height*dstStride;
	do {
		dst -= dstStride;
		unsigned w = width;
		uint32_t *line = dst;
		do {
			unpackPixel8888(line++, *src++);
		} while (--w);
		src += srcPad;
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

	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	if (!fb->isValid()) {
		setError(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
		return;
	}

	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	FGLSurface *draw = fba->surface;
	if (!draw || !draw->vaddr) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (width <= 0 || height <= 0 || x < 0 || y < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if ((GLuint)x >= fb->getWidth() || (GLuint)y >= fb->getHeight())
		// Nothing to copy
		return;

	glFinish();
	draw->flush();

	const FGLPixelFormat *cfg = FGLPixelFormat::get(fb->getColorFormat());
	unsigned srcBpp = cfg->pixelSize;
	unsigned srcStride = srcBpp * fb->getWidth();
	unsigned alignment = ctx->packAlignment;

	if (format == cfg->readFormat && type == cfg->readType) {
		// No format conversion needed
		unsigned yOffset = (fb->getHeight() - y - 1) * srcStride;
		const uint8_t *src = (const uint8_t *)draw->vaddr + yOffset;

		unsigned dstStride = (srcBpp*width + alignment - 1) & ~(alignment - 1);
		uint8_t *dst = (uint8_t *)pixels;

		if ((GLuint)y + height > fb->getHeight())
			height = fb->getHeight() - y;

		if (!x && (GLuint)width == fb->getWidth()) {
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
		if ((GLuint)(x + width) > fb->getWidth())
			width = fb->getWidth() - x;

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
		switch (fb->getColorFormat()) {
		case FGL_PIXFMT_XRGB1555:
			convertToUByteRGBA555(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGL_PIXFMT_RGB565:
			convertToUByteRGBA565(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGL_PIXFMT_ARGB4444:
			convertToUByteRGBA4444(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		case FGL_PIXFMT_ARGB1555:
			convertToUByteRGBA1555(ctx, (uint8_t *)pixels,
							x, y, width, height);
			break;
		/* BGRX8888 -> RGBX8888 */
		case FGL_PIXFMT_XRGB8888:
		case FGL_PIXFMT_ARGB8888:
			convertToUByteBGRA8888(ctx, (uint32_t *)pixels,
							x, y, width, height);
			break;
		default:
			LOGW("Unsupported pixel format %d in glReadPixels.",
							fb->getColorFormat());
		}
		// We are done
		return;
	}

	setError(GL_INVALID_ENUM);
}

/*
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
		"mov r4, %1\n\t"
		"mov r5, %1\n\t"
		"mov r6, %1\n\t"
		"mov r7, %1\n\t"
		"1:\n\t"
		"stmia %0!, {r0-r7}\n\t"
		"subs %2, %2, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(cnt)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
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
		"1:\n\t"
		"ldmia %0, {r0-r7}\n\t"
		"and r0, r0, %2\n\t"
		"and r1, r1, %2\n\t"
		"and r2, r2, %2\n\t"
		"and r3, r3, %2\n\t"
		"and r4, r4, %2\n\t"
		"and r5, r5, %2\n\t"
		"and r6, r6, %2\n\t"
		"and r7, r7, %2\n\t"
		"orr r0, r0, %1\n\t"
		"orr r1, r1, %1\n\t"
		"orr r2, r2, %1\n\t"
		"orr r3, r3, %1\n\t"
		"orr r4, r4, %1\n\t"
		"orr r5, r5, %1\n\t"
		"orr r6, r6, %1\n\t"
		"orr r7, r7, %1\n\t"
		"stmia %0!, {r0-r7}\n\t"
		"subs %3, %3, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(mask), "r"(cnt)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
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

	if(cnt / 8)
		buf32 = (uint32_t *)fillBurst32(buf32, val, cnt / 8);

	if(cnt % 8)
		fillSingle32(buf32, val, cnt % 8);
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

	if(cnt / 8)
		buf32 = (uint32_t *)fillBurst32masked(buf32, val, mask, cnt / 8);

	if(cnt % 8)
		fillSingle32masked(buf32, val, mask, cnt % 8);
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

	if(cnt / 16)
		buf16 = (uint16_t *)fillBurst32(buf16, (val << 16) | val, cnt / 16);

	if(cnt % 16)
		fillSingle16(buf16, val, cnt % 16);
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

	if(cnt / 16)
		buf16 = (uint16_t *)fillBurst32masked(buf16, (val << 16) | val,
						(mask << 16) | mask, cnt / 16);

	if(cnt % 16)
		fillSingle16masked(buf16, val, mask, cnt % 16);
}

static uint32_t getFillColor(FGLContext *ctx,
						uint32_t *mask, bool *is32bpp)
{
	uint8_t r, g, b, a;
	uint8_t rMask, gMask, bMask, aMask;
	uint32_t val = 0;
	uint32_t mval = 0;

	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	const FGLPixelFormat *configDesc =
				FGLPixelFormat::get(fb->getColorFormat());

	r	= ubyteFromClampf(ctx->clear.red);
	rMask	= 0xff;
	r	>>= (8 - configDesc->comp[FGL_COMP_RED].size);
	rMask	>>= (8 - configDesc->comp[FGL_COMP_RED].size);

	g	= ubyteFromClampf(ctx->clear.green);
	gMask	= 0xff;
	g	>>= (8 - configDesc->comp[FGL_COMP_GREEN].size);
	gMask	>>= (8 - configDesc->comp[FGL_COMP_GREEN].size);

	b	= ubyteFromClampf(ctx->clear.blue);
	bMask	= 0xff;
	b	>>= (8 - configDesc->comp[FGL_COMP_BLUE].size);
	bMask	>>= (8 - configDesc->comp[FGL_COMP_BLUE].size);

	if (configDesc->flags & FGL_PIX_OPAQUE)
		a = 0xff;
	else
		a = ubyteFromClampf(ctx->clear.alpha);
	aMask	= 0xff;
	a	>>= (8 - configDesc->comp[FGL_COMP_ALPHA].size);
	aMask	>>= (8 - configDesc->comp[FGL_COMP_ALPHA].size);

	val |= r << configDesc->comp[FGL_COMP_RED].pos;
	val |= g << configDesc->comp[FGL_COMP_GREEN].pos;
	val |= b << configDesc->comp[FGL_COMP_BLUE].pos;
	val |= a << configDesc->comp[FGL_COMP_ALPHA].pos;

	if (!ctx->perFragment.mask.red)
		mval |= rMask << configDesc->comp[FGL_COMP_RED].pos;
	if (!ctx->perFragment.mask.green)
		mval |= gMask << configDesc->comp[FGL_COMP_GREEN].pos;
	if (!ctx->perFragment.mask.blue)
		mval |= bMask << configDesc->comp[FGL_COMP_BLUE].pos;
	if (!ctx->perFragment.mask.alpha)
		mval |= aMask << configDesc->comp[FGL_COMP_ALPHA].pos;

	*is32bpp = (configDesc->pixelSize == 4);
	*mask = mval;

	return val;
}

static inline uint32_t getFillDepth(FGLContext *ctx,
			uint32_t *mask, GLbitfield mode, uint32_t depthFormat)
{
	uint32_t mval = 0xffffffff;
	uint32_t val = 0;

	if (mode & GL_DEPTH_BUFFER_BIT && ctx->perFragment.mask.depth) {
		mval &= ~0x00ffffff;
		val |= (uint32_t)(ctx->clear.depth * 0x00ffffff);
	}

	if (mode & GL_STENCIL_BUFFER_BIT) {
		mval &= ~(ctx->perFragment.mask.stencil << 24);
		val |= ctx->clear.stencil << 24;
	}

	if (mval == 0xffffffff) {
		*mask = mval;
		return val;
	}

	if (!(depthFormat & 0xff))
		mval &= ~0x00ffffff;

	if (!(depthFormat >> 8))
		mval &= ~0xff000000;

	*mask = mval;
	return val;
}

static inline void fglColorClear(FGLContext *ctx, bool lineByLine,
		uint32_t stride, int32_t l, int32_t t, int32_t w, int32_t h)
{
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba = fb->get(FGL_ATTACHMENT_COLOR);
	FGLSurface *draw = fba->surface;
	bool is32bpp = false;
	uint32_t mask = 0;
	uint32_t color = getFillColor(ctx, &mask, &is32bpp);

	if (mask == 0xffffffff || (!is32bpp && mask == 0xffff))
		return;

	if (lineByLine) {
		int32_t lines = h;
		if (!is32bpp) {
			uint16_t *buf16 = (uint16_t *)draw->vaddr;
			buf16 += t * stride + l;
			if (mask & 0xffff) {
				do {
					fill16masked(buf16, color, ~mask, w);
					buf16 += stride;
				} while (--lines);
			} else {
				do {
					fill16(buf16, color, w);
					buf16 += stride;
				} while (--lines);
			}
		} else {
			uint32_t *buf32 = (uint32_t *)draw->vaddr;
			buf32 += t * stride + l;
			if (mask) {
				do {
					fill32masked(buf32, color, ~mask, w);
					buf32 += stride;
				} while (--lines);
			} else {
				do {
					fill32(buf32, color, w);
					buf32 += stride;
				} while (--lines);
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

static inline void fglDepthClear(FGLContext *ctx, bool lineByLine,
		uint32_t stride, int32_t l, int32_t t, int32_t w, int32_t h,
		GLbitfield mode)
{
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	FGLFramebufferAttachable *fba;
	uint32_t depthFormat = fb->getDepthFormat();
	if (!depthFormat)
		return;

	fba = fb->get(FGL_ATTACHMENT_DEPTH);
	if (!fba)
		fba = fb->get(FGL_ATTACHMENT_STENCIL);

	FGLSurface *depth = fba->surface;
	uint32_t mask = 0;
	uint32_t val = getFillDepth(ctx, &mask, mode, depthFormat);

	if (mask == 0xffffffff)
		return;

	if (lineByLine) {
		int32_t lines = h;
		uint32_t *buf32 = (uint32_t *)depth->vaddr;
		buf32 += t * stride + l;
		if (mask) {
			do {
				fill32masked(buf32, val, ~mask, w);
				buf32 += stride;
			} while (--lines);
		} else {
			do {
				fill32(buf32, val, w);
				buf32 += stride;
			} while (--lines);
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

static void fglClear(FGLContext *ctx, GLbitfield mode)
{
	FUNCTION_TRACER;
	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	uint32_t stride = fb->getWidth();
	bool lineByLine = false;
	int32_t l, b, t, w, h;

	l = 0;
	b = 0;
	w = fb->getWidth();
	h = fb->getHeight();
	if (ctx->enable.scissorTest) {
		if (ctx->perFragment.scissor.left > 0)
			l = ctx->perFragment.scissor.left;
		if (ctx->perFragment.scissor.bottom > 0)
			b = ctx->perFragment.scissor.bottom;
		if ((GLuint)(ctx->perFragment.scissor.left + ctx->perFragment.scissor.width) <= fb->getWidth())
			w = ctx->perFragment.scissor.left + ctx->perFragment.scissor.width - l;
		else
			w = fb->getWidth() - l;
		if ((GLuint)(ctx->perFragment.scissor.bottom + ctx->perFragment.scissor.height) <= fb->getHeight())
			h = ctx->perFragment.scissor.bottom + ctx->perFragment.scissor.height - b;
		else
			h = fb->getHeight() - b;
	}
	t = fb->getHeight() - b - h;

	if (!h || !w)
		return;

	lineByLine |= ((GLuint)w < fb->getWidth());

	if (mode & GL_COLOR_BUFFER_BIT)
		fglColorClear(ctx, lineByLine, stride, l, t, w, h);

	mode &= (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	if (mode)
		fglDepthClear(ctx, lineByLine, stride, l, t, w, h, mode);
}

#define FGL_CLEAR_MASK \
	(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)

GL_API void GL_APIENTRY glClear (GLbitfield mask)
{
	FGLContext *ctx = getContext();

	FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
	if (!fb->isValid()) {
		setError(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
		return;
	}

	if ((mask & FGL_CLEAR_MASK) == 0)
		return;

	/* Make sure the hardware isn't rendering */
	glFinish();
	/* Clear the buffers in software */
	fglClear(ctx, mask);
}

GL_API void GL_APIENTRY glClearColor (GLclampf red, GLclampf green,
						GLclampf blue, GLclampf alpha)
{
	FGLContext *ctx = getContext();

	ctx->clear.red = clampFloat(red);
	ctx->clear.green = clampFloat(green);
	ctx->clear.blue = clampFloat(blue);
	ctx->clear.alpha = clampFloat(alpha);
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
}

GL_API void GL_APIENTRY glClearDepthx (GLclampx depth)
{
	glClearDepthf(floatFromFixed(depth));
}

GL_API void GL_APIENTRY glClearStencil (GLint s)
{
	FGLContext *ctx = getContext();

	ctx->clear.stencil = s;
}
