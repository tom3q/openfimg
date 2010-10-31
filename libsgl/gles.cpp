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
#include <fcntl.h>
#include <cutils/log.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "state.h"
#include "types.h"
#include "fglobjectmanager.h"
#include "fimg/fimg.h"
#include "s3c_g2d.h"

//#define GLES_DEBUG
#define GLES_ERR_DEBUG

/**
	Client API information
*/

static char const * const gVendorString     = "GLES6410";
static char const * const gRendererString   = "S3C6410 FIMG-3DSE";
static char const * const gVersionString    = "OpenGL ES-CM 1.1";
static char const * const gExtensionsString =
#if 0
	"GL_OES_read_format "                   // TODO
	"GL_OES_compressed_paletted_texture "   // TODO
	"GL_OES_matrix_get "                    // TODO
	"GL_OES_query_matrix "                  // TODO
	"GL_OES_point_sprite "                  // TODO
	"GL_OES_EGL_image "                     // TODO
	"GL_OES_compressed_ETC1_RGB8_texture "  // TODO
	"GL_ARB_texture_compression "           // TODO
	"GL_ARB_texture_non_power_of_two "      // TODO
	"GL_ANDROID_user_clip_plane "           // TODO
	"GL_ANDROID_vertex_buffer_object "      // TODO
	"GL_ANDROID_generate_mipmap "           // TODO
	"GL_ARB_texture_env_combine "		// TODO
	"GL_ARB_texture_env_crossbar "		// TODO
	"GL_ARB_texture_env_dot3 "		// TODO
	"GL_ARB_texture_mirrored_repeat "	// TODO
	"GL_ARB_vertex_buffer_object "		// TODO
	"GL_ATI_texture_compression_atitc "	// TODO
	"GL_EXT_blend_equation_separate "	// TODO
	"GL_EXT_blend_func_separate "		// TODO
	"GL_EXT_blend_minmax "			// TODO
	"GL_EXT_blend_subtract "		// TODO
	"GL_EXT_stencil_wrap "			// TODO
	"GL_OES_fixed_point "			// TODO
	"GL_OES_matrix_palette "		// TODO
	"GL_OES_vertex_buffer_object "		// TODO
	"GL_QUALCOMM_vertex_buffer_object "	// TODO
	"GL_QUALCOMM_direct_texture"		// TODO
#endif
	"GL_OES_byte_coordinates "
	"GL_OES_fixed_point "
	"GL_OES_single_precision "
	"GL_OES_draw_texture "
	"GL_OES_point_size_array "
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
	int name;

	if(n <= 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	while(n--) {
		name = fglBufferObjects.get();
		if(name < 0) {
			setError(GL_OUT_OF_MEMORY);
			return;
		}
		fglBufferObjects[name] = NULL;
		LOGD("Allocated buffer %d", name);
		*buffers = name;
		buffers++;
	}
}

GL_API void GL_APIENTRY glDeleteBuffers (GLsizei n, const GLuint *buffers)
{
	unsigned name;

	if(n <= 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

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
		FGLContext *ctx = getContext();
		if(binding->isBound())
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

	if(binding->isBound())
		binding->unbind();

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

	if (data != NULL)
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

	if(ctx->arrayBuffer.isBound())
		pointer = ctx->arrayBuffer.get()->getAddress(pointer);

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

	if(ctx->arrayBuffer.isBound())
		pointer = ctx->arrayBuffer.get()->getAddress(pointer);

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

	if(ctx->arrayBuffer.isBound())
		pointer = ctx->arrayBuffer.get()->getAddress(pointer);

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

	if(ctx->arrayBuffer.isBound())
		pointer = ctx->arrayBuffer.get()->getAddress(pointer);

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

	if(ctx->arrayBuffer.isBound())
		pointer = ctx->arrayBuffer.get()->getAddress(pointer);

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
		idx = FGL_ARRAY_TEXTURE(ctx->activeTexture);
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
		idx = FGL_ARRAY_TEXTURE(ctx->activeTexture);
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
	if (ctx->matrix.dirty[FGL_MATRIX_MODELVIEW]
				|| ctx->matrix.dirty[FGL_MATRIX_PROJECTION])
	{
		FGLmatrix *matrix;
		/* Calculate and load transformation matrix */
		matrix = &ctx->matrix.transformMatrix;
		*matrix = ctx->matrix.stack[FGL_MATRIX_PROJECTION].top();
		matrix->multiply(ctx->matrix.stack[FGL_MATRIX_MODELVIEW].top());
		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TRANSFORM, matrix->data);
		/* Load lighting matrix */
		matrix = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top();
		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_LIGHTING, matrix->data);
		/* Mark transformation matrices as clean */
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_PROJECTION] = GL_FALSE;
		ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_FALSE;
	}
	/* Load texture coordinate matrices */
	int i = FGL_MAX_TEXTURE_UNITS;
	while(i--) {
		if(!ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)])
			continue;

		fimgLoadMatrix(ctx->fimg, FGFP_MATRIX_TEXTURE(i),
			ctx->matrix.stack[FGL_MATRIX_TEXTURE(i)].top().data);
		ctx->matrix.dirty[FGL_MATRIX_TEXTURE(i)] = GL_FALSE;
	}
}

static inline void fglSetupTextures(FGLContext *ctx)
{
	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++) {
		bool enabled = ctx->texture[i].enabled;
		FGLTexture *obj = ctx->texture[i].getTexture();

		if(enabled && obj->surface.isValid() && obj->isComplete()) {
			/* Texture is ready */
			fimgCompatSetupTexture(ctx->fimg, obj->fimg, i);
			fimgCompatSetTextureEnable(ctx->fimg, i, 1);
		} else {
			/* Texture is not ready */
			fimgCompatSetTextureEnable(ctx->fimg, i, 0);
		}
	}
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
		if (count % 3)
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
	fimgDrawArraysBuffered(ctx->fimg, fglMode, arrays, first, count);
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
		if (count % 3)
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
		fimgDrawElementsUByteIdx(ctx->fimg, fglMode, arrays, count,
						(const uint8_t *)indices);
#else
		fimgDrawElementsBufferedUByteIdx(ctx->fimg, fglMode, arrays, count,
						(const uint8_t *)indices);
#endif
		break;
	case GL_UNSIGNED_SHORT:
#ifndef FIMG_USE_VERTEX_BUFFER
		fimgDrawElementsUShortIdx(ctx->fimg, fglMode, arrays, count,
						(const uint16_t *)indices);
#else
		fimgDrawElementsBufferedUShortIdx(ctx->fimg, fglMode, arrays, count,
						(const uint16_t *)indices);
#endif
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
	glRotatef(floatFromFixed(angle), floatFromFixed(x),
		  floatFromFixed(y), floatFromFixed(z));
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

