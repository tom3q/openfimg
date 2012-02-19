/*
 * libsgl/eglBase.cpp
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) EGL IMPLEMENTATION
 *
 * Copyrights:	2010-2012 by Tomasz Figa < tomasz.figa at gmail.com >
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty off
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "platform.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include "eglCommon.h"
#include "fglrendersurface.h"
#include "common.h"
#include "types.h"
#include "state.h"
#include "libfimg/fimg.h"
#include "fglsurface.h"
#include "glesFramebuffer.h"

#define FGL_EGL_MAJOR		1
#define FGL_EGL_MINOR		4

static const char *const gVendorString     = "OpenFIMG";
static const char *const gVersionString    = "1.4 pre-alpha";
static const char *const gClientApisString = "OpenGL_ES";
static const char *const gExtensionsString = "" PLATFORM_EXTENSIONS_STRING;

#ifndef PLATFORM_HAS_FAST_TLS
pthread_key_t eglContextKey = -1;
#endif

/*
 * Display
 */

struct FGLDisplay {
	EGLBoolean initialized;
	pthread_mutex_t lock;

	FGLDisplay() :
		initialized(0)
	{
		pthread_mutex_init(&lock, NULL);
	}
};

static FGLDisplay display;

static inline EGLBoolean fglIsDisplayInitialized(EGLDisplay dpy)
{
	EGLBoolean ret;

	pthread_mutex_lock(&display.lock);
	ret = display.initialized;
	pthread_mutex_unlock(&display.lock);

	return ret;
}

static pthread_mutex_t eglErrorKeyMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t eglErrorKey = (pthread_key_t)-1;

/*
 * Error handling
 */

EGLAPI EGLint EGLAPIENTRY eglGetError(void)
{
	if(unlikely(eglErrorKey == (pthread_key_t)-1))
		return EGL_SUCCESS;

	EGLint error = (EGLint)pthread_getspecific(eglErrorKey);
	pthread_setspecific(eglErrorKey, (void *)EGL_SUCCESS);
	return error;
}

void fglEGLSetError(EGLint error)
{
	if(unlikely(eglErrorKey == (pthread_key_t)-1)) {
		pthread_mutex_lock(&eglErrorKeyMutex);
		if(eglErrorKey == (pthread_key_t)-1)
			pthread_key_create(&eglErrorKey, NULL);
		pthread_mutex_unlock(&eglErrorKeyMutex);
	}

	pthread_setspecific(eglErrorKey, (void *)error);
}

/*
 * Initialization
 */

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
	if(display_id != EGL_DEFAULT_DISPLAY)
		return EGL_NO_DISPLAY;

	return (EGLDisplay)FGL_DISPLAY_MAGIC;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy,
						EGLint *major, EGLint *minor)
{
	EGLBoolean ret = EGL_TRUE;

	if(!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if(major != NULL)
		*major = FGL_EGL_MAJOR;

	if(minor != NULL)
		*minor = FGL_EGL_MINOR;

	pthread_mutex_lock(&display.lock);

	if(likely(display.initialized))
		goto finish;

#ifndef PLATFORM_HAS_FAST_TLS
	pthread_key_create(&eglContextKey, NULL);
#endif

	display.initialized = EGL_TRUE;

finish:
	pthread_mutex_unlock(&display.lock);
	return ret;
}

/*
 * FIXME:
 * Keep a list of all allocated contexts and surfaces and delete them
 * or mark for deletion here.
 */
EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	if(!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	pthread_mutex_lock(&display.lock);

	if(unlikely(!display.initialized))
		goto finish;

	display.initialized = EGL_FALSE;

finish:
	pthread_mutex_unlock(&display.lock);
	return EGL_TRUE;
}

EGLAPI const char *EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	if(!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return NULL;
	}

	if(!fglIsDisplayInitialized(dpy)) {
		setError(EGL_NOT_INITIALIZED);
		return NULL;
	}

	switch(name) {
	case EGL_CLIENT_APIS:
		return gClientApisString;
	case EGL_EXTENSIONS:
		return gExtensionsString;
	case EGL_VENDOR:
		return gVendorString;
	case EGL_VERSION:
		return gVersionString;
	}

	setError(EGL_BAD_PARAMETER);
	return NULL;
}

