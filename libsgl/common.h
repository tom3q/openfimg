/**
 * libsgl/common.h
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

#ifndef _LIBSGL_COMMON_H_
#define _LIBSGL_COMMON_H_

#define FGL_MAX_TEXTURE_UNITS	2
#define FGL_MAX_TEXTURE_OBJECTS 1024
#define FGL_MAX_MIPMAP_LEVEL 11

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

template<typename T>
static inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
static inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

void fglSetClipper(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

struct FGLSurface;

void fglFlushPmemSurface(FGLSurface *s);
int fglCreatePmemSurface(FGLSurface *s);
void fglDestroyPmemSurface(FGLSurface *s);

#endif // _LIBSGL_COMMON_H_