GL_API void GL_APIENTRY glFrustumf (GLfloat left, GLfloat right,
		GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
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

GL_API void GL_APIENTRY glFrustumx (GLfixed left, GLfixed right,
		GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
	glFrustumf(floatFromFixed(left), floatFromFixed(right),
		   floatFromFixed(bottom), floatFromFixed(top),
		   floatFromFixed(zNear), floatFromFixed(zFar));
}

GL_API void GL_APIENTRY glOrthof (GLfloat left, GLfloat right,
		GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
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
	glOrthof(floatFromFixed(left), floatFromFixed(right),
		 floatFromFixed(bottom), floatFromFixed(top),
		 floatFromFixed(zNear), floatFromFixed(zFar));
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

GL_API void GL_APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height)
{
	if(width < 0 || height < 0) {
		setError(GL_INVALID_VALUE);
		return;
	}

	FGLContext *ctx = getContext();

	ctx->perFragment.scissor.left	= x;
	ctx->perFragment.scissor.top	= y;
	ctx->perFragment.scissor.width	= width;
	ctx->perFragment.scissor.height	= height;

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

void fglSetClipper(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	FGLContext *ctx = getContext();

	fimgSetXClip(ctx->fimg, left, right);
	fimgSetYClip(ctx->fimg, top, bottom);
}

/**
	Texturing
*/

FGLObjectManager<FGLTexture, FGL_MAX_TEXTURE_OBJECTS> fglTextureObjects;

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
		obj = new FGLTextureObject(texture);
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

static int fglGetFormatInfo(GLenum format, GLenum type, unsigned *bpp, bool *conv)
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

static void fglGenerateMipmapsSW(FGLTexture *obj)
{
	int level = 0;

	int w = obj->surface.width;
	int h = obj->surface.height;
	if ((w&h) == 1)
		return;

	w = (w>>1) ? : 1;
	h = (h>>1) ? : 1;

	void *curLevel = obj->surface.vaddr;
	void *nextLevel = (uint8_t *)obj->surface.vaddr
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
		nextLevel = (uint8_t *)obj->surface.vaddr
				+ fimgGetTexMipmapOffset(obj->fimg, level + 1);
	}
}

static int fglGenerateMipmapsG2D(FGLTexture *obj, unsigned int format)
{
	int fd;
	struct s3c_g2d_req req;

	// Setup source image (level 0 image)
	req.src.base	= obj->surface.paddr;
	req.src.offs	= 0;
	req.src.w	= obj->surface.width;
	req.src.h	= obj->surface.height;
	req.src.l	= 0;
	req.src.t	= 0;
	req.src.r	= obj->surface.width - 1;
	req.src.b	= obj->surface.height - 1;
	req.src.fmt	= format;

	// Setup destination image template for generated mipmaps
	req.dst.base	= obj->surface.paddr;
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

	unsigned width = obj->surface.width;
	unsigned height = obj->surface.height;

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

static void fglLoadTextureDirect(FGLTexture *obj, unsigned level,
						const GLvoid *pixels)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->surface.width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->surface.height >> level;
	if (!height)
		height = 1;

	size_t size = width*height*obj->bpp;

	memcpy((uint8_t *)obj->surface.vaddr + offset, pixels, size);
}

static void fglLoadTexture(FGLTexture *obj, unsigned level,
		    const GLvoid *pixels, unsigned alignment)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->surface.width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->surface.height >> level;
	if (!height)
		height = 1;

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

static void fglConvertTexture(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->surface.width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->surface.height >> level;
	if (!height)
		height = 1;

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

		mipmapW = obj->surface.width >> level;
		if (!mipmapW)
			mipmapW = 1;

		mipmapH = obj->surface.height >> level;
		if (!mipmapH)
			mipmapH = 1;

		if (!obj->surface.isValid()) {
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

			fglFlushPmemSurface(&obj->surface);
		}

		return;
	}

	// level == 0

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

		obj->levels = (1 << 0);
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

		fglFlushPmemSurface(&obj->surface);
	}
}

static void fglLoadTexturePartial(FGLTexture *obj, unsigned level,
			const GLvoid *pixels, unsigned alignment,
			unsigned x, unsigned y, unsigned w, unsigned h)
{
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->surface.width >> level;
	if (!width)
		width = 1;
	unsigned height = obj->surface.height >> level;
	if (!height)
		height = 1;

	size_t line = w*obj->bpp;
	size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
	size_t dstStride = width*obj->bpp;
	size_t xoffset = x*obj->bpp;
	size_t yoffset = (height - y - h)*dstStride;
	const uint8_t *src8 = (const uint8_t *)pixels;
	uint8_t *dst8 = (uint8_t *)obj->surface.vaddr
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
	unsigned offset = fimgGetTexMipmapOffset(obj->fimg, level);

	unsigned width = obj->surface.width >> level;
	if (!width)
		width = 1;

	unsigned height = obj->surface.height >> level;
	if (!height)
		height = 1;

	switch (obj->format) {
	case GL_RGB: {
		size_t line = 3*w;
		size_t srcStride = (line + alignment - 1) & ~(alignment - 1);
		size_t dstStride = 4*width;
		size_t xOffset = 4*x;
		size_t yOffset = (height - y - h)*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint32_t *dst32 = (uint32_t *)((uint8_t *)obj->surface.vaddr
						+ offset + yOffset + xOffset);
		uint8_t r, g, b;
		do {
			unsigned x = w;
			do {
				r = *(src8++);
				g = *(src8++);
				b = *(src8++);
				*(dst32++) = fglPackRGBA8888(r, g, b, 255);
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
		size_t yOffset = (height - y - h)*dstStride;
		size_t srcPad = srcStride - line;
		size_t dstPad = width - w;
		const uint8_t *src8 = (const uint8_t *)pixels;
		uint16_t *dst16 = (uint16_t *)((uint8_t *)obj->surface.vaddr
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
	FGLContext *ctx = getContext();
	FGLTexture *obj =
		ctx->texture[ctx->activeTexture].getTexture();

	if (!obj->surface.isValid()) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (level > obj->maxLevel) {
		setError(GL_INVALID_VALUE);
		return;
	}

	GLint mipmapW, mipmapH;

	mipmapW = obj->surface.width >> level;
	if (!mipmapW)
		mipmapW = 1;

	mipmapH = obj->surface.height >> level;
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

	if (obj->convert)
		fglConvertTexturePartial(obj, level, pixels,
			ctx->unpackAlignment, xoffset, yoffset, width, height);
	else
		fglLoadTexturePartial(obj, level, pixels,
			ctx->unpackAlignment, xoffset, yoffset, width, height);
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

GL_API void GL_APIENTRY glPixelStorei (GLenum pname, GLint param)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_UNPACK_ALIGNMENT:
		switch (param) {
		case 1:
		case 2:
		case 4:
		case 8:
			ctx->unpackAlignment = param;
			break;
		default:
			setError(GL_INVALID_VALUE);
		}
		break;
	case GL_PACK_ALIGNMENT:
		switch (param) {
		case 1:
		case 2:
		case 4:
		case 8:
			ctx->packAlignment = param;
			break;
		default:
			setError(GL_INVALID_VALUE);
		}
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

/**
	Draw texture
*/

static void DUMP_VERTICES(const GLfloat *v, GLint comp, GLint num)
{
	do {
		fprintf(stderr, "(  ");
		for (int i = 0; i < comp; i++)
			fprintf(stderr, "%f  ", *(v++));
		fprintf(stderr, ")\n");
	} while(--num);
}

GL_API void GL_APIENTRY glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
{
	FGLContext *ctx = getContext();
	GLboolean arrayEnabled[4 + FGL_MAX_TEXTURE_UNITS];
	GLfloat vertices[3*4];
	GLfloat texcoords[2][2*4];

	// Save current state and prepare to drawing

	GLfloat viewportX = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_X);
	GLfloat viewportY = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_Y);
	GLfloat viewportW = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_W);
	GLfloat viewportH = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_H);
	GLfloat zNear = fimgGetRasterizerStateF(ctx->fimg, FIMG_DEPTH_RANGE_NEAR);
	GLfloat zFar = fimgGetRasterizerStateF(ctx->fimg, FIMG_DEPTH_RANGE_FAR);

	fimgSetDepthRange(ctx->fimg, -1.0f, 1.0f);
	//fimgSetViewportParams(ctx->fimg, -1.0f, -1.0f, 2.0f, 2.0f);

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

	GLfloat oX = viewportX + viewportW / 2;
	GLfloat oY = viewportY + viewportH / 2;
	GLfloat invViewportW = 2.0f / viewportW;
	GLfloat invViewportH = 2.0f / viewportH;

	// bottom-left
	vertices[0] = (x - oX)*invViewportW;
	vertices[1] = (y - oY)*invViewportH;
	vertices[2] = zD;
	// bottom-right
	vertices[3] = (x + width - oX)*invViewportW;
	vertices[4] = vertices[1];
	vertices[5] = zD;
	// top-left
	vertices[6] = vertices[0];
	vertices[7] = (y + height - oY)*invViewportH;
	vertices[8] = zD;
	// top-right
	vertices[9] = vertices[3];
	vertices[10] = vertices[7];
	vertices[11] = zD;

	/*
	vertices[0] = x;
	vertices[1] = y;
	vertices[2] = zD;
	vertices[3] = x + width;
	vertices[4] = y;
	vertices[5] = zD;
	vertices[6] = x;
	vertices[7] = y + height;
	vertices[8] = zD;
	vertices[9] = x + width;
	vertices[10] = y + height;
	vertices[11] = zD;
	*/

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
		FGLTexture *obj = ctx->texture[i].getTexture();
		if (obj->surface.isValid()) {
			texcoords[i][0]	= obj->cropRect[0];
			texcoords[i][1] = obj->cropRect[1] + obj->cropRect[3];
			texcoords[i][2] = obj->cropRect[0] + obj->cropRect[2];
			texcoords[i][3] = obj->cropRect[1] + obj->cropRect[3];
			texcoords[i][4] = obj->cropRect[0];
			texcoords[i][5] = obj->cropRect[1];
			texcoords[i][6] = obj->cropRect[0] + obj->cropRect[2];
			texcoords[i][7] = obj->cropRect[1];
			arrays[FGL_ARRAY_TEXTURE(i)].stride = 8;
			fimgSetTexCoordSys(obj->fimg, FGTU_TSTA_TEX_COOR_NON_PARAM);
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
	fimgDrawArrays(ctx->fimg, FGPE_TRIANGLE_STRIP, arrays, 0, 4);
#else
	fimgDrawArraysBuffered(ctx->fimg, FGPE_TRIANGLE_STRIP, arrays, 0, 4);
#endif
	// Restore previous state

	for (int i = 0; i < 4 + FGL_MAX_TEXTURE_UNITS; i++) {
		if (arrayEnabled[i])
			fglEnableClientState(ctx, i);
		else
			fglDisableClientState(ctx, i);
	}

	for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++) {
		FGLTexture *obj = ctx->texture[i].getTexture();
		if (obj->surface.isValid())
			fimgSetTexCoordSys(obj->fimg, FGTU_TSTA_TEX_COOR_PARAM);
	}

	fimgSetDepthRange(ctx->fimg, zNear, zFar);
	//fimgSetViewportParams(ctx->fimg, viewportX, viewportY, viewportW, viewportH);
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
	fglSet(cap, true);
}

GL_API void GL_APIENTRY glDisable (GLenum cap)
{
	fglSet(cap, false);
}

GL_API GLboolean GL_APIENTRY glIsEnabled (GLenum cap)
{
	FGLContext *ctx = getContext();

	switch (cap) {
	case GL_CULL_FACE:
		return fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_EN);
		break;
	case GL_POLYGON_OFFSET_FILL:
		return fimgGetRasterizerState(ctx->fimg, FIMG_DEPTH_OFFSET_EN);
		break;
	case GL_SCISSOR_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_SCISSOR_TEST);
		break;
	case GL_STENCIL_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_STENCIL_TEST);
		break;
	case GL_DEPTH_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_TEST);
		break;
	case GL_BLEND:
		return fimgGetFragmentState(ctx->fimg, FIMG_BLEND);
		break;
	case GL_DITHER:
		return fimgGetFragmentState(ctx->fimg, FIMG_DITHER);
		break;
