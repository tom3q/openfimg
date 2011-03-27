/**
 * libsgl/glesBase.cpp
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

//#define TRACE_FUNCTIONS
//#define GLES_DEBUG
#define GLES_ERR_DEBUG

/**
	Error handling
*/

pthread_mutex_t glErrorKeyMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_key_t glErrorKey = -1;

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

GL_API void GL_APIENTRY glColor4f (GLfloat red, GLfloat green,
						GLfloat blue, GLfloat alpha)
{
	FGLContext *ctx = getContext();

	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_RED] 	= red;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_GREEN]	= green;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_BLUE]	= blue;
	ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_ALPHA]	= alpha;
}

GL_API void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green,
						GLubyte blue, GLubyte alpha)
{
	glColor4f(clampfFromUByte(red), clampfFromUByte(green),
				clampfFromUByte(blue), clampfFromUByte(alpha));
}

GL_API void GL_APIENTRY glColor4x (GLfixed red, GLfixed green,
						GLfixed blue, GLfixed alpha)
{
	glColor4f(floatFromFixed(red), floatFromFixed(green),
				floatFromFixed(blue), floatFromFixed(alpha));
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

/*
 * Buffer objects
 */

FGLObjectManager<FGLBuffer, FGL_MAX_BUFFER_OBJECTS> fglBufferObjects;

GL_API void GL_APIENTRY glGenBuffers (GLsizei n, GLuint *buffers)
{
	if(n <= 0)
		return;

	int name;
	GLsizei i = n;
	GLuint *cur = buffers;
	FGLContext *ctx = getContext();

	do {
		name = fglBufferObjects.get(ctx);
		if(name < 0) {
			glDeleteBuffers(n - i, buffers);
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglBufferObjects[name] = NULL;
		LOGD("Allocated buffer %d", name);
		*cur = name;
		cur++;
	} while (--i);
}

GL_API void GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
	unsigned name;

	if(n <= 0)
		return;

	while(n--) {
		name = *buffers;
		buffers++;

		if(!fglBufferObjects.isValid(name)) {
			LOGD("Tried to free invalid buffer %d", name);
			continue;
		}

		LOGD("Freeing buffer %d", name);
		delete (fglBufferObjects[name]);
		fglBufferObjects.put(name);
	}
}

GL_API void GL_APIENTRY glBindBuffer (GLenum target, GLuint buffer)
{
	FGLBufferObjectBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_ARRAY_BUFFER:
		binding = &ctx->arrayBuffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		binding = &ctx->elementArrayBuffer;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if(buffer == 0) {
		binding->unbind();
		return;
	}

	if(!fglBufferObjects.isValid(buffer)) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLBufferObject *obj = fglBufferObjects[buffer];
	if(obj == NULL) {
		obj = new FGLBufferObject(buffer);
		if (obj == NULL) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglBufferObjects[buffer] = obj;
	}

	obj->bind(binding);
}

GL_API void GL_APIENTRY glBufferData (GLenum target, GLsizeiptr size,
					const GLvoid *data, GLenum usage)
{
	FGLBufferObjectBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_ARRAY_BUFFER:
		binding = &ctx->arrayBuffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		binding = &ctx->elementArrayBuffer;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if (!binding->isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLBuffer *buf = binding->get();

	if (buf->create(size)) {
		setError(GL_OUT_OF_MEMORY);
		return;
	}

	if (data != 0)
		memcpy(buf->memory, data, size);
}

GL_API void GL_APIENTRY glBufferSubData (GLenum target, GLintptr offset,
					GLsizeiptr size, const GLvoid *data)
{
	FGLBufferObjectBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_ARRAY_BUFFER:
		binding = &ctx->arrayBuffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		binding = &ctx->elementArrayBuffer;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	if (!binding->isBound()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	FGLBuffer *buf = binding->get();

	if (offset + size > buf->size) {
		setError(GL_INVALID_VALUE);
		return;
	}

	memcpy((uint8_t *)buf->memory + offset, data, size);
}

GL_API GLboolean GL_APIENTRY glIsBuffer (GLuint buffer)
{
	if (buffer == 0 || !fglBufferObjects.isValid(buffer))
		return GL_FALSE;

	return GL_TRUE;
}

/*
 * Arrays
 */

static inline void fglSetupAttribute(FGLContext *ctx, GLint idx, GLint size,
					GLint type, GLint stride, GLint width,
					const GLvoid *pointer)
{
	FGLBuffer *buf = ctx->arrayBuffer.get();

	if (buf)
		pointer = buf->getAddress(pointer);

	ctx->array[idx].buffer	= buf;
	ctx->array[idx].size	= size;
	ctx->array[idx].type	= type;
	ctx->array[idx].stride	= (stride) ? stride : width;
	ctx->array[idx].width	= width;
	ctx->array[idx].pointer	= pointer;

	if(ctx->array[idx].enabled)
		fimgSetAttribute(ctx->fimg, idx, type, size);
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
		fglType = FGHI_ATTRIB_DT_NBYTE;
		fglStride = size;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_NSHORT;
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
		fglType = FGHI_ATTRIB_DT_NBYTE;
		fglStride = 3;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_NSHORT;
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
		fglType = FGHI_ATTRIB_DT_NUBYTE;
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
		fglType = FGHI_ATTRIB_DT_NBYTE;
		fglStride = 1*size;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_NSHORT;
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

static void fglEnableClientState(FGLContext *ctx, GLint idx)
{
	ctx->array[idx].enabled = GL_TRUE;
	fimgSetAttribute(ctx->fimg, idx, ctx->array[idx].type, ctx->array[idx].size);
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
		idx = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	fglEnableClientState(ctx, idx);
}

static const GLint fglDefaultAttribSize[4 + FGL_MAX_TEXTURE_UNITS] = {
	4, 3, 4, 1, 4, 4
};

static void fglDisableClientState(FGLContext *ctx, GLint idx)
{
	ctx->array[idx].enabled = GL_FALSE;
	fimgSetAttribute(ctx->fimg, idx, FGHI_ATTRIB_DT_FLOAT, fglDefaultAttribSize[idx]);
}

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
		idx = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	fglDisableClientState(ctx, idx);
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

GL_API void GL_APIENTRY glShadeModel (GLenum mode)
{
	FGLContext *ctx = getContext();

	switch (mode) {
	case GL_SMOOTH:
		fimgSetShadingMode(ctx->fimg, 0, FGL_ARRAY_COLOR);
		break;
	case GL_FLAT:
		fimgSetShadingMode(ctx->fimg, 1, FGL_ARRAY_COLOR);
		break;
	}
}

static inline void fglSetupMatrices(FGLContext *ctx)
{
	if (ctx->matrix.dirty[FGL_MATRIX_MODELVIEW]
		|| ctx->matrix.dirty[FGL_MATRIX_PROJECTION])
	{
		/* Calculate and load transformation matrix */
		FGLmatrix *proj, *modview, *transform;

		transform = &ctx->matrix.transformMatrix;
		proj = &ctx->matrix.stack[FGL_MATRIX_PROJECTION].top();
		modview = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW].top();
		transform->multiply(*proj, *modview);

		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TRANSFORM, transform->data);

		/* Load lighting matrix */
		FGLmatrix *light;

		light = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top();

		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_LIGHTING, light->data);

		/* Mark transformation matrices as clean */
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_PROJECTION] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_FALSE;
	}

	/* Load texture coordinate matrices */
	int i = FGL_MAX_TEXTURE_UNITS - 1;
	do {
		if(!ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)])
			continue;

		FGLmatrix *tex = &ctx->matrix.stack[FGL_MATRIX_TEXTURE(i)].top();
		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TEXTURE(i), tex->data);
		ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)] = GL_FALSE;
	} while (i--);
}

