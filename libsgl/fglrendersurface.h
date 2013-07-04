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

/**
 * Render surface base class.
 * Render surfaces are used as framebuffers for rendering operations.
 */
class FGLRenderSurface {
	enum {
		TERMINATED	= 0x80000000,
		MAGIC		= 0x524c4746 /* FGLR */
	};

	uint32_t	magic;
	uint32_t	flags;

protected:
	/** Surface backing color buffer. */
	FGLSurface	*color;
	/** Surface backing depth buffer. */
	FGLSurface	*depth;
	/** Format of color buffer. */
	uint32_t	colorFormat;
	/** Format of depth buffer. */
	uint32_t	depthFormat;
	/** Render surface width. */
	uint32_t	width;
	/** Render surface height. */
	uint32_t	height;

public:
	/** EGL display owning this render surface. */
	EGLDisplay	dpy;
	/** EGL configuration used by this render surface. */
	uint32_t	config;
	/** EGL context bound currently to this render surface. */
	FGLContext	*ctx;

	/**
	 * Constructs render surface base.
	 * @param dpy EGL display owning the surface.
	 * @param config EGL configuration used for this surface.
	 * @param colorFormat Color format.
	 * @param depthFormat Depth format.
	 */
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

	/**
	 * Render surface destructor.
	 * Also destroys backing surfaces.
	 */
	virtual ~FGLRenderSurface()
	{
		magic = 0;
	}

	/**
	 * Allocates buffers backing the surface.
	 * @param gl Rendering context to allocate on behalf of.
	 * @return EGL_TRUE on success, EGL_FALSE on failure.
	 */
	virtual bool allocate(FGLContext *gl) = 0;

	/**
	 * Binds render surface to given rendering context.
	 * @param gl Rendering context.
	 * @return EGL_TRUE on success, EGL_FALSE on failure.
	 */
	bool bindContext(FGLContext *gl)
	{
		if (ctx) {
			LOGE("tried to bind context to an already bound surface");
			return EGL_FALSE;
		}

		if (!color && !allocate(gl))
			return EGL_FALSE;

		if (color && !color->bindContext(gl))
			return EGL_FALSE;
		if (depth && !depth->bindContext(gl))
			return EGL_FALSE;

		fglSetColorBuffer(gl, color, width, height, colorFormat);
		fglSetDepthStencilBuffer(gl, depth, width, height, depthFormat);

		ctx = gl;
		return EGL_TRUE;
	}

	/**
	 * Unbinds render surface from rendering context bound to it.
	 */
	void unbindContext(void)
	{
		if (!ctx)
			return;

		if (color)
			color->unbindContext();
		if (depth)
			depth->unbindContext();

		ctx = 0;
	}

	/**
	 * Gets horizontal resolution of the surface.
	 * @return Horizontal resolution (as specified by EGL specification).
	 */
	virtual EGLint getHorizontalResolution() const
	{
		return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
	}

	/**
	 * Gets vertical resolution of the surface.
	 * @return Vertical resolution (as specified by EGL specification).
	 */
	virtual EGLint getVerticalResolution() const
	{
		return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
	}

	/**
	 * Gets refresh rate of the surface.
	 * @return Refresh rate (as specified by EGL specification).
	 */
	virtual EGLint getRefreshRate() const
	{
		return (60 * EGL_DISPLAY_SCALING);
	}

	/**
	 * Sets swap rectangle of the surface.
	 * @param l Left-most X coordinate of swap rectangle.
	 * @param t Top-most Y coordinate of swap rectangle.
	 * @param w Width of swap rectangle.
	 * @param h Height of swap rectangle.
	 * @return EGL_TRUE on success, EGL_FALSE on failure.
	 */
	virtual bool setSwapRectangle(EGLint l,
						EGLint t, EGLint w, EGLint h)
	{
		return EGL_FALSE;
	}

	/**
	 * Determines buffer swap behavior of the surface.
	 * @return Swap behavior value as specified by EGL specification.
	 */
	virtual EGLint getSwapBehavior() const  { return EGL_BUFFER_PRESERVED; }
	/**
	 * Posts current framebuffer for displaying and gets next framebuffer
	 * ready for rendering.
	 * @return EGL_TRUE on success, EGL_FALSE on failure.
	 */
	virtual bool swapBuffers()  { return EGL_FALSE; }
	/**
	 * Gets native buffer backing this surface.
	 * @return Handle to native buffer.
	 */
	virtual EGLClientBuffer getRenderBuffer() const { return 0; }

	/**
	 * Checks initialization status of the surface.
	 * @return True if the surface is initialized, otherwise false.
	 */
	virtual bool initCheck() const = 0;

	/**
	 * Checks validity of the surface.
	 * @return True if the surface is valid, otherwise false.
	 */
	bool isValid() const
	{
		if (magic != MAGIC)
			LOGE("invalid EGLSurface (%p)", this);
		return magic == MAGIC;
	}

	/**
	 * Marks the surface for termination.
	 * The surface will be destroyed when no context is bound to it.
	 */
	void terminate()
	{
		flags |= TERMINATED;
	}

	/**
	 * Checks if the surface is marked for termination.
	 * @return True if the surface is marked for termination,
	 * otherwise false.
	 */
	bool isTerminated() const
	{
		return flags & TERMINATED;
	}

	/**
	 * Gets depth format of the surface.
	 * @return Depth format.
	 */
	uint32_t getDepthFormat() const { return depthFormat; }
	/**
	 * Gets color format of the surface.
	 * @return Color format.
	 */
	uint32_t getColorFormat() const { return colorFormat; }
	/**
	 * Gets width of the surface.
	 * @return Width.
	 */
	uint32_t getWidth() const { return width; }
	/**
	 * Gets height of the surface.
	 * @return Height.
	 */
	uint32_t getHeight() const { return height; }
};

#endif /* _FGLRENDERSURFACE_H_ */
