/**
 * libsgl/types.h
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

#ifndef _LIBSGL_TYPES_H_
#define _LIBSGL_TYPES_H_

#include <GLES/gl.h>

typedef GLfloat FGLvec4f[4];
typedef GLfloat FGLvec3f[3];
typedef GLfloat FGLvec2f[2];

static inline GLfloat floatFromUByte(GLubyte c)
{
	return (GLfloat)c / ((1 << 8) - 1);
}

static inline GLfloat floatFromByte(GLbyte c)
{
	return (GLfloat)(2*c + 1) / ((1 << 8) - 1);
}

static inline GLfloat floatFromUShort(GLushort c)
{
	return (GLfloat)c / ((1 << 16) - 1);
}

static inline GLfloat floatFromShort(GLshort c)
{
	return (GLfloat)(2*c + 1) / ((1 << 16) - 1);
}

static inline GLfloat floatFromInt(GLint c)
{
	return (GLfloat)(2*c + 1) / (0xFFFFFFFF);
}

static inline GLfloat floatFromFixed(GLfixed c)
{
	return (GLfloat)c / (1 << 16);
}

static inline GLclampf clampFloat(GLclampf f)
{
	if(f < 0)
		return 0;

	if(f > 1)
		return 1;

	return f;
}

static inline GLubyte ubyteFromClampf(GLclampf c)
{
	return clampFloat(c) * ((1 << 8) - 1);
}

static inline GLubyte ubyteFromClampx(GLclampx c)
{
	return c >> 24;
}

static inline GLint intFromFloat(GLfloat f)
{
	return (GLint)f;
}

static inline GLint intFromFixed(GLfixed x)
{
	return x >> 16;
}

typedef unsigned char FGLubyte;
typedef signed char FGLbyte;
typedef unsigned short FGLushort;
typedef signed short FGLshort;
typedef unsigned int FGLuint;
typedef signed int FGLint;

#define FGL_NO_ERROR	0

#endif // _LIBSGL_TYPES_H_
