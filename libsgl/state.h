#ifndef _LIBSGL_STATE_H_
#define _LIBSGL_STATE_H_

#include "common.h"
#include "types.h"
#include "fimg/host.h"

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
	const GLvoid *pointer;
	GLint stride;
	GLint type;
	GLint size;
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
	FGLstack<FGLmatrix> stack[2 + FGL_MAX_TEXTURE_UNITS];
	GLint activeMatrix;
	GLint activeTexture;
};

struct FGLVertexState {
	FGLvec4f color;
	FGLvec3f normal;
	FGLvec4f multiTexCoord[FGL_MAX_TEXTURE_UNITS];
};

struct FGLContext {
	FGLVertexState vertex;
	FGLArrayState array[4 + FGL_MAX_TEXTURE_UNITS];
	GLboolean enableArray[4 + FGL_MAX_TEXTURE_UNITS];
	GLint activeTexture;
	FGLViewportState viewport;
	FGLMatrixState matrix;
};

#endif // _LIBSGL_STATE_H_
