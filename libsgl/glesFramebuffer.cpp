/*
 * libsgl/glesFramebuffer.cpp
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:  2011 by Tomasz Figa < tomasz.figa at gmail.com >
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
#include "fglrenderbuffer.h"

/*
 * Buffers (render surfaces)
 */

void fglSetColorBuffer(FGLContext *gl, FGLSurface *cbuf,
				unsigned int width, unsigned int height,
				unsigned int format)
{
	FGLDefaultFramebuffer *fb = &gl->framebuffer.defFramebuffer;

	fb->setupSurface(FGL_ATTACHMENT_COLOR, cbuf, width, height, format);
}

void fglSetDepthStencilBuffer(FGLContext *gl, FGLSurface *zbuf,
				unsigned int width, unsigned int height,
				unsigned int format)
{
	FGLDefaultFramebuffer *fb = &gl->framebuffer.defFramebuffer;

	fb->setupSurface(FGL_ATTACHMENT_DEPTH, zbuf, width, height, format);
	fb->setupSurface(FGL_ATTACHMENT_STENCIL, zbuf, width, height, format);
}

/*
 * Renderbuffers
 */

static int fglSetRenderbufferFormatInfo(FGLFramebufferAttachable *fba,
								GLenum format)
{
	switch (format) {
	case GL_RGBA4_OES:
		/* Using ARGB4444 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_COLOR);
		fba->pixFormat = FGL_PIXFMT_ARGB4444;
		break;
	case GL_RGB5_A1_OES:
		/* Using ARGB1555 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_COLOR);
		fba->pixFormat = FGL_PIXFMT_ARGB1555;
		break;
	case GL_RGB565_OES:
		/* Using RGB565 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_COLOR);
		fba->pixFormat = FGL_PIXFMT_RGB565;
		break;
	case GL_RGBA:
	case GL_RGBA8_OES:
		/* Using ABGR8888 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_COLOR);
		fba->pixFormat = FGL_PIXFMT_ABGR8888;
		break;
	case GL_BGRA_EXT:
		/* Using ARGB8888 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_COLOR);
		fba->pixFormat = FGL_PIXFMT_ARGB8888;
		break;
	case GL_DEPTH_COMPONENT16_OES:
	case GL_DEPTH_COMPONENT24_OES:
		/* Using DEPTH_STENCIL_24_8 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_DEPTH);
		fba->pixFormat = 24;
		break;
	case GL_STENCIL_INDEX8_OES:
		/* Using DEPTH_STENCIL_24_8 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_STENCIL);
		fba->pixFormat = (8 << 8);
		break;
	case GL_DEPTH_STENCIL_OES:
		/* Using DEPTH_STENCIL_24_8 physical representation */
		fba->mask = (1 << FGL_ATTACHMENT_DEPTH);
		fba->mask |= (1 << FGL_ATTACHMENT_STENCIL);
		fba->pixFormat = (8 << 8) | 24;
		break;
	default:
		return -1;
	}

	fba->format = format;
	return 0;
}

FGLObjectManager<FGLRenderbuffer, FGL_MAX_RENDERBUFFER_OBJECTS> fglRenderbufferObjects;