static inline void fglSetupTextures(FGLContext *ctx)
{
	bool flush = false;
	int i = FGL_MAX_TEXTURE_UNITS - 1;

	do {
		bool enabled = ctx->texture[i].enabled;
		FGLTexture *tex = ctx->texture[i].getTexture();

		if(enabled && tex->surface && tex->isComplete()) {
			/* Texture is ready */
			if (tex->dirty)
				tex->surface->flush();
			fimgCompatSetupTexture(ctx->fimg, tex->fimg, i, tex->swap);
			fimgCompatSetTextureEnable(ctx->fimg, i, 1);
			ctx->busyTexture[i] = tex;
			flush = true;
			if (!tex->eglImage)
				tex->dirty = 0;
		} else {
			/* Texture is not ready */
			fimgCompatSetTextureEnable(ctx->fimg, i, 0);
		}
	} while (i--);

	if (flush)
		fimgInvalidateFlushCache(ctx->fimg, 0, 1, 0, 0);
}

GL_API void GL_APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	uint32_t fglMode;

	if(first < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	fimgArray arrays[4 + FGL_MAX_TEXTURE_UNITS];
	FGLContext *ctx = getContext();

	for(int i = 0; i < (4 + FGL_MAX_TEXTURE_UNITS); ++i) {
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

	fglSetupMatrices(ctx);
	fglSetupTextures(ctx);

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);

	switch (mode) {
	case GL_POINTS:
		if (count < 1)
			return;
		fglMode = FGPE_POINTS;
		break;
	case GL_LINE_STRIP:
		if (count < 2)
			return;
		fglMode = FGPE_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		if (count < 2)
			return;
		fglMode = FGPE_LINE_LOOP;
		break;
	case GL_LINES:
		if (count < 2)
			return;
		count &= ~1;
		fglMode = FGPE_LINES;
		break;
	case GL_TRIANGLE_STRIP:
		if (count < 3)
			return;
		fglMode = FGPE_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		if (count < 3)
			return;
		fglMode = FGPE_TRIANGLE_FAN;
		break;
	case GL_TRIANGLES:
		if (count < 3)
			return;
		count -= count % 3;
		fglMode = FGPE_TRIANGLES;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

#ifndef FIMG_USE_VERTEX_BUFFER
	fimgDrawArrays(ctx->fimg, fglMode, arrays, first, count);
#else
	if (count <= 24)
		fimgDrawArraysBuffered(ctx->fimg, fglMode, arrays, first, count);
	else
		fimgDrawArrays(ctx->fimg, fglMode, arrays, first, count);
#endif
}

GL_API void GL_APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type,
							const GLvoid *indices)
{
	uint32_t fglMode;
	fimgArray arrays[4 + FGL_MAX_TEXTURE_UNITS];
	FGLContext *ctx = getContext();

	if(ctx->elementArrayBuffer.isBound())
		indices = ctx->elementArrayBuffer.get()->getAddress(indices);

	for(int i = 0; i < (4 + FGL_MAX_TEXTURE_UNITS); ++i) {
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

	fglSetupMatrices(ctx);
	fglSetupTextures(ctx);

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);

	switch (mode) {
	case GL_POINTS:
		if (count < 1)
			return;
		fglMode = FGPE_POINTS;
		break;
	case GL_LINE_STRIP:
		if (count < 2)
			return;
		fglMode = FGPE_LINE_STRIP;
		break;
	case GL_LINE_LOOP:
		if (count < 2)
			return;
		fglMode = FGPE_LINE_LOOP;
		break;
	case GL_LINES:
		if (count < 2)
			return;
		count &= ~1;
		fglMode = FGPE_LINES;
		break;
	case GL_TRIANGLE_STRIP:
		if (count < 3)
			return;
		fglMode = FGPE_TRIANGLE_STRIP;
		break;
	case GL_TRIANGLE_FAN:
		if (count < 3)
			return;
		fglMode = FGPE_TRIANGLE_FAN;
		break;
	case GL_TRIANGLES:
		if (count < 3)
			return;
		count -= count % 3;
		fglMode = FGPE_TRIANGLES;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	switch (type) {
	case GL_UNSIGNED_BYTE:
#ifndef FIMG_USE_VERTEX_BUFFER
		fimgDrawElementsUByteIdx(ctx->fimg, fglMode, arrays,
					count, (const uint8_t *)indices);
#else
		if (count > 24)
			fimgDrawElementsUByteIdx(ctx->fimg, fglMode, arrays,
					count, (const uint8_t *)indices);
		else
			fimgDrawElementsBufferedUByteIdx(ctx->fimg, fglMode,
				arrays, count, (const uint8_t *)indices);
#endif
		break;
	case GL_UNSIGNED_SHORT:
#ifndef FIMG_USE_VERTEX_BUFFER
		fimgDrawElementsUShortIdx(ctx->fimg, fglMode, arrays, count,
						(const uint16_t *)indices);
#else
		if (count > 24)
			fimgDrawElementsUShortIdx(ctx->fimg, fglMode, arrays,
					count, (const uint16_t *)indices);
		else
			fimgDrawElementsBufferedUShortIdx(ctx->fimg, fglMode,
				arrays, count, (const uint16_t *)indices);
#endif
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

/**
	Draw texture
*/

GL_API void GL_APIENTRY glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
	FGLContext *ctx = getContext();
	GLboolean arrayEnabled[4 + FGL_MAX_TEXTURE_UNITS];
	GLfloat vertices[3*4];
	GLfloat texcoords[2][2*4];

	// Save current state and prepare to drawing

	GLint viewportX = ctx->viewport.x;
	GLint viewportY = ctx->viewport.y;
	GLsizei viewportW = ctx->viewport.width;
	GLsizei viewportH = ctx->viewport.height;
	GLfloat zNear = ctx->viewport.zNear;
	GLfloat zFar = ctx->viewport.zFar;

	fimgSetDepthRange(ctx->fimg, 0.0f, 1.0f);
	fimgSetViewportParams(ctx->fimg, 0, 0, ctx->surface.width, ctx->surface.height);

	for (int i = 0; i < 4 + FGL_MAX_TEXTURE_UNITS; i++) {
		arrayEnabled[i] = ctx->array[i].enabled;
		fglDisableClientState(ctx, i);
	}

	/* TODO: Replace this with dedicated shader or conditional operation */
	FGLmatrix *matrix = &ctx->matrix.transformMatrix;
	matrix->identity();

	fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TRANSFORM, matrix->data);
	fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_LIGHTING, matrix->data);
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW] = 1;
	fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TEXTURE(0), matrix->data);
	ctx->matrix.dirty[FGL_MATRIX_TEXTURE(0)] = 1;
	fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TEXTURE(1), matrix->data);
	ctx->matrix.dirty[FGL_MATRIX_TEXTURE(1)] = 1;

	// fimgCompatLightingEnable(ctx->fimg, 0);
	/* End of TODO */

	float zD;

	if (z <= 0)
		zD = zNear;
	else if (z >= 1)
		zD = zFar;
	else
		zD = zNear + z*(zFar - zNear);

	float invWidth = 2.0f/ctx->surface.width;
	float invHeight = 2.0f/ctx->surface.height;
	vertices[ 0] = invWidth * x - 1;
	vertices[ 1] = invHeight * (y + height) - 1;
	vertices[ 2] = zD;
	vertices[ 3] = invWidth * (x + width) - 1;
	vertices[ 4] = vertices[ 1];
	vertices[ 5] = zD;
	vertices[ 6] = vertices[ 3];
	vertices[ 7] = invHeight * y - 1;
	vertices[ 8] = zD;
	vertices[ 9] = vertices[ 0];
	vertices[10] = vertices[ 7];
	vertices[11] = zD;

	// Proceed with drawing

	fimgArray arrays[4 + FGL_MAX_TEXTURE_UNITS];

	arrays[FGL_ARRAY_VERTEX].pointer	= vertices;
	arrays[FGL_ARRAY_VERTEX].stride		= 12;
	arrays[FGL_ARRAY_VERTEX].width		= 12;
	fimgSetAttribute(ctx->fimg, FGL_ARRAY_VERTEX, FGHI_ATTRIB_DT_FLOAT, 3);

	for (int i = FGL_ARRAY_NORMAL; i < FGL_ARRAY_TEXTURE; i++) {
		arrays[i].pointer	= &ctx->vertex[i];
		arrays[i].stride	= 0;
		arrays[i].width		= 16;
	}

	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++) {
		FGLTexture *tex = ctx->texture[i].getTexture();
		if (ctx->texture[i].enabled && tex->surface) {
			float invHeight = 1.0f/tex->height;
			float invWidth = 1.0f/tex->width;
			texcoords[i][0]	= invWidth*tex->cropRect[0];
			texcoords[i][1] = 1 - invHeight*(tex->cropRect[1]);
			texcoords[i][2] = invWidth*(tex->cropRect[0] + tex->cropRect[2]);
			texcoords[i][3] = texcoords[i][1];
			texcoords[i][4] = texcoords[i][2];
			texcoords[i][5] = 1 - invHeight*(tex->cropRect[1] + tex->cropRect[3]);
			texcoords[i][6] = texcoords[i][0];
			texcoords[i][7] = texcoords[i][5];
			arrays[FGL_ARRAY_TEXTURE(i)].stride = 8;
		} else {
			texcoords[i][0] = 0;
			texcoords[i][1] = 0;
			arrays[FGL_ARRAY_TEXTURE(i)].stride = 0;
		}
		arrays[FGL_ARRAY_TEXTURE(i)].pointer	= texcoords[i];
		arrays[FGL_ARRAY_TEXTURE(i)].width	= 8;
		fimgSetAttribute(ctx->fimg, FGL_ARRAY_TEXTURE(i), FGHI_ATTRIB_DT_FLOAT, 2);
	}

	fglSetupTextures(ctx);

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);

