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
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>
#include <GLES/gl.h>
#include "state.h"
#include "types.h"
#include "fglpoolallocator.h"
#include "fimg/fimg.h"

/* Shaders */
#include "vshader.h"
#include "fshader.h"

//#define GLES_DEBUG
#define GLES_ERR_DEBUG

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
	Context management
*/

#ifdef GLES_DEBUG
#define getContext() ( \
	LOGD("%s called getContext()", __func__), \
	_getContext())
static inline FGLContext *_getContext(void)
#else
static inline FGLContext *getContext(void)
#endif
{
	FGLContext *ctx = getGlThreadSpecific();

	if(!ctx) {
		LOGE("GL context is NULL!");
		exit(EINVAL);
	}

	return ctx;
}

static void fglRestoreHardwareState(FGLContext *ctx)
{
	/*
		TODO:
		- implement shared context support (textures, shaders)
	*/

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

	for (int i = FGL_MATRIX_PROJECTION; i < FGL_MATRIX_TEXTURE(FGL_MAX_TEXTURE_UNITS); i++)
		ctx->matrix.dirty[i] = GL_TRUE;

	fprintf(stderr, "FGL: Invalidating caches\n");
	fflush(stderr);
	fimgInvalidateFlushCache(ctx->fimg, 1, 1, 0, 0);
}

static inline void getHardware(FGLContext *ctx)
{
	int ret;

	if(unlikely((ret = fimgAcquireHardwareLock(ctx->fimg)) != 0)) {
		if(likely(ret > 0)) {
			fglRestoreHardwareState(ctx);
		} else {
			LOGE("Could not acquire hardware lock");
			exit(EBUSY);
		}
	}

	fimgFlush(ctx->fimg);
}

static inline void putHardware(FGLContext *ctx)
{
	fimgReleaseHardwareLock(ctx->fimg);
}

/**
	Error handling
*/

static pthread_mutex_t glErrorKeyMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t glErrorKey = -1;

#ifdef GLES_ERR_DEBUG
#define setError(a) ( \
	LOGD("GLES error %s in %s", #a, __func__), \
	_setError(a))
static inline void _setError(GLenum error)
#else
static inline void setError(GLenum error)
#endif
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

static inline GLint unitFromTextureEnum(GLenum texture)
{
	GLint unit;

	switch(texture) {
	case GL_TEXTURE0:
		unit = 0;
		break;
	case GL_TEXTURE1:
		unit = 1;
		break;
#if 0
	case TEXTURE2:
		unit = 2;
		break;
	case TEXTURE3:
		unit = 3;
		break;
	case TEXTURE4:
		unit = 4;
		break;
	case TEXTURE5:
		unit = 5;
		break;
	case TEXTURE6:
		unit = 6;
		break;
	case TEXTURE7:
		unit = 7;
		break;
#endif
	default:
		return -1;
	}

	return unit;
}

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

GL_API void GL_APIENTRY glMultiTexCoord4f (GLenum target,
				GLfloat s, GLfloat t, GLfloat r, GLfloat q)
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

GL_API void GL_APIENTRY glMultiTexCoord4x (GLenum target,
				GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
	glMultiTexCoord4f(target, floatFromFixed(s), floatFromFixed(t),
					floatFromFixed(r), floatFromFixed(q));
}

static inline void fglSetupAttribute(FGLContext *ctx, GLint idx, GLint size,
					GLint type, GLint stride, GLint width,
					const GLvoid *pointer)
{
	ctx->array[idx].size	= size;
	ctx->array[idx].type	= type;
	ctx->array[idx].stride	= (stride) ? stride : width;
	ctx->array[idx].width	= width;
	ctx->array[idx].pointer	= pointer;

	if(ctx->array[idx].enabled)
		fimgSetAttribute(ctx->fimg, idx, ctx->array[idx].type,
							ctx->array[idx].size);
}

GL_API void GL_APIENTRY glVertexPointer (GLint size, GLenum type,
					GLsizei stride, const GLvoid *pointer)
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

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_VERTEX, size, fglType, stride,
							fglStride, pointer);
}

GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride,
							const GLvoid *pointer)
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

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_NORMAL, 3, fglType, stride,
							fglStride, pointer);
}

GL_API void GL_APIENTRY glColorPointer (GLint size, GLenum type,
				GLsizei stride, const GLvoid *pointer)
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

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_COLOR, 4, fglType, stride,
							fglStride, pointer);
}

