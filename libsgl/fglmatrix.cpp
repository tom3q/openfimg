/**
 * libsgl/matrix.cpp
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

#include <GLES/gl.h>
#include <cstring>
#include <cmath>
#include "fglmatrix.h"

/**
 *	4x4 matrix (for geometry transformation)
 */

/**
 *	Matrix loading
 */

void FGLmatrix::zero(void)
{
	for(int i = 0; i < 4; i++) {
		(*this)[i][0] = 0;
		(*this)[i][1] = 0;
		(*this)[i][2] = 0;
		(*this)[i][3] = 0;
	}
}

void FGLmatrix::identity(void)
{
	zero();

	(*this)[0][0] = 1;
	(*this)[1][1] = 1;
	(*this)[2][2] = 1;
	(*this)[3][3] = 1;
}

void FGLmatrix::load(const GLfloat *m)
{
	memcpy(data, m, 16 * sizeof(GLfloat));
}

void FGLmatrix::load(const GLfixed *m)
{
	for(int x = 0; x < 4; ++x) {
		(*this)[x][0] = floatFromFixed(m[MAT4(x, 0)]);
		(*this)[x][1] = floatFromFixed(m[MAT4(x, 1)]);
		(*this)[x][2] = floatFromFixed(m[MAT4(x, 2)]);
		(*this)[x][3] = floatFromFixed(m[MAT4(x, 3)]);
	}
}

void FGLmatrix::rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	GLfloat rad = (angle * M_PI) / 180;
	GLfloat s = sin(rad);
	GLfloat c = cos(rad);
	GLfloat ci = 1.0f - c;
	GLfloat module = sqrt(x*x + y*y + z*z);
	GLfloat invModule = 1.0f / module;
	GLfloat x1, y1, z1;
	GLfloat xs, ys, zs;
	GLfloat x2, y2, z2;
	GLfloat xy, yz, xz;

	identity();

	x1 = x * invModule;
	y1 = y * invModule;
	z1 = z * invModule;

	xs = x1 * s;
	ys = y1 * s;
	zs = z1 * s;

	x2 = x1 * x1;
	y2 = y1 * y1;
	z2 = z1 * z1;

	xy = x1 * y1;
	yz = y1 * z1;
	xz = x1 * z1;

	(*this)[0][0] = x2 * ci + c;  (*this)[1][0] = xy * ci - zs; (*this)[2][0] = xz * ci + ys;
	(*this)[0][1] = xy * ci + zs; (*this)[1][1] = y2 * ci + c;  (*this)[2][1] = yz * ci - xs;
	(*this)[0][2] = xz * ci - ys; (*this)[1][2] = yz * ci + xs; (*this)[2][2] = z2 * ci + c;
}

void FGLmatrix::translate(GLfloat x, GLfloat y, GLfloat z)
{
	identity();

	(*this)[3][0] = x;
	(*this)[3][1] = y;
	(*this)[3][2] = z;
}

void FGLmatrix::scale(GLfloat x, GLfloat y, GLfloat z)
{
	identity();

	(*this)[0][0] = x;
	(*this)[1][1] = y;
	(*this)[2][2] = z;
}

void FGLmatrix::frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	identity();

	(*this)[0][0] = (2.0f * n) / (r - l);

	(*this)[1][1] = (2.0f * n) / (t - b) ;

	(*this)[2][0] = (r + l) / (r - l);
	(*this)[2][1] = (t + b) / (t - b);
	(*this)[2][2] = -1.0f * (f + n) / (f - n);
	(*this)[2][3] = -1.0f;

	(*this)[3][2] = (-2.0 * f * n) / (f - n);
	(*this)[3][3] = 0.0f ;
}

void FGLmatrix::ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f)
{
	identity();

	(*this)[0][0] = 2.0f / (r - l);

	(*this)[1][1] = 2.0f / (t - b);

	(*this)[2][2] = 2.0f / (n - f);

	(*this)[3][0] = (r + l) / (l - r);
	(*this)[3][1] = (t + b) / (b - t);
	(*this)[3][2] = (f + n) / (n - f);
}

void FGLmatrix::inverseTranslate(GLfloat x, GLfloat y, GLfloat z)
{
	identity();

	(*this)[3][0] = -x;
	(*this)[3][1] = -y;
	(*this)[3][2] = -z;
}

