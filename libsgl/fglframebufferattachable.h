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

/**
 * An FGLObject that points to an FGLFramebufferAttachable object and can be
 * bound to an FGLObjectBinding of an FGLFramebuffer object.
 */
typedef FGLObject<FGLFramebufferAttachable, FGLFramebuffer>
						FGLFramebufferAttachableObject;
/**
 * An FGLObjectBinding that points to a FGLFramebuffer object
 * to which an FGLObject of an FGLFramebufferAttachable object can be bound.
 */
typedef FGLObjectBinding<FGLFramebufferAttachable, FGLFramebuffer>
						FGLFramebufferAttachableBinding;

/**
 * A class that represents a framebuffer attachable object,
 * as defined by OpenGL ES Framebuffer Object extension.
 */
struct FGLFramebufferAttachable {
	/** FGLObject that can be bound to FGLFramebuffer */
	FGLFramebufferAttachableObject object;

	/** Surface backing the object. */
	FGLSurface	*surface;

	/** Surface width. */
	GLint		width;
	/** Surface height. */
	GLint		height;
	/** Surface format. */
	GLenum		format;

	/** Pixel format of the object. */
	uint32_t	pixFormat;

	/** Mask of attachment points supported by this object. */
	uint32_t	mask;

	/** Constructor creating an attachable object. */
	FGLFramebufferAttachable() :
		object(this),
		surface(0),
		width(0),
		height(0),
		pixFormat(0),
		mask(0) {}

	/**
	 * Class destructor.
	 * Destroys attachable object and notifies the framebuffer to which it
	 * was bound that it is no longer available.
	 */
	virtual ~FGLFramebufferAttachable()
	{
		markFramebufferDirty();
	}

	/**
	 * Gets name of the object.
	 * @return Name of the object.
	 */
	virtual GLuint getName(void) const
	{
		return 0;
	}

	/**
	 * Gets type of the object.
	 * @return Type of the object (as defined by OpenGL ES specification).
	 */
	virtual GLenum getType(void) const
	{
		return GL_NONE_OES;
	}

	/**
	 * Notifies the framebuffer to which the object is bound that it should
	 * refresh its internal configuration.
	 */
	void markFramebufferDirty(void);
};

#endif /* _FGLFRAMEBUFFERATTACHABLE_H_ */