/*
 * Configurations
 */

struct FGLConfigMatcher {
	GLint key;
	bool (*match)(GLint reqValue, GLint confValue);

	static bool atLeast(GLint reqValue, GLint confValue)
	{
		return (reqValue == EGL_DONT_CARE) || (confValue >= reqValue);
	}
	static bool exact(GLint reqValue, GLint confValue)
	{
		return (reqValue == EGL_DONT_CARE) || (confValue == reqValue);
	}
	static bool mask(GLint reqValue, GLint confValue)
	{
		return (confValue & reqValue) == reqValue;
	}
	static bool ignore(GLint reqValue, GLint confValue)
	{
		return true;
	}
};

/*
 * In the lists below, attributes names MUST be sorted.
 * Additionally, all configs must be sorted according to
 * the EGL specification.
 */

#define FGL_MAX_VIEWPORT_PIXELS \
				(FGL_MAX_VIEWPORT_DIMS*FGL_MAX_VIEWPORT_DIMS)

static const FGLConfigPair baseConfigAttributes[] = {
	{ EGL_CONFIG_CAVEAT,              EGL_NONE                          },
	{ EGL_LEVEL,                      0                                 },
	{ EGL_MAX_PBUFFER_HEIGHT,         FGL_MAX_VIEWPORT_DIMS             },
	{ EGL_MAX_PBUFFER_PIXELS,         FGL_MAX_VIEWPORT_PIXELS           },
	{ EGL_MAX_PBUFFER_WIDTH,          FGL_MAX_VIEWPORT_DIMS             },
	{ EGL_NATIVE_RENDERABLE,          EGL_TRUE                          },
	{ EGL_NATIVE_VISUAL_ID,           0                                 },
	{ EGL_NATIVE_VISUAL_TYPE,         0                                 },
	{ EGL_SAMPLES,                    0                                 },
	{ EGL_SAMPLE_BUFFERS,             0                                 },
	{ EGL_SURFACE_TYPE,               EGL_WINDOW_BIT|EGL_PBUFFER_BIT    },
	{ EGL_TRANSPARENT_TYPE,           EGL_NONE                          },
	{ EGL_TRANSPARENT_BLUE_VALUE,     0                                 },
	{ EGL_TRANSPARENT_GREEN_VALUE,    0                                 },
	{ EGL_TRANSPARENT_RED_VALUE,      0                                 },
	{ EGL_BIND_TO_TEXTURE_RGB,        EGL_FALSE                         },
	{ EGL_BIND_TO_TEXTURE_RGBA,       EGL_FALSE                         },
	{ EGL_MIN_SWAP_INTERVAL,          1                                 },
	{ EGL_MAX_SWAP_INTERVAL,          1                                 },
	{ EGL_LUMINANCE_SIZE,             0                                 },
	{ EGL_ALPHA_MASK_SIZE,            0                                 },
	{ EGL_COLOR_BUFFER_TYPE,          EGL_RGB_BUFFER                    },
	{ EGL_RENDERABLE_TYPE,            EGL_OPENGL_ES_BIT                 },
	{ EGL_CONFORMANT,                 0                                 }
};

/* Configs are platform specific. See egl[Platform].cpp. */