#ifndef FIMG_USE_VERTEX_BUFFER
	fimgDrawArrays(ctx->fimg, FGPE_TRIANGLE_FAN, arrays, 0, 4);
#else
	fimgDrawArraysBuffered(ctx->fimg, FGPE_TRIANGLE_FAN, arrays, 0, 4);
#endif
	// Restore previous state

	for (int i = 0; i < 4 + FGL_MAX_TEXTURE_UNITS; i++) {
		if (arrayEnabled[i])
			fglEnableClientState(ctx, i);
		else
			fglDisableClientState(ctx, i);
	}

	fimgSetDepthRange(ctx->fimg, zNear, zFar);
	fimgSetViewportParams(ctx->fimg, viewportX, viewportY, viewportW, viewportH);
}

GL_API void GL_APIENTRY glDrawTexsOES (GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
{
	glDrawTexfOES(x, y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexiOES (GLint x, GLint y, GLint z, GLint width, GLint height)
{
	glDrawTexfOES(x, y, z, width, height);
}

GL_API void GL_APIENTRY glDrawTexxOES (GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height)
{
	glDrawTexfOES(floatFromFixed(x), floatFromFixed(y), floatFromFixed(z),
		      floatFromFixed(width), floatFromFixed(height));
}

GL_API void GL_APIENTRY glDrawTexsvOES (const GLshort *coords)
{
	glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

GL_API void GL_APIENTRY glDrawTexivOES (const GLint *coords)
{
	glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

GL_API void GL_APIENTRY glDrawTexxvOES (const GLfixed *coords)
{
	glDrawTexfOES(floatFromFixed(coords[0]), floatFromFixed(coords[1]),
		      floatFromFixed(coords[2]), floatFromFixed(coords[3]),
		      floatFromFixed(coords[4]));
}

GL_API void GL_APIENTRY glDrawTexfvOES (const GLfloat *coords)
{
	glDrawTexfOES(coords[0], coords[1], coords[2], coords[3], coords[4]);
}

/**
	Transformations
*/

GL_API void GL_APIENTRY glDepthRangef (GLclampf zNear, GLclampf zFar)
{
	FGLContext *ctx = getContext();

	zNear	= clampFloat(zNear);
	zFar	= clampFloat(zFar);

	ctx->viewport.zNear = zNear;
	ctx->viewport.zFar = zFar;

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

	// Clamp the width
	if (width > ctx->surface.width)
		width = ctx->surface.width;

	// Clamp the height
	if (height > ctx->surface.height)
		height = ctx->surface.height;

	ctx->viewport.x = x;
	ctx->viewport.y = y;
	ctx->viewport.width = width;
	ctx->viewport.height = height;

	fimgSetViewportParams(ctx->fimg, x, y, width, height);
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

	fimgSetFaceCullFace(ctx->fimg, face);
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

	fimgSetFaceCullFront(ctx->fimg, cw);
}

/**
	Per-fragment operations
*/

static inline void fglSetScissor(FGLContext *ctx, GLint x, GLint y,
						GLsizei width, GLsizei height)
{
#if 0
	unsigned int xmin = clamp(x, ctx->viewport.x,
			ctx->viewport.x + ctx->viewport.width);
	unsigned int xmax = clamp(x + width, ctx->viewport.x,
			ctx->viewport.x + ctx->viewport.width);
	unsigned int ymin = clamp(y, ctx->viewport.y,
			ctx->viewport.y + ctx->viewport.height);
	unsigned int ymax = clamp(y + height, ctx->viewport.y,
			ctx->viewport.y + ctx->viewport.height);
#elif 0
	unsigned int xmin = x;
	unsigned int xmax = x + width;
	unsigned int ymin = y;
	unsigned int ymax = y + height;
#else
	unsigned int xmin = clamp(x, 0, ctx->surface.width);
	unsigned int xmax = clamp(x + width, 0, ctx->surface.width);
	unsigned int ymin = clamp(y, 0, ctx->surface.height);
	unsigned int ymax = clamp(y + height, 0, ctx->surface.height);
#endif

	fimgSetXClip(ctx->fimg, xmin, xmax);
	fimgSetYClip(ctx->fimg, 0, ctx->surface.height);
	fimgSetScissorParams(ctx->fimg, ctx->surface.width, 0, ymax, ymin);
}

GL_API void GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(width < 0 || height < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->perFragment.scissor.left	= x;
	ctx->perFragment.scissor.bottom	= y;
	ctx->perFragment.scissor.width	= width;
	ctx->perFragment.scissor.height	= height;

#if 0
	fimgSetScissorParams(ctx->fimg, x + width, x, y + height, y);
#else
	if (ctx->perFragment.scissor.enabled)
		fglSetScissor(ctx, x, y, width, height);
#endif
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
	fglAlphaFunc(func, ubyteFromClampf(clampFloat(ref)));
}

GL_API void GL_APIENTRY glAlphaFuncx (GLenum func, GLclampx ref)
{
	fglAlphaFunc(func, ubyteFromClampx(clampFixed(ref)));
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

	fimgSetFrontStencilOp(ctx->fimg, (fimgTestAction)fglFail,
			(fimgTestAction)fglZFail, (fimgTestAction)fglZPass);
	fimgSetBackStencilOp(ctx->fimg, (fimgTestAction)fglFail,
			(fimgTestAction)fglZFail, (fimgTestAction)fglZPass);
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

	if (fglColorConfigs[ctx->surface.format].alpha)
		fimgSetBlendFunc(ctx->fimg, fglSrc, fglSrc, fglDest, fglDest);
	else
		fimgSetBlendFuncNoAlpha(ctx->fimg, fglSrc, fglSrc, fglDest, fglDest);
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

#if 0
void fglSetClipper(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	FGLContext *ctx = getContext();

	fimgSetXClip(ctx->fimg, left, right);
	fimgSetYClip(ctx->fimg, top, bottom);
}
#endif

/**
	Enable/disable
*/

static inline void fglSet(GLenum cap, bool state)
{
	FGLContext *ctx = getContext();

	switch (cap) {
	case GL_TEXTURE_2D:
		ctx->texture[ctx->activeTexture].enabled = state;
		break;
	case GL_CULL_FACE:
		fimgSetFaceCullEnable(ctx->fimg, state);
		break;
	case GL_POLYGON_OFFSET_FILL:
		fimgEnableDepthOffset(ctx->fimg, state);
		break;
	case GL_SCISSOR_TEST:
		ctx->perFragment.scissor.enabled = state;
#if 1
		if (state) {
			fglSetScissor(ctx, ctx->perFragment.scissor.left,
					ctx->perFragment.scissor.bottom,
					ctx->perFragment.scissor.width,
					ctx->perFragment.scissor.height);
		} else {
			fglSetScissor(ctx, 0, 0, ctx->surface.width,
							ctx->surface.height);
		}
		fimgSetScissorEnable(ctx->fimg, state);
#else
		fimgSetScissorEnable(ctx->fimg, state);
#endif
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
	case GL_LIGHTING:
	case GL_LIGHT0:
	case GL_LIGHT1:
	case GL_LIGHT2:
	case GL_LIGHT3:
	case GL_LIGHT4:
	case GL_LIGHT5:
	case GL_LIGHT6:
	case GL_LIGHT7:
	case GL_NORMALIZE:
		break;
	default:
		LOGD("Unimplemented or unsupported enum %d in %s", cap, __func__);
		//setError(GL_INVALID_ENUM);
	}
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
	Flush/Finish
*/

GL_API void GL_APIENTRY glFlush (void)
{
	//FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFinish (void)
{
	FGLContext *ctx = getContext();

	fimgFinish(ctx->fimg);

	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; ++i)
		ctx->busyTexture[i] = 0;
}

/**
	Stubs
*/

GL_API void GL_APIENTRY glClipPlanef (GLenum plane, const GLfloat *equation)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glClipPlanex (GLenum plane, const GLfixed *equation)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glColorMask (GLboolean red, GLboolean green,
						GLboolean blue, GLboolean alpha)
{
	FGLContext *ctx = getContext();

	ctx->perFragment.mask.red = red;
	ctx->perFragment.mask.green = green;
	ctx->perFragment.mask.blue = blue;
	ctx->perFragment.mask.alpha = alpha;

	fimgSetColorBufWriteMask(ctx->fimg, red, green, blue, alpha);
}

GL_API void GL_APIENTRY glDepthMask (GLboolean flag)
{
	FGLContext *ctx = getContext();

	ctx->perFragment.mask.depth = flag;

	if (ctx->surface.depthFormat & 0xff)
		fimgSetZBufWriteMask(ctx->fimg, flag);
}

GL_API void GL_APIENTRY glFogf (GLenum pname, GLfloat param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFogfv (GLenum pname, const GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFogx (GLenum pname, GLfixed param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFogxv (GLenum pname, const GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetBufferParameteriv (GLenum target,
						GLenum pname, GLint *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetClipPlanef (GLenum pname, GLfloat eqn[4])
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetClipPlanex (GLenum pname, GLfixed eqn[4])
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetLightfv (GLenum light, GLenum pname,
							GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetMaterialfv (GLenum face, GLenum pname,
							GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glHint (GLenum target, GLenum mode)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightModelf (GLenum pname, GLfloat param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightModelfv (GLenum pname, const GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightModelx (GLenum pname, GLfixed param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightModelxv (GLenum pname, const GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightfv (GLenum light, GLenum pname,
							const GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightx (GLenum light, GLenum pname, GLfixed param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLightxv (GLenum light, GLenum pname,
							const GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLineWidth (GLfloat width)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glLineWidthx (GLfixed width)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glMaterialfv (GLenum face, GLenum pname,
							const GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glMaterialx (GLenum face, GLenum pname, GLfixed param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glMaterialxv (GLenum face, GLenum pname,
							const GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointParameterf (GLenum pname, GLfloat param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointParameterfv (GLenum pname, const GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointParameterx (GLenum pname, GLfixed param)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointParameterxv (GLenum pname, const GLfixed *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointSize (GLfloat size)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPointSizex (GLfixed size)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPolygonOffset (GLfloat factor, GLfloat units)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glPolygonOffsetx (GLfixed factor, GLfixed units)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glStencilMask (GLuint mask)
{
	FGLContext *ctx = getContext();

	ctx->perFragment.mask.stencil = mask & 0xff;

	if (ctx->surface.depthFormat >> 8) {
		fimgSetStencilBufWriteMask(ctx->fimg, 0, mask & 0xff);
		fimgSetStencilBufWriteMask(ctx->fimg, 1, mask & 0xff);
	}
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

	for(int i = 0; i < 4 + FGL_MAX_TEXTURE_UNITS; i++)
		fimgSetAttribute(ctx->fimg, i, FGHI_ATTRIB_DT_FLOAT,
						fglDefaultAttribSize[i]);

	return ctx;
}

void fglDestroyContext(FGLContext *ctx)
{
	fglBufferObjects.clean(ctx);
	fglCleanTextureObjects(ctx);
	fimgDestroyContext(ctx->fimg);
	delete ctx;
}