GL_API void GL_APIENTRY glGenRenderbuffersOES (GLsizei n, GLuint* renderbuffers)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = renderbuffers;
	FGLContext *ctx = getContext();

	do {
		name = fglRenderbufferObjects.get(ctx);
		if(name < 0) {
			glDeleteRenderbuffersOES (n - i, renderbuffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderbufferObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}


GL_API void GL_APIENTRY glDeleteRenderbuffersOES (GLsizei n, const GLuint* renderbuffers)
{
	unsigned name;

	if(n <= 0)
		return;

	do {
		name = *renderbuffers;
		renderbuffers++;

		if(!fglRenderbufferObjects.isValid(name)) {
			LOGD("Tried to free invalid renderbuffer %d", name);
			continue;
		}

		delete (fglRenderbufferObjects[name]);
		fglRenderbufferObjects.put(name);
	} while (--n);
}

GL_API void GL_APIENTRY glBindRenderbufferOES (GLenum target, GLuint renderbuffer)
{
	FGLRenderbufferBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_RENDERBUFFER_OES:
		binding = &ctx->renderbuffer;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(renderbuffer == 0) {
		binding->bind(0);
		return;
	}

	if(!fglRenderbufferObjects.isValid(renderbuffer)
	    && fglRenderbufferObjects.get(renderbuffer, ctx) < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLRenderbuffer *rb = fglRenderbufferObjects[renderbuffer];
	if(rb == NULL) {
		rb = new FGLRenderbuffer(renderbuffer);
		if (rb == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglRenderbufferObjects[renderbuffer] = rb;
	}

	binding->bind(&rb->object);
}

GL_API GLboolean GL_APIENTRY glIsRenderbufferOES (GLuint renderbuffer)
{
	if (renderbuffer == 0 || !fglRenderbufferObjects.isValid(renderbuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API void GL_APIENTRY glRenderbufferStorageOES (GLenum target,
			GLenum internalformat, GLsizei width, GLsizei height)
{
	if (target != GL_RENDERBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if (width > FGL_MAX_TEXTURE_SIZE || height > FGL_MAX_TEXTURE_SIZE) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	FGLRenderbuffer *obj = ctx->renderbuffer.get();
	if (!obj) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	const FGLPixelFormat *pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned oldSize = obj->width * obj->height * pix->pixelSize;
	int ret = fglSetRenderbufferFormatInfo(obj, internalformat);
	if (ret < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	pix = FGLPixelFormat::get(obj->pixFormat);
	unsigned size = width * height * pix->pixelSize;
	if (size != oldSize) {
		delete obj->surface;
		obj->surface = 0;
	}

	obj->width = width;
	obj->height = height;

	if (obj->width != width || obj->height != height
	    || obj->format != internalformat)
		obj->markFramebufferDirty();

	if (!size) {
		obj->mask = 0;
		return;
	}

	if (!obj->surface) {
		obj->surface = new FGLLocalSurface(size);
		if(!obj->surface || !obj->surface->isValid()) {
			delete obj->surface;
			obj->surface = 0;
			setError(GL_OUT_OF_MEMORY);
			return;
		}
	}
}

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES (GLenum target, GLenum pname, GLint* params)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();

	if(!ctx->renderbuffer.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *obj = ctx->renderbuffer.get();

	switch (pname) {
	case GL_RENDERBUFFER_WIDTH_OES:
		*params = obj->width;
		return;
	case GL_RENDERBUFFER_HEIGHT_OES:
		*params = obj->height;
		return;
	case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
		*params = obj->format;
		return;
	}

	if (obj->mask & FGL_ATTACHMENT_COLOR) {
		const FGLPixelFormat *cfg = FGLPixelFormat::get(obj->pixFormat);
		switch (pname) {
		case GL_RENDERBUFFER_RED_SIZE_OES:
			*params = cfg->comp[FGL_COMP_RED].size;
			return;
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
			*params = cfg->comp[FGL_COMP_GREEN].size;
			return;
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
			*params = cfg->comp[FGL_COMP_BLUE].size;
			return;
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
			*params = cfg->comp[FGL_COMP_ALPHA].size;
			return;
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = 0;
			return;
		}
	}

	if (obj->mask & (FGL_ATTACHMENT_DEPTH | FGL_ATTACHMENT_STENCIL)) {
		switch (pname)
		{
		case GL_RENDERBUFFER_RED_SIZE_OES:
		case GL_RENDERBUFFER_GREEN_SIZE_OES:
		case GL_RENDERBUFFER_BLUE_SIZE_OES:
		case GL_RENDERBUFFER_ALPHA_SIZE_OES:
			*params = 0;
			return;
		case GL_RENDERBUFFER_DEPTH_SIZE_OES:
			*params = obj->pixFormat & 0xff;
			return;
		case GL_RENDERBUFFER_STENCIL_SIZE_OES:
			*params = obj->pixFormat >> 8;
			return;
		}
	}

	/* Renderbuffer without storage */
	switch (pname) {
	case GL_RENDERBUFFER_RED_SIZE_OES:
	case GL_RENDERBUFFER_GREEN_SIZE_OES:
	case GL_RENDERBUFFER_BLUE_SIZE_OES:
	case GL_RENDERBUFFER_ALPHA_SIZE_OES:
	case GL_RENDERBUFFER_DEPTH_SIZE_OES:
	case GL_RENDERBUFFER_STENCIL_SIZE_OES:
		*params = 0;
		return;
	}

	setError(GL_INVALID_ENUM);
}

/*
 * Framebuffer Objects
 */

FGLObjectManager<FGLFramebuffer, FGL_MAX_FRAMEBUFFER_OBJECTS> fglFramebufferObjects;

GL_API void GL_APIENTRY glGenFramebuffersOES (GLsizei n, GLuint* framebuffers)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = framebuffers;
	FGLContext *ctx = getContext();

	do {
		name = fglFramebufferObjects.get(ctx);
		if(name < 0) {
			glDeleteFramebuffersOES (n - i, framebuffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglFramebufferObjects[name] = NULL;
		*cur = name;
		cur++;
	} while (--i);
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES (GLsizei n, const GLuint* framebuffers)
{
	unsigned name;

	if(n <= 0)
		return;

	while(n--) {
		name = *framebuffers;
		framebuffers++;

		if(!fglFramebufferObjects.isValid(name)) {
			LOGD("Tried to free invalid framebuffer %d", name);
			continue;
		}

		delete (fglFramebufferObjects[name]);
		fglFramebufferObjects.put(name);
	}
}

GL_API void GL_APIENTRY glBindFramebufferOES (GLenum target, GLuint framebuffer)
{
	FGLFramebufferObjectBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_FRAMEBUFFER_OES:
		binding = &ctx->framebuffer.binding;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(framebuffer == 0) {
		binding->bind(0);
		return;
	}

	if(!fglFramebufferObjects.isValid(framebuffer)
	    && fglFramebufferObjects.get(framebuffer, ctx) < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLFramebuffer *fb = fglFramebufferObjects[framebuffer];
	if(fb == NULL) {
		fb = new FGLFramebuffer(framebuffer);
		if (fb == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglFramebufferObjects[framebuffer] = fb;
	}

	binding->bind(&fb->object);
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES (GLuint framebuffer)
{
	if (framebuffer == 0 || !fglFramebufferObjects.isValid(framebuffer))
		return GL_FALSE;

	return GL_TRUE;
}

GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES (GLenum target)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_ENUM);
		return 0;
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return 0;
	}

	FGLFramebuffer *fb = ctx->framebuffer.binding.get();
	return fb->checkStatus();
}

GL_API void GL_APIENTRY glFramebufferRenderbufferOES(GLenum target,
				GLenum attachment, GLenum renderbuffertarget,
				GLuint renderbuffer)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLRenderbuffer *rb = NULL;
	if (renderbuffer != 0) {
		if (renderbuffertarget != GL_RENDERBUFFER_OES) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		if(!fglRenderbufferObjects.isValid(renderbuffer)) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		rb = fglRenderbufferObjects[renderbuffer];
		if(rb == NULL) {
			rb = new FGLRenderbuffer(renderbuffer);
			if (rb == NULL) {
				setError(GL_OUT_OF_MEMORY);
				return;
			}
			fglRenderbufferObjects[renderbuffer] = rb;
		}
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLFramebuffer *fb = ctx->framebuffer.binding.get();

	FGLAttachmentIndex index;
	switch (attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
		index = FGL_ATTACHMENT_COLOR;
		break;
	case GL_DEPTH_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_DEPTH;
		break;
	case GL_STENCIL_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_STENCIL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	fb->attach(index, rb);
}

extern FGLObjectManager<FGLTexture, FGL_MAX_TEXTURE_OBJECTS> fglTextureObjects;

GL_API void GL_APIENTRY glFramebufferTexture2DOES (GLenum target,
					GLenum attachment, GLenum textarget,
					GLuint texture, GLint level)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLTexture *tex = NULL;
	if (texture != 0) {
		if (textarget != GL_TEXTURE_2D) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		if (level != 0) {
			setError(GL_INVALID_VALUE);
			return;
		}

		if(!fglTextureObjects.isValid(texture)) {
			setError(GL_INVALID_OPERATION);
			return;
		}

		tex = fglTextureObjects[texture];
		if(tex == NULL) {
			tex = new FGLTexture(texture);
			if (tex == NULL) {
				setError(GL_OUT_OF_MEMORY);
				return;
			}
			fglTextureObjects[texture] = tex;
			tex->target = textarget;
		}
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLFramebuffer *fb = ctx->framebuffer.binding.get();
	FGLAttachmentIndex index;
	switch (attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
		index = FGL_ATTACHMENT_COLOR;
		break;
	case GL_DEPTH_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_DEPTH;
		break;
	case GL_STENCIL_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_STENCIL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	fb->attach(index, tex);
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES (GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
	if(target != GL_FRAMEBUFFER_OES) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLContext *ctx = getContext();

	if (!ctx->framebuffer.binding.isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLAttachmentIndex index;
	switch (attachment)
	{
	case GL_COLOR_ATTACHMENT0_OES:
		index = FGL_ATTACHMENT_COLOR;
		break;
	case GL_DEPTH_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_DEPTH;
		break;
	case GL_STENCIL_ATTACHMENT_OES:
		index = FGL_ATTACHMENT_STENCIL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLFramebuffer *fb = ctx->framebuffer.binding.get();
	FGLFramebufferAttachable *fba = fb->get(index);

	if (fba) {
		switch(pname)
		{
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES:
			*params = fba->getType();
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES:
			*params = fba->getName();
			break;
		case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES:
			if (fba->getType() != GL_TEXTURE_2D) {
				setError(GL_INVALID_ENUM);
				return;
			}
			*params = 0;
			break;
		default:
			setError(GL_INVALID_ENUM);
		}
		return;
	}

	switch(pname)
	{
	case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES:
		*params = GL_NONE_OES;
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}