static const FGLConfigMatcher gConfigManagement[] = {
	{ EGL_BUFFER_SIZE,                FGLConfigMatcher::atLeast },
	{ EGL_ALPHA_SIZE,                 FGLConfigMatcher::atLeast },
	{ EGL_BLUE_SIZE,                  FGLConfigMatcher::atLeast },
	{ EGL_GREEN_SIZE,                 FGLConfigMatcher::atLeast },
	{ EGL_RED_SIZE,                   FGLConfigMatcher::atLeast },
	{ EGL_DEPTH_SIZE,                 FGLConfigMatcher::atLeast },
	{ EGL_STENCIL_SIZE,               FGLConfigMatcher::atLeast },
	{ EGL_CONFIG_CAVEAT,              FGLConfigMatcher::exact   },
	{ EGL_CONFIG_ID,                  FGLConfigMatcher::exact   },
	{ EGL_LEVEL,                      FGLConfigMatcher::exact   },
	{ EGL_MAX_PBUFFER_HEIGHT,         FGLConfigMatcher::ignore  },
	{ EGL_MAX_PBUFFER_PIXELS,         FGLConfigMatcher::ignore  },
	{ EGL_MAX_PBUFFER_WIDTH,          FGLConfigMatcher::ignore  },
	{ EGL_NATIVE_RENDERABLE,          FGLConfigMatcher::exact   },
	{ EGL_NATIVE_VISUAL_ID,           FGLConfigMatcher::ignore  },
	{ EGL_NATIVE_VISUAL_TYPE,         FGLConfigMatcher::exact   },
	{ EGL_SAMPLES,                    FGLConfigMatcher::exact   },
	{ EGL_SAMPLE_BUFFERS,             FGLConfigMatcher::exact   },
	{ EGL_SURFACE_TYPE,               FGLConfigMatcher::mask    },
	{ EGL_TRANSPARENT_TYPE,           FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_BLUE_VALUE,     FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_GREEN_VALUE,    FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_RED_VALUE,      FGLConfigMatcher::exact   },
	{ EGL_BIND_TO_TEXTURE_RGB,        FGLConfigMatcher::exact   },
	{ EGL_BIND_TO_TEXTURE_RGBA,       FGLConfigMatcher::exact   },
	{ EGL_MIN_SWAP_INTERVAL,          FGLConfigMatcher::exact   },
	{ EGL_MAX_SWAP_INTERVAL,          FGLConfigMatcher::exact   },
	{ EGL_LUMINANCE_SIZE,             FGLConfigMatcher::atLeast },
	{ EGL_ALPHA_MASK_SIZE,            FGLConfigMatcher::atLeast },
	{ EGL_COLOR_BUFFER_TYPE,          FGLConfigMatcher::exact   },
	{ EGL_RENDERABLE_TYPE,            FGLConfigMatcher::mask    },
	{ EGL_CONFORMANT,                 FGLConfigMatcher::mask    }
};

static const FGLConfigPair defaultConfigAttributes[] = {
/*
 * attributes that are not specified are simply ignored, if a particular
 * one needs not be ignored, it must be specified here, eg:
 * { EGL_SURFACE_TYPE, EGL_WINDOW_BIT },
 */
};

/*
 * Internal configuration management
 */

static bool fglGetConfigAttrib(uint32_t config, EGLint attribute, EGLint *value)
{
	size_t numConfigs = gPlatformConfigsNum;

	if (config >= numConfigs) {
		setError(EGL_BAD_CONFIG);
		return false;
	}

	if (attribute == EGL_CONFIG_ID) {
		*value = config;
		return true;
	}

	int attrIndex = binarySearch(gPlatformConfigs[config].array, 0,
				gPlatformConfigs[config].size - 1, attribute);

	if (attrIndex >= 0) {
		*value = gPlatformConfigs[config].array[attrIndex].value;
		return true;
	}

	attrIndex = binarySearch(baseConfigAttributes, 0,
				NELEM(baseConfigAttributes) - 1, attribute);

	if (attrIndex >= 0) {
		*value = baseConfigAttributes[attrIndex].value;
		return true;
	}

	setError(EGL_BAD_ATTRIBUTE);
	return false;
}

static bool fglGetConfigFormatInfo(uint32_t config,
				uint32_t *pixelFormat, uint32_t *depthFormat)
{
	EGLint color, alpha, depth, stencil;

	if (!fglGetConfigAttrib(config, EGL_BUFFER_SIZE, &color))
		return false;

	if (!fglGetConfigAttrib(config, EGL_ALPHA_SIZE, &alpha))
		return false;

	if (!fglGetConfigAttrib(config, EGL_DEPTH_SIZE, &depth))
		return false;

	if (!fglGetConfigAttrib(config, EGL_STENCIL_SIZE, &stencil))
		return false;

	switch (color) {
	case 32:
		if (alpha)
			*pixelFormat = FGL_PIXFMT_ARGB8888;
		else
			*pixelFormat = FGL_PIXFMT_XRGB8888;
		break;
	case 16:
	default:
		*pixelFormat = FGL_PIXFMT_RGB565;
		break;
	}

	*depthFormat = (stencil << 8) | depth;

	return true;
}

