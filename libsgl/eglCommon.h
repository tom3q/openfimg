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

struct FGLPixelFormatInfo {
	int bpp;
	int red;
	int green;
	int blue;
	int alpha;

	FGLPixelFormatInfo() : bpp(0), red(0), green(0), blue(0), alpha(0) {}
};

#define EGL_ERR_DEBUG

#ifdef EGL_ERR_DEBUG
#define setError(a) ( \
	LOGD("EGL error %s in %s in line %d", #a, __func__, __LINE__), \
	fglEGLSetError(a))
#else
#define setError fglEGLSetError
#endif

extern void fglEGLSetError(EGLint error);
extern EGLBoolean fglEGLValidateDisplay(EGLDisplay dpy);

extern const FGLConfigs gPlatformConfigs[];
extern const int gPlatformConfigsNum;

extern const FGLExtensionMap gPlatformExtensionMap[];

extern FGLRenderSurface *platformCreateWindowSurface(EGLDisplay dpy,
	EGLConfig config, int32_t depthFormat, EGLNativeWindowType window,
	int32_t pixelFormat);

extern EGLBoolean fglEGLValidatePixelFormat(EGLConfig config,
						FGLPixelFormatInfo *fmt);

#endif
