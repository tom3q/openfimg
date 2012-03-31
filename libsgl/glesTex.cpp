/*
 * libsgl/glesTex.cpp
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
#include <unistd.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "fglimage.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

/*
	Texturing
*/

FGLObjectManager<FGLTexture, FGL_MAX_TEXTURE_OBJECTS> fglTextureObjects;

GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = textures;
	FGLContext *ctx = getContext();

	do {
		name = fglTextureObjects.get(ctx);
		if(name < 0) {
			glDeleteTextures (n - i, textures);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglTextureObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}

GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{
	unsigned name;

	if(n <= 0)
		return;

	do {
		name = *textures;
		textures++;

		if(!fglTextureObjects.isValid(name)) {
			LOGD("Tried to free invalid texture %d", name);
			continue;
		}

		delete (fglTextureObjects[name]);
		fglTextureObjects.put(name);
	} while (--n);
}

GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture)
{
	FGLTextureObjectBinding *binding;
	FGLContext *ctx = getContext();

	switch (target) {
	case GL_TEXTURE_2D:
		binding = &ctx->texture[ctx->activeTexture].binding;
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		binding = &ctx->textureExternal[ctx->activeTexture].binding;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(texture == 0) {
		binding->bind(0);
		return;
	}

	if(!fglTextureObjects.isValid(texture)
	    && fglTextureObjects.get(texture, ctx) < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLTexture *tex = fglTextureObjects[texture];
	if(tex == NULL) {
		tex = new FGLTexture(texture);
		if (tex == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglTextureObjects[texture] = tex;
		tex->target = target;
	} else if (tex->target != target) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (tex->eglImage)
		tex->dirty = true;

	binding->bind(&tex->object);
}

static int fglGetFormatInfo(GLenum format, GLenum type, bool *conv)
{
	*conv = 0;
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB:
		/* Needs conversion */
			*conv = 1;
			return FGL_PIXFMT_XRGB8888;
		case GL_RGBA:
			return FGL_PIXFMT_ABGR8888;
		case GL_BGRA_EXT:
			return FGL_PIXFMT_ARGB8888;
		case GL_ALPHA:
		case GL_LUMINANCE:
			*conv = 1;
			/* Fall through */
		case GL_LUMINANCE_ALPHA:
			return FGL_PIXFMT_AL88;
		default:
			return -1;
		}
	case GL_UNSIGNED_SHORT_5_6_5:
		if (format != GL_RGB)
			return -1;
		return FGL_PIXFMT_RGB565;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		if (format != GL_RGBA)
			return -1;
		return FGL_PIXFMT_RGBA4444;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		if (format != GL_RGBA)
			return -1;
		return FGL_PIXFMT_RGBA5551;
	default:
		return -1;
	}
}

static void fglGenerateMipmaps(FGLTexture *obj)
{
	int level = 0;
	int skip = 1;

	int w = obj->width;
	int h = obj->height;
	if ((w&h) == 1)
		return;

	w = (w>>1) ? : 1;
	h = (h>>1) ? : 1;

	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	void *curLevel = obj->surface->vaddr;
	void *nextLevel = (uint8_t *)obj->surface->vaddr
			+ pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, 1);

processNextLevel:
	++level;
	int stride = w;
	int bs = w;
	switch (obj->pixFormat) {
	case FGL_PIXFMT_RGB565: {
		uint16_t const * src = (uint16_t const *)curLevel;
		uint16_t* dst = (uint16_t*)nextLevel;
		const uint32_t mask = 0x07E0F81F;
		for (int y=0 ; y<h ; y++) {
			size_t offset = (y*2) * bs;
			for (int x=0 ; x<w ; x++) {
				uint32_t p00 = src[offset];
				uint32_t p10 = src[offset+1];
				uint32_t p01 = src[offset+bs];
				uint32_t p11 = src[offset+bs+1];
				p00 = (p00 | (p00 << 16)) & mask;
				p01 = (p01 | (p01 << 16)) & mask;
				p10 = (p10 | (p10 << 16)) & mask;
				p11 = (p11 | (p11 << 16)) & mask;
				uint32_t grb = ((p00 + p10 + p01 + p11) >> 2) & mask;
				uint32_t rgb = (grb & 0xFFFF) | (grb >> 16);
				dst[x + y*stride] = rgb;
				offset += 2;
			}
		}
		break; }
	case FGL_PIXFMT_RGBA5551: {
		uint16_t const * src = (uint16_t const *)curLevel;
		uint16_t* dst = (uint16_t*)nextLevel;
		for (int y=0 ; y<h ; y++) {
			size_t offset = (y*2) * bs;
			for (int x=0 ; x<w ; x++) {
				uint32_t p00 = src[offset];
				uint32_t p10 = src[offset+1];
				uint32_t p01 = src[offset+bs];
				uint32_t p11 = src[offset+bs+1];
				uint32_t r = ((p00>>11)+(p10>>11)+(p01>>11)+(p11>>11)+2)>>2;
				uint32_t g = (((p00>>6)+(p10>>6)+(p01>>6)+(p11>>6)+2)>>2)&0x3F;
				uint32_t b = ((p00&0x3E)+(p10&0x3E)+(p01&0x3E)+(p11&0x3E)+4)>>3;
				uint32_t a = ((p00&1)+(p10&1)+(p01&1)+(p11&1)+2)>>2;
				dst[x + y*stride] = (r<<11)|(g<<6)|(b<<1)|a;
				offset += 2;
			}
		}
		break; }
	case FGL_PIXFMT_XRGB8888:
	case FGL_PIXFMT_ARGB8888:
	case FGL_PIXFMT_XBGR8888:
	case FGL_PIXFMT_ABGR8888: {
		uint32_t const * src = (uint32_t const *)curLevel;
		uint32_t* dst = (uint32_t*)nextLevel;
		for (int y=0 ; y<h ; y++) {
			size_t offset = (y*2) * bs;
			for (int x=0 ; x<w ; x++) {
				uint32_t p00 = src[offset];
				uint32_t p10 = src[offset+1];
				uint32_t p01 = src[offset+bs];
				uint32_t p11 = src[offset+bs+1];
				uint32_t rb00 = p00 & 0x00FF00FF;
				uint32_t rb01 = p01 & 0x00FF00FF;
				uint32_t rb10 = p10 & 0x00FF00FF;
				uint32_t rb11 = p11 & 0x00FF00FF;
				uint32_t ga00 = (p00 >> 8) & 0x00FF00FF;
				uint32_t ga01 = (p01 >> 8) & 0x00FF00FF;
				uint32_t ga10 = (p10 >> 8) & 0x00FF00FF;
				uint32_t ga11 = (p11 >> 8) & 0x00FF00FF;
				uint32_t rb = (rb00 + rb01 + rb10 + rb11)>>2;
				uint32_t ga = (ga00 + ga01 + ga10 + ga11)>>2;
				uint32_t rgba = (rb & 0x00FF00FF) | ((ga & 0x00FF00FF)<<8);
				dst[x + y*stride] = rgba;
				offset += 2;
			}
		}
		break; }
	case FGL_PIXFMT_AL88:
		skip = 2;
		/* Fall-through */
	case FGL_PIXFMT_L8: {
		uint8_t const * src = (uint8_t const *)curLevel;
		uint8_t* dst = (uint8_t*)nextLevel;
		bs *= skip;
		stride *= skip;
		for (int y=0 ; y<h ; y++) {
			size_t offset = (y*2) * bs;
			for (int x=0 ; x<w ; x++) {
				for (int c=0 ; c<skip ; c++) {
					uint32_t p00 = src[c+offset];
					uint32_t p10 = src[c+offset+skip];
					uint32_t p01 = src[c+offset+bs];
					uint32_t p11 = src[c+offset+bs+skip];
					dst[x + y*stride + c] = (p00 + p10 + p01 + p11) >> 2;
				}
				offset += 2*skip;
			}
		}
		break; }
	case FGL_PIXFMT_RGBA4444: {
		uint16_t const * src = (uint16_t const *)curLevel;
		uint16_t* dst = (uint16_t*)nextLevel;
		for (int y=0 ; y<h ; y++) {
			size_t offset = (y*2) * bs;
			for (int x=0 ; x<w ; x++) {
				uint32_t p00 = src[offset];
				uint32_t p10 = src[offset+1];
				uint32_t p01 = src[offset+bs];
				uint32_t p11 = src[offset+bs+1];
				p00 = ((p00 << 12) & 0x0F0F0000) | (p00 & 0x0F0F);
				p10 = ((p10 << 12) & 0x0F0F0000) | (p10 & 0x0F0F);
				p01 = ((p01 << 12) & 0x0F0F0000) | (p01 & 0x0F0F);
				p11 = ((p11 << 12) & 0x0F0F0000) | (p11 & 0x0F0F);
				uint32_t rbga = (p00 + p10 + p01 + p11) >> 2;
				uint32_t rgba = (rbga & 0x0F0F) | ((rbga>>12) & 0xF0F0);
				dst[x + y*stride] = rgba;
				offset += 2;
			}
		}
		break; }
	default:
		LOGE("Unsupported format (%d)", obj->pixFormat);
		return;
	}

	// exit condition: we just processed the 1x1 LODs
	if ((w&h) == 1)
		return;

	w = (w>>1) ? : 1;
	h = (h>>1) ? : 1;

	curLevel = nextLevel;
	nextLevel = (uint8_t *)obj->surface->vaddr
		+ pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level + 1);

	goto processNextLevel;
}