#if 0
	case GL_ALPHA_TEST_EXP:
		break;
#endif
	case GL_COLOR_LOGIC_OP:
		return fimgGetFragmentState(ctx->fimg, FIMG_COLOR_LOGIC_OP);
		break;
#if 0
	case GL_POINT_SPRITE_OES_EXP:
		break;
#endif
	default:
		setError(GL_INVALID_ENUM);
		return GL_FALSE;
	}
}

/**
	Reading pixels
*/

GL_API void GL_APIENTRY glReadPixels (GLint x, GLint y,
				GLsizei width, GLsizei height, GLenum format,
				GLenum type, GLvoid *pixels)
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

static void *fillSingle16(void *buf, uint16_t val, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;

	do {
		*(buf16++) = val;
	} while (--cnt);

	return buf16;
}

static void *fillSingle16masked(void *buf, uint16_t val,
						uint16_t mask, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint16_t tmp;

	do {
		tmp = *buf16 & mask;
		tmp |= val;
		*(buf16++) = tmp;
	} while (--cnt);

	return buf16;
}

static void *fillSingle32(void *buf, uint32_t val, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;

	do {
		*(buf32++) = val;
	} while (--cnt);

	return buf32;
}

static void *fillBurst32(void *buf, uint32_t val, size_t cnt)
{
	asm volatile (
		"mov r0, %1\n\t"
		"mov r1, %1\n\t"
		"mov r2, %1\n\t"
		"mov r3, %1\n\t"
		"1:\n\t"
		"stmia %0!, {r0-r3}\n\t"
		"subs %2, %2, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(cnt)
		: "r0", "r1", "r2", "r3"
	);

	return buf;
}

static void *fillSingle32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t tmp;

	do {
		tmp = *buf32 & mask;
		tmp |= val;
		*(buf32++) = tmp;
	} while (--cnt);

	return buf32;
}

static void *fillBurst32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	asm volatile (
		"mov r0, %1\n\t"
		"mov r1, %1\n\t"
		"mov r2, %1\n\t"
		"mov r3, %1\n\t"
		"1:\n\t"
		"ldmia %0, {r0-r3}\n\t"
		"and r0, r0, %2\n\t"
		"and r1, r1, %2\n\t"
		"and r2, r2, %2\n\t"
		"and r3, r3, %2\n\t"
		"orr r0, r0, %1\n\t"
		"orr r1, r1, %1\n\t"
		"orr r2, r2, %1\n\t"
		"orr r3, r3, %1\n\t"
		"stmia %0!, {r0-r3}\n\t"
		"subs %3, %3, $1\n\t"
		"bne 1b\n\t"
		: "+r"(buf)
		: "r"(val), "r"(mask), "r"(cnt)
		: "r0", "r1", "r2", "r3"
	);

	return buf;
}

static void fill32(void *buf, uint32_t val, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t align = (intptr_t)buf32 % 16;

	if (align) {
		align = 16 - align;
		align /= 4;
		if (align > cnt) {
			fillSingle32(buf32, val, cnt);
			return;
		}
		buf32 = (uint32_t *)fillSingle32(buf32, val, align);
		cnt -= align;
	}

	if(cnt / 4)
		buf32 = (uint32_t *)fillBurst32(buf32, val, cnt / 4);

	if(cnt % 4)
		fillSingle32(buf32, val, cnt % 4);
}

static void fill32masked(void *buf, uint32_t val, uint32_t mask, size_t cnt)
{
	uint32_t *buf32 = (uint32_t *)buf;
	uint32_t align = (intptr_t)buf32 % 16;

	val &= mask;
	mask = ~mask;

	if (align) {
		align = 16 - align;
		align /= 4;
		if (align > cnt) {
			fillSingle32masked(buf32, val, mask, cnt);
			return;
		}
		buf32 = (uint32_t *)fillSingle32masked(buf32, val, mask, align);
		cnt -= align;
	}

	if(cnt / 4)
		buf32 = (uint32_t *)fillBurst32masked(buf32, val, mask, cnt / 4);

	if(cnt % 4)
		fillSingle32masked(buf32, val, mask, cnt % 4);
}

static void fill16(void *buf, uint16_t val, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint32_t align = (intptr_t)buf16 % 16;

	if (align) {
		align = 16 - align;
		align /= 2;
		if (align > cnt) {
			fillSingle16(buf16, val, cnt);
			return;
		}
		buf16 = (uint16_t *)fillSingle16(buf16, val, align);
		cnt -= align;
	}

	if(cnt / 8)
		buf16 = (uint16_t *)fillBurst32(buf16, (val << 16) | val, cnt / 8);

	if(cnt % 8)
		fillSingle16(buf16, val, cnt % 8);
}

static void fill16masked(void *buf, uint16_t val, uint16_t mask, size_t cnt)
{
	uint16_t *buf16 = (uint16_t *)buf;
	uint32_t align = (intptr_t)buf16 % 16;

	val &= mask;
	mask = ~mask;

	if (align) {
		align = 16 - align;
		align /= 2;
		if (align > cnt) {
			fillSingle16masked(buf16, val, mask, cnt);
			return;
		}
		buf16 = (uint16_t *)fillSingle16masked(buf16, val, mask, align);
		cnt -= align;
	}

	if(cnt / 8)
		buf16 = (uint16_t *)fillBurst32masked(buf16, (val << 16) | val,
						(mask << 16) | mask, cnt / 8);

	if(cnt % 8)
		fillSingle16masked(buf16, val, mask, cnt % 8);
}

