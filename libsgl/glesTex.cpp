/**
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

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

/**
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

void fglCleanTextureObjects(FGLContext *ctx)
{
	fglTextureObjects.clean(ctx);
}

GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture)
{
	if(target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(texture == 0) {
		FGLContext *ctx = getContext();
		ctx->texture[ctx->activeTexture].binding.unbind();
		return;
	}

	if(!fglTextureObjects.isValid(texture)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	FGLTextureObject *obj = fglTextureObjects[texture];
	if(obj == NULL) {
		obj = new FGLTextureObject(texture);
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglTextureObjects[texture] = obj;
	}

	obj->bind(&ctx->texture[ctx->activeTexture].binding);
}

static int fglGetFormatInfo(GLenum format, GLenum type,
					unsigned *bpp, bool *conv, bool *swap)
{
	*conv = 0;
	*swap = 0;
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB: // Needs conversion
			*conv = 1;
			*bpp = 4;
			return FGTU_TSTA_TEXTURE_FORMAT_8888;
		case GL_RGBA: // Needs swapping in pixel shader
			*swap = 1;
			*bpp = 4;
			return FGTU_TSTA_TEXTURE_FORMAT_8888;
		case GL_ALPHA:
			*bpp = 1;
			return FGTU_TSTA_TEXTURE_FORMAT_8;
		case GL_LUMINANCE: // Needs conversion
			*conv = 1;
		case GL_LUMINANCE_ALPHA:
			*bpp = 2;
			return FGTU_TSTA_TEXTURE_FORMAT_88;
		default:
			return -1;
		}
	case GL_UNSIGNED_SHORT_5_6_5:
		if (format != GL_RGB)
			return -1;
		*bpp = 2;
		return FGTU_TSTA_TEXTURE_FORMAT_565;
	case GL_UNSIGNED_SHORT_4_4_4_4:
		if (format != GL_RGBA)
			return -1;
		*bpp = 2;
		return FGTU_TSTA_TEXTURE_FORMAT_4444;
	case GL_UNSIGNED_SHORT_5_5_5_1:
		if (format != GL_RGBA)
			return -1;
		*bpp = 2;
		return FGTU_TSTA_TEXTURE_FORMAT_1555;
	default:
		return -1;
	}
}

static void fglGenerateMipmapsSW(FGLTexture *obj)
{
 FUNCTION_TRACER;
	int level = 0;

	int w = obj->width;
	int h = obj->height;
	if ((w&h) == 1)
		return;

	w = (w>>1) ? : 1;
	h = (h>>1) ? : 1;

	void *curLevel = obj->surface->vaddr;
	void *nextLevel = (uint8_t *)obj->surface->vaddr
				+ fimgGetTexMipmapOffset(obj->fimg, 1);

	while(true) {
		++level;
		int stride = w;
		int bs = w;

		if (obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_565) {
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
		} else if (obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_1555) {
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
		} else if (obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_8888) {
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
		} else if ((obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_88) ||
		(obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_8)) {
			int skip;
			switch (obj->fglFormat) {
			case FGTU_TSTA_TEXTURE_FORMAT_88:	skip = 2;   break;
			default:				skip = 1;   break;
			}
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
		} else if (obj->fglFormat == FGTU_TSTA_TEXTURE_FORMAT_4444) {
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
		} else {
			LOGE("Unsupported format (%d)", obj->fglFormat);
			return;
		}

		// exit condition: we just processed the 1x1 LODs
		if ((w&h) == 1)
		break;

		w = (w>>1) ? : 1;
		h = (h>>1) ? : 1;

		curLevel = nextLevel;
		nextLevel = (uint8_t *)obj->surface->vaddr
				+ fimgGetTexMipmapOffset(obj->fimg, level + 1);
	}
}

static int fglGenerateMipmapsG2D(FGLTexture *obj, unsigned int format)
{
 FUNCTION_TRACER;
	int fd;
	struct s3c_g2d_req req;

	// Setup source image (level 0 image)
	req.src.base	= obj->surface->paddr;
	req.src.offs	= 0;
	req.src.w	= obj->width;
	req.src.h	= obj->height;
	req.src.l	= 0;
	req.src.t	= 0;
	req.src.r	= obj->width - 1;
	req.src.b	= obj->height - 1;
	req.src.fmt	= format;

	// Setup destination image template for generated mipmaps
	req.dst.base	= obj->surface->paddr;
	req.dst.l	= 0;
	req.dst.t	= 0;
	req.dst.fmt	= format;

	fd = open("/dev/s3c-g2d", O_RDWR, 0);
	if (fd < 0) {
		LOGW("Failed to open G2D device. Falling back to software.");
		return -1;
	}

	if (ioctl(fd, S3C_G2D_SET_BLENDING, G2D_NO_ALPHA) < 0) {
		LOGW("Failed to set G2D parameters. Falling back to software.");
		close(fd);
		return -1;
	}

	unsigned width = obj->width;
	unsigned height = obj->height;

	for (int lvl = 1; lvl <= obj->maxLevel; lvl++) {
		if (width > 1)
			width /= 2;

		if (height > 1)
			height /= 2;

		req.dst.offs	= fimgGetTexMipmapOffset(obj->fimg, lvl);
		req.dst.w	= width;
		req.dst.h	= height;
		req.dst.r	= width - 1;
		req.dst.b	= height - 1;

		if (ioctl(fd, S3C_G2D_BITBLT, &req) < 0) {
			LOGW("Failed to perform G2D blit operation. "
			     "Falling back to software.");
			close(fd);
			return -1;
		}
	}

	close(fd);
	return 0;
}

static void fglGenerateMipmaps(FGLTexture *obj)
{
 FUNCTION_TRACER;
	/* Handle cases supported by G2D hardware */
	switch (obj->fglFormat) {
	case FGTU_TSTA_TEXTURE_FORMAT_565:
		if(fglGenerateMipmapsG2D(obj, G2D_RGB16))
			break;
		return;
	case FGTU_TSTA_TEXTURE_FORMAT_1555:
		if(fglGenerateMipmapsG2D(obj, G2D_RGBA16))
			break;
		return;
	case FGTU_TSTA_TEXTURE_FORMAT_8888:
		if(fglGenerateMipmapsG2D(obj, G2D_ARGB32))
			break;
		return;
	}

	/* Handle other cases (including G2D failure) */
	fglGenerateMipmapsSW(obj);
}