static size_t fglCalculateMipmaps(FGLTexture *obj, unsigned int width,
					unsigned int height, unsigned int bpp)
{
	size_t offset, size;
	unsigned int lvl, check;

	size = width * height;
	offset = 0;
	check = max(width, height);
	lvl = 0;

	do {
		fimgSetTexMipmapOffset(obj->fimg, lvl, offset);
		offset += size;

		if(lvl == FGL_MAX_MIPMAP_LEVEL)
			break;

		check /= 2;
		if(check == 0)
			break;

		++lvl;

		if (width >= 2)
			width /= 2;

		if (height >= 2)
			height /= 2;

		size = width * height;
	} while (1);

	obj->maxLevel = lvl;
	return offset;
}

static void fglLoadTextureDirect(FGLTexture *obj, unsigned level,
						const GLvoid *pixels)
{
	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned offset = pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t size = width*height*pix->pixelSize;

	memcpy((uint8_t *)obj->surface->vaddr + offset, pixels, size);
}

static void fglLoadTexture(FGLTexture *obj, unsigned level,
		    const GLvoid *pixels, unsigned alignment)
{
	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned offset = pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t line = width*pix->pixelSize;
	size_t stride = (line + alignment - 1) & ~(alignment - 1);
	const uint8_t *src8 = (const uint8_t *)pixels;
	uint8_t *dst8 = (uint8_t *)obj->surface->vaddr + offset;

	do {
		memcpy(dst8, src8, line);
		src8 += stride;
		dst8 += line;
	} while(--height);
}

