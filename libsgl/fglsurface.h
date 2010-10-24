/**
 * libsgl/fglsurface.h
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

#ifndef _LIBSGL_FGLSURFACE_
#define _LIBSGL_FGLSURFACE_

#include "types.h"

struct FGLSurface {
	int		fd;	// buffer descriptor
	FGLint		width;	// width in pixels
	FGLint		height;	// height in pixels
	FGLuint		stride;	// stride in pixels
	FGLuint		size;
	void		*vaddr;	// pointer to the bits
	unsigned long	paddr;	// physical address
	FGLint		format;	// pixel format

	FGLSurface() :
		fd(-1), vaddr(NULL) {};

	inline bool isValid(void)
	{
		return vaddr != NULL;
	}

	inline bool isPMEM(void)
	{
		return fd != -1;
	}
};

#endif
