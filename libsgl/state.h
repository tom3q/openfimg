#ifndef _LIBSGL_STATE_H_
#define _LIBSGL_STATE_H_

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <bionic_tls.h>

#include "common.h"
#include "types.h"
#include "fimg/fimg.h"

enum {
	FGL_COMP_RED = 0,
	FGL_COMP_BLUE,
	FGL_COMP_GREEN,
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

struct FGLArrayState {
	GLboolean enabled;
	const GLvoid *pointer;
	GLint stride;
	GLint type;
	GLint size;

	FGLArrayState() :
	enabled(GL_FALSE), pointer(NULL), stride(0),
	type(FGHI_ATTRIB_DT_FLOAT), size(FGHI_NUMCOMP(4)) {};
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
#define FGL_MATRIX_TEXTURE(i)	(FGL_MATRIX_TEXTURE + (i))

struct FGLMatrixState {
	FGLstack<FGLmatrix> stack[3 + FGL_MAX_TEXTURE_UNITS];
	GLboolean dirty[3 + FGL_MAX_TEXTURE_UNITS];
	GLint activeMatrix;
	GLint activeTexture;
	
	static unsigned int stackSizes[3 + FGL_MAX_TEXTURE_UNITS];
	
	FGLMatrixState() : activeMatrix(0), activeTexture(0)
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

struct FGLSurface {
	FGLuint		version;
	FGLuint		width;      // width in pixels
	FGLuint		height;     // height in pixels
	FGLuint		stride;     // stride in pixels
	FGLubyte	*vaddr;     // pointer to the bits
	unsigned long	paddr;      // physical address
	unsigned long	size;       // block size
	FGLint		format;     // pixel format
};

#define FGL_IS_CURRENT		0x00010000
#define FGL_NEVER_CURRENT	0x00020000

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

struct FGLShaderState {
	const unsigned int *data;
};

struct FGLContext {
	/* GL state */
	FGLvec4f vertex[4 + FGL_MAX_TEXTURE_UNITS];
	FGLArrayState array[4 + FGL_MAX_TEXTURE_UNITS];
//	GLboolean enableArray[4 + FGL_MAX_TEXTURE_UNITS];
	GLint activeTexture;
	FGLMatrixState matrix;
	FGLShaderState vertexShader;
	FGLShaderState pixelShader;
	/* EGL state */
	FGLEGLState egl;
	/* HW state */
	fimgContext *fimg;

	/* Static initializers */
	static FGLvec4f defaultVertex[4 + FGL_MAX_TEXTURE_UNITS];

	FGLContext(fimgContext *fctx)
	: activeTexture(0), fimg(fctx)
	{
		memcpy(vertex, defaultVertex, (4 + FGL_MAX_TEXTURE_UNITS) * sizeof(FGLvec4f));
	}
};

// We have a dedicated TLS slot in bionic
inline void setGlThreadSpecific(FGLContext* value)
{
	((uint32_t *)__get_tls())[TLS_SLOT_OPENGL] = (uint32_t)value;
}

inline FGLContext* getGlThreadSpecific()
{
	return (FGLContext *)(((unsigned *)__get_tls())[TLS_SLOT_OPENGL]);
}

#endif // _LIBSGL_STATE_H_