static inline uint32_t fglPackARGB8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static inline uint16_t fglPackAL88(uint8_t l, uint8_t a)
{
	return (a << 8) | l;
}

static void fglConvertTexture(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment)
{
	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned offset = pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	switch (obj->format) {
	case GL_RGB: {
		size_t line = 3*width;
		size_t stride = (line + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - line;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface->vaddr + offset);
		do {
			unsigned x = width;
			do {
				*(dst32++) = fglPackARGB8888(src8[0],
							src8[1], src8[2], 255);
				src8 += 3;
			} while(--x);
			src8 += padding;
		} while (--height);
		break;
	}
	case GL_RGBA: {
		size_t line = 4*width;
		size_t stride = (line + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - line;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface->vaddr + offset);
		do {
			unsigned x = width;
			do {
				*(dst32++) = fglPackARGB8888(src8[0],
						src8[1], src8[2], src8[3]);
				src8 += 4;
			} while(--x);
			src8 += padding;
		} while (--height);
		break;
	}
	case GL_LUMINANCE: {
		size_t stride = (width + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - width;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface->vaddr + offset);
		do {
			unsigned x = width;
			do {
				*(dst16++) = fglPackAL88(*(src8++), 255);
			} while (--x);
			src8 += padding;
		} while (--height);
		break;
	}
	case GL_ALPHA: {
		size_t stride = (width + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - width;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface->vaddr + offset);
		do {
			unsigned x = width;
			do {
				*(dst16++) = fglPackAL88(0xff, *(src8++));
			} while (--x);
			src8 += padding;
		} while (--height);
		break;
	}
	default:
		LOGW("Unsupported texture conversion %d", obj->format);
		return;
	}
}

static inline void fglWaitForTexture(FGLContext *ctx, FGLTexture *tex)
{
	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; ++i) {
		if (ctx->busyTexture[i] == tex) {
			glFinish();
			break;
		}
	}
}

GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level,
	GLint internalformat, GLsizei width, GLsizei height, GLint border,
	GLenum format, GLenum type, const GLvoid *pixels)
{
	/* Check conditions required by specification */
	if (target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (level < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if (border != 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if ((GLenum)internalformat != format) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj = ctx->texture[ctx->activeTexture].getTexture();

	/* Mipmap image specification */
	if (level > 0) {
		if (obj->eglImage) {
			/* TODO: Copy eglImage contents into new texture */
			setError(GL_INVALID_OPERATION);
			return;
		}

		GLint mipmapW, mipmapH;

		mipmapW = obj->width >> level;
		if (!mipmapW)
			mipmapW = 1;

		mipmapH = obj->height >> level;
		if (!mipmapH)
			mipmapH = 1;

		if (!obj->surface) {
			/* Mipmaps can be specified only if base level exists */
			setError(GL_INVALID_OPERATION);
			return;
		}

		/* Check dimensions */
		if (mipmapW != width || mipmapH != height) {
			/* Invalid size */
			setError(GL_INVALID_VALUE);
			return;
		}

		/* Check format */
		if (obj->format != format || obj->type != type) {
			/* Must be the same format as base level */
			setError(GL_INVALID_ENUM);
			return;
		}

		/* Copy the image (with conversion if needed) */
		if (pixels != NULL) {
			const FGLPixelFormat *pix =
					FGLPixelFormat::get(obj->pixFormat);

			fglWaitForTexture(ctx, obj);

			if (obj->convert) {
				fglConvertTexture(obj, level, pixels,
							ctx->unpackAlignment);
			} else {
				if (ctx->unpackAlignment <= pix->pixelSize)
					fglLoadTextureDirect(obj, level, pixels);
				else
					fglLoadTexture(obj, level, pixels,
						       ctx->unpackAlignment);
			}

			obj->dirty = true;
		}

		return;
	}

	/* Base image specification */
	bool convert;
	int pixFormat = fglGetFormatInfo(format, type, &convert);
	if (pixFormat < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglWaitForTexture(ctx, obj);

	if (obj->eglImage) {
		obj->eglImage->disconnect();
		obj->eglImage = 0;
		obj->surface = 0;
	}

	if (width != obj->width || height != obj->height
	    || (uint32_t)pixFormat != obj->pixFormat)
		obj->markFramebufferDirty();

	const FGLPixelFormat *pix = FGLPixelFormat::get(pixFormat);
	obj->invReady = false;
	obj->width = width;
	obj->height = height;
	obj->format = format;
	obj->type = type;
	obj->pixFormat = pixFormat;
	obj->convert = convert;
	obj->mask = 0;
	if (pix->pixFormat != (uint32_t)-1)
		obj->mask = BIT_VAL(FGL_ATTACHMENT_COLOR);

	if (!width || !height) {
		delete obj->surface;
		obj->surface = 0;
		return;
	}

	/* Calculate mipmaps */
	uint32_t size = pix->pixelSize*fglCalculateMipmaps(obj,
						width, height, pix->pixelSize);

	if (obj->surface) {
		int32_t delta = obj->surface->size - size;
		if (delta < 0 || delta > 16384) {
			delete obj->surface;
			obj->surface = 0;
		}
	}

	/* (Re)allocate the texture if needed */
	if (!obj->surface) {
		obj->surface = new FGLLocalSurface(size);
		if(!obj->surface || !obj->surface->isValid()) {
			delete obj->surface;
			obj->surface = 0;
			obj->width = 0;
			obj->height = 0;
			obj->format = 0;
			obj->type = 0;
			obj->pixFormat = 0;
			setError(GL_OUT_OF_MEMORY);
			return;
		}
	}

	fimgInitTexture(obj->fimg, pix->flags,
					pix->texFormat, obj->surface->paddr);
	fimgSetTex2DSize(obj->fimg, width, height, obj->maxLevel);

	/* Copy the image (with conversion if needed) */
	if (pixels != NULL) {
		if (obj->convert) {
			fglConvertTexture(obj, level, pixels,
						ctx->unpackAlignment);
		} else {
			if (ctx->unpackAlignment <= pix->pixelSize)
				fglLoadTextureDirect(obj, level, pixels);
			else
				fglLoadTexture(obj, level, pixels,
						ctx->unpackAlignment);
		}

		if (obj->genMipmap)
			fglGenerateMipmaps(obj);

		obj->dirty = true;
	}
}

static void fglLoadTexturePartial(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment,
			unsigned x, unsigned y, unsigned w, unsigned h)
{
	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned offset = pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;
	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t line = w*pix->pixelSize;
	size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
	size_t dstStride = width*pix->pixelSize;
	size_t xoffset = x*pix->pixelSize;
	size_t yoffset = y*dstStride;
	const uint8_t *src8 = (const uint8_t *)pixels;
	uint8_t *dst8 = (uint8_t *)obj->surface->vaddr
						+ offset + yoffset + xoffset;
	do {
		memcpy(dst8, src8, line);
		src8 += srcStride;
		dst8 += dstStride;
	} while (--h);
}

static void fglConvertTexturePartial(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment,
			unsigned x, unsigned y, unsigned w, unsigned h)
{
	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned offset = pix->pixelSize*fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	switch (obj->format) {
	case GL_RGB: {
		size_t line = 3*w;
		size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
		size_t dstStride = 4*width;
		size_t xOffset = 4*x;
		size_t yOffset = y*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface->vaddr
						+ offset + yOffset + xOffset);
		do {
			unsigned x = w;
			do {
				*(dst32++) = fglPackARGB8888(src8[0],
						src8[1], src8[2], 255);
				src8 += 3;
			} while (--x);
			src8 += srcPad;
			dst32 += dstPad;
		} while (--h);
		break;
	}
	case GL_RGBA: {
		size_t line = 4*w;
		size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
		size_t dstStride = 4*width;
		size_t xOffset = 4*x;
		size_t yOffset = y*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface->vaddr
						+ offset + yOffset + xOffset);
		do {
			unsigned x = w;
			do {
				*(dst32++) = fglPackARGB8888(src8[0],
						src8[1], src8[2], src8[3]);
				src8 += 4;
			} while (--x);
			src8 += srcPad;
			dst32 += dstPad;
		} while (--h);
		break;
	}
	case GL_LUMINANCE: {
		size_t line = w;
		size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
		size_t dstStride = 2*width;
		size_t xOffset = 2*x;
		size_t yOffset = y*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface->vaddr
						+ offset + yOffset + xOffset);
		do {
			unsigned x = w;
			do {
				*(dst16++) = fglPackAL88(*(src8++), 255);
			} while (--x);
			src8 += srcPad;
			dst16 += dstPad;
		} while (--h);
		break;
	}
	case GL_ALPHA: {
		size_t line = w;
		size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
		size_t dstStride = 2*width;
		size_t xOffset = 2*x;
		size_t yOffset = y*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface->vaddr
						+ offset + yOffset + xOffset);
		do {
			unsigned x = w;
			do {
				*(dst16++) = fglPackAL88(0xff, *(src8++));
			} while (--x);
			src8 += srcPad;
			dst16 += dstPad;
		} while (--h);
		break;
	}
	default:
		LOGW("Unsupported texture conversion %d", obj->format);
		return;
	}
}