GL_API void GL_APIENTRY glPointSizePointerOES (GLenum type, GLsizei stride,
							const GLvoid *pointer)
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

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_POINT_SIZE, 1, fglType, stride,
							fglStride, pointer);
}

GL_API void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type,
					GLsizei stride, const GLvoid *pointer)
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

	FGLContext *ctx = getContext();
	fglSetupAttribute(ctx, FGL_ARRAY_TEXTURE(ctx->clientActiveTexture),
				size, fglType, stride, fglStride, pointer);
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

	ctx->clientActiveTexture = unit;
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

static inline void fglSetupMatrices(FGLContext *ctx)
{
	if (ctx->matrix.dirty[FGL_MATRIX_MODELVIEW] || ctx->matrix.dirty[FGL_MATRIX_PROJECTION])
	{
		FGLmatrix matrix = ctx->matrix.stack[FGL_MATRIX_PROJECTION].top();
		matrix.multiply(ctx->matrix.stack[FGL_MATRIX_MODELVIEW].top());
		fimgLoadMatrix(ctx->fimg, FGVS_MATRIX_TRANSFORM, matrix.data);
		fimgLoadMatrix(ctx->fimg, FGVS_MATRIX_NORMALS, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_PROJECTION] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_FALSE;
	}

	int i = FGL_MAX_TEXTURE_UNITS;
	while(i--) {
		if(!ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)])
			continue;

		fimgLoadMatrix(ctx->fimg, FGVS_MATRIX_TEXTURE(i), ctx->matrix.stack[FGL_MATRIX_TEXTURE(i)].top().data);
		ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)] = GL_FALSE;
	}
}

static inline void fglSetupTextures(FGLContext *ctx)
{
	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++) {
		bool enabled = ctx->texture[i].enabled;
		FGLTexture *obj = ctx->texture[i].getTexture();

		if(enabled && obj->surface.isValid() && obj->isComplete()) {
			fimgSetupTexture(ctx->fimg, obj->fimg, i);
			fimgEnableTexture(ctx->fimg, i);
		} else {
			fimgDisableTexture(ctx->fimg, i);
		}
	}
}

#ifndef FIMG_USE_VERTEX_BUFFER

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

	getHardware(ctx);

	fglSetupMatrices(ctx);
	fglSetupTextures(ctx);

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

	putHardware(ctx);
}

#else /* !FIMG_USE_VERTEX_BUFFER */

GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	if(first < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fimgArray arrays[4 + FGL_MAX_TEXTURE_UNITS];
	FGLContext *ctx = getContext();

	for(int i = 0; i < (4 + FGL_MAX_TEXTURE_UNITS); i++) {
		if(ctx->array[i].enabled) {
			arrays[i].pointer	= ctx->array[i].pointer;
			arrays[i].stride	= ctx->array[i].stride;
			arrays[i].width		= ctx->array[i].width;
		} else {
			arrays[i].pointer	= &ctx->vertex[i];
			arrays[i].stride	= 0;
			arrays[i].width		= 16;
		}
	}

	getHardware(ctx);

	fglSetupMatrices(ctx);
	fglSetupTextures(ctx);

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);

	switch (mode) {
	case GL_POINTS:
		if (count < 1)
			break;
		fimgSetVertexContext(ctx->fimg, FGPE_POINTS, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedSeparate(ctx->fimg, arrays, first, count);
		break;
	case GL_LINE_STRIP:
		if (count < 2)
			break;
		fimgSetVertexContext(ctx->fimg, FGPE_LINE_STRIP, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedRepeatLast(ctx->fimg, arrays, first, count);
		break;
	case GL_LINE_LOOP:
		if (count < 2)
			break;
		/* Workaround for line loops in buffered mode */
		fimgSetVertexContext(ctx->fimg, FGPE_LINE_STRIP, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedRepeatLastLoop(ctx->fimg, arrays, first, count);
		break;
	case GL_LINES:
		if (count < 2)
			break;
		count &= ~1;
		fimgSetVertexContext(ctx->fimg, FGPE_LINES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedSeparate(ctx->fimg, arrays, first, count);
		break;
	case GL_TRIANGLE_STRIP:
		if (count < 3)
			break;
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLE_STRIP, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedRepeatLastTwo(ctx->fimg, arrays, first, count);
		break;
	case GL_TRIANGLE_FAN:
		if (count < 3)
			break;
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLE_FAN, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedRepeatFirstLast(ctx->fimg, arrays, first, count);
		break;
	case GL_TRIANGLES:
		if (count < 3)
			break;
		if (count % 3)
			count -= count % 3;
		fimgSetVertexContext(ctx->fimg, FGPE_TRIANGLES, 4 + FGL_MAX_TEXTURE_UNITS);
		fimgDrawArraysBufferedSeparate(ctx->fimg, arrays, first, count);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}

	putHardware(ctx);
}

#endif

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

	getHardware(ctx);

	fglSetupMatrices(ctx);
	fglSetupTextures(ctx);

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

	putHardware(ctx);
}

/**
	Transformations
*/

GL_API void GL_APIENTRY glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	FGLContext *ctx = getContext();

	zNear	= clampFloat(zNear);
	zFar	= clampFloat(zFar);

	getHardware(ctx);

	fimgSetDepthRange(ctx->fimg, zNear, zFar);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetViewportParams(ctx->fimg, x, y, width, height);

	putHardware(ctx);
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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

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

	ctx->activeTexture = unit;
}

GL_API void GL_APIENTRY glPopMatrix (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

	if(ctx->matrix.stack[idx].pop()) {
		setError(GL_STACK_UNDERFLOW);
		return;
	}

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
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

	if(ctx->matrix.stack[idx].push()) {
		setError(GL_STACK_OVERFLOW);
		return;
	}

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].push();
}

