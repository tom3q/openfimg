/*
 * libsgl/fglframebufferattachable.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
 * 		2011 by Jordi Santiago Provencio < >
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

#ifndef _FGLFRAMEBUFFERATTACHABLE_H_
#define _FGLFRAMEBUFFERATTACHABLE_H_

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "fglsurface.h"
#include "fglobject.h"

class FGLFramebuffer;
struct FGLFramebufferAttachable;

typedef FGLObject<FGLFramebufferAttachable, FGLFramebuffer>
						FGLFramebufferAttachableObject;
typedef FGLObjectBinding<FGLFramebufferAttachable, FGLFramebuffer>
						FGLFramebufferAttachableBinding;

struct FGLFramebufferAttachable {
	FGLFramebufferAttachableObject object;

	FGLSurface	*surface;

	GLint		width;
	GLint		height;
	GLenum		format;

	uint32_t	pixFormat;

	uint32_t	mask;

	FGLFramebufferAttachable() :
		object(this),
		surface(0),
		width(0),
		height(0),
		pixFormat(0),
		mask(0) {}

	virtual ~FGLFramebufferAttachable()
	{
		markFramebufferDirty();
	}

	virtual GLuint getName(void) const
	{
		return 0;
	}

	virtual GLenum getType(void) const
	{
		return GL_NONE_OES;
	}

	void markFramebufferDirty(void);
};

#endif /* _FGLFRAMEBUFFERATTACHABLE_H_ */