GL_API void GL_APIENTRY glTexSubImage2D (GLenum target, GLint level,
		GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
		GLenum format, GLenum type, const GLvoid *pixels)
{
	if (target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj = ctx->texture[ctx->activeTexture].getTexture();

	if (obj->eglImage) {
		/* TODO: Copy eglImage contents into new texture */
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (!obj->surface) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (level > obj->maxLevel) {
		setError(GL_INVALID_VALUE);
		return;
	}

	GLint mipmapW, mipmapH;

	mipmapW = obj->width >> level;
	if (!mipmapW)
		mipmapW = 1;

	mipmapH = obj->height >> level;
	if (!mipmapH)
		mipmapH = 1;

	if (!xoffset && !yoffset && mipmapW == width && mipmapH == height) {
		glTexImage2D(target, level, format, width, height,
						0, format, type, pixels);
		return;
	}

	if (format != obj->format || type != obj->type) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (xoffset < 0 || yoffset < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if (xoffset + width > mipmapW || yoffset + height > mipmapH) {
		setError(GL_INVALID_VALUE);
		return;
	}

	if (!pixels)
		return;

	fglWaitForTexture(ctx, obj);

	if (obj->convert)
		fglConvertTexturePartial(obj, level, pixels,
			ctx->unpackAlignment, xoffset, yoffset, width, height);
	else
		fglLoadTexturePartial(obj, level, pixels,
			ctx->unpackAlignment, xoffset, yoffset, width, height);

	obj->dirty = true;
}

GL_API void GL_APIENTRY glCompressedTexImage2D (GLenum target, GLint level,
		GLenum internalformat, GLsizei width, GLsizei height,
		GLint border, GLsizei imageSize, const GLvoid *data)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glCompressedTexSubImage2D (GLenum target, GLint level,
		GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
		GLenum format, GLsizei imageSize, const GLvoid *data)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glCopyTexImage2D (GLenum target, GLint level,
		GLenum internalformat, GLint x, GLint y, GLsizei width,
		GLsizei height, GLint border)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glCopyTexSubImage2D (GLenum target, GLint level,
		GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width,
		GLsizei height)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES (GLenum target,
							GLeglImageOES img)
{
	FGLContext *ctx = getContext();
	FGLTexture *tex;

	switch (target) {
	case GL_TEXTURE_2D:
		tex = ctx->texture[ctx->activeTexture].getTexture();
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		tex = ctx->textureExternal[ctx->activeTexture].getTexture();
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLImage *image = (FGLImage *)img;
	if (!image || !image->isValid()) {
		setError(GL_INVALID_VALUE);
		return;
	}

	const FGLPixelFormat *cfg = FGLPixelFormat::get(image->pixelFormat);

	if (tex->eglImage)
		tex->eglImage->disconnect();
	else
		delete tex->surface;

	tex->invReady	= false;
	tex->surface	= image->surface;
	tex->eglImage	= image;
	tex->format	= cfg->readFormat;
	tex->type	= cfg->readType;
	tex->pixFormat	= image->pixelFormat;
	tex->convert	= 0;
	tex->maxLevel	= 0;
	tex->dirty	= true;
	tex->width	= image->width;
	tex->height	= image->height;

	// Setup fimgTexture
	fimgInitTexture(tex->fimg,
			cfg->flags, cfg->texFormat, tex->surface->paddr);
	fimgSetTex2DSize(tex->fimg, image->width, image->height, tex->maxLevel);
	fimgSetTexMipmap(tex->fimg, FGTU_TSTA_MIPMAP_DISABLED);
	if (target == GL_TEXTURE_EXTERNAL_OES)
		fimgSetTexMinFilter(tex->fimg, FGTU_TSTA_FILTER_LINEAR);

	tex->eglImage->connect();
}

#if 0
GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
	FUNC_UNIMPLEMENTED;
}
#endif

GL_API void GL_APIENTRY glActiveTexture (GLenum texture)
{
	GLint unit;

	if((unit = unitFromTextureEnum(texture)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->activeTexture = unit;
}

GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	FGLTexture *obj;
	FGLContext *ctx = getContext();

	switch (target) {
	case GL_TEXTURE_2D:
		obj = ctx->texture[ctx->activeTexture].getTexture();
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		obj = ctx->textureExternal[ctx->activeTexture].getTexture();
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch (pname) {
	case GL_TEXTURE_WRAP_S:
		obj->sWrap = param;
		switch(param) {
		case GL_REPEAT:
			fimgSetTexUAddrMode(obj->fimg, FGTU_TSTA_ADDR_MODE_REPEAT);
			break;
		case GL_CLAMP_TO_EDGE:
			fimgSetTexUAddrMode(obj->fimg, FGTU_TSTA_ADDR_MODE_CLAMP);
			break;
		default:
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_TEXTURE_WRAP_T:
		obj->tWrap = param;
		switch(param) {
		case GL_REPEAT:
			fimgSetTexVAddrMode(obj->fimg, FGTU_TSTA_ADDR_MODE_REPEAT);
			break;
		case GL_CLAMP_TO_EDGE:
			fimgSetTexVAddrMode(obj->fimg, FGTU_TSTA_ADDR_MODE_CLAMP);
			break;
		default:
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		obj->minFilter = param;
		switch(param) {
		case GL_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_DISABLED);
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_NEAREST);
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_LINEAR);
			break;
		case GL_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_DISABLED);
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_NEAREST);
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_LINEAR);
			break;
		default:
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_TEXTURE_MAG_FILTER:
		obj->magFilter = param;
		switch(param) {
		case GL_NEAREST:
		case GL_NEAREST_MIPMAP_NEAREST:
		case GL_NEAREST_MIPMAP_LINEAR:
			fimgSetTexMagFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			break;
		case GL_LINEAR:
		case GL_LINEAR_MIPMAP_NEAREST:
		case GL_LINEAR_MIPMAP_LINEAR:
			fimgSetTexMagFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			break;
		default:
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_GENERATE_MIPMAP:
		obj->genMipmap = param;
		break;
	default:
		setError(GL_INVALID_ENUM);
		LOGE("Invalid enum value %08x.", pname);
	}
}

GL_API void GL_APIENTRY glTexParameteriv (GLenum target, GLenum pname,
							const GLint *params)
{
	FGLTexture *obj;
	FGLContext *ctx = getContext();

	switch (target) {
	case GL_TEXTURE_2D:
		obj = ctx->texture[ctx->activeTexture].getTexture();
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		obj = ctx->textureExternal[ctx->activeTexture].getTexture();
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch (pname) {
	case GL_TEXTURE_CROP_RECT_OES:
		memcpy(obj->cropRect, params, 4*sizeof(GLint));
		break;
	default:
		glTexParameteri(target, pname, *params);
	}
}

GL_API void GL_APIENTRY glTexParameterf (GLenum target, GLenum pname,
								GLfloat param)
{
	glTexParameteri(target, pname, (GLint)(param));
}

GL_API void GL_APIENTRY glTexParameterfv (GLenum target, GLenum pname,
							const GLfloat *params)
{
	FGLTexture *obj;
	FGLContext *ctx = getContext();

	switch (target) {
	case GL_TEXTURE_2D:
		obj = ctx->texture[ctx->activeTexture].getTexture();
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		obj = ctx->textureExternal[ctx->activeTexture].getTexture();
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch (pname) {
	case GL_TEXTURE_CROP_RECT_OES:
		obj->cropRect[0] = round(params[0]);
		obj->cropRect[1] = round(params[1]);
		obj->cropRect[2] = round(params[2]);
		obj->cropRect[3] = round(params[3]);
		break;
	default:
		glTexParameteri(target, pname, (GLint)(*params));
	}
}

GL_API void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname,
								GLfixed param)
{
	glTexParameteri(target, pname, param);
}

GL_API void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname,
							const GLfixed *params)
{
	FGLTexture *obj;
	FGLContext *ctx = getContext();

	switch (target) {
	case GL_TEXTURE_2D:
		obj = ctx->texture[ctx->activeTexture].getTexture();
		break;
	case GL_TEXTURE_EXTERNAL_OES:
		obj = ctx->textureExternal[ctx->activeTexture].getTexture();
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch (pname) {
	case GL_TEXTURE_CROP_RECT_OES:
		obj->cropRect[0] = round(floatFromFixed(params[0]));
		obj->cropRect[1] = round(floatFromFixed(params[1]));
		obj->cropRect[2] = round(floatFromFixed(params[2]));
		obj->cropRect[3] = round(floatFromFixed(params[3]));
		break;
	default:
		glTexParameteri(target, pname, *params);
	}
}

GL_API void GL_APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_TEXTURE_ENV_MODE:
		switch (param) {
		case GL_REPLACE:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_REPLACE;
			break;
		case GL_MODULATE:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_MODULATE;
			break;
		case GL_DECAL:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_DECAL;
			break;
		case GL_BLEND:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_BLEND;
			break;
		case GL_ADD:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_ADD;
			break;
		case GL_COMBINE:
			ctx->texture[unit].fglFunc = FGFP_TEXFUNC_COMBINE;
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_COMBINE_RGB:
		switch (param) {
		case GL_REPLACE:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_REPLACE);
			break;
		case GL_MODULATE:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_MODULATE);
			break;
		case GL_ADD:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_ADD);
			break;
		case GL_ADD_SIGNED:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_ADD_SIGNED);
			break;
		case GL_INTERPOLATE:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_INTERPOLATE);
			break;
		case GL_SUBTRACT:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_SUBTRACT);
			break;
		case GL_DOT3_RGB:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_DOT3_RGB);
			break;
		case GL_DOT3_RGBA:
			fimgCompatSetColorCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_DOT3_RGBA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_COMBINE_ALPHA:
		switch (param) {
		case GL_REPLACE:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_REPLACE);
			break;
		case GL_MODULATE:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_MODULATE);
			break;
		case GL_ADD:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_ADD);
			break;
		case GL_ADD_SIGNED:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_ADD_SIGNED);
			break;
		case GL_INTERPOLATE:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_INTERPOLATE);
			break;
		case GL_SUBTRACT:
			fimgCompatSetAlphaCombiner(ctx->fimg,
						unit, FGFP_COMBFUNC_SUBTRACT);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_RGB_SCALE:
		if (param != 1 && param != 2 && param != 4) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, param);
		break;
	case GL_ALPHA_SCALE:
		if (param != 1 && param != 2 && param != 4) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
			return;
		}
		fimgCompatSetAlphaScale(ctx->fimg, unit, param);
		break;
	case GL_SRC0_RGB:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND0_RGB:
		switch (param) {
		case GL_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 0, FGFP_COMBARG_SRC_COLOR);
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 0, FGFP_COMBARG_ONE_MINUS_SRC_COLOR);
			break;
		case GL_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 0, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 0, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_SRC0_ALPHA:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 0, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND0_ALPHA:
		switch (param) {
		case GL_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
					unit, 0, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
				unit, 0, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_SRC1_RGB:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND1_RGB:
		switch (param) {
		case GL_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 1, FGFP_COMBARG_SRC_COLOR);
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 1, FGFP_COMBARG_ONE_MINUS_SRC_COLOR);
			break;
		case GL_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 1, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 1, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_SRC1_ALPHA:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 1, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND1_ALPHA:
		switch (param) {
		case GL_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
					unit, 1, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
				unit, 1, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_SRC2_RGB:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetColorCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND2_RGB:
		switch (param) {
		case GL_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 2, FGFP_COMBARG_SRC_COLOR);
			break;
		case GL_ONE_MINUS_SRC_COLOR:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 2, FGFP_COMBARG_ONE_MINUS_SRC_COLOR);
			break;
		case GL_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
					unit, 2, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetColorCombineArgMod(ctx->fimg,
				unit, 2, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_SRC2_ALPHA:
		switch (param) {
		case GL_TEXTURE:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_TEX);
			break;
		case GL_CONSTANT:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_CONST);
			break;
		case GL_PRIMARY_COLOR:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_COL);
			break;
		case GL_PREVIOUS:
			fimgCompatSetAlphaCombineArgSrc(ctx->fimg,
						unit, 2, FGFP_COMBARG_PREV);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	case GL_OPERAND2_ALPHA:
		switch (param) {
		case GL_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
					unit, 2, FGFP_COMBARG_SRC_ALPHA);
			break;
		case GL_ONE_MINUS_SRC_ALPHA:
			fimgCompatSetAlphaCombineArgMod(ctx->fimg,
				unit, 2, FGFP_COMBARG_ONE_MINUS_SRC_ALPHA);
			break;
		default:
			setError(GL_INVALID_ENUM);
			LOGD("Invalid value %x for %x", param, pname);
		}
		break;
	default:
		setError(GL_INVALID_ENUM);
		LOGD("Invalid enum: %x", pname);
	}
}