/**
	Required conditions:
	x != 0, y != 0, z != 0
*/
void FGLmatrix::inverseScale(GLfloat x, GLfloat y, GLfloat z)
{
	identity();

	(*this)[0][0] = 1.0f/x;
	(*this)[1][1] = 1.0f/y;
	(*this)[2][2] = 1.0f/z;
}

/**
	Required conditions:
	zNear != 0, zFar != 0
*/
void FGLmatrix::inverseFrustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	identity();

	(*this)[0][0] = (right - left) / (2 * zNear);
	(*this)[1][1] = (top - bottom) / (2 * zNear);
	(*this)[2][2] = 0;
	(*this)[2][3] = (zFar - zNear) / (-2 * zFar * zNear);
	(*this)[3][0] = (left + right) / (2 * zNear);
	(*this)[3][1] = (bottom + top) / (2 * zNear);
	(*this)[3][2] = -1;
	(*this)[3][3] = (zFar + zNear) / (-2 * zFar * zNear);
}

/**
	Required conditions:
	zNear != zFar, right != left, top != bottom
*/
void FGLmatrix::inverseOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	identity();

	(*this)[0][0] = (right - left) / 2;
	(*this)[1][1] = (top - bottom) / 2;
	(*this)[2][2] = (zNear - zFar) / 2;
	(*this)[3][0] = (left + right) / 2;
	(*this)[3][1] = (bottom + top) / 2;
	(*this)[3][2] = (zNear + zFar) / -2;
}

/**
	Matrix modification
*/

void FGLmatrix::multiply(const GLfloat *m)
{
	FGLmatrix temp;

	for(int i = 0; i < 4; ++i) {
		temp[i][0] = (*this)[0][0]*m[MAT4(i, 0)] + (*this)[1][0]*m[MAT4(i, 1)] + (*this)[2][0]*m[MAT4(i, 2)] + (*this)[3][0]*m[MAT4(i, 3)];
		temp[i][1] = (*this)[0][1]*m[MAT4(i, 0)] + (*this)[1][1]*m[MAT4(i, 1)] + (*this)[2][1]*m[MAT4(i, 2)] + (*this)[3][1]*m[MAT4(i, 3)];
		temp[i][2] = (*this)[0][2]*m[MAT4(i, 0)] + (*this)[1][2]*m[MAT4(i, 1)] + (*this)[2][2]*m[MAT4(i, 2)] + (*this)[3][2]*m[MAT4(i, 3)];
		temp[i][3] = (*this)[0][3]*m[MAT4(i, 0)] + (*this)[1][3]*m[MAT4(i, 1)] + (*this)[2][3]*m[MAT4(i, 2)] + (*this)[3][3]*m[MAT4(i, 3)];
	}

	*this = temp;
}

void FGLmatrix::multiply(const GLfixed *m)
{
	FGLmatrix temp;

	for(int i = 0; i < 4; ++i) {
		temp[0][0] = (*this)[0][0]*floatFromFixed(m[MAT4(i, 0)]) + (*this)[1][0]*floatFromFixed(m[MAT4(i, 1)]) + (*this)[2][0]*floatFromFixed(m[MAT4(i, 2)]) + (*this)[3][0]*floatFromFixed(m[MAT4(i, 3)]);
		temp[0][1] = (*this)[0][1]*floatFromFixed(m[MAT4(i, 0)]) + (*this)[1][1]*floatFromFixed(m[MAT4(i, 1)]) + (*this)[2][1]*floatFromFixed(m[MAT4(i, 2)]) + (*this)[3][1]*floatFromFixed(m[MAT4(i, 3)]);
		temp[0][2] = (*this)[0][2]*floatFromFixed(m[MAT4(i, 0)]) + (*this)[1][2]*floatFromFixed(m[MAT4(i, 1)]) + (*this)[2][2]*floatFromFixed(m[MAT4(i, 2)]) + (*this)[3][2]*floatFromFixed(m[MAT4(i, 3)]);
		temp[0][3] = (*this)[0][3]*floatFromFixed(m[MAT4(i, 0)]) + (*this)[1][3]*floatFromFixed(m[MAT4(i, 1)]) + (*this)[2][3]*floatFromFixed(m[MAT4(i, 2)]) + (*this)[3][3]*floatFromFixed(m[MAT4(i, 3)]);
	}

	*this = temp;
}

