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

typedef FGLObject<FGLTexture, FGLTextureState> FGLTextureObject;
typedef FGLObjectBinding<FGLTexture, FGLTextureState> FGLTextureObjectBinding;

struct FGLTexture : public FGLFramebufferAttachable {
	FGLTextureObject object;

	unsigned int	name;
	/* Memory surface */
	using FGLFramebufferAttachable::surface;
	/* GL state */
	using FGLFramebufferAttachable::width;
	using FGLFramebufferAttachable::height;
	GLboolean	compressed;
	GLint		maxLevel;
	using FGLFramebufferAttachable::format;
	GLenum		type;
	GLenum		minFilter;
	GLenum		magFilter;
	GLenum		sWrap;
	GLenum		tWrap;
	GLboolean	genMipmap;
	GLint		cropRect[4];
	FGLImage	*eglImage;
	GLfloat		invWidth;
	GLfloat		invHeight;
	bool		invReady;
	GLenum		target;
	/* HW state */
	fimgTexture	*fimg;
	bool		convert;
	bool		valid;
	bool		dirty;

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

	inline bool isValid(void)
	{
		return valid;
	}

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