bool fglEGLValidatePixelFormat(uint32_t config, uint32_t fmt)
{
	EGLint bpp, red, green, blue, alpha;

	if (!fglGetConfigAttrib(config, EGL_BUFFER_SIZE, &bpp))
		return false;

	if (!fglGetConfigAttrib(config, EGL_RED_SIZE, &red))
		return false;

	if (!fglGetConfigAttrib(config, EGL_GREEN_SIZE, &green))
		return false;

	if (!fglGetConfigAttrib(config, EGL_BLUE_SIZE, &blue))
		return false;

	if (!fglGetConfigAttrib(config, EGL_ALPHA_SIZE, &alpha))
		return false;

	const FGLPixelFormat *pix = FGLPixelFormat::get(fmt);
	if (8*pix->pixelSize != (unsigned)bpp)
		return false;

	if (pix->comp[FGL_COMP_RED].size != red)
		return false;

	if (pix->comp[FGL_COMP_GREEN].size != green)
		return false;

	if (pix->comp[FGL_COMP_BLUE].size != blue)
		return false;

	if (pix->comp[FGL_COMP_ALPHA].size != alpha)
		return false;

	return true;
}

static int fglIsAttributeMatching(int i, EGLint attr, EGLint val)
{
	/* look for the attribute in all of our configs */
	const FGLConfigPair *configFound = gPlatformConfigs[i].array;
	int index = binarySearch(gPlatformConfigs[i].array, 0,
					gPlatformConfigs[i].size - 1, attr);
	if (index < 0) {
		/* look in base attributes */
		configFound = baseConfigAttributes;
		index = binarySearch(baseConfigAttributes, 0,
					NELEM(baseConfigAttributes) - 1, attr);
	}
	if (index < 0)
		/* error, this attribute doesn't exist */
		return 0;

	/* attribute found, check if this config could match */
	int cfgMgtIndex = binarySearch(gConfigManagement, 0,
					NELEM(gConfigManagement) - 1, attr);
	if (cfgMgtIndex < 0)
		/* attribute not found. this should NEVER happen. */
		return 0;

	return gConfigManagement[cfgMgtIndex].match(val,
						configFound[index].value);
}