static inline uint32_t getFillColor(FGLContext *ctx,
						uint32_t *mask, bool *is32bpp)
{
	uint8_t r8, g8, b8, a8;
	uint32_t val = 0;
	uint32_t mval = 0xffffffff;

	r8 = ubyteFromClampf(ctx->clear.red);
	g8 = ubyteFromClampf(ctx->clear.green);
	b8 = ubyteFromClampf(ctx->clear.blue);
	a8 = ubyteFromClampf(ctx->clear.alpha);

	switch (ctx->surface.draw.format) {
	case FGPF_COLOR_MODE_555:
		val |= ((r8 & 0xf8) << 7);
		if (ctx->perFragment.mask.red)
			mval &= ~(0x1f << 10);
		val |= ((g8 & 0xf8) << 2);
		if (ctx->perFragment.mask.green)
			mval &= ~(0x1f << 5);
		val |= (b8 >> 3);
		if (ctx->perFragment.mask.blue)
			mval &= ~0x1f;
		*mask = mval;
		*is32bpp = false;
		return val;
	case FGPF_COLOR_MODE_565:
		val |= ((r8 & 0xf8) << 8);
		if (ctx->perFragment.mask.red)
			mval &= ~(0x1f << 11);
		val |= ((g8 & 0xfc) << 2);
		if (ctx->perFragment.mask.green)
			mval &= ~(0x3f << 5);
		val |= (b8 >> 3);
		if (ctx->perFragment.mask.blue)
			mval &= ~0x1f;
		*mask = mval;
		*is32bpp = false;
		return val;
	case FGPF_COLOR_MODE_4444:
		val |= ((a8 & 0xf0) << 8);
		if (ctx->perFragment.mask.alpha)
			mval &= ~(0x0f << 12);
		val |= ((r8 & 0xf0) << 4);
		if (ctx->perFragment.mask.red)
			mval &= ~(0x0f << 8);
		val |= (g8 & 0xf0);
		if (ctx->perFragment.mask.green)
			mval &= ~(0x0f << 4);
		val |= (b8 >> 4);
		if (ctx->perFragment.mask.blue)
			mval &= ~0x0f;
		*mask = mval;
		*is32bpp = false;
		return val;
	case FGPF_COLOR_MODE_1555:
		val |= ((a8 & 0x80) << 15);
		if (ctx->perFragment.mask.alpha)
			mval &= ~(0x01 << 15);
		val |= ((r8 & 0xf8) << 7);
		if (ctx->perFragment.mask.red)
			mval &= ~(0x1f << 10);
		val |= ((g8 & 0xf8) << 2);
		if (ctx->perFragment.mask.green)
			mval &= ~(0x1f << 5);
		val |= (b8 >> 3);
		if (ctx->perFragment.mask.blue)
			mval &= ~0x1f;
		*mask = mval;
		*is32bpp = false;
		return val;
	case FGPF_COLOR_MODE_0888:
		val |= 0xff << 24;
		if (ctx->perFragment.mask.alpha)
			mval &= ~(0xff << 24);
		val |= r8 << 16;
		if (ctx->perFragment.mask.red)
			mval &= ~(0xff << 16);
		val |= g8 << 8;
		if (ctx->perFragment.mask.green)
			mval &= ~(0xff << 8);
		val |= b8;
		if (ctx->perFragment.mask.blue)
			mval &= ~0xff;
		*mask = mval;
		*is32bpp = true;
		return val;
	case FGPF_COLOR_MODE_8888:
		val |= a8 << 24;
		if (ctx->perFragment.mask.alpha)
			mval &= ~(0xff << 24);
		val |= r8 << 16;
		if (ctx->perFragment.mask.red)
			mval &= ~(0xff << 16);
		val |= g8 << 8;
		if (ctx->perFragment.mask.green)
			mval &= ~(0xff << 8);
		val |= b8;
		if (ctx->perFragment.mask.blue)
			mval &= ~0xff;
		*mask = mval;
		*is32bpp = true;
		return val;
	default:
		return 0;
	}
}

static inline uint32_t getFillDepth(FGLContext *ctx,
					uint32_t *mask, GLbitfield mode)
{
	uint32_t mval = 0xffffffff;
	uint32_t val = 0;

	if (mode & GL_DEPTH_BUFFER_BIT && ctx->perFragment.mask.depth) {
		mval &= ~0x00ffffff;
		val |= (uint32_t)(ctx->clear.depth * 0x00ffffff);
	}

	if (mode & GL_STENCIL_BUFFER_BIT) {
		mval &= ~ctx->perFragment.mask.stencil << 24;
		val |= ctx->clear.stencil << 24;
	}

	*mask = mval;
	return val;
}

static void fglClear(FGLContext *ctx, GLbitfield mode)
{
	FGLSurface *draw = &ctx->surface.draw;
	uint32_t stride = draw->width;
	bool lineByLine = false;
	int32_t l, t, w, h;

	l = max(ctx->perFragment.scissor.left, 0);
	t = max(ctx->perFragment.scissor.top, 0);
	w = min(ctx->perFragment.scissor.width, draw->width);
	h = min(ctx->perFragment.scissor.height, draw->height);

	lineByLine |= l > 0;
	lineByLine |= w < draw->width;
	lineByLine &= ctx->perFragment.scissor.enabled == true;

	if (mode & GL_COLOR_BUFFER_BIT) {
		bool is32bpp = false;
		uint32_t mask = 0;
		uint32_t color = getFillColor(ctx, &mask, &is32bpp);

		if (lineByLine) {
			if (!is32bpp) {
				uint16_t *buf16 = (uint16_t *)draw->vaddr;
				buf16 += t * stride;
				if (mask & 0xffff) {
					while (h--) {
						uint16_t *line = buf16 + l;
						fill16masked(line, color, ~mask, w);
						buf16 += stride;
					}
				} else {
					while (h--) {
						uint16_t *line = buf16 + l;
						fill16(line, color, w);
						buf16 += stride;
					}
				}
			} else {
				uint32_t *buf32 = (uint32_t *)draw->vaddr;
				buf32 += t * stride;
				if (mask) {
					while (h--) {
						uint32_t *line = buf32 + l;
						fill32masked(line, color, ~mask, w);
						buf32 += stride;
					}
				} else {
					while (h--) {
						uint32_t *line = buf32 + l;
						fill32(line, color, w);
						buf32 += stride;
					}
				}
			}
		} else {
			if (!is32bpp) {
				uint16_t *buf16 = (uint16_t *)draw->vaddr;
				buf16 += t * stride;
				if (mask & 0xffff)
					fill16masked(buf16, color, ~mask, w*h);
				else
					fill16(buf16, color, w*h);
			} else {
				uint32_t *buf32 = (uint32_t *)draw->vaddr;
				buf32 += t * stride;
				if (mask)
					fill32masked(buf32, color, ~mask, w*h);
				else
					fill32(buf32, color, w*h);
			}
		}

		fglFlushPmemSurface(draw);
	}

	if (mode & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
		FGLSurface *depth = &ctx->surface.depth;
		if (!depth->format)
			return;

		uint32_t mask;
		uint32_t val = getFillDepth(ctx, &mask, mode);

		if (lineByLine) {
			uint32_t *buf32 = (uint32_t *)depth->vaddr;
			buf32 += t * stride;
			if (mask) {
				while (h--) {
					uint32_t *line = buf32 + l;
					fill32masked(line, val, ~mask, w);
					buf32 += stride;
				}
			} else {
				while (h--) {
					uint32_t *line = buf32 + l;
					fill32(line, val, w);
					buf32 += stride;
				}
			}
		} else {
			uint32_t *buf32 = (uint32_t *)depth->vaddr;
			buf32 += t * stride;
			if (mask)
				fill32masked(buf32, val, ~mask, w*h);
			else
				fill32(buf32, val, w*h);
		}

		fglFlushPmemSurface(depth);
	}
}

#define FGL_CLEAR_MASK \
	(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)

//#define FGL_HARDWARE_CLEAR

GL_API void GL_APIENTRY glClear (GLbitfield mask)
{
	FGLContext *ctx = getContext();

	if ((mask & FGL_CLEAR_MASK) == 0)
		return;

#ifdef FGL_HARDWARE_CLEAR
	uint32_t mode = 0;

	if (mask & GL_COLOR_BUFFER_BIT)
		mode |= FGFP_CLEAR_COLOR;

	if (mask & GL_DEPTH_BUFFER_BIT)
		mode |= FGFP_CLEAR_DEPTH;

	if (mask & GL_STENCIL_BUFFER_BIT)
		mode |= FGFP_CLEAR_STENCIL;

	fimgClear(ctx->fimg, mode);
#else
	fglClear(ctx, mask);
#endif
}

GL_API void GL_APIENTRY glClearColor (GLclampf red, GLclampf green,
						GLclampf blue, GLclampf alpha)
{
	FGLContext *ctx = getContext();

	ctx->clear.red = clampFloat(red);
	ctx->clear.green = clampFloat(green);
	ctx->clear.blue = clampFloat(blue);
	ctx->clear.alpha = clampFloat(alpha);

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearColor(ctx->fimg, clampFloat(red), clampFloat(green),
				clampFloat(blue), clampFloat(alpha));
#endif
}

GL_API void GL_APIENTRY glClearColorx (GLclampx red, GLclampx green,
						GLclampx blue, GLclampx alpha)
{
	glClearColor(floatFromFixed(red), floatFromFixed(green),
				floatFromFixed(blue), floatFromFixed(alpha));
}

GL_API void GL_APIENTRY glClearDepthf (GLclampf depth)
{
	FGLContext *ctx = getContext();

	ctx->clear.depth = clampFloat(depth);

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearDepth(ctx->fimg, clampFloat(depth));
#endif
}

GL_API void GL_APIENTRY glClearDepthx (GLclampx depth)
{
	glClearDepthf(floatFromFixed(depth));
}

GL_API void GL_APIENTRY glClearStencil (GLint s)
{
	FGLContext *ctx = getContext();

	ctx->clear.stencil = s;

#ifdef FGL_HARDWARE_CLEAR
	fimgSetClearStencil(ctx->fimg, s);
#endif
}