static size_t fglCalculateMipmaps(FGLTexture *obj, unsigned int width,
					unsigned int height, unsigned int bpp)
{
	size_t offset, size;
	unsigned int lvl, check;

	size = width * height * bpp;
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

		if (width >= 2) {
			size /= 2;
			width /= 2;
		}

		if (height >= 2) {
			size /= 2;
			height /= 2;
		}
	} while (1);

	obj->maxLevel = lvl;
	return offset;
}

static void fglLoadTextureDirect(FGLTexture *obj, unsigned level,
						const GLvoid *pixels)
{
 FUNCTION_TRACER;
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t size = width*height*obj->bpp;

	memcpy((uint8_t *)obj->surface->vaddr + offset, pixels, size);
}

static void fglLoadTexture(FGLTexture *obj, unsigned level,
		    const GLvoid *pixels, unsigned alignment)
{
 FUNCTION_TRACER;
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t line = width*obj->bpp;
	size_t stride = (line + alignment - 1) & ~(alignment - 1);
	const uint8_t *src8 = (const uint8_t *)pixels;
	uint8_t *dst8 = (uint8_t *)obj->surface->vaddr + offset;

	do {
		memcpy(dst8, src8, line);
		src8 += stride;
		dst8 += line;
	} while(--height);
}

static inline uint32_t fglPackRGBA8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	//return (a << 24) | (b << 16) | (g << 8) | r;
	return (r << 24) | (g << 16) | (b << 8) | a;
}

static inline uint16_t fglPackLA88(uint8_t l, uint8_t a)
{
	return (l << 8) | a;
}

static void fglConvertTexture(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment)
{
 FUNCTION_TRACER;
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

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
				*(dst32++) = fglPackRGBA8888(src8[0],
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
				*(dst32++) = fglPackRGBA8888(src8[0],
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
				*(dst16++) = fglPackLA88(*(src8++), 255);
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
			glFlush();
			break;
		}
	}
}

GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level,
	GLint internalformat, GLsizei width, GLsizei height, GLint border,
	GLenum format, GLenum type, const GLvoid *pixels)
{
 FUNCTION_TRACER;
	// Check conditions required by specification
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
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

	if (!width || !height) {
		// Null texture specified
		obj->levels &= ~(1 << level);
		return;
	}

	// Specifying mipmaps
	if (level > 0) {
		GLint mipmapW, mipmapH;

		mipmapW = obj->width >> level;
		if (!mipmapW)
			mipmapW = 1;

		mipmapH = obj->height >> level;
		if (!mipmapH)
			mipmapH = 1;

		if (!obj->surface) {
			// Mipmaps can be specified only if the texture exists
			setError(GL_INVALID_OPERATION);
			return;
		}

		// Check dimensions
		if (mipmapW != width || mipmapH != height) {
			// Invalid size
			setError(GL_INVALID_VALUE);
			return;
		}

		// Check format
		if (obj->format != format || obj->type != type) {
			// Must be the same format as level 0
			setError(GL_INVALID_ENUM);
			return;
		}

		// Copy the image (with conversion if needed)
		if (pixels != NULL) {
			fglWaitForTexture(ctx, obj);

			if (obj->convert) {
				fglConvertTexture(obj, level, pixels,
							ctx->unpackAlignment);
			} else {
				if (ctx->unpackAlignment <= obj->bpp)
					fglLoadTextureDirect(obj, level, pixels);
				else
					fglLoadTexture(obj, level, pixels,
						       ctx->unpackAlignment);
			}

			obj->levels |= (1 << level);
			obj->dirty = true;
		}

		return;
	}

	// level == 0

#ifndef FGL_NPOT_TEXTURES
	// Check if width is a power of two
	GLsizei tmp;

	for (tmp = 1; 2*tmp <= width; tmp = 2*tmp) ;
	if (tmp != width) {
		setError(GL_INVALID_VALUE);
		return;
	}

	// Check if height is a power of two
	for (tmp = 1; 2*tmp <= height; tmp = 2*tmp) ;
	if (tmp != height) {
		setError(GL_INVALID_VALUE);
		return;
	}
#endif

	// Get format information
	unsigned bpp;
	bool convert;
	bool swap;
	int fglFormat = fglGetFormatInfo(format, type, &bpp, &convert, &swap);
	if (fglFormat < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglWaitForTexture(ctx, obj);

	// Level 0 with different size or bpp means dropping whole texture
	if (width != obj->width || height != obj->height || bpp != obj->bpp) {
		delete obj->surface;
		obj->surface = 0;
	}

	// (Re)allocate the texture
	if (!obj->surface) {
		obj->width = width;
		obj->height = height;
		obj->format = format;
		obj->type = type;
		obj->fglFormat = fglFormat;
		obj->bpp = bpp;
		obj->convert = convert;
		obj->swap = swap;

		// Calculate mipmaps
		unsigned size = fglCalculateMipmaps(obj, width, height, bpp);

		// Setup surface
		obj->surface = new FGLLocalSurface(size);
		if(!obj->surface || !obj->surface->isValid()) {
			delete obj->surface;
			obj->surface = 0;
			setError(GL_OUT_OF_MEMORY);
			return;
		}

		fimgInitTexture(obj->fimg, obj->fglFormat, obj->maxLevel,
							obj->surface->paddr);
		fimgSetTex2DSize(obj->fimg, width, height);

		obj->levels = (1 << 0);
		obj->dirty = true;
		obj->eglImage = 0;
	}

	// Copy the image (with conversion if needed)
	if (pixels != NULL) {
		if (obj->convert) {
			fglConvertTexture(obj, level, pixels,
						ctx->unpackAlignment);
		} else {
			if (ctx->unpackAlignment <= bpp)
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
 FUNCTION_TRACER;
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->width >> level;
	if (!width)
		width = 1;
	unsigned height = obj->height >> level;
	if (!height)
		height = 1;

	size_t line = w*obj->bpp;
	size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
	size_t dstStride = width*obj->bpp;
	size_t xoffset = x*obj->bpp;
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
 FUNCTION_TRACER;
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

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
				*(dst32++) = fglPackRGBA8888(src8[0],
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
				*(dst32++) = fglPackRGBA8888(src8[0],
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
				*(dst16++) = fglPackLA88(*(src8++), 255);
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
 FUNCTION_TRACER;
	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

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

	//LOGD("glTexSubImage2D: x = %d, y = %d, w = %d, h = %d",
	//				xoffset, yoffset, width, height);

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

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES (GLenum target, GLeglImageOES image)
{
	if (unlikely(target != GL_TEXTURE_2D)) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (unlikely(!image)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	android_native_buffer_t* native_buffer = (android_native_buffer_t*)image;

	if (native_buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		setError(GL_INVALID_VALUE);
	}

	if (native_buffer->common.version != sizeof(android_native_buffer_t)) {
		setError(GL_INVALID_VALUE);
	}

	FGLContext *ctx = getContext();
	FGLTexture *tex =
		ctx->texture[ctx->activeTexture].getTexture();

	GLint format, type, fglFormat, bpp, swap = 0, argb = 0;

	switch (native_buffer->format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_8888;
		bpp = 4;
		swap = 1;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_8888;
		bpp = 4;
		swap = 1;
		break;
/*
	case HAL_PIXEL_FORMAT_RGB_888:
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_8888;
		break;
*/
	case HAL_PIXEL_FORMAT_RGB_565:
		format = GL_RGB;
		type = GL_UNSIGNED_SHORT_5_6_5;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_565;
		bpp = 2;
		break;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		format = GL_BGRA_EXT;
		type = GL_UNSIGNED_BYTE;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_8888;
		bpp = 4;
		argb = 1;
		break;
	case HAL_PIXEL_FORMAT_RGBA_5551:
		format = GL_RGBA;
		type = GL_UNSIGNED_SHORT_5_5_5_1;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_1555;
		bpp = 2;
		break;
	case HAL_PIXEL_FORMAT_RGBA_4444:
		format = GL_RGBA;
		type = GL_UNSIGNED_SHORT_4_4_4_4;
		fglFormat = FGTU_TSTA_TEXTURE_FORMAT_4444;
		bpp = 2;
		break;
	default:
		setError(GL_INVALID_VALUE);
		return;
	}

	delete tex->surface;
	tex->surface = new FGLImageSurface(image);
	if (!tex->surface || !tex->surface->isValid()) {
		delete tex->surface;
		tex->surface = 0;
		setError(GL_OUT_OF_MEMORY);
		return;
	}

	tex->eglImage	= image;
	tex->format	= format;
	tex->type	= type;
	tex->fglFormat	= fglFormat;
	tex->bpp	= bpp;
	tex->convert	= 0;
	tex->maxLevel	= 0;
	tex->levels	= (1 << 0);
	tex->dirty	= true;
	tex->width	= native_buffer->stride;
	tex->height	= native_buffer->height;
	tex->swap	= swap;

	// Setup fimgTexture
	fimgInitTexture(tex->fimg, (argb << 4) | tex->fglFormat, tex->maxLevel,
						tex->surface->paddr);
	fimgSetTex2DSize(tex->fimg, native_buffer->stride, native_buffer->height);
}

#if 0
GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
	FUNC_UNIMPLEMENTED;
}
#endif

GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{
	if(target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

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
		}
		break;
	case GL_TEXTURE_MIN_FILTER:
		obj->minFilter = param;
		switch(param) {
		case GL_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_DISABLED);
			obj->useMipmap = GL_FALSE;
			break;
		case GL_NEAREST_MIPMAP_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_NEAREST);
			obj->useMipmap = GL_TRUE;
			break;
		case GL_NEAREST_MIPMAP_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_LINEAR);
			obj->useMipmap = GL_TRUE;
			break;
		case GL_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_DISABLED);
			obj->useMipmap = GL_FALSE;
			break;
		case GL_LINEAR_MIPMAP_NEAREST:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_NEAREST);
			obj->useMipmap = GL_TRUE;
			break;
		case GL_LINEAR_MIPMAP_LINEAR:
			fimgSetTexMinFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			fimgSetTexMipmap(obj->fimg, FGTU_TSTA_MIPMAP_LINEAR);
			obj->useMipmap = GL_TRUE;
			break;
		default:
			setError(GL_INVALID_VALUE);
		}
		break;
	case GL_TEXTURE_MAG_FILTER:
		obj->magFilter = param;
		switch(param) {
		case GL_NEAREST:
			fimgSetTexMagFilter(obj->fimg, FGTU_TSTA_FILTER_NEAREST);
			break;
		case GL_LINEAR:
			fimgSetTexMagFilter(obj->fimg, FGTU_TSTA_FILTER_LINEAR);
			break;
		default:
			setError(GL_INVALID_VALUE);
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
	if(target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

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
	glTexParameteri(target, pname, (GLint)(*params));
}

GL_API void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname,
								GLfixed param)
{
	glTexParameteri(target, pname, param);
}

GL_API void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname,
							const GLfixed *params)
{
	glTexParameteri(target, pname, *params);
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
			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_REPLACE);
			break;
		case GL_MODULATE:
			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_MODULATE);
			break;
		case GL_DECAL:
			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_DECAL);
			break;
		case GL_BLEND:
			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_BLEND);
			break;
		case GL_ADD:
			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_ADD);
			break;
		case GL_COMBINE:
 			fimgCompatSetTextureFunc(ctx->fimg,
						unit, FGFP_TEXFUNC_COMBINE);
			break;
		default:
			setError(GL_INVALID_ENUM);
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
		}
		break;
	case GL_RGB_SCALE:
		if (param != 1 && param != 2 && param != 4) {
			setError(GL_INVALID_VALUE);
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, param);
		break;
	case GL_ALPHA_SCALE:
		if (param != 1 && param != 2 && param != 4) {
			setError(GL_INVALID_VALUE);
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
		}
		break;
	default:
		setError(GL_INVALID_ENUM);
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
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, param);
		break;
	case GL_ALPHA_SCALE:
		if (param != 1.0 && param != 2.0 && param != 4.0) {
			setError(GL_INVALID_VALUE);
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
			return;
		}
		fimgCompatSetColorScale(ctx->fimg, unit, floatFromFixed(param));
		break;
	case GL_ALPHA_SCALE:
		if (param != (1 << 16) && param != (2 << 16) && param != (4 << 16)) {
			setError(GL_INVALID_VALUE);
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
