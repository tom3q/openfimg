/**
 * libsgl/state.h
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

#ifndef _LIBSGL_STATE_H_
#define _LIBSGL_STATE_H_

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <bionic_tls.h>

#include "common.h"
#include "types.h"
#include "libfimg/fimg.h"

#include "fglsurface.h"
#include "fglstack.h"
#include "fglmatrix.h"
#include "fgltextureobject.h"
#include "fglbufferobject.h"
#include "fglobject.h"

enum {
	FGL_COMP_RED = 0,
	FGL_COMP_GREEN,
	FGL_COMP_BLUE,
	FGL_COMP_ALPHA
};

enum {
	FGL_COMP_NX = 0,
	FGL_COMP_NY,
	FGL_COMP_NZ
};

enum {
	FGL_COMP_S = 0,
	FGL_COMP_T,
	FGL_COMP_R,
	FGL_COMP_Q
};

enum {
	FGL_ARRAY_VERTEX = 0,
	FGL_ARRAY_NORMAL,
	FGL_ARRAY_COLOR,
	FGL_ARRAY_POINT_SIZE,
	FGL_ARRAY_TEXTURE
};
#define FGL_ARRAY_TEXTURE(i)	(FGL_ARRAY_TEXTURE + (i))

#include <cutils/log.h>

struct FGLArrayState {
	GLboolean enabled;
	const GLvoid *pointer;
	GLint stride;
	GLint width;
	GLint type;
	GLint size;
	FGLBuffer *buffer;

	FGLArrayState() :
		enabled(GL_FALSE), pointer(NULL), stride(0), width(4),
		type(FGHI_ATTRIB_DT_FLOAT), size(FGHI_NUMCOMP(4)),
		buffer(0) {};
};

struct FGLViewportState {
	GLclampf zNear;
	GLclampf zFar;
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
};

enum {
	FGL_MATRIX_PROJECTION = 0,
	FGL_MATRIX_MODELVIEW,
	FGL_MATRIX_MODELVIEW_INVERSE,
	FGL_MATRIX_TEXTURE
};
#define FGL_MATRIX_TEXTURE(__mtx)	(FGL_MATRIX_TEXTURE + (__mtx))

struct FGLMatrixState {
	FGLstack<FGLmatrix> stack[3 + FGL_MAX_TEXTURE_UNITS];
	GLboolean dirty[3 + FGL_MAX_TEXTURE_UNITS];
	FGLmatrix transformMatrix;
	GLint activeMatrix;

	static unsigned int stackSizes[3 + FGL_MAX_TEXTURE_UNITS];

	FGLMatrixState() : activeMatrix(0)
	{
		for(int i = 0; i < 3 + FGL_MAX_TEXTURE_UNITS; i++) {
			stack[i].create(stackSizes[i]);
			stack[i].top().identity();
			dirty[i] = GL_TRUE;
		}
	}

	~FGLMatrixState()
	{
		for(int i = 0; i < 3 + FGL_MAX_TEXTURE_UNITS; i++)
			stack[i].destroy();
	}
};

#define FGL_IS_CURRENT		0x00010000
#define FGL_NEVER_CURRENT	0x00020000
#define FGL_NEEDS_RESTORE	0x00100000
#define FGL_TERMINATE		0x80000000

struct FGLEGLState {
	EGLint flags;
	EGLDisplay dpy;
	EGLConfig config;
	EGLSurface draw;
	EGLSurface depth;
	EGLSurface read;

	FGLEGLState() :
		flags(0), dpy(0), config(0), draw(0), depth(0), read(0) {};
};

struct FGLSurfaceState {
	FGLSurface *draw;
	FGLSurface *read;
	FGLSurface *depth;
	GLint width;
	GLint stride;
	GLint height;
	unsigned int format;
	unsigned int depthFormat;

	FGLSurfaceState() : draw(0), read(0), depth(0) {};
};

struct FGLTextureState {
	FGLTexture defTexture;
	FGLTextureObjectBinding binding;
	bool enabled;

	FGLTextureState() :
		defTexture(), binding(), enabled(false) {};

	inline FGLTexture *getTexture(void)
	{
		if(binding.isBound())
			return binding.get();
		else
			return &defTexture;
	}
};

struct FGLScissorState {
	GLint left;
	GLint bottom;
	GLint width;
	GLint height;
	GLboolean enabled;

	FGLScissorState() :
		left(0), bottom(0), width(2048), height(2048), enabled(GL_FALSE) {};
};

struct FGLMaskState {
	GLboolean alpha;
	GLboolean red;
	GLboolean green;
	GLboolean blue;
	GLboolean depth;
	GLint stencil;

	FGLMaskState() :
		alpha(1), red(1), green(1), blue(1), depth(1), stencil(0xff) {};
};

struct FGLPerFragmentState {
	FGLScissorState scissor;
	FGLMaskState mask;

	FGLPerFragmentState() :
		scissor() {};
};

struct FGLClearState {
	GLclampf red;
	GLclampf green;
	GLclampf blue;
	GLclampf alpha;
	GLclampf depth;
	GLint stencil;

	FGLClearState() :
		red(0), green(0), blue(0), alpha(0), depth(1.0), stencil(0) {};
};

struct FGLContext {
	/* HW state */
	fimgContext *fimg;
	/* GL state */
	FGLvec4f vertex[4 + FGL_MAX_TEXTURE_UNITS];
	FGLArrayState array[4 + FGL_MAX_TEXTURE_UNITS];
	GLint activeTexture;
	GLint clientActiveTexture;
	FGLMatrixState matrix;
	FGLTextureState texture[FGL_MAX_TEXTURE_UNITS];
	FGLuint unpackAlignment;
	FGLuint packAlignment;
	FGLBufferObjectBinding arrayBuffer;
	FGLBufferObjectBinding elementArrayBuffer;
	FGLViewportState viewport;
	FGLPerFragmentState perFragment;
	FGLClearState clear;
	FGLTexture *busyTexture[FGL_MAX_TEXTURE_UNITS];
	/* EGL state */
	FGLEGLState egl;
	FGLSurfaceState surface;

	/* Static initializers */
	static FGLvec4f defaultVertex[4 + FGL_MAX_TEXTURE_UNITS];

	FGLContext(fimgContext *fctx) :
		fimg(fctx), activeTexture(0), clientActiveTexture(0), matrix(),
		unpackAlignment(4), packAlignment(4), egl(), surface()
	{
		memcpy(vertex, defaultVertex, (4 + FGL_MAX_TEXTURE_UNITS) * sizeof(FGLvec4f));
		for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; ++i)
			busyTexture[i] = 0;
	}
};

// We attempt to run in parallel with software GL
#define FGL_AGL_COEXIST

#ifndef FGL_AGL_COEXIST
// We have a dedicated TLS slot in bionic

inline void setGlThreadSpecific(FGLContext* value)
{
	((uint32_t *)__get_tls())[TLS_SLOT_OPENGL] = (uint32_t)value;
}

inline FGLContext* getGlThreadSpecific()
{
	return (FGLContext *)(((unsigned *)__get_tls())[TLS_SLOT_OPENGL]);
}
#else // FGL_AGL_COEXIST
// We have to coexist with software GL

#include <pthread.h>

extern pthread_key_t eglContextKey;

inline void setGlThreadSpecific(FGLContext* value)
{
	pthread_setspecific(eglContextKey, value);
}

inline FGLContext* getGlThreadSpecific()
{
	return (FGLContext *)pthread_getspecific(eglContextKey);
}
#endif // FGL_AGL_COEXIST

#endif // _LIBSGL_STATE_H_