/**
	Rasterization
*/

GL_API void GL_APIENTRY glCullFace (GLenum mode)
{
	unsigned int face;

	switch (mode) {
	case GL_FRONT:
		face = FGRA_BFCULL_FACE_FRONT;
		break;
	case GL_BACK:
		face = FGRA_BFCULL_FACE_BACK;
		break;
	case GL_FRONT_AND_BACK:
		face = FGRA_BFCULL_FACE_BOTH;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	getHardware(ctx);

	fimgSetFaceCullFace(ctx->fimg, face);

	putHardware(ctx);
}

GL_API void GL_APIENTRY glFrontFace (GLenum mode)
{
	int cw;

	switch (mode) {
	case GL_CW:
		cw = 1;
		break;
	case GL_CCW:
		cw = 0;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLContext *ctx = getContext();

	getHardware(ctx);

	fimgSetFaceCullFront(ctx->fimg, cw);

	putHardware(ctx);
}

/**
	Per-fragment operations
*/

GL_API void GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(width < 0 || height < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	getHardware(ctx);

	fimgSetScissorParams(ctx->fimg, x + width, x, y + height, y);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetAlphaParams(ctx->fimg, ref, fglFunc);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetFrontStencilFunc(ctx->fimg, fglFunc, ref & 0xff, mask & 0xff);
	fimgSetBackStencilFunc(ctx->fimg, fglFunc, ref & 0xff, mask & 0xff);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetFrontStencilOp(ctx->fimg, (fimgTestAction)fglFail, (fimgTestAction)fglZFail, (fimgTestAction)fglZPass);
	fimgSetBackStencilOp(ctx->fimg, (fimgTestAction)fglFail, (fimgTestAction)fglZFail, (fimgTestAction)fglZPass);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetDepthParams(ctx->fimg, fglFunc);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetBlendFunc(ctx->fimg, fglSrc, fglSrc, fglDest, fglDest);

	putHardware(ctx);
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

	getHardware(ctx);

	fimgSetLogicalOpParams(ctx->fimg, fglOp, fglOp);

	putHardware(ctx);
}

void fglSetClipper(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	FGLContext *ctx = getContext();

	getHardware(ctx);

	fimgSetXClip(ctx->fimg, left, right);
	fimgSetYClip(ctx->fimg, top, bottom);

	putHardware(ctx);
}

/**
	Texturing
*/

FGLPoolAllocator<FGLTextureObject, FGL_MAX_TEXTURE_OBJECTS> fglTextureObjects;

GL_API void GL_APIENTRY glGenTextures (GLsizei n, GLuint *textures)
{
	int name;

	if(n <= 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	while(n--) {
		name = fglTextureObjects.get();
		if(name < 0) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglTextureObjects[name] = NULL;
		LOGD("Allocated texture %d", name);
		*textures = name;
		textures++;
	}
}

GL_API void GL_APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures)
{
	unsigned name;

	if(n <= 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	while(n--) {
		name = *textures;
		textures++;

		if(!fglTextureObjects.isValid(name)) {
			LOGD("Tried to free invalid texture %d", name);
			continue;
		}

		LOGD("Freeing texture %d", name);
		delete (fglTextureObjects[name]);
		fglTextureObjects.put(name);
	}
}

GL_API void GL_APIENTRY glBindTexture (GLenum target, GLuint texture)
{
	if(target != GL_TEXTURE_2D) {
		setError(GL_INVALID_ENUM);
		return;
	}

	if(texture == 0) {
		FGLContext *ctx = getContext();
		if(ctx->texture[ctx->activeTexture].binding.isBound())
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
		obj = new FGLTextureObject;
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglTextureObjects[texture] = obj;
	}

	if(ctx->texture[ctx->activeTexture].binding.isBound())
		ctx->texture[ctx->activeTexture].binding.unbind();

	obj->bind(&ctx->texture[ctx->activeTexture].binding);
}

int fglGetFormatInfo(GLenum format, GLenum type, unsigned *bpp, bool *conv)
{
	*conv = 0;
	switch (type) {
	case GL_UNSIGNED_BYTE:
		switch (format) {
		case GL_RGB: // Needs conversion
			*conv = 1;
		case GL_RGBA:
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

void fglGenerateMipmaps(FGLTexture *obj)
{

}

inline size_t fglCalculateMipmaps(FGLTexture *obj, unsigned int width,
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
	} while (lvl < FGL_MAX_MIPMAP_LEVEL);

	obj->maxLevel = lvl;
	return offset;
}

void fglLoadTextureDirect(FGLTexture *obj, unsigned level,
						const GLvoid *pixels)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);
	unsigned width = obj->surface.width >> level;
	unsigned height = obj->surface.height >> level;
	size_t size = width*height*obj->bpp;

	memcpy((uint8_t *)obj->surface.vaddr + offset, pixels, size);
}

void fglLoadTexture(FGLTexture *obj, unsigned level,
		    const GLvoid *pixels, unsigned alignment)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);
	unsigned width = obj->surface.width >> level;
	unsigned height = obj->surface.height >> level;
	size_t line = width*obj->bpp;
	size_t stride = (line + alignment - 1) & ~(alignment - 1);
	const uint8_t *src8 = (const uint8_t *)pixels;
	uint8_t *dst8 = (uint8_t *)obj->surface.vaddr + offset;

	for(unsigned i = 0; i < height; i++) {
		memcpy(dst8, src8, line);
		src8 += stride;
		dst8 += line;
	}
}

static inline uint32_t fglPackRGBA8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	//return (a << 24) | (b << 16) | (g << 8) | r;
	return (r << 24) | (g << 16) | (b << 8) | a;
}

