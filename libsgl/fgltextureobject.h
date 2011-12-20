/**
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

struct FGLTexture {
	/* Memory surface */
	FGLSurface	*surface;
	/* GL state */
	GLint		width;
	GLint		height;
	GLboolean	compressed;
	GLint		maxLevel;
	GLenum		format;
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
	uint32_t	fglFormat;
	uint32_t	bpp;
	bool		convert;
	bool		valid;
	bool		dirty;
	bool		swap;

	FGLTexture() :
		surface(0), compressed(0), maxLevel(0), format(GL_RGB),
		type(GL_UNSIGNED_BYTE), minFilter(GL_NEAREST_MIPMAP_LINEAR),
		magFilter(GL_LINEAR), sWrap(GL_REPEAT), tWrap(GL_REPEAT),
		genMipmap(0), eglImage(0), invReady(false),
		fimg(NULL), valid(false), dirty(false), swap(false)
	{
		fimg = fimgCreateTexture();
		if(fimg == NULL)
			return;

		valid = true;
	}

	~FGLTexture()
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
};

typedef FGLObject<FGLTexture> FGLTextureObject;
typedef FGLObjectBinding<FGLTexture> FGLTextureObjectBinding;

#endif
