/*
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
#include <GLES/glext.h>

#include "common.h"
#include "types.h"
#include "libfimg/fimg.h"

#include "fglsurface.h"
#include "fglstack.h"
#include "fglmatrix.h"
#include "fgltextureobject.h"
#include "fglbufferobject.h"
#include "fglobject.h"
#include "fglframebuffer.h"
#include "fglrenderbuffer.h"

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

struct FGLArrayState {
	GLboolean enabled;
	const GLvoid *pointer;
	GLint stride;
	GLint width;
	GLint type;
	GLint size;
	FGLBuffer *buffer;

	FGLArrayState() :
		enabled(GL_FALSE),
		pointer(NULL),
		stride(0),
		width(4),
		type(FGHI_ATTRIB_DT_FLOAT),
		size(FGHI_NUMCOMP(4)),
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

	FGLEGLState() :
		flags(0),
		dpy(0),
		config(0),
		draw(0),
		depth(0) {};
};

struct FGLTextureState {
	FGLTexture defTexture;
	FGLTextureObjectBinding binding;
	fimgTexFunc fglFunc;
	bool enabled;

	FGLTextureState() :
		defTexture(),
		binding(this),
		fglFunc(FGFP_TEXFUNC_MODULATE),
		enabled(false) {};

	inline FGLTexture *getTexture(void)
	{
		FGLTexture *tex = binding.get();
		if (!tex)
			tex = &defTexture;
		return tex;
	}
};

struct FGLScissorState {
	GLint left;
	GLint bottom;
	GLint width;
	GLint height;

	FGLScissorState() :
		left(0),
		bottom(0),
		width(2048),
		height(2048) {};
};

struct FGLMaskState {
	GLboolean alpha;
	GLboolean red;
	GLboolean green;
	GLboolean blue;
	GLboolean depth;
	GLint stencil;

	FGLMaskState() :
		alpha(1),
		red(1),
		green(1),
		blue(1),
		depth(1),
		stencil(0xff) {};
};

struct FGLStencilState {
	GLuint mask;
	GLint ref;
	GLenum func;
	GLenum fail;
	GLenum passDepthFail;
	GLenum passDepthPass;

	FGLStencilState() :
		mask(0xffffffff),
		ref(0),
		func(GL_ALWAYS),
		fail(GL_KEEP),
		passDepthFail(GL_KEEP),
		passDepthPass(GL_KEEP) {};
};

struct FGLPerFragmentState {
	FGLScissorState scissor;
	FGLStencilState stencil;
	FGLMaskState mask;
	GLenum depthFunc;
	GLenum blendSrc;
	GLenum blendDst;
	GLenum logicOp;
	bool masked;
	fimgBlendFunction fglBlendSrc;
	fimgBlendFunction fglBlendDst;

	FGLPerFragmentState() :
		blendSrc(GL_ONE),
		blendDst(GL_ZERO),
		logicOp(GL_COPY),
		masked(false),
		fglBlendSrc(FGPF_BLEND_FUNC_ONE),
		fglBlendDst(FGPF_BLEND_FUNC_ZERO) {};
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

struct FGLRasterizerState {
	float lineWidth;
	float pointSize;
	GLenum cullFace;
	GLenum frontFace;
	GLenum shadeModel;
	GLfloat polyOffFactor;
	GLfloat polyOffUnits;

	FGLRasterizerState() :
		lineWidth(1.0f),
		pointSize(1.0f),
		cullFace(GL_BACK),
		frontFace(GL_CCW),
		shadeModel(GL_SMOOTH),
		polyOffFactor(0.0f),
		polyOffUnits(0.0f) {};
};

struct FGLEnableState {
	unsigned cullFace	:1;
	unsigned polyOffFill	:1;
	unsigned scissorTest	:1;
	unsigned stencilTest	:1;
	unsigned depthTest	:1;
	unsigned blend		:1;
	unsigned dither		:1;
	unsigned colorLogicOp	:1;
	unsigned alphaTest	:1;

	FGLEnableState() :
		cullFace(0),
		polyOffFill(0),
		scissorTest(0),
		stencilTest(0),
		depthTest(0),
		blend(0),
		dither(1),
		colorLogicOp(0) {};
};

struct FGLFramebufferState {
	FGLDefaultFramebuffer defFramebuffer;
	FGLFramebufferObjectBinding binding;
	FGLAbstractFramebuffer *current;
	uint32_t curWidth;
	uint32_t curHeight;
	uint32_t curColorFormat;
	int curFlipY;

	FGLFramebufferState() :
		defFramebuffer(),
		binding(this),
		current(0),
		curWidth(0),
		curHeight(0),
		curColorFormat(0),
		curFlipY(-1) {};

	inline FGLAbstractFramebuffer *get(void)
	{
		FGLAbstractFramebuffer *fb = binding.get();
		if (!fb)
			fb = &defFramebuffer;
		return fb;
	}
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
	FGLTextureState textureExternal[FGL_MAX_TEXTURE_UNITS];
	GLuint unpackAlignment;
	GLuint packAlignment;
	FGLBufferObjectBinding arrayBuffer;
	FGLBufferObjectBinding elementArrayBuffer;
	FGLViewportState viewport;
	FGLRasterizerState rasterizer;
	FGLPerFragmentState perFragment;
	FGLClearState clear;
	FGLTexture *busyTexture[FGL_MAX_TEXTURE_UNITS];
	FGLEnableState enable;
	FGLFramebufferState framebuffer;
	FGLRenderbufferBinding renderbuffer;
	/* EGL state */
	FGLEGLState egl;
	bool finished;

	/* Static initializers */
	static FGLvec4f defaultVertex[4 + FGL_MAX_TEXTURE_UNITS];

	FGLContext(fimgContext *fctx) :
		fimg(fctx),
		activeTexture(0),
		clientActiveTexture(0),
		unpackAlignment(4),
		packAlignment(4),
		finished(true)
	{
		memcpy(vertex, defaultVertex, (4 + FGL_MAX_TEXTURE_UNITS) * sizeof(FGLvec4f));
		for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; ++i) {
			busyTexture[i] = 0;
			texture[i].defTexture.target = GL_TEXTURE_2D;
			textureExternal[i].defTexture.target = GL_TEXTURE_EXTERNAL_OES;
		}
	}
};

#endif // _LIBSGL_STATE_H_
