/*
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "types.h"

class FGLSurface {
public:
	intptr_t	paddr;
	void		*vaddr;
	size_t		size;

			FGLSurface() : paddr(0), vaddr(0), size(0) {};
			FGLSurface(unsigned long p, void *v, unsigned long s) :
				paddr(p), vaddr(v), size(s) {};
	virtual		~FGLSurface() {};

	virtual void	flush(void) = 0;
	virtual int	lock(int usage = 0) = 0;
	virtual int	unlock(void) = 0;

	virtual bool	isValid(void) = 0;
};

class FGLLocalSurface : public FGLSurface {
	int		fd;
public:
			FGLLocalSurface(unsigned long size);
	virtual		~FGLLocalSurface();

	virtual void	flush(void);
	virtual int	lock(int usage = 0);
	virtual int	unlock(void);

	virtual bool	isValid(void) { return fd >= 0; };
};

class FGLExternalSurface : public FGLSurface {
public:
			FGLExternalSurface(void *v, intptr_t p, size_t s);
	virtual		~FGLExternalSurface();

	virtual void	flush(void);
	virtual int	lock(int usage = 0);
	virtual int	unlock(void);

	virtual bool	isValid(void) { return true; };
};

#endif
