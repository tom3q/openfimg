/*
 * libsgl/fgltextureobject.h
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

#ifndef _LIBSGL_FGLTEXTUREOBJECT_
#define _LIBSGL_FGLTEXTUREOBJECT_

#include "fglsurface.h"
#include "fglobject.h"
#include "fglimage.h"
#include "fglframebufferattachable.h"

struct FGLTexture;
struct FGLTextureState;

/**
 * An FGLObject that points to an FGLTexture object and can be bound
 * to an FGLObjectBinding of an FGLTextureState object.
 */
typedef FGLObject<FGLTexture, FGLTextureState> FGLTextureObject;
/**
 * An FGLObjectBinding that points to a FGLTextureState object to which
 * an FGLObject of an FGLTexture object can be bound.
 */
typedef FGLObjectBinding<FGLTexture, FGLTextureState> FGLTextureObjectBinding;

/** A class representing OpenGL ES texture object. */
struct FGLTexture : public FGLFramebufferAttachable {
	/** FGLObject that can be bound to FGLTextureState */
	FGLTextureObject object;

	/** Name of the texture object. */
	unsigned int	name;
	/** Memory surface */
	using FGLFramebufferAttachable::surface;
	/** Texture width */
	using FGLFramebufferAttachable::width;
	/** Texture height */
	using FGLFramebufferAttachable::height;
	/** Flag indicating that the texture is compressed. */
	GLboolean	compressed;
	/** Highest mipmap level supported by this texture. */
	GLint		maxLevel;
	/** OpenGL ES image format. */
	using FGLFramebufferAttachable::format;
	/** OpenGL ES image type. */
	GLenum		type;
	/** Minification filter. */
	GLenum		minFilter;
	/** Magnification filter. */
	GLenum		magFilter;
	/** S wrap mode. */
	GLenum		sWrap;
	/** T wrap mode. */
	GLenum		tWrap;
	/** Flag indicating whether to automatically generate mipmaps. */
	GLboolean	genMipmap;
	/** Texture crop rectangle. Used for glDrawTex*. */
	GLint		cropRect[4];
	/** The eglImage backing the texture. */
	FGLImage	*eglImage;
	/** Reciprocal of #width. Used for glDrawTex*. */
	GLfloat		invWidth;
	/** Reciprocal of #height. Used for glDrawTex*. */
	GLfloat		invHeight;
	/** Flag indicating that #invWidth and #invHeight are valid. */
	bool		invReady;
	/** OpenGL ES texture target. */
	GLenum		target;
	/** libfimg texture object backing this texture. */
	fimgTexture	*fimg;
	/** Flag indicating that this texture needs format conversion. */
	bool		convert;
	/** Flag indicating that this texture is valid. */
	bool		valid;
	/**
	 * Flag indicating that this texture has been modified since
	 * last rendering using it.
	 */
	bool		dirty;

	/**
	 * Creates texture object.
	 * @param name Texture object name.
	 */
	FGLTexture(unsigned int name = 0) :
		object(this),
		name(name),
		compressed(0),
		maxLevel(0),
		type(GL_UNSIGNED_BYTE),
		minFilter(GL_NEAREST_MIPMAP_LINEAR),
		magFilter(GL_LINEAR),
		sWrap(GL_REPEAT),
		tWrap(GL_REPEAT),
		genMipmap(0),
		eglImage(0),
		invReady(false),
		fimg(NULL),
		valid(false),
		dirty(false)
	{
		fimg = fimgCreateTexture();
		if(fimg == NULL)
			return;

		valid = true;
	}

	/**
	 * Destroys texture object.
	 * If eglImage is backing the texture it is disconnected, otherwise
	 * the backing surface is deleted. The backing libfimg texture object
	 * is also destroyed.
	 */
	virtual ~FGLTexture()
	{
		if(!isValid())
			return;

		if (eglImage)
			eglImage->disconnect();
		else
			delete surface;

		fimgDestroyTexture(fimg);
	}

	/**
	 * Checks whether the texture is valid.
	 * @return True if the texture is valid, otherwise false.
	 */
	inline bool isValid(void)
	{
		return valid;
	}

	/**
	 * Checks whether the texture is complete.
	 * A texture is considered complete if it has backing surface.
	 * @return True if the texture is complete, otherwise false.
	 */
	inline bool isComplete(void)
	{
		return (surface != 0);
	}

	virtual GLenum getType(void) const
	{
		return GL_TEXTURE;
	}

	virtual GLuint getName(void) const
	{
		return name;
	}
};

#endif
