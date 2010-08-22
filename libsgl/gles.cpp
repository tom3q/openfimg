/**
 * libsgl/gles.cpp
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
#include <pthread.h>
#include <GLES/gl.h>
#include "state.h"
#include "types.h"
#include "fimg/fimg.h"
#include <cutils/log.h>

/* Shaders */
#include "vshader.h"
#include "fshader.h"

//#define GLES_DEBUG

/**
	Client API information
*/

static char const * const gVendorString     = "notSamsung";
static char const * const gRendererString   = "S3C6410 FIMG-3DSE";
static char const * const gVersionString    = "OpenGL ES-CM 1.1";
static char const * const gExtensionsString =
	"GL_OES_byte_coordinates "
	"GL_OES_fixed_point "
	"GL_OES_single_precision "
#if 0
	"GL_OES_read_format "                   // TODO
	"GL_OES_compressed_paletted_texture "   // TODO
	"GL_OES_draw_texture "                  // TODO
	"GL_OES_matrix_get "                    // TODO
	"GL_OES_query_matrix "                  // TODO
#endif
	"GL_OES_point_size_array "
#if 0
	"GL_OES_point_sprite "                  // TODO
	"GL_OES_EGL_image "                     // TODO
	"GL_OES_compressed_ETC1_RGB8_texture "  // TODO
	"GL_ARB_texture_compression "           // TODO
	"GL_ARB_texture_non_power_of_two "      // TODO
	"GL_ANDROID_user_clip_plane "           // TODO
	"GL_ANDROID_vertex_buffer_object "      // TODO
	"GL_ANDROID_generate_mipmap "           // TODO
#endif
;

// ----------------------------------------------------------------------------

/**
	Error handling
*/

GLenum errorCode = GL_NO_ERROR;

#ifndef GLES_DEBUG
static inline FGLContext *getContext(void)
{
	return getGlThreadSpecific();
}
#else
static inline FGLContext *_getContext(void)
{
	FGLContext *ctx = getGlThreadSpecific();

	if(!ctx) {
		LOGE("GL context is NULL!");
		exit(1);
	}

	return ctx;
}
#define getContext() ( \
	LOGD("%s called getContext()", __func__), \
	_getContext())
#endif

static pthread_mutex_t glErrorKeyMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t glErrorKey = -1;

static inline void setError(GLenum error)
{
	GLenum errorCode;

	if(unlikely(glErrorKey == -1)) {
		pthread_mutex_lock(&glErrorKeyMutex);
		if(glErrorKey == -1)
			pthread_key_create(&glErrorKey, NULL);
		pthread_mutex_unlock(&glErrorKeyMutex);
		errorCode = GL_NO_ERROR;
	} else {
		errorCode = (GLenum)pthread_getspecific(glErrorKey);
	}

	pthread_setspecific(glErrorKey, (void *)error);

	LOGD("GL error %d", error);
	if(errorCode == GL_NO_ERROR)
		errorCode = error;
}

GL_API GLenum GL_APIENTRY glGetError (void)
{
	if(unlikely(glErrorKey == -1))
		return GL_NO_ERROR;

	GLenum error = (GLenum)pthread_getspecific(glErrorKey);
	pthread_setspecific(glErrorKey, (void *)GL_NO_ERROR);
	return error;
}

/**
	Vertex state
*/

FGLvec4f FGLContext::defaultVertex[4 + FGL_MAX_TEXTURE_UNITS] = {
	/* Vertex - unused */
	{ 0.0f, 0.0f, 0.0f, 0.0f },
	/* Normal */
	{ 0.0f, 0.0f, 1.0f },
	/* Color */
	{ 1.0f, 1.0f, 1.0f, 1.0f },
	/* Point size */
	{ 1.0f },
	/* Texture 0 */
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	/* Texture 1 */
	{ 0.0f, 0.0f, 0.0f, 1.0f },
};

GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	FGLContext *ctx = getContext();

	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_RED] 	= red;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_GREEN]	= green;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_BLUE]	= blue;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_ALPHA]	= alpha;
}

GL_API void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	glColor4f(floatFromUByte(red), floatFromUByte(green), floatFromUByte(blue), floatFromUByte(alpha));
}

GL_API void GL_APIENTRY glColor4x (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
	glColor4f(floatFromFixed(red), floatFromFixed(green), floatFromFixed(blue), floatFromFixed(alpha));
}

GL_API void GL_APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
	FGLContext *ctx = getContext();

	ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NX] = nx;
	ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NY] = ny;
	ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NZ] = nz;
}

