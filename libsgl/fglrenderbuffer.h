/*
 * libsgl/fglrenderbuffer.h
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

#ifndef _FGLRENDERBUFFER_H_
#define _FGLRENDERBUFFER_H_

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "fglobject.h"
#include "fglframebufferattachable.h"

struct FGLRenderbuffer;

class FGLRenderbufferBinding {
	FGLObjectBinding<FGLRenderbuffer, FGLRenderbufferBinding> binding;

public:
	FGLRenderbufferBinding() :
		binding(this) {};

	inline bool isBound(void)
	{
		return binding.isBound();
	}

	inline void bind(FGLObject<FGLRenderbuffer, FGLRenderbufferBinding> *o)
	{
		binding.bind(o);
	}

	inline FGLRenderbuffer *get(void)
	{
		return binding.get();
	}
};

struct FGLRenderbuffer : public FGLFramebufferAttachable {
	FGLObject<FGLRenderbuffer, FGLRenderbufferBinding> object;
	unsigned int name;

	FGLRenderbuffer(unsigned int name) :
		object(this),
		name(name) {}

	virtual ~FGLRenderbuffer()
	{
		delete surface;
	}

	virtual GLenum getType(void) const
	{
		return GL_RENDERBUFFER_OES;
	}

	virtual GLuint getName(void) const
	{
		return name;
	}
};

#endif /* _FGLRENDERBUFFER_H_ */