/**
	Flush/Finish
*/

GL_API void GL_APIENTRY glFlush (void)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glFinish (void)
{
	FGLContext *ctx = getContext();

	fimgFinish(ctx->fimg);
}

/**
	glGet*
*/

GL_API const GLubyte * GL_APIENTRY glGetString (GLenum name)
{
	switch (name) {
	case GL_VENDOR:
		return (const GLubyte *)gVendorString;
	case GL_RENDERER:
		return (const GLubyte *)gRendererString;
	case GL_VERSION:
		return (const GLubyte *)gVersionString;
	case GL_EXTENSIONS:
		return (const GLubyte *)gExtensionsString;
	default:
		setError(GL_INVALID_ENUM);
		return NULL;
	}
}

GL_API void GL_APIENTRY glGetBooleanv (GLenum pname, GLboolean *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			params[0] = !!ctx->arrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			params[0] = !!ctx->elementArrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_VIEWPORT:
		params[0] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_X) != 0;
		params[1] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_Y) != 0;
		params[2] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_W) != 0;
		params[3] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_H) != 0;
		break;
	case GL_DEPTH_RANGE:
		params[0] = fimgGetPrimitiveStateF(ctx->fimg,
						   FIMG_DEPTH_RANGE_NEAR) != 0;
		params[1] = fimgGetPrimitiveStateF(ctx->fimg,
						   FIMG_DEPTH_RANGE_FAR) != 0;
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
							FIMG_POINT_SIZE) != 0;
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
							FIMG_LINE_WIDTH) != 0;
		break;
	case GL_CULL_FACE_MODE:
		switch (fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_MODE)) {
		case FGRA_BFCULL_FACE_FRONT:
			params[0] = !!GL_FRONT;
			break;
		case FGRA_BFCULL_FACE_BACK:
			params[0] = !!GL_BACK;
			break;
		case FGRA_BFCULL_FACE_BOTH:
			params[0] = !!GL_FRONT_AND_BACK;
			break;
		}
		break;
	case GL_FRONT_FACE:
		if (fimgGetRasterizerState(ctx->fimg, FIMG_FRONT_FACE))
			params[0] = !!GL_CW;
		else
			params[0] = !!GL_CCW;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR) != 0;
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS) != 0;
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			params[0] = !!b->getName();
		else
			params[0] = 0;
		break; }
	case GL_ACTIVE_TEXTURE:
		params[0] = !!ctx->activeTexture;
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_DEPTH_WRITEMASK:
		params[0] = ctx->perFragment.mask.depth;
		break;
	case GL_STENCIL_WRITEMASK :
		params[0] = !!ctx->perFragment.mask.stencil;
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = ctx->clear.red != 0;
		params[1] = ctx->clear.green != 0;
		params[2] = ctx->clear.blue != 0;
		params[3] = ctx->clear.alpha != 0;
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = ctx->clear.depth != 0;
		break;
	case GL_STENCIL_CLEAR_VALUE:
		params[0] = !!ctx->clear.stencil;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left != 0;
		params[1] = ctx->perFragment.scissor.top != 0;
		params[2] = ctx->perFragment.scissor.width != 0;
		params[3] = ctx->perFragment.scissor.height != 0;
		break;
	case GL_STENCIL_FUNC:
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_FUNC)) {
		case FGPF_STENCIL_MODE_NEVER:
			params[0] = !!GL_NEVER;
			break;
		case FGPF_STENCIL_MODE_ALWAYS:
			params[0] = !!GL_ALWAYS;
			break;
		case FGPF_STENCIL_MODE_GREATER:
			params[0] = !!GL_GREATER;
			break;
		case FGPF_STENCIL_MODE_GEQUAL:
			params[0] = !!GL_GEQUAL;
			break;
		case FGPF_STENCIL_MODE_EQUAL:
			params[0] = !!GL_EQUAL;
			break;
		case FGPF_STENCIL_MODE_LESS:
			params[0] = !!GL_LESS;
			break;
		case FGPF_STENCIL_MODE_LEQUAL:
			params[0] = !!GL_LEQUAL;
			break;
		case FGPF_STENCIL_MODE_NOTEQUAL:
			params[0] = !!GL_NOTEQUAL;
			break;
		}
		break;
	case GL_STENCIL_VALUE_MASK:
		params[0] = !!fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_MASK);
		break;
	case GL_STENCIL_REF :
		params[0] = !!fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_REF);
		break;
	case GL_STENCIL_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_SFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = !!GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = !!GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = !!GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = !!GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = !!GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = !!GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = !!GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = !!GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = !!GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = !!GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = !!GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = !!GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPPASS)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = !!GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = !!GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = !!GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = !!GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = !!GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = !!GL_INVERT;
			break;
		}
		break;
#if 0
	case GL_STENCIL_BACK_FUNC :
		params[0] = (float)ctx->stencil_test_data.back_face_func;
		break;
	case GL_STENCIL_BACK_VALUE_MASK :
		params[0] = (float)ctx->stencil_test_data.back_face_mask;
		break;
	case GL_STENCIL_BACK_REF :
		params[0] = (float)ctx->stencil_test_data.back_face_ref;
		break;
	case GL_STENCIL_BACK_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_fail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_zfail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
		params[0] = (float)ctx->stencil_test_data.back_face_zpass;
		break;
#endif
	case GL_DEPTH_FUNC:
		switch (fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_FUNC)) {
		case FGPF_TEST_MODE_NEVER:
			params[0] = !!GL_NEVER;
			break;
		case FGPF_TEST_MODE_ALWAYS:
			params[0] = !!GL_ALWAYS;
			break;
		case FGPF_TEST_MODE_GREATER:
			params[0] = !!GL_GREATER;
			break;
		case FGPF_TEST_MODE_GEQUAL:
			params[0] = !!GL_GEQUAL;
			break;
		case FGPF_TEST_MODE_EQUAL:
			params[0] = !!GL_EQUAL;
			break;
		case FGPF_TEST_MODE_LESS:
			params[0] = !!GL_LESS;
			break;
		case FGPF_TEST_MODE_LEQUAL:
			params[0] = !!GL_LEQUAL;
			break;
		case FGPF_TEST_MODE_NOTEQUAL:
			params[0] = !!GL_NOTEQUAL;
			break;
		}
		break;
#if 0
	case GL_BLEND_SRC_RGB :
		params[0] = (float)ctx->blend_data.fn_srcRGB;
		break;
	case GL_BLEND_SRC_ALPHA:
		params[0] = (float)ctx->blend_data.fn_srcAlpha;
		break;
	case GL_BLEND_DST_RGB :
		params[0] = (float)ctx->blend_data.fn_dstRGB;
		break;
	case GL_BLEND_DST_ALPHA:
		params[0] = (float)ctx->blend_data.fn_dstAlpha;
		break;
	case GL_BLEND_EQUATION_RGB :
		params[0] =(float)ctx->blend_data.eqn_modeRGB ;
		break;
	case GL_BLEND_EQUATION_ALPHA:
		params[0] = (float)ctx->blend_data.eqn_modeAlpha;
		break;
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_UNPACK_ALIGNMENT:
		params[0] = !!ctx->unpackAlignment;
		break;
	case GL_PACK_ALIGNMENT:
		params[0] = !!ctx->packAlignment;
		break;
	case GL_SUBPIXEL_BITS:
		params[0] = !!FGL_MAX_SUBPIXEL_BITS;
		break;
	case GL_MAX_TEXTURE_SIZE:
		params[0] = !!FGL_MAX_TEXTURE_SIZE;
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = !!FGL_MAX_VIEWPORT_DIMS;
		params[1] = !!FGL_MAX_VIEWPORT_DIMS;
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
	case GL_MAX_ELEMENTS_INDICES :
		params[0] = MAX_ELEMENTS_INDICES;
		break;
	case GL_MAX_ELEMENTS_VERTICES :
		params[0] = MAX_ELEMENTS_VERTICES;
		break;
#endif
	case GL_SAMPLE_BUFFERS :
		params[0] = 0;
		break;
	case GL_SAMPLES :
		params[0] = 0;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		params[0] = !!GL_PALETTE4_RGB8_OES;
		params[1] = !!GL_PALETTE4_RGBA8_OES;
		params[2] = !!GL_PALETTE4_R5_G6_B5_OES;
		params[3] = !!GL_PALETTE4_RGBA4_OES;
		params[4] = !!GL_PALETTE4_RGB5_A1_OES;
		params[5] = !!GL_PALETTE8_RGB8_OES;
		params[6] = !!GL_PALETTE8_RGBA8_OES;
		params[7] = !!GL_PALETTE8_R5_G6_B5_OES;
		params[8] = !!GL_PALETTE8_RGBA4_OES;
		params[9] = !!GL_PALETTE8_RGB5_A1_OES;