static inline uint16_t fglPackLA88(uint8_t l, uint8_t a)
{
	return (a << 8) | l;
}

void fglConvertTexture(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);
	unsigned width = obj->surface.width >> level;
	unsigned height = obj->surface.height >> level;

	switch (obj->format) {
	case GL_RGB: {
		size_t line = 3*width;
		size_t stride = (line + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - line;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint8_t r, g, b;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface.vaddr + offset);
		for (unsigned y = 0; y < height; y++) {
			for (unsigned x = 0; x < width; x++) {
				r = *(src8++);
				g = *(src8++);
				b = *(src8++);
				*(dst32++) = fglPackRGBA8888(r, g, b, 255);
			}
			src8 += padding;
		}
		break;
	}
	case GL_LUMINANCE: {
		size_t stride = (width + alignment - 1) & ~(alignment - 1);
		size_t padding = stride - width;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface.vaddr + offset);
		for (unsigned y = 0; y < height; y++) {
			for (unsigned x = 0; x < width; x++)
				*(dst16++) = fglPackLA88(*(src8++), 255);
			src8 += padding;
		}
		break;
	}
	default:
		LOGW("Unsupported texture conversion %d", obj->format);
		return;
	}
}

GL_API void GL_APIENTRY glTexImage2D (GLenum target, GLint level,
	GLint internalformat, GLsizei width, GLsizei height, GLint border,
	GLenum format, GLenum type, const GLvoid *pixels)
{
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

	// Check if width is a power of two
	GLsizei tmp;

	for (tmp = 1; 2*tmp <= width; tmp = 2*tmp);
	if (tmp != width) {
		setError(GL_INVALID_VALUE);
		return;
	}

	// Check if height is a power of two
	for (tmp = 1; 2*tmp <= height; tmp = 2*tmp);
	if (tmp != height) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

	// Specifying mipmaps
	if (level > 0) {
		if (!obj->surface.isValid()) {
			// Mipmaps can be specified only if the texture exists
			setError(GL_INVALID_OPERATION);
			return;
		}

		// Check dimensions
		if ((obj->surface.width >> level) != width) {
			setError(GL_INVALID_VALUE);
			return;
		}

		if ((obj->surface.height >> level) != height) {
			setError(GL_INVALID_VALUE);
			return;
		}

		// Check format
		if (obj->format != format || obj->type != type) {
			setError(GL_INVALID_VALUE);
			return;
		}

		// Copy the image (with conversion if needed)
		if (pixels != NULL) {
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
		}

		obj->levels |= 1 << level;

		fglFlushPmemSurface(&obj->surface);

		return;
	}

	// level == 0

	// Get format information
	unsigned bpp;
	bool convert;
	int fglFormat = fglGetFormatInfo(format, type, &bpp, &convert);
	if (fglFormat < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	// Level 0 with different size or bpp means dropping whole texture
	if (width != obj->surface.width || height != obj->surface.height || bpp != obj->bpp)
		fglDestroyPmemSurface(&obj->surface);

	// (Re)allocate the texture
	if (!obj->surface.isValid()) {
		obj->format = format;
		obj->type = type;
		obj->fglFormat = fglFormat;
		obj->bpp = bpp;
		obj->convert = convert;

		// Calculate mipmaps
		obj->surface.size = fglCalculateMipmaps(obj, width, height, bpp);

		// Setup the surface
		obj->surface.width = width;
		obj->surface.height = height;
		if(fglCreatePmemSurface(&obj->surface)) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}

		fimgInitTexture(obj->fimg, obj->fglFormat, obj->maxLevel,
							obj->surface.paddr);
		fimgSetTex2DSize(obj->fimg, width, height);

		obj->levels = 1;
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
	}

	if (obj->genMipmap)
		fglGenerateMipmaps(obj);

	fglFlushPmemSurface(&obj->surface);
}

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
	}
}