void FGLmatrix::leftMultiply(FGLmatrix const &m)
{
	FGLmatrix temp;

	for(int i = 0; i < 4; ++i) {
		temp[i][0] = m[0][0]*(*this)[i][0] + m[1][0]*(*this)[i][1] + m[2][0]*(*this)[i][2] + m[3][0]*(*this)[i][3];
		temp[i][1] = m[0][1]*(*this)[i][0] + m[1][1]*(*this)[i][1] + m[2][1]*(*this)[i][2] + m[3][1]*(*this)[i][3];
		temp[i][2] = m[0][2]*(*this)[i][0] + m[1][2]*(*this)[i][1] + m[2][2]*(*this)[i][2] + m[3][2]*(*this)[i][3];
		temp[i][3] = m[0][3]*(*this)[i][0] + m[1][3]*(*this)[i][1] + m[2][3]*(*this)[i][2] + m[3][3]*(*this)[i][3];
	}

	*this = temp;
}

void FGLmatrix::multiply(const FGLmatrix &a, const FGLmatrix &b)
{
	for(int i = 0; i < 4; ++i) {
		(*this)[i][0] = a[0][0]*b[i][0] + a[1][0]*b[i][1] + a[2][0]*b[i][2] + a[3][0]*b[i][3];
		(*this)[i][1] = a[0][1]*b[i][0] + a[1][1]*b[i][1] + a[2][1]*b[i][2] + a[3][1]*b[i][3];
		(*this)[i][2] = a[0][2]*b[i][0] + a[1][2]*b[i][1] + a[2][2]*b[i][2] + a[3][2]*b[i][3];
		(*this)[i][3] = a[0][3]*b[i][0] + a[1][3]*b[i][1] + a[2][3]*b[i][2] + a[3][3]*b[i][3];
	}
}

