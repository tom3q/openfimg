/*
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
#include <cmath>

/** 4-dimensional floating point vector. */
typedef GLfloat FGLvec4f[4];
/** 3-dimensional floating point vector. */
typedef GLfloat FGLvec3f[3];
/** 2-dimensional floating point vector. */
typedef GLfloat FGLvec2f[2];

/**
 * Converts unsigned byte value into clamped floating point value.
 * @param c Unsigned byte value to convert.
 * @return Resulting clamped floating point value.
 */
static inline GLclampf clampfFromUByte(GLubyte c)
{
	return (GLclampf)c / ((1 << 8) - 1);
}

/**
 * Converts signed byte value into clamped floating point value.
 * @param c Signed byte value to convert.
 * @return Resulting clamped floating point value.
 */
static inline GLclampf clampfFromByte(GLbyte c)
{
	return (GLclampf)(2*c + 1) / ((1 << 8) - 1);
}

/**
 * Converts unsigned short value into clamped floating point value.
 * @param c Unsigned short value to convert.
 * @return Resulting clamped floating point value.
 */
static inline GLclampf clampfFromUShort(GLushort c)
{
	return (GLclampf)c / ((1 << 16) - 1);
}

/**
 * Converts signed short value into clamped floating point value.
 * @param c Signed short value to convert.
 * @return Resulting clamped floating point value.
 */
static inline GLclampf clampfFromShort(GLshort c)
{
	return (GLclampf)(2*c + 1) / ((1 << 16) - 1);
}

/**
 * Converts integer value into clamped floating point value.
 * @param c Integer value to convert.
 * @return Resulting clamped floating point value.
 */
static inline GLclampf clampfFromInt(GLint c)
{
	return (GLclampf)(2*c + 1) / (0xffffffff);
}

/**
 * Converts fixed point value into floating point value.
 * @param c Fixed point value to convert.
 * @return Resulting floating point value.
 */
static inline GLfloat floatFromFixed(GLfixed c)
{
	const double div = 1.0f / 65536.0f;
	double tmp = c;
	return (float)(div * tmp);
}

/**
 * Clamps floating point value into [0.0; 1.0] range.
 * @param f Floating point value to clamp.
 * @return Clamped value.
 */
static inline GLclampf clampFloat(GLclampf f)
{
	if(f < 0)
		f = 0;

	if(f > 1)
		f = 1;

	return f;
}

/**
 * Clamps fixed point value into [0.0; 1.0] range.
 * @param x Fixed point value to clamp.
 * @return Clamped value.
 */
static inline GLclampx clampFixed(GLclampx x)
{
	if(x < 0)
		x = 0;

	if(x > (1 << 16))
		x = 1 << 16;

	return x;
}

/**
 * Converts clamped floating point value into unsigned byte value.
 * @param c Clamped floating point value.
 * @return Resulting unsigned byte value.
 */
static inline GLubyte ubyteFromClampf(GLclampf c)
{
	return c * ((1 << 8) - 1);
}

/**
 * Converts clamped fixed point value into unsigned byte value.
 * @param c Clamped fixed point value.
 * @return Resulting unsigned byte value.
 */
static inline GLubyte ubyteFromClampx(GLclampx c)
{
	return ubyteFromClampf(floatFromFixed(c));
}

/**
 * Converts floating point value into integer value.
 * @param f Floating point value.
 * @return Resulting integer value.
 */
static inline GLint intFromFloat(GLfloat f)
{
	return (GLint)f;
}

/**
 * Converts fixed point value into integer value.
 * @param x Fixed point value.
 * @return Resulting integer value.
 */
static inline GLint intFromFixed(GLfixed x)
{
	return x >> 16;
}

/**
 * Converts clamped floating point value into integer value.
 * @param c Clamped floating point value.
 * @return Resulting integer value.
 */
static inline GLint intFromClampf(GLclampf c)
{
	if (c < 0.5f)
		return 0;
	return 1;
}

/**
 * Converts clamped fixed point value into integer value.
 * @param c Clamped fixed point value.
 * @return Resulting integer value.
 */
static inline GLint intFromClampx(GLclampx c)
{
	return intFromClampf(floatFromFixed(c));
}

/**
 * Converts floating point value into fixed point value.
 * @param f Floating point value.
 * @return Resulting fixed point value.
 */
static inline GLfixed fixedFromFloat(GLfloat f)
{
	return f * (1 << 16);
}

/**
 * Converts integer value into fixed point value.
 * @param i Floating point value.
 * @return Resulting fixed point value.
 */
static inline GLfixed fixedFromInt(GLint i)
{
	return i << 16;
}

/**
 * Converts boolean value into fixed point value.
 * @param b Boolean value.
 * @return Resulting fixed point value.
 */
static inline GLfixed fixedFromBool(GLboolean b)
{
	return (!!b) << 16;
}

/**
 * Converts floating point value into boolean value.
 * @param f Floating point value.
 * @return Resulting boolean value.
 */
static inline GLboolean boolFromFloat(GLfloat f)
{
	return f != 0;
}

/**
 * Converts fixed point value into boolean value.
 * @param x Fixed point value.
 * @return Resulting boolean value.
 */
static inline GLboolean boolFromFixed(GLfixed x)
{
	return !!x;
}

/**
 * Converts integer value into boolean value.
 * @param i Integer value.
 * @return Resulting boolean value.
 */
static inline GLboolean boolFromInt(GLint i)
{
	return !!i;
}

/**
 * Converts normalized floating point value into integer value.
 * @param f Normalized floating point value.
 * @return Resulting integer value.
 */
static inline GLint intFromNormalized(GLfloat f)
{
        return (double)f*2147483648.0f - 0.5f*f - 0.5f;
}

/**
 * Rounds floating point value to nearest integer value.
 * @param f Floating point value.
 * @return Nearest integer value.
 */
static inline GLint round(GLfloat f)
{
	return (f > 0.0f) ? floor(f + 0.5f) : ceil(f - 0.5f);
}

#endif // _LIBSGL_TYPES_H_
