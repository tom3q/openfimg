/**
 * libsgl/eglMem.h
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

#ifndef _EGLMEM_H_
#define _EGLMEM_H_

#include <ui/android_native_buffer.h>
#include "fglsurface.h"

struct FGLSurface;

void fglFlushPmemSurface(FGLSurface *s);
int fglCreatePmemSurface(FGLSurface *s, android_native_buffer_t* buffer);
int fglCreatePmemSurface(FGLSurface *s);
void fglDestroyPmemSurface(FGLSurface *s);
unsigned long fglGetBufferPhysicalAddress(android_native_buffer_t *buffer);

#endif