void FGLmatrix::inverse(void)
{
	FGLmatrix mat;
	GLfloat det, invDet;

	det =	((*this)[0][3]*(*this)[1][2]*(*this)[2][1]*(*this)[3][0]) -
		((*this)[0][2]*(*this)[1][3]*(*this)[2][1]*(*this)[3][0]) -
		((*this)[0][3]*(*this)[1][1]*(*this)[2][2]*(*this)[3][0]) +
		((*this)[0][1]*(*this)[1][3]*(*this)[2][2]*(*this)[3][0]) +
		((*this)[0][2]*(*this)[1][1]*(*this)[2][3]*(*this)[3][0]) -
		((*this)[0][1]*(*this)[1][2]*(*this)[2][3]*(*this)[3][0]) -
		((*this)[0][3]*(*this)[1][2]*(*this)[2][0]*(*this)[3][1]) +
		((*this)[0][2]*(*this)[1][3]*(*this)[2][0]*(*this)[3][1]) +
		((*this)[0][3]*(*this)[1][0]*(*this)[2][2]*(*this)[3][1]) -
		((*this)[0][0]*(*this)[1][3]*(*this)[2][2]*(*this)[3][1]) -
		((*this)[0][2]*(*this)[1][0]*(*this)[2][3]*(*this)[3][1]) +
		((*this)[0][0]*(*this)[1][2]*(*this)[2][3]*(*this)[3][1]) +
		((*this)[0][3]*(*this)[1][1]*(*this)[2][0]*(*this)[3][2]) -
		((*this)[0][1]*(*this)[1][3]*(*this)[2][0]*(*this)[3][2]) -
		((*this)[0][3]*(*this)[1][0]*(*this)[2][1]*(*this)[3][2]) +
		((*this)[0][0]*(*this)[1][3]*(*this)[2][1]*(*this)[3][2]) +
		((*this)[0][1]*(*this)[1][0]*(*this)[2][3]*(*this)[3][2]) -
		((*this)[0][0]*(*this)[1][1]*(*this)[2][3]*(*this)[3][2]) -
		((*this)[0][2]*(*this)[1][1]*(*this)[2][0]*(*this)[3][3]) +
		((*this)[0][1]*(*this)[1][2]*(*this)[2][0]*(*this)[3][3]) +
		((*this)[0][2]*(*this)[1][0]*(*this)[2][1]*(*this)[3][3]) -
		((*this)[0][0]*(*this)[1][2]*(*this)[2][1]*(*this)[3][3]) -
		((*this)[0][1]*(*this)[1][0]*(*this)[2][2]*(*this)[3][3]) +
		((*this)[0][0]*(*this)[1][1]*(*this)[2][2]*(*this)[3][3]);

	if(det == 0)
		// Singular matrix
		return;

	invDet = 1/det;

	mat[0][0] = invDet *	((-(*this)[1][3]*(*this)[2][2]*(*this)[3][1]) +
				((*this)[1][2]*(*this)[2][3]*(*this)[3][1]) +
				((*this)[1][3]*(*this)[2][1]*(*this)[3][2]) -
				((*this)[1][1]*(*this)[2][3]*(*this)[3][2]) -
				((*this)[1][2]*(*this)[2][1]*(*this)[3][3]) +
				((*this)[1][1]*(*this)[2][2]*(*this)[3][3]));
	mat[0][1] = invDet *	(((*this)[0][3]*(*this)[2][2]*(*this)[3][1]) -
				((*this)[0][2]*(*this)[2][3]*(*this)[3][1]) -
				((*this)[0][3]*(*this)[2][1]*(*this)[3][2]) +
				((*this)[0][1]*(*this)[2][3]*(*this)[3][2]) +
				((*this)[0][2]*(*this)[2][1]*(*this)[3][3]) -
				((*this)[0][1]*(*this)[2][2]*(*this)[3][3]));
	mat[0][2] = invDet *	((-(*this)[0][3]*(*this)[1][2]*(*this)[3][1]) +
				((*this)[0][2]*(*this)[1][3]*(*this)[3][1]) +
				((*this)[0][3]*(*this)[1][1]*(*this)[3][2]) -
				((*this)[0][1]*(*this)[1][3]*(*this)[3][2]) -
				((*this)[0][2]*(*this)[1][1]*(*this)[3][3]) +
				((*this)[0][1]*(*this)[1][2]*(*this)[3][3]));
	mat[0][3] = invDet *	(((*this)[0][3]*(*this)[1][2]*(*this)[2][1]) -
				((*this)[0][2]*(*this)[1][3]*(*this)[2][1]) -
				((*this)[0][3]*(*this)[1][1]*(*this)[2][2]) +
				((*this)[0][1]*(*this)[1][3]*(*this)[2][2]) +
				((*this)[0][2]*(*this)[1][1]*(*this)[2][3]) -
				((*this)[0][1]*(*this)[1][2]*(*this)[2][3]));

	mat[1][0] = invDet *	(((*this)[1][3]*(*this)[2][2]*(*this)[3][0]) -
				((*this)[1][2]*(*this)[2][3]*(*this)[3][0]) -
				((*this)[1][3]*(*this)[2][0]*(*this)[3][2]) +
				((*this)[1][0]*(*this)[2][3]*(*this)[3][2]) +
				((*this)[1][2]*(*this)[2][0]*(*this)[3][3]) -
				((*this)[1][0]*(*this)[2][2]*(*this)[3][3]));
	mat[1][1] = invDet *	((-(*this)[0][3]*(*this)[2][2]*(*this)[3][0]) +
				((*this)[0][2]*(*this)[2][3]*(*this)[3][0]) +
				((*this)[0][3]*(*this)[2][0]*(*this)[3][2]) -
				((*this)[0][0]*(*this)[2][3]*(*this)[3][2]) -
				((*this)[0][2]*(*this)[2][0]*(*this)[3][3]) +
				((*this)[0][0]*(*this)[2][2]*(*this)[3][3]));
	mat[1][2] = invDet *	(((*this)[0][3]*(*this)[1][2]*(*this)[3][0]) -
				((*this)[0][2]*(*this)[1][3]*(*this)[3][0]) -
				((*this)[0][3]*(*this)[1][0]*(*this)[3][2]) +
				((*this)[0][0]*(*this)[1][3]*(*this)[3][2]) +
				((*this)[0][2]*(*this)[1][0]*(*this)[3][3]) -
				((*this)[0][0]*(*this)[1][2]*(*this)[3][3]));
	mat[1][3] = invDet *	((-(*this)[0][3]*(*this)[1][2]*(*this)[2][0]) +
				((*this)[0][2]*(*this)[1][3]*(*this)[2][0]) +
				((*this)[0][3]*(*this)[1][0]*(*this)[2][2]) -
				((*this)[0][0]*(*this)[1][3]*(*this)[2][2]) -
				((*this)[0][2]*(*this)[1][0]*(*this)[2][3]) +
				((*this)[0][0]*(*this)[1][2]*(*this)[2][3]));

	mat[2][0] = invDet *	((-(*this)[1][3]*(*this)[2][1]*(*this)[3][0]) +
				((*this)[1][1]*(*this)[2][3]*(*this)[3][0]) +
				((*this)[1][3]*(*this)[2][0]*(*this)[3][1]) -
				((*this)[1][0]*(*this)[2][3]*(*this)[3][1]) -
				((*this)[1][1]*(*this)[2][0]*(*this)[3][3]) +
				((*this)[1][0]*(*this)[2][1]*(*this)[3][3]));
	mat[2][1] = invDet *	(((*this)[0][3]*(*this)[2][1]*(*this)[3][0]) -
				((*this)[0][1]*(*this)[2][3]*(*this)[3][0]) -
				((*this)[0][3]*(*this)[2][0]*(*this)[3][1]) +
				((*this)[0][0]*(*this)[2][3]*(*this)[3][1]) +
				((*this)[0][1]*(*this)[2][0]*(*this)[3][3]) -
				((*this)[0][0]*(*this)[2][1]*(*this)[3][3]));
	mat[2][2] = invDet *	((-(*this)[0][3]*(*this)[1][1]*(*this)[3][0]) +
				((*this)[0][1]*(*this)[1][3]*(*this)[3][0]) +
				((*this)[0][3]*(*this)[1][0]*(*this)[3][1]) -
				((*this)[0][0]*(*this)[1][3]*(*this)[3][1]) -
				((*this)[0][1]*(*this)[1][0]*(*this)[3][3]) +
				((*this)[0][0]*(*this)[1][1]*(*this)[3][3]));
	mat[2][3] = invDet *	(((*this)[0][3]*(*this)[1][1]*(*this)[2][0]) -
				((*this)[0][1]*(*this)[1][3]*(*this)[2][0]) -
				((*this)[0][3]*(*this)[1][0]*(*this)[2][1]) +
				((*this)[0][0]*(*this)[1][3]*(*this)[2][1]) +
				((*this)[0][1]*(*this)[1][0]*(*this)[2][3]) -
				((*this)[0][0]*(*this)[1][1]*(*this)[2][3]));

	mat[3][0] = invDet *	(((*this)[1][2]*(*this)[2][1]*(*this)[3][0]) -
				((*this)[1][1]*(*this)[2][2]*(*this)[3][0]) -
				((*this)[1][2]*(*this)[2][0]*(*this)[3][1]) +
				((*this)[1][0]*(*this)[2][2]*(*this)[3][1]) +
				((*this)[1][1]*(*this)[2][0]*(*this)[3][2]) -
				((*this)[1][0]*(*this)[2][1]*(*this)[3][2]));
	mat[3][1] = invDet *	((-(*this)[0][2]*(*this)[2][1]*(*this)[3][0]) +
				((*this)[0][1]*(*this)[2][2]*(*this)[3][0]) +
				((*this)[0][2]*(*this)[2][0]*(*this)[3][1]) -
				((*this)[0][0]*(*this)[2][2]*(*this)[3][1]) -
				((*this)[0][1]*(*this)[2][0]*(*this)[3][2]) +
				((*this)[0][0]*(*this)[2][1]*(*this)[3][2]));
	mat[3][2] = invDet *	(((*this)[0][2]*(*this)[1][1]*(*this)[3][0]) -
				((*this)[0][1]*(*this)[1][2]*(*this)[3][0]) -
				((*this)[0][2]*(*this)[1][0]*(*this)[3][1]) +
				((*this)[0][0]*(*this)[1][2]*(*this)[3][1]) +
				((*this)[0][1]*(*this)[1][0]*(*this)[3][2]) -
				((*this)[0][0]*(*this)[1][1]*(*this)[3][2]));
	mat[3][3] = invDet *	((-(*this)[0][2]*(*this)[1][1]*(*this)[2][0]) +
				((*this)[0][1]*(*this)[1][2]*(*this)[2][0]) +
				((*this)[0][2]*(*this)[1][0]*(*this)[2][1]) -
				((*this)[0][0]*(*this)[1][2]*(*this)[2][1]) -
				((*this)[0][1]*(*this)[1][0]*(*this)[2][2]) +
				((*this)[0][0]*(*this)[1][1]*(*this)[2][2]));

	*this = mat;
}

void FGLmatrix::transpose(void)
{
	FGLmatrix mat;

	for(int i = 0; i < 4; i++) {
		mat[0][i] = (*this)[i][0];
		mat[1][i] = (*this)[i][1];
		mat[2][i] = (*this)[i][2];
		mat[3][i] = (*this)[i][3];
	}

	*this = mat;
}