GL_API void GL_APIENTRY glNormal3x (GLfixed nx, GLfixed ny, GLfixed nz)
{
	glNormal3f(floatFromFixed(nx), floatFromFixed(ny), floatFromFixed(nz));
}

GL_API void GL_APIENTRY glMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
	GLint unit;

	if((unit = unitFromTextureEnum(target)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->vertex[FGL_ARRAY_TEXTURE(unit)][FGL_COMP_S] = s;
	ctx->vertex[FGL_ARRAY_TEXTURE(unit)][FGL_COMP_T] = t;
	ctx->vertex[FGL_ARRAY_TEXTURE(unit)][FGL_COMP_R] = r;
	ctx->vertex[FGL_ARRAY_TEXTURE(unit)][FGL_COMP_Q] = q;
}

GL_API void GL_APIENTRY glMultiTexCoord4x (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
	glMultiTexCoord4f(target, floatFromFixed(s), floatFromFixed(t), floatFromFixed(r), floatFromFixed(q));
}

static inline void fglSetupAttribute(FGLContext *ctx, GLint idx, GLint size, GLint type, GLint stride, const GLvoid *pointer)
{
	ctx->array[idx].size	= size;
	ctx->array[idx].type	= type;
	ctx->array[idx].stride	= stride;
	ctx->array[idx].pointer	= pointer;

	
	if(ctx->array[idx].enabled)
		fimgSetAttribute(ctx->fimg, idx, ctx->array[idx].type, ctx->array[idx].size);
}

GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(size) {
	case 2:
	case 3:
	case 4:
		break;
	default:
		setError(GL_INVALID_VALUE);
		return;
	}

	switch(type) {
	case GL_BYTE:
		fglType = FGHI_ATTRIB_DT_BYTE;
		fglStride = size;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 2*size;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4*size;
		break;
	case GL_FLOAT:
		fglType = FGHI_ATTRIB_DT_FLOAT;
		fglStride = 4*size;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(stride < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglStride = (stride) ? stride : fglStride;

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_VERTEX, size, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(type) {
	case GL_BYTE:
		fglType = FGHI_ATTRIB_DT_BYTE;
		fglStride = 3;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 6;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 12;
		break;
	case GL_FLOAT:
		fglType = FGHI_ATTRIB_DT_FLOAT;
		fglStride = 12;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(stride < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglStride = (stride) ? stride : fglStride;

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_NORMAL, 3, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(size) {
	case 4:
		break;
	default:
		setError(GL_INVALID_VALUE);
		return;
	}

	switch(type) {
	case GL_UNSIGNED_BYTE:
		fglType = FGHI_ATTRIB_DT_UBYTE;
		fglStride = 1*size;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4*size;
		break;
	case GL_FLOAT:
		fglType = FGHI_ATTRIB_DT_FLOAT;
		fglStride = 4*size;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(stride < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglStride = (stride) ? stride : fglStride;

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_COLOR, size, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glPointSizePointerOES (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(type) {
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4;
	case GL_FLOAT:
		fglType = FGHI_ATTRIB_DT_FLOAT;
		fglStride = 4;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(stride < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglStride = (stride) ? stride : fglStride;

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_POINT_SIZE, 1, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(size) {
	case 2:
	case 3:
	case 4:
		break;
	default:
		setError(GL_INVALID_VALUE);
		return;
	}

	switch(type) {
	case GL_BYTE:
		fglType = FGHI_ATTRIB_DT_BYTE;
		fglStride = 1*size;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 2*size;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4*size;
		break;
	case GL_FLOAT:
		fglType = FGHI_ATTRIB_DT_FLOAT;
		fglStride = 4*size;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(stride < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fglStride = (stride) ? stride : fglStride;

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_TEXTURE(ctx->activeTexture), size, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glEnableClientState (GLenum array)
{
	FGLContext *ctx = getContext();
	GLint idx;

	switch (array) {
	case GL_VERTEX_ARRAY:
		idx = FGL_ARRAY_VERTEX;
		break;
	case GL_NORMAL_ARRAY:
		idx = FGL_ARRAY_NORMAL;
		break;
	case GL_COLOR_ARRAY:
		idx = FGL_ARRAY_COLOR;
		break;
	case GL_POINT_SIZE_ARRAY_OES:
		idx = FGL_ARRAY_POINT_SIZE;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		idx = FGL_ARRAY_TEXTURE(ctx->activeTexture);
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	ctx->array[idx].enabled = GL_TRUE;
	fimgSetAttribute(ctx->fimg, idx, ctx->array[idx].type, ctx->array[idx].size);
}

static const GLint fglDefaultAttribSize[4 + FGL_MAX_TEXTURE_UNITS] = {
	4, 3, 4, 1, 4, 4
};

GL_API void GL_APIENTRY glDisableClientState (GLenum array)
{
	FGLContext *ctx = getContext();
	GLint idx;

	switch (array) {
	case GL_VERTEX_ARRAY:
		idx = FGL_ARRAY_VERTEX;
		break;
	case GL_NORMAL_ARRAY:
		idx = FGL_ARRAY_NORMAL;
		break;
	case GL_COLOR_ARRAY:
		idx = FGL_ARRAY_COLOR;
		break;
	case GL_POINT_SIZE_ARRAY_OES:
		idx = FGL_ARRAY_POINT_SIZE;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		idx = FGL_ARRAY_TEXTURE(ctx->activeTexture);
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	ctx->array[idx].enabled = GL_FALSE;
	fimgSetAttribute(ctx->fimg, idx, FGHI_ATTRIB_DT_FLOAT, fglDefaultAttribSize[idx]);
}

GL_API void GL_APIENTRY glClientActiveTexture (GLenum texture)
{
	GLint unit;

	if((unit = unitFromTextureEnum(texture)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->activeTexture = unit;
}

/**
	Drawing
*/

static inline GLint fglModeFromModeEnum(GLenum mode)
{
	GLint fglMode;

	switch (mode) {
	case GL_POINTS:
		fglMode = FGPE_POINTS;
		break;
	case GL_LINE_STRIP:
		fglMode = FGPE_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		fglMode = FGPE_LINE_LOOP;
		break;
	case GL_LINES:
		fglMode = FGPE_LINES;
		break;
	case GL_TRIANGLE_STRIP:
		fglMode = FGPE_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		fglMode = FGPE_TRIANGLE_FAN;
		break;
	case GL_TRIANGLES:
		fglMode = FGPE_TRIANGLES;
		break;
	default:
		return -1;
	}

	return fglMode;
}

static inline void fglUpdateMatrices(FGLContext *ctx)
{
	for(int i = 0; i < 3 + FGL_MAX_TEXTURE_UNITS; i++) {
		if(ctx->matrix.dirty[i]) {
			fimgLoadMatrix(ctx->fimg, i, ctx->matrix.stack[i].top().data);
			ctx->matrix.dirty[i] = GL_FALSE;
		}
	}
}

GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	if(first < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	GLint fglMode;
	if((fglMode = fglModeFromModeEnum(mode)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	unsigned int stride[4 + FGL_MAX_TEXTURE_UNITS];
	const void *pointers[4 + FGL_MAX_TEXTURE_UNITS];
	FGLContext *ctx = getContext();

	for(int i = 0; i < (4 + FGL_MAX_TEXTURE_UNITS); i++) {
		if(ctx->array[i].enabled) {
			pointers[i] 			= (const unsigned char *)ctx->array[i].pointer/* + first * ctx->array[i].stride*/;
			stride[i] 			= ctx->array[i].stride;
		} else {
			pointers[i]			= &ctx->vertex[i];
			stride[i]			= 0;
		}
	}

	fglUpdateMatrices(ctx);

	fimgFlush(ctx->fimg);
	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);
#if FIMG_INTERPOLATION_WORKAROUND != 2
	fimgSetVertexContext(ctx->fimg, fglMode, 4 + FGL_MAX_TEXTURE_UNITS);
#endif

	switch(mode) {
#if FIMG_INTERPOLATION_WORKAROUND == 2
	case GL_POINTS:
		fimgSetVertexContext(ctx->fimg, FGPE_POINTS, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysPoints(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_LINES:
		fimgSetVertexContext(ctx->fimg, FGPE_LINES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysLines(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_LINE_LOOP:
		fimgSetVertexContext(ctx->fimg, FGPE_LINES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysLineLoops(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_LINE_STRIP:
		fimgSetVertexContext(ctx->fimg, FGPE_LINES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysLineStrips(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_TRIANGLES:
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysTriangles(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_TRIANGLE_FAN:
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysTriangleFans(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_TRIANGLE_STRIP:
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawNonIndexArraysTriangleStrips(ctx->fimg, first, count, pointers, stride);
		break;
#else /* FIMG_INTERPOLATION_WORKAROUND == 2 */
#ifdef FIMG_CLIPPER_WORKAROUND
	case GL_TRIANGLE_FAN:
		fimgDrawNonIndexArraysTriangleFans(ctx->fimg, first, count, pointers, stride);
		break;
	case GL_TRIANGLE_STRIP:
		fimgDrawNonIndexArraysTriangleStrips(ctx->fimg, first, count, pointers, stride);
		break;
#endif /* FIMG_CLIPPER_WORKAROUND */
#endif /* FIMG_INTERPOLATION_WORKAROUND == 2 */
	default:
#if FIMG_INTERPOLATION_WORKAROUND == 2
		fimgSetVertexContext(ctx->fimg, fglMode, 4 + FGL_MAX_TEXTURE_UNITS);
#endif
		fimgDrawNonIndexArrays(ctx->fimg, first, count, pointers, stride);
	}
}

GL_API void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
	GLint fglMode;
	if((fglMode = fglModeFromModeEnum(mode)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	unsigned int stride[4 + FGL_MAX_TEXTURE_UNITS];
	const void *pointers[4 + FGL_MAX_TEXTURE_UNITS];
	FGLContext *ctx = getContext();

	fglUpdateMatrices(ctx);

	for(int i = 0; i < (4 + FGL_MAX_TEXTURE_UNITS); i++) {
		if(ctx->array[i].enabled) {
			pointers[i] 			= ctx->array[i].pointer;
			stride[i] 			= ctx->array[i].stride;
		} else {
			pointers[i]			= &ctx->vertex[i];
			stride[i]			= 0;
		}
	}

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);
	fimgSetVertexContext(ctx->fimg, fglMode, 4 + FGL_MAX_TEXTURE_UNITS);

	switch (type) {
	case GL_UNSIGNED_BYTE:
		fimgDrawArraysUByteIndex(ctx->fimg, count, pointers, stride, (const unsigned char *)indices);
		break;
	case GL_UNSIGNED_SHORT:
		fimgDrawArraysUShortIndex(ctx->fimg, count, pointers, stride, (const unsigned short *)indices);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

/**
	Transformations
*/

GL_API void GL_APIENTRY glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	FGLContext *ctx = getContext();

	zNear	= clampFloat(zNear);
	zFar	= clampFloat(zFar);

	fimgSetDepthRange(ctx->fimg, zNear, zFar);
}

GL_API void GL_APIENTRY glDepthRangex (GLclampx zNear, GLclampx zFar)
{
	glDepthRangef(floatFromFixed(zNear), floatFromFixed(zFar));
}

GL_API void GL_APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(width < 0 || height < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	fimgSetViewportParams(ctx->fimg, x, y, width, height);
}

/**
	Matrices
*/

unsigned int FGLMatrixState::stackSizes[3 + FGL_MAX_TEXTURE_UNITS] = {
	2,	// Projection matrices
	16,	// Model-view matrices
	16,	// Inverted model-view matrices
	2,	// Texture 0 matrices
	2	// Texture 1 matrices
};

GL_API void GL_APIENTRY glMatrixMode (GLenum mode)
{
	GLint fglMode;

	switch (mode) {
	case GL_PROJECTION:
		fglMode = FGL_MATRIX_PROJECTION;
		break;
	case GL_MODELVIEW:
		fglMode = FGL_MATRIX_MODELVIEW;
		break;
	case GL_TEXTURE:
		fglMode = FGL_MATRIX_TEXTURE;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->matrix.activeMatrix = fglMode;
}

GL_API void GL_APIENTRY glLoadMatrixf (const GLfloat *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].top().load(m);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(m);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().inverse();
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glLoadMatrixx (const GLfixed *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].top().load(m);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(m);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().inverse();
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glMultMatrixf (const GLfloat *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.load(ctx->matrix.stack[idx].top());
	mat.multiply(m);

	ctx->matrix.stack[idx].top().load(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverse();

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glMultMatrixx (const GLfixed *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.load(ctx->matrix.stack[idx].top());
	mat.multiply(m);

	ctx->matrix.stack[idx].top().load(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverse();

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glLoadIdentity (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].top().identity();
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().identity();
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.rotate(angle, x, y, z);

	ctx->matrix.stack[idx].top().multiply(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;
	
	if(idx != FGL_MATRIX_MODELVIEW)
		return;
	
	mat.transpose();

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glRotatex (GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
	glRotatef(floatFromFixed(angle), floatFromFixed(x), floatFromFixed(y), floatFromFixed(z));
}

GL_API void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.translate(x, y, z);

	ctx->matrix.stack[idx].top().multiply(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseTranslate(x, y, z);

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glTranslatex (GLfixed x, GLfixed y, GLfixed z)
{
	glTranslatef(floatFromFixed(x), floatFromFixed(y), floatFromFixed(z));
}

GL_API void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.scale(x, y, z);

	ctx->matrix.stack[idx].top().multiply(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseScale(x, y, z);

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glScalex (GLfixed x, GLfixed y, GLfixed z)
{
	glScalef(floatFromFixed(x), floatFromFixed(y), floatFromFixed(z));
}

GL_API void GL_APIENTRY glFrustumf (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	if(zNear <= 0 || zFar <= 0 || left == right || bottom == top || zNear == zFar) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.frustum(left, right, bottom, top, zNear, zFar);

	ctx->matrix.stack[idx].top().multiply(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseFrustum(left, right, bottom, top, zNear, zFar);

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glFrustumx (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	glFrustumf(floatFromFixed(left), floatFromFixed(right), floatFromFixed(bottom), floatFromFixed(top), floatFromFixed(zNear), floatFromFixed(zFar));
}

GL_API void GL_APIENTRY glOrthof (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	if(left == right || bottom == top || zNear == zFar) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	FGLmatrix mat;
	mat.ortho(left, right, bottom, top, zNear, zFar);

	ctx->matrix.stack[idx].top().multiply(mat);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseOrtho(left, right, bottom, top, zNear, zFar);

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glOrthox (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	glOrthof(floatFromFixed(left), floatFromFixed(right), floatFromFixed(bottom), floatFromFixed(top), floatFromFixed(zNear), floatFromFixed(zFar));
}

GL_API void GL_APIENTRY glActiveTexture (GLenum texture)
{
	GLint unit;

	if((unit = unitFromTextureEnum(texture)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->matrix.activeTexture = unit;
}

GL_API void GL_APIENTRY glPopMatrix (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	if(ctx->matrix.stack[idx].size() == 1) {
		setError(GL_STACK_UNDERFLOW);
		return;
	}

	ctx->matrix.stack[idx].pop();
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].pop();
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glPushMatrix (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	if(!ctx->matrix.stack[idx].space()) {
		setError(GL_STACK_OVERFLOW);
		return;
	}

	ctx->matrix.stack[idx].push();

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].push();
}

/**
	Rasterization
*/

GL_API void GL_APIENTRY glCullFace (GLenum mode)
{

}

GL_API void GL_APIENTRY glFrontFace (GLenum mode)
{

}

/**
	Per-fragment operations
*/

GL_API void GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	FGLContext *ctx = getContext();

	if(width < 0 || height < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fimgSetScissorParams(ctx->fimg, x + width, x, y + height, y);
}

static inline void fglAlphaFunc (GLenum func, GLubyte ref)
{
	fimgTestMode fglFunc;

	switch(func) {
	case GL_NEVER:
		fglFunc = FGPF_TEST_MODE_NEVER;
		break;
	case GL_ALWAYS:
		fglFunc = FGPF_TEST_MODE_ALWAYS;
		break;
	case GL_LESS:
		fglFunc = FGPF_TEST_MODE_LESS;
		break;
	case GL_LEQUAL:
		fglFunc = FGPF_TEST_MODE_LEQUAL;
		break;
	case GL_EQUAL:
		fglFunc = FGPF_TEST_MODE_EQUAL;
		break;
	case GL_GEQUAL:
		fglFunc = FGPF_TEST_MODE_GEQUAL;
		break;
	case GL_GREATER:
		fglFunc = FGPF_TEST_MODE_GREATER;
		break;
	case GL_NOTEQUAL:
		fglFunc = FGPF_TEST_MODE_NOTEQUAL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetAlphaParams(ctx->fimg, ref, fglFunc);
}

GL_API void GL_APIENTRY glAlphaFunc (GLenum func, GLclampf ref)
{
	fglAlphaFunc(func, ubyteFromClampf(ref));
}

GL_API void GL_APIENTRY glAlphaFuncx (GLenum func, GLclampx ref)
{
	fglAlphaFunc(func, ubyteFromClampx(ref));
}

GL_API void GL_APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask)
{
	fimgStencilMode fglFunc;

	switch(func) {
	case GL_NEVER:
		fglFunc = FGPF_STENCIL_MODE_NEVER;
		break;
	case GL_ALWAYS:
		fglFunc = FGPF_STENCIL_MODE_ALWAYS;
		break;
	case GL_LESS:
		fglFunc = FGPF_STENCIL_MODE_LESS;
		break;
	case GL_LEQUAL:
		fglFunc = FGPF_STENCIL_MODE_LEQUAL;
		break;
	case GL_EQUAL:
		fglFunc = FGPF_STENCIL_MODE_EQUAL;
		break;
	case GL_GEQUAL:
		fglFunc = FGPF_STENCIL_MODE_GEQUAL;
		break;
	case GL_GREATER:
		fglFunc = FGPF_STENCIL_MODE_GREATER;
		break;
	case GL_NOTEQUAL:
		fglFunc = FGPF_STENCIL_MODE_NOTEQUAL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetFrontStencilFunc(ctx->fimg, fglFunc, ref & 0xff, mask & 0xff);
	fimgSetBackStencilFunc(ctx->fimg, fglFunc, ref & 0xff, mask & 0xff);
}

static inline GLint fglActionFromEnum(GLenum action)
{
	GLint fglAction;
	
	switch(action) {
	case GL_KEEP:
		fglAction = FGPF_TEST_ACTION_KEEP;
		break;
	case GL_ZERO:
		fglAction = FGPF_TEST_ACTION_ZERO;
		break;
	case GL_REPLACE:
		fglAction = FGPF_TEST_ACTION_REPLACE;
		break;
	case GL_INCR:
		fglAction = FGPF_TEST_ACTION_INCR;
		break;
	case GL_DECR:
		fglAction = FGPF_TEST_ACTION_DECR;
		break;
	case GL_INVERT:
		fglAction = FGPF_TEST_ACTION_INVERT;
		break;
	default:
		fglAction = -1;
	}

	return fglAction;
}

GL_API void GL_APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass)
{
	GLint fglFail, fglZFail, fglZPass;

	if((fglFail = fglActionFromEnum(fail)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if((fglZFail = fglActionFromEnum(zfail)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if((fglZPass = fglActionFromEnum(zpass)) < 0) {
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetFrontStencilOp(ctx->fimg, (fimgTestAction)fglFail, (fimgTestAction)fglZFail, (fimgTestAction)fglZPass);
	fimgSetBackStencilOp(ctx->fimg, (fimgTestAction)fglFail, (fimgTestAction)fglZFail, (fimgTestAction)fglZPass);
}

GL_API void GL_APIENTRY glDepthFunc (GLenum func)
{
	fimgTestMode fglFunc;

	switch(func) {
	case GL_NEVER:
		fglFunc = FGPF_TEST_MODE_NEVER;
		break;
	case GL_ALWAYS:
		fglFunc = FGPF_TEST_MODE_ALWAYS;
		break;
	case GL_LESS:
		fglFunc = FGPF_TEST_MODE_LESS;
		break;
	case GL_LEQUAL:
		fglFunc = FGPF_TEST_MODE_LEQUAL;
		break;
	case GL_EQUAL:
		fglFunc = FGPF_TEST_MODE_EQUAL;
		break;
	case GL_GEQUAL:
		fglFunc = FGPF_TEST_MODE_GEQUAL;
		break;
	case GL_GREATER:
		fglFunc = FGPF_TEST_MODE_GREATER;
		break;
	case GL_NOTEQUAL:
		fglFunc = FGPF_TEST_MODE_NOTEQUAL;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetDepthParams(ctx->fimg, fglFunc);
}

GL_API void GL_APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor)
{
	fimgBlendFunction fglSrc, fglDest;

	switch(sfactor) {
	case GL_ZERO:
		fglSrc = FGPF_BLEND_FUNC_ZERO;
		break;
	case GL_ONE:
		fglSrc = FGPF_BLEND_FUNC_ONE;
		break;
	case GL_DST_COLOR:
		fglSrc = FGPF_BLEND_FUNC_DST_COLOR;
		break;
	case GL_ONE_MINUS_DST_COLOR:
		fglSrc = FGPF_BLEND_FUNC_ONE_MINUS_DST_COLOR;
		break;
	case GL_SRC_ALPHA:
		fglSrc = FGPF_BLEND_FUNC_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		fglSrc = FGPF_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		fglSrc = FGPF_BLEND_FUNC_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		fglSrc = FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
		break;
	case GL_SRC_ALPHA_SATURATE:
		fglSrc = FGPF_BLEND_FUNC_SRC_ALPHA_SATURATE;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch(dfactor) {
	case GL_ZERO:
		fglDest = FGPF_BLEND_FUNC_ZERO;
		break;
	case GL_ONE:
		fglDest = FGPF_BLEND_FUNC_ONE;
		break;
	case GL_SRC_COLOR:
		fglDest = FGPF_BLEND_FUNC_SRC_COLOR;
		break;
	case GL_ONE_MINUS_SRC_COLOR:
		fglDest = FGPF_BLEND_FUNC_ONE_MINUS_SRC_COLOR;
		break;
	case GL_SRC_ALPHA:
		fglDest = FGPF_BLEND_FUNC_SRC_ALPHA;
		break;
	case GL_ONE_MINUS_SRC_ALPHA:
		fglDest = FGPF_BLEND_FUNC_ONE_MINUS_SRC_ALPHA;
		break;
	case GL_DST_ALPHA:
		fglDest = FGPF_BLEND_FUNC_DST_ALPHA;
		break;
	case GL_ONE_MINUS_DST_ALPHA:
		fglDest = FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetBlendFunc(ctx->fimg, fglSrc, fglSrc, fglDest, fglDest);
}

GL_API void GL_APIENTRY glLogicOp (GLenum opcode)
{
	fimgLogicalOperation fglOp;
	
	switch(opcode) {
	case GL_CLEAR:
		fglOp = FGPF_LOGOP_CLEAR;
		break;
	case GL_AND:
		fglOp = FGPF_LOGOP_AND;
		break;
	case GL_AND_REVERSE:
		fglOp = FGPF_LOGOP_AND_REVERSE;
		break;
	case GL_COPY:
		fglOp = FGPF_LOGOP_COPY;
		break;
	case GL_AND_INVERTED:
		fglOp = FGPF_LOGOP_AND_INVERTED;
		break;
	case GL_NOOP:
		fglOp = FGPF_LOGOP_NOOP;
		break;
	case GL_XOR:
		fglOp = FGPF_LOGOP_XOR;
		break;
	case GL_OR:
		fglOp = FGPF_LOGOP_OR;
		break;
	case GL_NOR:
		fglOp = FGPF_LOGOP_NOR;
		break;
	case GL_EQUIV:
		fglOp = FGPF_LOGOP_EQUIV;
		break;
	case GL_INVERT:
		fglOp = FGPF_LOGOP_INVERT;
		break;
	case GL_OR_REVERSE:
		fglOp = FGPF_LOGOP_OR_REVERSE;
		break;
	case GL_COPY_INVERTED:
		fglOp = FGPF_LOGOP_COPY_INVERTED;
		break;
	case GL_OR_INVERTED:
		fglOp = FGPF_LOGOP_OR_INVERTED;
		break;
	case GL_NAND:
		fglOp = FGPF_LOGOP_NAND;
		break;
	case GL_SET:
		fglOp = FGPF_LOGOP_SET;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();
	fimgSetLogicalOpParams(ctx->fimg, fglOp, fglOp);
}

/**
	Texturing
*/

GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures)
{

}

GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{

}

GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture)
{

}

GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{

}

GL_API void GL_APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{

}

GL_API void GL_APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{

}

GL_API void GL_APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param)
{

}

GL_API void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname, GLfixed param)
{

}

GL_API void GL_APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{

}

GL_API void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname, const GLfixed *params)
{

}

/**
	Enable/disable
*/

static inline void fglSet(GLenum cap, GLboolean state)
{
	FGLContext *ctx = getContext();

	switch(cap) {
	case GL_CULL_FACE:
		fimgSetFaceCullEnable(ctx->fimg, state);
		break;
	case GL_POLYGON_OFFSET_FILL:
		fimgEnableDepthOffset(ctx->fimg, state);
		break;
	case GL_SCISSOR_TEST:
		fimgSetScissorEnable(ctx->fimg, state);
		break;
	case GL_ALPHA_TEST:
		fimgSetAlphaEnable(ctx->fimg, state);
		break;
	case GL_STENCIL_TEST:
		fimgSetStencilEnable(ctx->fimg, state);
		break;
	case GL_DEPTH_TEST:
		fimgSetDepthEnable(ctx->fimg, state);
		break;
	case GL_BLEND:
		fimgSetBlendEnable(ctx->fimg, state);
		break;
	case GL_DITHER:
		fimgSetDitherEnable(ctx->fimg, state);
		break;
	case GL_COLOR_LOGIC_OP:
		fimgSetLogicalOpEnable(ctx->fimg, state);
		break;
	default:
		LOGD("Unimplemented or unsupported enum %d in %s", cap, __func__);
		//setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glEnable (GLenum cap)
{
	fglSet(cap, GL_TRUE);
}

GL_API void GL_APIENTRY glDisable (GLenum cap)
{
	fglSet(cap, GL_FALSE);
}

/**
	Reading pixels
*/

GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	FGLContext *ctx = getContext();

	if(!format && !type) {
		if(!x && !y && width == ctx->surface.draw.width && height == ctx->surface.draw.height) {
			memcpy(pixels, ctx->surface.draw.vaddr, width * height * ctx->surface.draw.bpp);
		}
	}
}

/**
	Clearing buffers
*/

GL_API void GL_APIENTRY glClear (GLbitfield mask)
{
	FGLContext *ctx = getContext();

	if(ctx->surface.draw.vaddr)
		memset(ctx->surface.draw.vaddr, 0, ctx->surface.draw.width * ctx->surface.draw.height * ctx->surface.draw.bpp);
}

GL_API void GL_APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{

}

GL_API void GL_APIENTRY glClearColorx (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
{

}

GL_API void GL_APIENTRY glClearDepthf (GLclampf depth)
{

}

GL_API void GL_APIENTRY glClearDepthx (GLclampx depth)
{

}

GL_API void GL_APIENTRY glClearStencil (GLint s)
{

}

/**
	Flush/Finish
*/

GL_API void GL_APIENTRY glFlush (void)
{
	// TODO
}

GL_API void GL_APIENTRY glFinish (void)
{
	FGLContext *ctx = getContext();
	fimgFlush(ctx->fimg);
	fimgInvalidateFlushCache(ctx->fimg, 0, 0, 1, 1);
}

/**
	Context management
*/

FGLContext *fglCreateContext(void)
{
	fimgContext *fimg;
	FGLContext *ctx;

	fimg = fimgCreateContext();
	if(!fimg)
		return NULL;

	ctx = new FGLContext(fimg);
	if(!ctx) {
		fimgDestroyContext(fimg);
		return NULL;
	}

	ctx->vertexShader.data = vshaderVshader;
	ctx->vertexShader.numAttribs = 4 + FGL_MAX_TEXTURE_UNITS;

	ctx->pixelShader.data = fshaderFshader;
#ifdef FIMG_INTERPOLATION_WORKAROUND	
	ctx->pixelShader.numAttribs = 8;
#else
	ctx->pixelShader.numAttribs = 4 + FGL_MAX_TEXTURE_UNITS;
#endif

	for(int i = 0; i < 4 + FGL_MAX_TEXTURE_UNITS; i++)
		fimgSetAttribute(ctx->fimg, i, FGHI_ATTRIB_DT_FLOAT, fglDefaultAttribSize[i]);

	return ctx;
}

void fglDestroyContext(FGLContext *ctx)
{
	fimgDestroyContext(ctx->fimg);
	delete ctx;
}

void fglRestoreGLState(void)
{
	/*
		TODO:
		- check if restoring state is really required
		- implement shared context support (textures, shaders)
	*/
	
	FGLContext *ctx = getContext();

	/* Restore device specific context */
	fprintf(stderr, "FGL: Restoring hardware context\n");
	fflush(stderr);
	fimgRestoreContext(ctx->fimg);

	/* Restore shaders */
	fprintf(stderr, "FGL: Loading vertex shader\n");
	fflush(stderr);
	if(fimgLoadVShader(ctx->fimg, ctx->vertexShader.data, ctx->vertexShader.numAttribs)) {
		fprintf(stderr, "FGL: Failed to load vertex shader\n");
		fflush(stderr);
	}

	fprintf(stderr, "FGL: Loading pixel shader\n");
	fflush(stderr);
	if(fimgLoadPShader(ctx->fimg, ctx->pixelShader.data, ctx->pixelShader.numAttribs)) {
		fprintf(stderr, "FGL: Failed to load pixel shader\n");
		fflush(stderr);
	}

	// TODO
#if 0
	/* Restore textures */
	for(int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++)
		fimgSetTexUnitParams(i, NULL /* TODO */);
#endif

	fprintf(stderr, "FGL: Invalidating caches\n");
	fflush(stderr);
	fimgInvalidateFlushCache(ctx->fimg, 1, 1, 0, 0);
}
