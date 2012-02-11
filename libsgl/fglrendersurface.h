/*
 * libsgl/eglCommon.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
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

struct FGLSurface;

struct FGLRenderSurface
{
	enum {
		TERMINATED = 0x80000000,
		MAGIC     = 0x31415265
	};

	uint32_t	magic;
	uint32_t	flags;
	EGLDisplay	dpy;
	EGLConfig	config;
	EGLContext	ctx;

	FGLRenderSurface(EGLDisplay dpy, EGLConfig config, int32_t pixelFormat,
							int32_t depthFormat);
	virtual 		~FGLRenderSurface();
		bool		isValid() const;
		void		terminate();
		bool		isTerminated() const;
	virtual bool		initCheck() const = 0;

	virtual EGLBoolean	bindDrawSurface(FGLContext *gl);
	virtual EGLBoolean	connect() { return EGL_TRUE; }
	virtual void		disconnect() {}
	virtual EGLint		getWidth() const = 0;
	virtual EGLint		getHeight() const = 0;
	virtual int32_t		getDepthFormat() { return depthFormat; }

	virtual EGLint		getHorizontalResolution() const;
	virtual EGLint		getVerticalResolution() const;
	virtual EGLint		getRefreshRate() const;
	virtual EGLint		getSwapBehavior() const;
	virtual EGLBoolean	swapBuffers();
	virtual EGLBoolean	setSwapRectangle(EGLint l, EGLint t, EGLint w, EGLint h);
	virtual EGLClientBuffer	getRenderBuffer() const;
protected:
	FGLSurface		*color;
	FGLSurface              *depth;
	int32_t			depthFormat;
	int			width;
	int			height;
	int32_t			format;
};

#endif /* _FGLRENDERSURFACE_H_ */