/*
 * EGL configuration queries
 */

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
			EGLint config_size, EGLint *num_config)
{
	if (unlikely(!fglEGLValidateDisplay(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (unlikely(!fglIsDisplayInitialized(dpy))) {
		setError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}

	if (unlikely(!num_config)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	if (!configs) {
		*num_config = gPlatformConfigsNum;
		return EGL_TRUE;
	}

	EGLint num = min(gPlatformConfigsNum, config_size);
	for(EGLint i = 0; i < num; ++i)
		*(configs++) = (EGLConfig)i;

	*num_config = num;
	return EGL_TRUE;
}

static const EGLint dummyAttribList = EGL_NONE;

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy,
				const EGLint *attrib_list, EGLConfig *configs,
				EGLint config_size, EGLint *num_config)
{
	if (unlikely(!fglEGLValidateDisplay(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (unlikely(!num_config)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	if (unlikely(!attrib_list)) {
		/*
		 * A NULL attrib_list should be treated as though it was
		 * an empty one (terminated with EGL_NONE) as defined in
		 * section 3.4.1 "Querying Configurations" in the EGL
		 * specification.
		 */
		attrib_list = &dummyAttribList;
	}

	uint32_t numAttributes = 0;
	uint32_t numConfigs = gPlatformConfigsNum;
	uint32_t possibleMatch = BIT_MASK(numConfigs);

	while (possibleMatch && *attrib_list != EGL_NONE) {
		EGLint attr = *(attrib_list++);
		EGLint val  = *(attrib_list++);

		for (uint32_t i = 0; i < numConfigs; ++i) {
			if (!possibleMatch)
				break;

			if (!(possibleMatch & BIT_VAL(i)))
				continue;

			if (!fglIsAttributeMatching(i, attr, val))
				possibleMatch &= ~BIT_VAL(i);
		}

		++numAttributes;
	}

	uint32_t numDefaults = NELEM(defaultConfigAttributes);
	/* now, handle the attributes which have a useful default value */
	for (uint32_t j = 0; j < numDefaults; ++j) {
		if (!possibleMatch)
			break;
		/*
		 * see if this attribute was specified,
		 * if not, apply its default value
		 */
		const FGLConfigPair *array = (const FGLConfigPair *)attrib_list;
		int index = binarySearch(array, 0, numAttributes - 1,
						defaultConfigAttributes[j].key);
		if (index >= 0)
			continue;

		for (uint32_t i = 0; i < numConfigs; ++i) {
			if (!possibleMatch)
				break;

			if (!(possibleMatch & BIT_VAL(i)))
				continue;

			const FGLConfigPair *tmp = &defaultConfigAttributes[i];
			int matched = fglIsAttributeMatching(i,
							tmp->key, tmp->value);
			if (!matched)
				possibleMatch &= ~BIT_VAL(i);
		}
	}

	/* return the configurations found */
	if (!possibleMatch) {
		*num_config = 0;
		return EGL_TRUE;
	}

	uint32_t n = 0;

	if (!configs) {
		while (possibleMatch) {
			n += possibleMatch & 1;
			possibleMatch >>= 1;
		}
		*num_config = n;
		return EGL_TRUE;
	}

	for (uint32_t i = 0; config_size && i < numConfigs; ++i) {
		if (!(possibleMatch & BIT_VAL(i)))
			continue;

		*(configs++) = (EGLConfig)i;
		--config_size;
		++n;
	}

	*num_config = n;
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy,
			EGLConfig config, EGLint attribute, EGLint *value)
{
	if (unlikely(!fglEGLValidateDisplay(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (!fglGetConfigAttrib((uint32_t)config, attribute, value))
		return EGL_FALSE;

	return EGL_TRUE;
}

/*
 * EGL PBuffer surface
 */

class FGLPbufferSurface : public FGLRenderSurface {
public:
	FGLPbufferSurface(EGLDisplay dpy, uint32_t config,
				uint32_t colorFormat, uint32_t depthFormat,
				uint32_t width, uint32_t height) :
		FGLRenderSurface(dpy, config, colorFormat, depthFormat)
	{
		const FGLPixelFormat *fmt = FGLPixelFormat::get(colorFormat);
		unsigned int size = width * height * fmt->pixelSize;

		this->width = width;
		this->height = height;

		color = new FGLLocalSurface(size);
		if (!color || !color->isValid()) {
			setError(EGL_BAD_ALLOC);
			return;
		}

		if (depthFormat) {
			size = width * height * 4;

			depth = new FGLLocalSurface(size);
			if (!depth || !depth->isValid()) {
				setError(EGL_BAD_ALLOC);
				return;
			}
		}
	}

	virtual ~FGLPbufferSurface() {}

	virtual bool initCheck() const
	{
		if (depthFormat && (!depth || !depth->isValid()))
			return false;

		return color && color->isValid();
	}
};

/*
 * EGL surface management
 */

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy,
	EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
	uint32_t configID = (uint32_t)config;

	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	if (win == 0) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	EGLint surfaceType;
	if (!fglGetConfigAttrib(configID, EGL_SURFACE_TYPE, &surfaceType))
		return EGL_NO_SURFACE;

	if (!(surfaceType & EGL_WINDOW_BIT)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	uint32_t depthFormat;
	uint32_t pixelFormat;
	if (!fglGetConfigFormatInfo(configID, &pixelFormat, &depthFormat)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	FGLRenderSurface *surface = platformCreateWindowSurface(dpy,
				configID, pixelFormat, depthFormat, win);
	if (!surface || !surface->initCheck()) {
		if (!surface)
			setError(EGL_BAD_ALLOC);
		/* otherwise platform code should have set error value for us */
		delete surface;
		return EGL_NO_SURFACE;
	}

	return (EGLSurface)surface;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy,
				EGLConfig config, const EGLint *attrib_list)
{
	uint32_t configID = (uint32_t)config;

	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	EGLint surfaceType;
	if (!fglGetConfigAttrib(configID, EGL_SURFACE_TYPE, &surfaceType))
		return EGL_NO_SURFACE;

	if (!(surfaceType & EGL_PBUFFER_BIT)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	uint32_t depthFormat;
	uint32_t pixelFormat;
	if (!fglGetConfigFormatInfo(configID, &pixelFormat, &depthFormat)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	if (unlikely(!attrib_list))
		attrib_list = &dummyAttribList;

	int32_t width = 0;
	int32_t height = 0;
	while (attrib_list[0] != EGL_NONE) {
		if (attrib_list[0] == EGL_WIDTH)
			width = attrib_list[1];

		if (attrib_list[0] == EGL_HEIGHT)
			height = attrib_list[1];

		attrib_list += 2;
	}

	if (width <= 0 || height <= 0) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_SURFACE;
	}

	FGLRenderSurface *surface = new FGLPbufferSurface(dpy,
			configID, pixelFormat, depthFormat, width, height);
	if (!surface || !surface->initCheck()) {
		if (!surface)
			setError(EGL_BAD_ALLOC);
		/* otherwise constructor should have set error value for us */
		delete surface;
		return EGL_NO_SURFACE;
	}

	return (EGLSurface)surface;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy,
				EGLConfig config, EGLNativePixmapType pixmap,
				const EGLint *attrib_list)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_MATCH);
	return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy,
							EGLSurface surface)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (surface == EGL_NO_SURFACE)
		return EGL_TRUE;

	FGLRenderSurface *fglSurface = (FGLRenderSurface *)surface;

	if (!fglSurface->isValid() || fglSurface->isTerminated()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if (fglSurface->ctx) {
		// Mark the surface for destruction on context detach
		fglSurface->terminate();
		return EGL_TRUE;
	}

	delete fglSurface;
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy,
			EGLSurface surface, EGLint attribute, EGLint *value)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface *fglSurface = static_cast<FGLRenderSurface *>(surface);

	if (!fglSurface->isValid() || fglSurface->isTerminated()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	EGLBoolean ret = EGL_TRUE;
	switch (attribute) {
	case EGL_CONFIG_ID:
		*value = (EGLint)fglSurface->config;
		break;
	case EGL_WIDTH:
		*value = fglSurface->getWidth();
		break;
	case EGL_HEIGHT:
		*value = fglSurface->getHeight();
		break;
	case EGL_LARGEST_PBUFFER:
		/* FIXME: Return something */
		break;
	case EGL_TEXTURE_FORMAT:
		*value = EGL_NO_TEXTURE;
		break;
	case EGL_TEXTURE_TARGET:
		*value = EGL_NO_TEXTURE;
		break;
	case EGL_MIPMAP_TEXTURE:
		*value = EGL_FALSE;
		break;
	case EGL_MIPMAP_LEVEL:
		*value = 0;
		break;
	case EGL_RENDER_BUFFER:
		*value = EGL_BACK_BUFFER;
		break;
	case EGL_HORIZONTAL_RESOLUTION:
		/* pixel/mm * EGL_DISPLAY_SCALING */
		*value = fglSurface->getHorizontalResolution();
		break;
	case EGL_VERTICAL_RESOLUTION:
		/* pixel/mm * EGL_DISPLAY_SCALING */
		*value = fglSurface->getVerticalResolution();
		break;
	case EGL_PIXEL_ASPECT_RATIO: {
		/* w/h * EGL_DISPLAY_SCALING */
		int wr = fglSurface->getHorizontalResolution();
		int hr = fglSurface->getVerticalResolution();
		*value = (wr * EGL_DISPLAY_SCALING) / hr;
		break; }
	case EGL_SWAP_BEHAVIOR:
		*value = fglSurface->getSwapBehavior();
		break;
	default:
		setError(EGL_BAD_ATTRIBUTE);
		ret = EGL_FALSE;
	}

	return ret;
}

/*
 * Client APIs
 */

EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api)
{
	if (api != EGL_OPENGL_ES_API) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	return EGL_TRUE;
}

EGLAPI EGLenum EGLAPIENTRY eglQueryAPI(void)
{
	return EGL_OPENGL_ES_API;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitClient(void)
{
	EGLContext ctx = eglGetCurrentContext();

	if (ctx != EGL_NO_CONTEXT)
		glFinish();

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
	EGLContext ctx = eglGetCurrentContext();

	if (ctx == EGL_NO_CONTEXT)
		return EGL_TRUE;

	EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglMakeCurrent(dpy, EGL_NO_CONTEXT, EGL_NO_SURFACE, EGL_NO_SURFACE);

	return EGL_TRUE;
}

/*
 * TODO: Unimplemented functions
 */

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(
	EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	EGLConfig config, const EGLint *attrib_list)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_MATCH);
	return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy,
			EGLSurface surface, EGLint attribute, EGLint value)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_MATCH);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy,
					EGLSurface surface, EGLint buffer)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_MATCH);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy,
					EGLSurface surface, EGLint buffer)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_MATCH);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	FUNC_UNIMPLEMENTED;
	return EGL_FALSE;
}

/*
 * Context management
 */

extern FGLContext *fglCreateContext(void);
extern void fglDestroyContext(FGLContext *ctx);

EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy,
				EGLConfig config, EGLContext share_context,
				const EGLint *attrib_list)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	FGLContext *gl = fglCreateContext();
	if (!gl) {
		setError(EGL_BAD_ALLOC);
		return EGL_NO_CONTEXT;
	}

	gl->egl.flags	= FGL_NEVER_CURRENT;
	gl->egl.dpy	= dpy;
	gl->egl.config	= config;

	return (EGLContext)gl;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	FGLContext *c = (FGLContext *)ctx;

	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (c->egl.flags & FGL_TERMINATE) {
		/* already scheduled for deletion */
		setError(EGL_BAD_CONTEXT);
		return EGL_FALSE;
	}

	if (c->egl.flags & FGL_IS_CURRENT) {
		/* mark the context for deletion on context switch */
		c->egl.flags |= FGL_TERMINATE;
		return EGL_TRUE;
	}

	fglDestroyContext(c);
	return EGL_TRUE;
}

