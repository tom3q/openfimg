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

/**
 * A wrapper class for renderbuffer object binding.
 * The class wraps an FGLObjectBinding object into a class that can be
 * embedded as a member in another class.
 */
class FGLRenderbufferBinding {
	FGLObjectBinding<FGLRenderbuffer, FGLRenderbufferBinding> binding;

public:
	/** Default constructor. */
	FGLRenderbufferBinding() :
		binding(this) {};

	/**
	 * Checks if there is a renderbuffer object bound to this binding.
	 * @return True if there is a renderbuffer object bound, otherwise false.
	 */
	inline bool isBound(void)
	{
		return binding.isBound();
	}

	/**
	 * Binds a renderbuffer object to this binding.
	 * @param o Pointer of FGLObject of renderbuffer object to bind.
	 */
	inline void bind(FGLObject<FGLRenderbuffer, FGLRenderbufferBinding> *o)
	{
		binding.bind(o);
	}

	/**
	 * Returns renderbuffer object bound to this binding.
	 * @return Pointer to bound renderbuffer object.
	 */
	inline FGLRenderbuffer *get(void)
	{
		return binding.get();
	}
};

/** A class representing OpenGL ES renderbuffer object */
struct FGLRenderbuffer : public FGLFramebufferAttachable {
	FGLObject<FGLRenderbuffer, FGLRenderbufferBinding> object;
	unsigned int name;

	/**
	 * Creates a renderbuffer object with given name.
	 * @param name Name of the object.
	 */
	FGLRenderbuffer(unsigned int name) :
		object(this),
		name(name) {}

	/**
	 * Class destructor.
	 * Deletes the backing surface and then destroys the object.
	 */
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
