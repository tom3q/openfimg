/**
 * libsgl/glesMatrix.cpp
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

	FGLmatrix *mat = &ctx->matrix.stack[idx].top();
	mat->multiply(m);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	FGLmatrix *inv = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top();
	inv->load(*mat);
	inv->inverse();
	ctx->matrix.dirty[FGL_MATRIX_MODELVIEW_INVERSE] = GL_TRUE;
}

GL_API void GL_APIENTRY glMultMatrixx (const GLfixed *m)
{
	FGLContext *ctx = getContext();
	GLint idx = ctx->matrix.activeMatrix;

	if(idx == FGL_MATRIX_TEXTURE)
		idx = FGL_MATRIX_TEXTURE(ctx->activeTexture);

	FGLmatrix *mat = &ctx->matrix.stack[idx].top();
	mat->multiply(m);
	ctx->matrix.dirty[idx] = GL_TRUE;

	if(idx != FGL_MATRIX_MODELVIEW)
		return;

	FGLmatrix *inv = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW_INVERSE].top();
	inv->load(*mat);
	inv->inverse();
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