static void fglUnbindContext(FGLContext *c)
{
	/* Make sure all the work finished */
	glFinish();

	/* Mark the context as not current anymore */
	c->egl.flags &= ~FGL_IS_CURRENT;

	/* Unbind draw surface */
	FGLRenderSurface *d = (FGLRenderSurface *)c->egl.draw;
	d->disconnect();
	d->ctx = EGL_NO_CONTEXT;
	c->egl.draw = EGL_NO_SURFACE;

	/* Delete it if it's terminated */
	if (d->isTerminated())
		delete d;

	/* Delete the context if it's terminated */
	if (c->egl.flags & FGL_TERMINATE)
		fglDestroyContext(c);
}

static EGLBoolean fglMakeCurrent(FGLContext *gl, FGLRenderSurface *d)
{
	FGLContext *current = getGlThreadSpecific();

	/* Current context (if available) should get detached */
	if (!gl) {
		if (!current)
			/* Nothing changed */
			return EGL_TRUE;

		fglUnbindContext(current);

		/* this thread has no context attached to it from now on */
		setGlThreadSpecific(0);

		return EGL_TRUE;
	}

	/* New context should get attached */
	if (gl->egl.flags & FGL_IS_CURRENT) {
		if (current != gl) {
			/*
			 * it is an error to set a context current,
			 * if it's already current to another thread
			 */
			setError(EGL_BAD_ACCESS);
			return EGL_FALSE;
		}

		/* Nothing changed */
		return EGL_TRUE;
	}

	/* Detach old context if present */
	if (current)
		fglUnbindContext(current);

	/* Attach draw surface */
	gl->egl.draw = (EGLSurface)d;
	if (!d->connect())
		/* Error should have been set for us. */
		return EGL_FALSE;
	d->ctx = (EGLContext)gl;
	d->bindDrawSurface(gl);

	/* Make the new context current */
	setGlThreadSpecific(gl);
	gl->egl.flags |= FGL_IS_CURRENT;

	/* Perform first time initialization if needed */
	if (gl->egl.flags & FGL_NEVER_CURRENT) {
		GLint w = d->getWidth();
		GLint h = d->getHeight();

		glViewport(0, 0, w, h);
		glScissor(0, 0, w, h);
		glDisable(GL_SCISSOR_TEST);

		gl->egl.flags &= ~FGL_NEVER_CURRENT;
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy,
			EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	/*
	 * Do all the EGL sanity checks
	 */
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (read != draw) {
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	if (draw == EGL_NO_SURFACE && ctx != EGL_NO_CONTEXT) {
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	if (draw != EGL_NO_SURFACE  && ctx == EGL_NO_CONTEXT) {
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	FGLRenderSurface *surface = (FGLRenderSurface *)draw;

	if (ctx != EGL_NO_CONTEXT) {
		if (surface->isTerminated() || !surface->isValid()) {
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}

		if (surface->ctx && surface->ctx != ctx) {
			// already bound to another thread
			setError(EGL_BAD_ACCESS);
			return EGL_FALSE;
		}
	}

	/*
	 * Proceed with the main part
	 */
	return fglMakeCurrent((FGLContext *)ctx, surface);
}

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void)
{
	return (EGLContext)getGlThreadSpecific();
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw)
{
	EGLContext ctx = (EGLContext)getGlThreadSpecific();

	if (ctx == EGL_NO_CONTEXT)
		return EGL_NO_SURFACE;

	FGLContext *c = (FGLContext *)ctx;

	if (readdraw == EGL_READ || readdraw == EGL_DRAW)
		return c->egl.draw;

	setError(EGL_BAD_ATTRIBUTE);
	return EGL_NO_SURFACE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
	EGLContext ctx = (EGLContext)getGlThreadSpecific();

	if (ctx == EGL_NO_CONTEXT)
		return EGL_NO_DISPLAY;

	FGLContext *c = (FGLContext *)ctx;
	return c->egl.dpy;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
			EGLint attribute, EGLint *value)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLContext *c = (FGLContext *)ctx;

	switch (attribute) {
	case EGL_CONFIG_ID:
		/*
		 * Returns the ID of the EGL frame buffer configuration with
		 * respect to which the context was created.
		 */
		*value = (EGLint)c->egl.config;
		break;
	default:
		setError(EGL_BAD_ATTRIBUTE);
		return EGL_FALSE;
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
	EGLContext ctx = (EGLContext)getGlThreadSpecific();

	if (ctx == EGL_NO_CONTEXT)
		glFinish();

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitNative(EGLint engine)
{
	FUNC_UNIMPLEMENTED;
	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface *d = (FGLRenderSurface *)surface;

	if (!d->isValid() || d->isTerminated()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	/* Flush the context attached to the surface if it's current */
	FGLContext *ctx = getGlThreadSpecific();
	if ((FGLContext *)d->ctx == ctx)
		glFinish();

	/* post the surface */
	if (!d->swapBuffers())
		/* Error code should have been set */
		return EGL_FALSE;

	/* if it's bound to a context, update the buffer */
	if (d->ctx != EGL_NO_CONTEXT) {
		FGLContext *c = (FGLContext *)d->ctx;
		d->bindDrawSurface(c);
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
			EGLNativePixmapType target)
{
	FUNC_UNIMPLEMENTED;
	return EGL_FALSE;
}

/*
 * Extension management
 */

#define EGLFunc		__eglMustCastToProperFunctionPointerType

static const FGLExtensionMap gExtensionMap[] = {
	{ "glDrawTexsOES",
		(EGLFunc)&glDrawTexsOES },
	{ "glDrawTexiOES",
		(EGLFunc)&glDrawTexiOES },
	{ "glDrawTexfOES",
		(EGLFunc)&glDrawTexfOES },
	{ "glDrawTexxOES",
		(EGLFunc)&glDrawTexxOES },
	{ "glDrawTexsvOES",
		(EGLFunc)&glDrawTexsvOES },
	{ "glDrawTexivOES",
		(EGLFunc)&glDrawTexivOES },
	{ "glDrawTexfvOES",
		(EGLFunc)&glDrawTexfvOES },
	{ "glDrawTexxvOES",
		(EGLFunc)&glDrawTexxvOES },
	{ "glBindBuffer",
		(EGLFunc)&glBindBuffer },
	{ "glBufferData",
		(EGLFunc)&glBufferData },
	{ "glBufferSubData",
		(EGLFunc)&glBufferSubData },
	{ "glDeleteBuffers",
		(EGLFunc)&glDeleteBuffers },
	{ "glGenBuffers",
		(EGLFunc)&glGenBuffers },
	{ "glEGLImageTargetTexture2DOES",
		(EGLFunc)&glEGLImageTargetTexture2DOES },
	{ NULL, NULL }
};

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char *procname)
{
	const FGLExtensionMap *map;

	for (map = gExtensionMap; map->name; ++map) {
		if (!strcmp(procname, map->name))
			return map->address;
	}

	for (map = gPlatformExtensionMap; map->name; ++map) {
		if (!strcmp(procname, map->name))
			return map->address;
	}

	return NULL;
}