#if 0
		params[10] = GL_RGB_S3TC_OES;
		params[11] = GL_RGBA_S3TC_OES;
#endif
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		params[0] = !!10;
		break;
#if 0
	case GL_MAX_RENDERBUFFER_SIZE :
		params[0] = FGL_MAX_RENDERBUFFER_SIZE;
		break;
#endif
	case GL_RED_BITS :
		params[0] = !!8;
		break;
	case GL_GREEN_BITS:
		params[0] = !!8;
		break;
	case GL_BLUE_BITS :
		params[0] = !!8;
		break;
	case GL_ALPHA_BITS :
		params[0] = !!8;
		break;
	case GL_DEPTH_BITS :
		params[0] = !!24;
		break;
	case GL_STENCIL_BITS:
		params[0] = !!8;
		break;
#if 0
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
		params[0] = (float)determineTypeFormat(translateToGLenum(fbData.nativeColorFormat), GL_FALSE);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
		params[0] = (float)determineTypeFormat(translateToGLenum(ctx->defFBData.nativeColorFormat), GL_TRUE);
		break;
	case GL_ALPHA_TEST_FUNC_EXP:
		params[0] = (float)ctx->alpha_test_data.func;
		break;
	case GL_ALPHA_TEST_REF_EXP:
		params[0] = ctx->alpha_test_data.refValue;
		break;
	case GL_LOGIC_OP_MODE_EXP:
		break;
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetFixedv (GLenum pname, GLfixed *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			params[0] = fixedFromInt(ctx->arrayBuffer.getName());
		else
			params[0] = fixedFromInt(0);
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			params[0] = fixedFromInt(ctx->elementArrayBuffer.getName());
		else
			params[0] = fixedFromInt(0);
		break;
	case GL_VIEWPORT:
		params[0] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_X));
		params[1] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_Y));
		params[2] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_W));
		params[3] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_VIEWPORT_H));
		break;
	case GL_DEPTH_RANGE:
		params[0] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_NEAR));
		params[1] = fixedFromFloat(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_FAR));
		break;
	case GL_POINT_SIZE:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
							FIMG_POINT_SIZE));
		break;
	case GL_LINE_WIDTH:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
							FIMG_LINE_WIDTH));
		break;
	case GL_CULL_FACE_MODE:
		switch (fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_MODE)) {
		case FGRA_BFCULL_FACE_FRONT:
			params[0] = GL_FRONT;
			break;
		case FGRA_BFCULL_FACE_BACK:
			params[0] = GL_BACK;
			break;
		case FGRA_BFCULL_FACE_BOTH:
			params[0] = GL_FRONT_AND_BACK;
			break;
		}
		break;
	case GL_FRONT_FACE:
		if (fimgGetRasterizerState(ctx->fimg, FIMG_FRONT_FACE))
			params[0] = GL_CW;
		else
			params[0] = GL_CCW;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR));
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS));
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			params[0] = fixedFromInt(b->getName());
		else
			params[0] = fixedFromInt(0);
		break; }
	case GL_ACTIVE_TEXTURE:
		params[0] = fixedFromInt(ctx->activeTexture);
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = fixedFromBool(ctx->perFragment.mask.red);
		params[1] = fixedFromBool(ctx->perFragment.mask.green);
		params[2] = fixedFromBool(ctx->perFragment.mask.blue);
		params[3] = fixedFromBool(ctx->perFragment.mask.alpha);
		break;
	case GL_DEPTH_WRITEMASK:
		params[0] = fixedFromBool(ctx->perFragment.mask.depth);
		break;
	case GL_STENCIL_WRITEMASK :
		params[0] = fixedFromInt(ctx->perFragment.mask.stencil);
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = fixedFromFloat(ctx->clear.red);
		params[1] = fixedFromFloat(ctx->clear.green);
		params[2] = fixedFromFloat(ctx->clear.blue);
		params[3] = fixedFromFloat(ctx->clear.alpha);
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = fixedFromFloat(ctx->clear.depth);
		break;
	case GL_STENCIL_CLEAR_VALUE:
		params[0] = fixedFromInt(ctx->clear.stencil);
		break;
	case GL_SCISSOR_BOX:
		params[0] = fixedFromInt(ctx->perFragment.scissor.left);
		params[1] = fixedFromInt(ctx->perFragment.scissor.top);
		params[2] = fixedFromInt(ctx->perFragment.scissor.width);
		params[3] = fixedFromInt(ctx->perFragment.scissor.height);
		break;
	case GL_STENCIL_FUNC:
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_FUNC)) {
		case FGPF_STENCIL_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_STENCIL_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_STENCIL_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_STENCIL_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_STENCIL_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_STENCIL_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_STENCIL_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_STENCIL_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
	case GL_STENCIL_VALUE_MASK:
		params[0] = fixedFromInt(fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_MASK));
		break;
	case GL_STENCIL_REF :
		params[0] = fixedFromInt(fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_REF));
		break;
	case GL_STENCIL_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_SFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPPASS)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
#if 0
	case GL_STENCIL_BACK_FUNC :
		params[0] = (float)ctx->stencil_test_data.back_face_func;
		break;
	case GL_STENCIL_BACK_VALUE_MASK :
		params[0] = (float)ctx->stencil_test_data.back_face_mask;
		break;
	case GL_STENCIL_BACK_REF :
		params[0] = (float)ctx->stencil_test_data.back_face_ref;
		break;
	case GL_STENCIL_BACK_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_fail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_zfail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
		params[0] = (float)ctx->stencil_test_data.back_face_zpass;
		break;