GL_API void GL_APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params)
{
	glTexParameteri(target, pname, *params);
}

GL_API void GL_APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param)
{
	glTexParameteri(target, pname, intFromFloat(param));
}

GL_API void GL_APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params)
{
	glTexParameteri(target, pname, intFromFloat(*params));
}

GL_API void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname, GLfixed param)
{
	glTexParameteri(target, pname, intFromFixed(param));
}

GL_API void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname, const GLfixed *params)
{
	glTexParameteri(target, pname, intFromFixed(*params));
}

/**
	Enable/disable
*/

static inline void fglSet(GLenum cap, bool state)
{
	FGLContext *ctx = getContext();

	switch (cap) {
	case GL_TEXTURE_2D:
		ctx->texture[ctx->activeTexture].enabled = state;
		return;
	}

	getHardware(ctx);

	switch (cap) {
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

	putHardware(ctx);
}

GL_API void GL_APIENTRY glEnable (GLenum cap)
{
	fglSet(cap, true);
}

GL_API void GL_APIENTRY glDisable (GLenum cap)
{
	fglSet(cap, false);
}

/**
	Reading pixels
*/

GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	FGLContext *ctx = getContext();
	FGLSurface *draw = &ctx->surface.draw;

	if(!format && !type) {
		if(!x && !y && width == draw->width && height == draw->height) {
			if(draw->vaddr) {
				fglFlushPmemSurface(draw);
				memcpy(pixels, draw->vaddr, draw->size);
			}
		}
	}
}

/**
	Clearing buffers
*/

/* HACK ALERT: Implement a proper clear function using G2D hardware */
GL_API void GL_APIENTRY glClear (GLbitfield mask)
{
	FGLContext *ctx = getContext();
	FGLSurface *draw = &ctx->surface.draw;

	if(draw->vaddr) {
		memset(draw->vaddr, 0, draw->size);
		fglFlushPmemSurface(draw);
	}
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

	getHardware(ctx);

	fimgInvalidateFlushCache(ctx->fimg, 0, 0, 1, 1);

	putHardware(ctx);
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