GL_API void GL_APIENTRY glTexEnvfv (GLenum target, GLenum pname,
							const GLfloat *params)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_TEXTURE_ENV_COLOR:
		fimgCompatSetEnvColor(ctx->fimg, unit,
				params[0], params[1], params[2], params[3]);
		break;
	default:
		glTexEnvf(target, pname, *params);
	}
}

GL_API void GL_APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_RGB_SCALE:
		if (param != 1.0 && param != 2.0 && param != 4.0) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %f for %x", param, pname);
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, param);
		break;
	case GL_ALPHA_SCALE:
		if (param != 1.0 && param != 2.0 && param != 4.0) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %f for %x", param, pname);
			return;
		}
		fimgCompatSetAlphaScale(ctx->fimg, unit, param);
		break;
	default:
		glTexEnvi(target, pname, (GLint)(param));
	}
}

GL_API void GL_APIENTRY glTexEnvx (GLenum target, GLenum pname, GLfixed param)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_RGB_SCALE:
		if (param != (1 << 16) && param != (2 << 16) && param != (4 << 16)) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, floatFromFixed(param));
		break;
	case GL_ALPHA_SCALE:
		if (param != (1 << 16) && param != (2 << 16) && param != (4 << 16)) {
			setError(GL_INVALID_VALUE);
			LOGD("Invalid value %x for %x", param, pname);
			return;
		}
		fimgCompatSetAlphaScale(ctx->fimg, unit, floatFromFixed(param));
		break;
	default:
		glTexEnvi(target, pname, param);
	}
}