#endif
	case GL_DEPTH_FUNC:
		switch (fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_FUNC)) {
		case FGPF_TEST_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_TEST_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_TEST_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_TEST_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_TEST_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_TEST_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_TEST_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_TEST_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
#if 0
	case GL_BLEND_SRC_RGB :
		params[0] = (float)ctx->blend_data.fn_srcRGB;
		break;
	case GL_BLEND_SRC_ALPHA:
		params[0] = (float)ctx->blend_data.fn_srcAlpha;
		break;
	case GL_BLEND_DST_RGB :
		params[0] = (float)ctx->blend_data.fn_dstRGB;
		break;
	case GL_BLEND_DST_ALPHA:
		params[0] = (float)ctx->blend_data.fn_dstAlpha;
		break;
	case GL_BLEND_EQUATION_RGB :
		params[0] =(float)ctx->blend_data.eqn_modeRGB ;
		break;
	case GL_BLEND_EQUATION_ALPHA:
		params[0] = (float)ctx->blend_data.eqn_modeAlpha;
		break;
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_UNPACK_ALIGNMENT:
		params[0] = fixedFromInt(ctx->unpackAlignment);
		break;
	case GL_PACK_ALIGNMENT:
		params[0] = fixedFromInt(ctx->packAlignment);
		break;
	case GL_SUBPIXEL_BITS:
		params[0] = fixedFromInt(FGL_MAX_SUBPIXEL_BITS);
		break;
	case GL_MAX_TEXTURE_SIZE:
		params[0] = fixedFromInt(FGL_MAX_TEXTURE_SIZE);
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = fixedFromInt(FGL_MAX_VIEWPORT_DIMS);
		params[1] = fixedFromInt(FGL_MAX_VIEWPORT_DIMS);
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
	case GL_MAX_ELEMENTS_INDICES :
		params[0] = MAX_ELEMENTS_INDICES;
		break;
	case GL_MAX_ELEMENTS_VERTICES :
		params[0] = MAX_ELEMENTS_VERTICES;
		break;
#endif
	case GL_SAMPLE_BUFFERS :
		params[0] = fixedFromInt(0);
		break;
	case GL_SAMPLES :
		params[0] = fixedFromInt(0);
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		params[0] = GL_PALETTE4_RGB8_OES;
		params[1] = GL_PALETTE4_RGBA8_OES;
		params[2] = GL_PALETTE4_R5_G6_B5_OES;
		params[3] = GL_PALETTE4_RGBA4_OES;
		params[4] = GL_PALETTE4_RGB5_A1_OES;
		params[5] = GL_PALETTE8_RGB8_OES;
		params[6] = GL_PALETTE8_RGBA8_OES;
		params[7] = GL_PALETTE8_R5_G6_B5_OES;
		params[8] = GL_PALETTE8_RGBA4_OES;
		params[9] = GL_PALETTE8_RGB5_A1_OES;
#if 0
		params[10] = GL_RGB_S3TC_OES;
		params[11] = GL_RGBA_S3TC_OES;
#endif
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		params[0] = fixedFromInt(10);
		break;
#if 0
	case GL_MAX_RENDERBUFFER_SIZE :
		params[0] = FGL_MAX_RENDERBUFFER_SIZE;
		break;
#endif
	case GL_RED_BITS :
		params[0] = fixedFromInt(8);
		break;
	case GL_GREEN_BITS:
		params[0] = fixedFromInt(8);
		break;
	case GL_BLUE_BITS :
		params[0] = fixedFromInt(8);
		break;
	case GL_ALPHA_BITS :
		params[0] = fixedFromInt(8);
		break;
	case GL_DEPTH_BITS :
		params[0] = fixedFromInt(24);
		break;
	case GL_STENCIL_BITS:
		params[0] = fixedFromInt(8);
		break;
#if 0
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
		params[0] = (float)determineTypeFormat(translateToGLenum(fbData.nativeColorFormat), GL_FALSE);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
		params[0] = (float)determineTypeFormat(translateToGLenum(ctx->defFBData.nativeColorFormat), GL_TRUE);
		break;
	case GL_ALPHA_TEST_FUNC_EXP:
		params[0] = (float)ctx->alpha_test_data.func;
		break;
	case GL_ALPHA_TEST_REF_EXP:
		params[0] = ctx->alpha_test_data.refValue;
		break;
	case GL_LOGIC_OP_MODE_EXP:
		break;
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_COLOR_LOGIC_OP:
		params[0] = fixedFromBool(glIsEnabled(pname));
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			params[0] = ctx->arrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			params[0] = ctx->elementArrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_VIEWPORT:
		params[0] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_X);
		params[1] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_Y);
		params[2] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_W);
		params[3] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_H);
		break;
	case GL_DEPTH_RANGE:
		params[0] = intFromClampf(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_NEAR));
		params[1] = intFromClampf(fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_FAR));
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_POINT_SIZE);
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_LINE_WIDTH);
		break;
	case GL_CULL_FACE_MODE:
		switch (fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_MODE)) {
		case FGRA_BFCULL_FACE_FRONT:
			params[0] = GL_FRONT;
			break;
		case FGRA_BFCULL_FACE_BACK:
			params[0] = GL_BACK;
			break;
		case FGRA_BFCULL_FACE_BOTH:
			params[0] = GL_FRONT_AND_BACK;
			break;
		}
		break;
	case GL_FRONT_FACE:
		if (fimgGetRasterizerState(ctx->fimg, FIMG_FRONT_FACE))
			params[0] = GL_CW;
		else
			params[0] = GL_CCW;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR);
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS);
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			params[0] = b->getName();
		else
			params[0] = 0.0f;
		break; }
	case GL_ACTIVE_TEXTURE:
		params[0] = ctx->activeTexture;
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_DEPTH_WRITEMASK:
		params[0] = ctx->perFragment.mask.depth;
		break;
	case GL_STENCIL_WRITEMASK :
		params[0] = ctx->perFragment.mask.stencil;
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = intFromClampf(ctx->clear.red);
		params[1] = intFromClampf(ctx->clear.green);
		params[2] = intFromClampf(ctx->clear.blue);
		params[3] = intFromClampf(ctx->clear.alpha);
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = intFromClampf(ctx->clear.depth);
		break;
	case GL_STENCIL_CLEAR_VALUE:
		params[0] = ctx->clear.stencil;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left;
		params[1] = ctx->perFragment.scissor.top;
		params[2] = ctx->perFragment.scissor.width;
		params[3] = ctx->perFragment.scissor.height;
		break;
	case GL_STENCIL_FUNC:
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_FUNC)) {
		case FGPF_STENCIL_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_STENCIL_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_STENCIL_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_STENCIL_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_STENCIL_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_STENCIL_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_STENCIL_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_STENCIL_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
	case GL_STENCIL_VALUE_MASK:
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_MASK);
		break;
	case GL_STENCIL_REF :
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_REF);
		break;
	case GL_STENCIL_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_SFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPPASS)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
#if 0
	case GL_STENCIL_BACK_FUNC :
		params[0] = (float)ctx->stencil_test_data.back_face_func;
		break;
	case GL_STENCIL_BACK_VALUE_MASK :
		params[0] = (float)ctx->stencil_test_data.back_face_mask;
		break;
	case GL_STENCIL_BACK_REF :
		params[0] = (float)ctx->stencil_test_data.back_face_ref;
		break;
	case GL_STENCIL_BACK_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_fail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_zfail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
		params[0] = (float)ctx->stencil_test_data.back_face_zpass;
		break;
#endif
	case GL_DEPTH_FUNC:
		switch (fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_FUNC)) {
		case FGPF_TEST_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_TEST_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_TEST_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_TEST_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_TEST_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_TEST_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_TEST_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_TEST_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
#if 0
	case GL_BLEND_SRC_RGB :
		params[0] = (float)ctx->blend_data.fn_srcRGB;
		break;
	case GL_BLEND_SRC_ALPHA:
		params[0] = (float)ctx->blend_data.fn_srcAlpha;
		break;
	case GL_BLEND_DST_RGB :
		params[0] = (float)ctx->blend_data.fn_dstRGB;
		break;
	case GL_BLEND_DST_ALPHA:
		params[0] = (float)ctx->blend_data.fn_dstAlpha;
		break;
	case GL_BLEND_EQUATION_RGB :
		params[0] =(float)ctx->blend_data.eqn_modeRGB ;
		break;
	case GL_BLEND_EQUATION_ALPHA:
		params[0] = (float)ctx->blend_data.eqn_modeAlpha;
		break;
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_UNPACK_ALIGNMENT:
		params[0] = ctx->unpackAlignment;
		break;
	case GL_PACK_ALIGNMENT:
		params[0] = ctx->packAlignment;
		break;
	case GL_SUBPIXEL_BITS:
		params[0] = FGL_MAX_SUBPIXEL_BITS;
		break;
	case GL_MAX_TEXTURE_SIZE:
		params[0] = FGL_MAX_TEXTURE_SIZE;
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = FGL_MAX_VIEWPORT_DIMS;
		params[1] = FGL_MAX_VIEWPORT_DIMS;
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
	case GL_MAX_ELEMENTS_INDICES :
		params[0] = MAX_ELEMENTS_INDICES;
		break;
	case GL_MAX_ELEMENTS_VERTICES :
		params[0] = MAX_ELEMENTS_VERTICES;
		break;
#endif
	case GL_SAMPLE_BUFFERS :
		params[0] = 0;
		break;
	case GL_SAMPLES :
		params[0] = 0;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		params[0] = GL_PALETTE4_RGB8_OES;
		params[1] = GL_PALETTE4_RGBA8_OES;
		params[2] = GL_PALETTE4_R5_G6_B5_OES;
		params[3] = GL_PALETTE4_RGBA4_OES;
		params[4] = GL_PALETTE4_RGB5_A1_OES;
		params[5] = GL_PALETTE8_RGB8_OES;
		params[6] = GL_PALETTE8_RGBA8_OES;
		params[7] = GL_PALETTE8_R5_G6_B5_OES;
		params[8] = GL_PALETTE8_RGBA4_OES;
		params[9] = GL_PALETTE8_RGB5_A1_OES;
#if 0
		params[10] = GL_RGB_S3TC_OES;
		params[11] = GL_RGBA_S3TC_OES;
#endif
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		params[0] = 10;
		break;
#if 0
	case GL_MAX_RENDERBUFFER_SIZE :
		params[0] = FGL_MAX_RENDERBUFFER_SIZE;
		break;
#endif
	case GL_RED_BITS :
		params[0] = 8;
		break;
	case GL_GREEN_BITS:
		params[0] = 8;
		break;
	case GL_BLUE_BITS :
		params[0] = 8;
		break;
	case GL_ALPHA_BITS :
		params[0] = 8;
		break;
	case GL_DEPTH_BITS :
		params[0] = 24;
		break;
	case GL_STENCIL_BITS:
		params[0] = 8;
		break;
#if 0
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
		params[0] = (float)determineTypeFormat(translateToGLenum(fbData.nativeColorFormat), GL_FALSE);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
		params[0] = (float)determineTypeFormat(translateToGLenum(ctx->defFBData.nativeColorFormat), GL_TRUE);
		break;
	case GL_ALPHA_TEST_FUNC_EXP:
		params[0] = (float)ctx->alpha_test_data.func;
		break;
	case GL_ALPHA_TEST_REF_EXP:
		params[0] = ctx->alpha_test_data.refValue;
		break;
	case GL_LOGIC_OP_MODE_EXP:
		break;
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			params[0] = ctx->arrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			params[0] = ctx->elementArrayBuffer.getName();
		else
			params[0] = 0;
		break;
	case GL_VIEWPORT:
		params[0] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_X);
		params[1] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_Y);
		params[2] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_W);
		params[3] = fimgGetPrimitiveStateF(ctx->fimg, FIMG_VIEWPORT_H);
		break;
	case GL_DEPTH_RANGE:
		params[0] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_NEAR);
		params[1] = fimgGetPrimitiveStateF(ctx->fimg,
							FIMG_DEPTH_RANGE_FAR);
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_POINT_SIZE);
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_LINE_WIDTH);
		break;
	case GL_CULL_FACE_MODE:
		switch (fimgGetRasterizerState(ctx->fimg,
							FIMG_CULL_FACE_MODE)) {
		case FGRA_BFCULL_FACE_FRONT:
			params[0] = GL_FRONT;
			break;
		case FGRA_BFCULL_FACE_BACK:
			params[0] = GL_BACK;
			break;
		case FGRA_BFCULL_FACE_BOTH:
			params[0] = GL_FRONT_AND_BACK;
			break;
		}
		break;
	case GL_FRONT_FACE:
		if (fimgGetRasterizerState(ctx->fimg, FIMG_FRONT_FACE))
			params[0] = GL_CW;
		else
			params[0] = GL_CCW;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR);
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS);
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			params[0] = b->getName();
		else
			params[0] = 0.0f;
		break; }
	case GL_ACTIVE_TEXTURE:
		params[0] = ctx->activeTexture;
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_DEPTH_WRITEMASK:
		params[0] = ctx->perFragment.mask.depth;
		break;
	case GL_STENCIL_WRITEMASK :
		params[0] = ctx->perFragment.mask.stencil;
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = ctx->clear.red;
		params[1] = ctx->clear.green;
		params[2] = ctx->clear.blue;
		params[3] = ctx->clear.alpha;
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = ctx->clear.depth;
		break;
	case GL_STENCIL_CLEAR_VALUE:
		params[0] = ctx->clear.stencil;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left;
		params[1] = ctx->perFragment.scissor.top;
		params[2] = ctx->perFragment.scissor.width;
		params[3] = ctx->perFragment.scissor.height;
		break;
	case GL_STENCIL_FUNC:
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_FUNC)) {
		case FGPF_STENCIL_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_STENCIL_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_STENCIL_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_STENCIL_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_STENCIL_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_STENCIL_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_STENCIL_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_STENCIL_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
	case GL_STENCIL_VALUE_MASK:
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_MASK);
		break;
	case GL_STENCIL_REF :
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_REF);
		break;
	case GL_STENCIL_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_SFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPPASS)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
