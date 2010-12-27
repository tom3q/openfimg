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

static inline GLclampf clampfFromUByte(GLubyte c)
{
	return (GLclampf)c / ((1 << 8) - 1);
}

static inline GLclampf clampfFromByte(GLbyte c)
{
	return (GLclampf)(2*c + 1) / ((1 << 8) - 1);
}

static inline GLclampf clampfFromUShort(GLushort c)
{
	return (GLclampf)c / ((1 << 16) - 1);
}

static inline GLclampf clampfFromShort(GLshort c)
{
	return (GLclampf)(2*c + 1) / ((1 << 16) - 1);
}

static inline GLclampf clampfFromInt(GLint c)
{
	return (GLclampf)(2*c + 1) / (0xffffffff);
}

static inline GLfloat floatFromFixed(GLfixed c)
{
	return (GLfloat)c / (1 << 16);
}

static inline GLclampf clampFloat(GLclampf f)
{
	if(f < 0)
		f = 0;

	if(f > 1)
		f = 1;

	return f;
}

static inline GLclampx clampFixed(GLclampx x)
{
	if(x < 0)
		x = 0;

	if(x > (1 << 16))
		x = 1 << 16;

	return x;
}

static inline GLubyte ubyteFromClampf(GLclampf c)
{
	return c * ((1 << 8) - 1);
}

static inline GLubyte ubyteFromClampx(GLclampx c)
{
	return ubyteFromClampf(floatFromFixed(c));
}

static inline GLint intFromFloat(GLfloat f)
{
	return (GLint)f;
}

static inline GLint intFromFixed(GLfixed x)
{
	return x >> 16;
}

static inline GLint intFromClampf(GLclampf c)
{
	return (c * (0xffffffff) - 1) / 2;
}

static inline GLint intFromClampx(GLclampx c)
{
	return intFromClampf(floatFromFixed(c));
}

static inline GLfixed fixedFromFloat(GLfloat f)
{
	return f * (1 << 16);
}

static inline GLfixed fixedFromInt(GLint i)
{
	return i << 16;
}

static inline GLfixed fixedFromBool(GLboolean b)
{
	return (!!b) << 16;
}

static inline GLboolean boolFromFloat(GLfloat f)
{
	return f != 0;
}

static inline GLboolean boolFromFixed(GLfixed x)
{
	return !!x;
}

static inline GLboolean boolFromInt(GLint i)
{
	return !!i;
}

typedef unsigned char FGLubyte;
typedef signed char FGLbyte;
typedef unsigned short FGLushort;
typedef signed short FGLshort;
typedef unsigned int FGLuint;
typedef signed int FGLint;

#define FGL_NO_ERROR	0

#endif // _LIBSGL_TYPES_H_
