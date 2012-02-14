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

#ifndef _EGLCOMMON_H_
#define _EGLCOMMON_H_

#include <pthread.h>
#include "common.h"
#include "fglpixelformat.h"

struct FGLRenderSurface;

struct FGLConfigPair {
	EGLint key;
	EGLint value;
};

struct FGLConfigs {
	const FGLConfigPair *array;
	int size;
};

struct FGLExtensionMap {
	const char *const name;
	__eglMustCastToProperFunctionPointerType address;
};

#define EGL_ERR_DEBUG

#ifdef EGL_ERR_DEBUG
#define setError(a) ( \
	LOGD("EGL error %s in %s in line %d", #a, __func__, __LINE__), \
	fglEGLSetError(a))
#else
#define setError fglEGLSetError
#endif

#define FGL_DISPLAY_MAGIC	0x444c4746 /* FGLD */

static inline bool fglEGLValidateDisplay(EGLDisplay dpy)
{
	return (uint32_t)dpy == FGL_DISPLAY_MAGIC;
}

extern void fglEGLSetError(EGLint error);

extern const FGLConfigs gPlatformConfigs[];
extern const int gPlatformConfigsNum;

extern const FGLExtensionMap gPlatformExtensionMap[];

extern FGLRenderSurface *platformCreateWindowSurface(EGLDisplay dpy,
			uint32_t config, uint32_t pixelFormat,
			uint32_t depthFormat, EGLNativeWindowType window);

extern bool fglEGLValidatePixelFormat(uint32_t config, uint32_t fmt);

#endif
