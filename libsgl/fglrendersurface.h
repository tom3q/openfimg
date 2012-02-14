/*
 * libsgl/fglrendersurface.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011-2012 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifndef _FGLRENDERSURFACE_H_
#define _FGLRENDERSURFACE_H_

#include "fglsurface.h"
#include "glesFramebuffer.h"

/*
 * Render surface base class
 */

class FGLRenderSurface {
	enum {
		TERMINATED	= 0x80000000,
		MAGIC		= 0x524c4746 /* FGLR */
	};

	uint32_t	magic;
	uint32_t	flags;

protected:
	FGLSurface	*color;
	FGLSurface	*depth;
	uint32_t	colorFormat;
	uint32_t	depthFormat;
	uint32_t	width;
	uint32_t	height;

public:
	EGLDisplay	dpy;
	uint32_t	config;
	EGLContext	ctx;

	FGLRenderSurface(EGLDisplay dpy, uint32_t config,
				uint32_t colorFormat, uint32_t depthFormat) :
		magic(MAGIC),
		flags(0),
		color(0),
		depth(0),
		colorFormat(colorFormat),
		depthFormat(depthFormat),
		dpy(dpy),
		config(config),
		ctx(0) {}

	virtual ~FGLRenderSurface()
	{
		magic = 0;
		delete depth;
		delete color;
	}

	virtual bool bindDrawSurface(FGLContext *gl)
	{
		fglSetColorBuffer(gl, color, width, height, colorFormat);
		fglSetDepthStencilBuffer(gl, depth, width, height, depthFormat);

		return EGL_TRUE;
	}

	virtual EGLint getHorizontalResolution() const
	{
		return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
	}

	virtual EGLint getVerticalResolution() const
	{
		return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
	}

	virtual EGLint getRefreshRate() const
	{
		return (60 * EGL_DISPLAY_SCALING);
	}

	virtual bool setSwapRectangle(EGLint l,
						EGLint t, EGLint w, EGLint h)
	{
		return EGL_FALSE;
	}

	virtual bool connect() { return EGL_TRUE; }
	virtual void disconnect() {}
	virtual EGLint getSwapBehavior() const  { return EGL_BUFFER_PRESERVED; }
	virtual bool swapBuffers()  { return EGL_FALSE; }
	virtual EGLClientBuffer getRenderBuffer() const { return 0; }

	virtual bool initCheck() const = 0;

	bool isValid() const
	{
		if (magic != MAGIC)
			LOGE("invalid EGLSurface (%p)", this);
		return magic == MAGIC;
	}

	void terminate()
	{
		flags |= TERMINATED;
	}

	bool isTerminated() const
	{
		return flags & TERMINATED;
	}

	uint32_t getDepthFormat() const { return depthFormat; }
	uint32_t getColorFormat() const { return colorFormat; }
	uint32_t getWidth() const { return width; }
	uint32_t getHeight() const { return height; }
};

#endif /* _FGLRENDERSURFACE_H_ */
