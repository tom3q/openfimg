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
#include <GLES/gl.h>
#include "state.h"
#include "types.h"
#include "fimg/fimg.h"

/**
	Client API information
*/

static char const * const gVendorString     = "notSamsung";
static char const * const gRendererString   = "S3C6410 FIMG-3DSE";
static char const * const gVersionString    = "OpenGL ES-CM 1.1";
static char const * const gExtensionsString =
#if 0
	"GL_OES_byte_coordinates "              // TODO
	"GL_OES_fixed_point "                   // TODO
	"GL_OES_single_precision "              // TODO
	"GL_OES_read_format "                   // TODO
	"GL_OES_compressed_paletted_texture "   // TODO
	"GL_OES_draw_texture "                  // TODO
	"GL_OES_matrix_get "                    // TODO
	"GL_OES_query_matrix "                  // TODO
#endif
	"GL_OES_point_size_array "              // OK
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

static inline FGLContext *getContext(void)
{
	return getGlThreadSpecific();
}

static inline void setError(GLenum error)
{
#warning Implementation only for testing.
	fprintf(stderr, "GLESv1_fimg: GL error %d\n", error);
	if(errorCode == GL_NO_ERROR)
		errorCode = error;
}

GL_API GLenum GL_APIENTRY glGetError (void)
{
#warning Implementation only for testing.
	GLenum error = errorCode;
	errorCode = GL_NO_ERROR;
	return error;
}

/**
	Vertex state
*/

FGLvec4f FGLContext::defaultVertex[4 + FGL_MAX_TEXTURE_UNITS] = {
	/* Vertex - unused */
	{ 0, 0, 0, 0 },
	/* Normal */
	{ 0, 0, 1 },
	/* Color */
	{ 1, 1, 1, 1 },
	/* Point size */
	{ 1 },
	/* Texture 0 */
	{ 0, 0, 0, 1 },
	/* Texture 1 */
	{ 0, 0, 0, 1 },
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
	ctx->array[idx].size	= FGHI_NUMCOMP(size);
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
		fglStride = 1;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 2;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4;
		break;
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
	fglSetupAttribute(ctx, FGL_ARRAY_VERTEX, size, fglType, fglStride, pointer);
}

GL_API void GL_APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GLint fglType, fglStride;

	switch(type) {
	case GL_BYTE:
		fglType = FGHI_ATTRIB_DT_BYTE;
		fglStride = 1;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 2;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4;
		break;
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
		fglStride = 1;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4;
		break;
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
	fglSetupAttribute(ctx, FGL_ARRAY_COLOR, 3, fglType, fglStride, pointer);
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
		fglStride = 1;
		break;
	case GL_SHORT:
		fglType = FGHI_ATTRIB_DT_SHORT;
		fglStride = 2;
		break;
	case GL_FIXED:
		fglType = FGHI_ATTRIB_DT_FIXED;
		fglStride = 4;
		break;
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

GL_API void GL_APIENTRY glDisableClientState (GLenum array)
{
	FGLContext *ctx = getContext();
	GLint idx, defSize;

	switch (array) {
	case GL_VERTEX_ARRAY:
		idx = FGL_ARRAY_VERTEX;
		defSize = 4;
		break;
	case GL_NORMAL_ARRAY:
		idx = FGL_ARRAY_NORMAL;
		defSize = 3;
		break;
	case GL_COLOR_ARRAY:
		idx = FGL_ARRAY_COLOR;
		defSize = 4;
		break;
	case GL_POINT_SIZE_ARRAY_OES:
		idx = FGL_ARRAY_POINT_SIZE;
		defSize = 1;
		break;
	case GL_TEXTURE_COORD_ARRAY:
		idx = FGL_ARRAY_TEXTURE(ctx->activeTexture);
		defSize = 4;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	ctx->array[idx].enabled = GL_FALSE;
	fimgSetAttribute(ctx->fimg, idx, FGHI_ATTRIB_DT_FLOAT, defSize);
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
			pointers[i] 			= (const unsigned char *)ctx->array[i].pointer + first * ctx->array[i].stride;
			stride[i] 			= ctx->array[i].stride;
		} else {
			pointers[i]			= &ctx->vertex[i];
			stride[i]			= 0;
		}
	}

	fimgSetAttribCount(ctx->fimg, 4 + FGL_MAX_TEXTURE_UNITS);
	fimgSetVertexContext(ctx->fimg, fglMode, 4 + FGL_MAX_TEXTURE_UNITS);
	fimgDrawNonIndexArrays(ctx->fimg, count, pointers, stride);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(m);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().inverse();

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
}

GL_API void GL_APIENTRY glLoadMatrixx (const GLfixed *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].top().load(m);

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(m);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().inverse();

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverse();
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverse();
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().load(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
}

GL_API void GL_APIENTRY glLoadIdentity (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].top().identity();

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().identity();

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);
	
	if(idx != FGL_MATRIX_MODELVIEW)
		return;
	
	mat.transpose();
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);
	
	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseTranslate(x, y, z);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseScale(x, y, z);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseFrustum(left, right, bottom, top, zNear, zFar);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	mat.inverseOrtho(left, right, bottom, top, zNear, zFar);
	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().leftMultiply(mat);

	fimgLoadMatrix(FGL_MATRIX_MODELVIEW_INVERSE, ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top().data);
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

	fimgLoadMatrix(idx, ctx->matrix.stack[idx].top().data);

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].pop();
}

GL_API void GL_APIENTRY glPushMatrix (void)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->matrix.activeTexture);

	ctx->matrix.stack[idx].push();

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].push();
}

/**
	Special functions
*/

GL_API void GL_APIENTRY glFlush (void)
{

}

GL_API void GL_APIENTRY glFinish (void)
{

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
	fimgRestoreContext(ctx->fimg);

	/* Restore shaders */
	fimgLoadVShader(ctx->vertexShader.data);
	fimgLoadPShader(ctx->pixelShader.data);

	/* Restore textures */
	for(int i = 0; i < FGL_MAX_TEXTURE_UNITS; i++)
		fimgSetTexUnitParams(i, NULL /* TODO */);
}