GL_API void GL_APIENTRY glTexEnviv (GLenum target, GLenum pname,
							const GLint *params)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_TEXTURE_ENV_COLOR:
		fimgCompatSetEnvColor(ctx->fimg, unit,
			clampfFromInt(params[0]), clampfFromInt(params[1]),
			clampfFromInt(params[2]), clampfFromInt(params[3]));
		break;
	default:
		glTexEnvi(target, pname, *params);
	}
}

GL_API void GL_APIENTRY glTexEnvxv (GLenum target, GLenum pname,
							const GLfixed *params)
{
	if (target != GL_TEXTURE_ENV) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	GLint unit = ctx->activeTexture;

	switch (pname) {
	case GL_TEXTURE_ENV_COLOR:
		fimgCompatSetEnvColor(ctx->fimg, unit,
			floatFromFixed(params[0]), floatFromFixed(params[1]),
			floatFromFixed(params[2]), floatFromFixed(params[3]));
		break;
	default:
		glTexEnvx(target, pname, *params);
	}
}

GL_API void GL_APIENTRY glGetTexEnvfv (GLenum env, GLenum pname, GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetTexEnviv (GLenum env, GLenum pname, GLint *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetTexEnvxv (GLenum env, GLenum pname, GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetTexParameterfv (GLenum target, GLenum pname,
								GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetTexParameteriv (GLenum target, GLenum pname,
								GLint *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetTexParameterxv (GLenum target, GLenum pname,
								GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API GLboolean GL_APIENTRY glIsTexture (GLuint texture)
{
	if (texture == 0 || !fglTextureObjects.isValid(texture))
		return GL_FALSE;

	return GL_TRUE;
}