#if 0
	case GL_STENCIL_BACK_FUNC :
		params[0] = (float)ctx->stencil_test_data.back_face_func;
		break;
	case GL_STENCIL_BACK_VALUE_MASK :
		params[0] = (float)ctx->stencil_test_data.back_face_mask;
		break;
	case GL_STENCIL_BACK_REF :
		params[0] = (float)ctx->stencil_test_data.back_face_ref;
		break;
	case GL_STENCIL_BACK_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_fail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_zfail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
		params[0] = (float)ctx->stencil_test_data.back_face_zpass;
		break;
#endif
	case GL_DEPTH_FUNC:
		switch (fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_FUNC)) {
		case FGPF_TEST_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_TEST_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_TEST_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_TEST_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_TEST_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_TEST_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_TEST_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_TEST_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
#if 0
	case GL_BLEND_SRC_RGB :
		params[0] = (float)ctx->blend_data.fn_srcRGB;
		break;
	case GL_BLEND_SRC_ALPHA:
		params[0] = (float)ctx->blend_data.fn_srcAlpha;
		break;
	case GL_BLEND_DST_RGB :
		params[0] = (float)ctx->blend_data.fn_dstRGB;
		break;
	case GL_BLEND_DST_ALPHA:
		params[0] = (float)ctx->blend_data.fn_dstAlpha;
		break;
	case GL_BLEND_EQUATION_RGB :
		params[0] =(float)ctx->blend_data.eqn_modeRGB ;
		break;
	case GL_BLEND_EQUATION_ALPHA:
		params[0] = (float)ctx->blend_data.eqn_modeAlpha;
		break;
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_UNPACK_ALIGNMENT:
		params[0] = ctx->unpackAlignment;
		break;
	case GL_PACK_ALIGNMENT:
		params[0] = ctx->packAlignment;
		break;
	case GL_SUBPIXEL_BITS:
		params[0] = FGL_MAX_SUBPIXEL_BITS;
		break;
	case GL_MAX_TEXTURE_SIZE:
		params[0] = FGL_MAX_TEXTURE_SIZE;
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = FGL_MAX_VIEWPORT_DIMS;
		params[1] = FGL_MAX_VIEWPORT_DIMS;
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
	case GL_MAX_ELEMENTS_INDICES :
		params[0] = MAX_ELEMENTS_INDICES;
		break;
	case GL_MAX_ELEMENTS_VERTICES :
		params[0] = MAX_ELEMENTS_VERTICES;
		break;
#endif
	case GL_SAMPLE_BUFFERS :
		params[0] = 0;
		break;
	case GL_SAMPLES :
		params[0] = 0;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		params[0] = GL_PALETTE4_RGB8_OES;
		params[1] = GL_PALETTE4_RGBA8_OES;
		params[2] = GL_PALETTE4_R5_G6_B5_OES;
		params[3] = GL_PALETTE4_RGBA4_OES;
		params[4] = GL_PALETTE4_RGB5_A1_OES;
		params[5] = GL_PALETTE8_RGB8_OES;
		params[6] = GL_PALETTE8_RGBA8_OES;
		params[7] = GL_PALETTE8_R5_G6_B5_OES;
		params[8] = GL_PALETTE8_RGBA4_OES;
		params[9] = GL_PALETTE8_RGB5_A1_OES;
#if 0
		params[10] = GL_RGB_S3TC_OES;
		params[11] = GL_RGBA_S3TC_OES;
#endif
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		params[0] = 10;
		break;
#if 0
	case GL_MAX_RENDERBUFFER_SIZE :
		params[0] = FGL_MAX_RENDERBUFFER_SIZE;
		break;
#endif
	case GL_RED_BITS :
		params[0] = 8;
		break;
	case GL_GREEN_BITS:
		params[0] = 8;
		break;
	case GL_BLUE_BITS :
		params[0] = 8;
		break;
	case GL_ALPHA_BITS :
		params[0] = 8;
		break;
	case GL_DEPTH_BITS :
		params[0] = 24;
		break;
	case GL_STENCIL_BITS:
		params[0] = 8;
		break;
#if 0
	case GL_IMPLEMENTATION_COLOR_READ_TYPE:
		params[0] = (float)determineTypeFormat(translateToGLenum(fbData.nativeColorFormat), GL_FALSE);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
		params[0] = (float)determineTypeFormat(translateToGLenum(ctx->defFBData.nativeColorFormat), GL_TRUE);
		break;
	case GL_ALPHA_TEST_FUNC_EXP:
		params[0] = (float)ctx->alpha_test_data.func;
		break;
	case GL_ALPHA_TEST_REF_EXP:
		params[0] = ctx->alpha_test_data.refValue;
		break;
	case GL_LOGIC_OP_MODE_EXP:
		break;
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetPointerv (GLenum pname, void **params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_VERTEX_ARRAY_POINTER:
		params[0] = (void *)ctx->array[FGL_ARRAY_VERTEX].pointer;
		break;
	case GL_NORMAL_ARRAY_POINTER:
		params[0] = (void *)ctx->array[FGL_ARRAY_NORMAL].pointer;
		break;
	case GL_COLOR_ARRAY_POINTER:
		params[0] = (void *)ctx->array[FGL_ARRAY_COLOR].pointer;
		break;
	case GL_TEXTURE_COORD_ARRAY_POINTER: {
		GLint unit = ctx->clientActiveTexture;
		params[0] = (void *)ctx->array[FGL_ARRAY_TEXTURE(unit)].pointer;
		break; }
	case GL_POINT_SIZE_ARRAY_POINTER_OES:
		params[0] = (void *)ctx->array[FGL_ARRAY_POINT_SIZE].pointer;
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
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
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glDepthMask (GLboolean flag)
{
	FUNC_UNIMPLEMENTED;
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

GL_API void GL_APIENTRY glShadeModel (GLenum mode)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glStencilMask (GLuint mask)
{
	FUNC_UNIMPLEMENTED;
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
	fimgDestroyContext(ctx->fimg);
	delete ctx;
}
