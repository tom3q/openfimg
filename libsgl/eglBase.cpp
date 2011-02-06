/**
 * libsgl/eglBase.cpp
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) EGL IMPLEMENTATION
 *
 * Copyrights:	2010 by Tomasz Figa < tomasz.figa at gmail.com >
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

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/native_handle.h>

#include <utils/threads.h>
#include <pthread.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include <private/ui/sw_gralloc_handle.h>
#include <ui/android_native_buffer.h>
#include <ui/PixelFormat.h>
#include <hardware/copybit.h>
#include <hardware/gralloc.h>

#include <linux/android_pmem.h>
#include <linux/fb.h>

#include "common.h"
#include "types.h"
#include "state.h"
#include "libfimg/fimg.h"
#include "eglMem.h"

#define FGL_EGL_MAJOR		1
#define FGL_EGL_MINOR		4

using namespace android;

static char const * const gVendorString     = "GLES6410";
static char const * const gVersionString    = "1.4 S3C6410 Android 0.1";
static char const * const gClientApisString = "OpenGL_ES";
static char const * const gExtensionsString =
	"EGL_KHR_image_base "
	"EGL_ANDROID_image_native_buffer "
	"EGL_ANDROID_swap_rectangle "
	"EGL_ANDROID_get_render_buffer"
;

pthread_key_t eglContextKey = -1;

struct FGLDisplay {
	EGLBoolean initialized;
	pthread_mutex_t lock;

	FGLDisplay() : initialized(0)
	{
		pthread_mutex_init(&lock, NULL);
	};
};

#define FGL_MAX_DISPLAYS	1
static FGLDisplay displays[FGL_MAX_DISPLAYS];

static inline EGLBoolean isDisplayValid(EGLDisplay dpy)
{
	EGLint disp = (EGLint)dpy;

	if(likely(disp == 1))
		return EGL_TRUE;

	return EGL_FALSE;
}

static inline FGLDisplay *getDisplay(EGLDisplay dpy)
{
	EGLint disp = (EGLint)dpy;

	return &displays[disp - 1];
}

static inline EGLBoolean isDisplayInitialized(EGLDisplay dpy)
{
	EGLBoolean ret;
	FGLDisplay *disp = getDisplay(dpy);

	pthread_mutex_lock(&disp->lock);
	ret = disp->initialized;
	pthread_mutex_unlock(&disp->lock);

	return ret;
}

static pthread_mutex_t eglErrorKeyMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t eglErrorKey = -1;

/**
	Error handling
*/

EGLAPI EGLint EGLAPIENTRY eglGetError(void)
{
	if(unlikely(eglErrorKey == -1))
		return EGL_SUCCESS;

	EGLint error = (EGLint)pthread_getspecific(eglErrorKey);
	pthread_setspecific(eglErrorKey, (void *)EGL_SUCCESS);
	return error;
}

#define EGL_ERR_DEBUG

#ifdef EGL_ERR_DEBUG
#define setError(a) ( \
	LOGD("EGL error %s in %s in line %d", #a, __func__, __LINE__), \
	_setError(a))
static void _setError(EGLint error)
#else
static void setError(EGLint error)
#endif
{
	if(unlikely(eglErrorKey == -1)) {
		pthread_mutex_lock(&eglErrorKeyMutex);
		if(eglErrorKey == -1)
			pthread_key_create(&eglErrorKey, NULL);
		pthread_mutex_unlock(&eglErrorKeyMutex);
	}

	pthread_setspecific(eglErrorKey, (void *)error);
}

/**
	Initialization
*/

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id)
{
	if(display_id != EGL_DEFAULT_DISPLAY)
		return EGL_NO_DISPLAY;

	return (EGLDisplay)1;
}

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	EGLBoolean ret = EGL_TRUE;

	if(!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if(major != NULL)
		*major = FGL_EGL_MAJOR;

	if(minor != NULL)
		*minor = FGL_EGL_MINOR;

	FGLDisplay *disp = getDisplay(dpy);

	pthread_mutex_lock(&disp->lock);

	if(likely(disp->initialized))
		goto finish;

#ifdef FGL_AGL_COEXIST
	pthread_key_create(&eglContextKey, NULL);
#endif

	disp->initialized = EGL_TRUE;

finish:
	pthread_mutex_unlock(&disp->lock);
	return ret;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay dpy)
{
	if(!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLDisplay *disp = getDisplay(dpy);

	pthread_mutex_lock(&disp->lock);

	if(unlikely(!disp->initialized))
		goto finish;

	disp->initialized = EGL_FALSE;

finish:
	pthread_mutex_unlock(&disp->lock);
	return EGL_TRUE;
}

EGLAPI const char * EGLAPIENTRY eglQueryString(EGLDisplay dpy, EGLint name)
{
	if(!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return NULL;
	}

	if(!isDisplayInitialized(dpy)) {
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

/**
	Configurations
*/

struct FGLConfigPair {
	EGLint key;
	EGLint value;
};

struct FGLConfigs {
	const FGLConfigPair* array;
	int size;
};

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

#define FGL_MAX_VIEWPORT_DIMS		2048
#define FGL_MAX_VIEWPORT_PIXELS		(FGL_MAX_VIEWPORT_DIMS*FGL_MAX_VIEWPORT_DIMS)

static FGLConfigPair const baseConfigAttributes[] = {
	{ EGL_CONFIG_CAVEAT,              EGL_NONE                          },
	{ EGL_LEVEL,                      0                                 },
	{ EGL_MAX_PBUFFER_HEIGHT,         FGL_MAX_VIEWPORT_DIMS             },
	{ EGL_MAX_PBUFFER_PIXELS,         FGL_MAX_VIEWPORT_PIXELS           },
	{ EGL_MAX_PBUFFER_WIDTH,          FGL_MAX_VIEWPORT_DIMS             },
	{ EGL_NATIVE_RENDERABLE,          EGL_TRUE                          },
	{ EGL_NATIVE_VISUAL_ID,           0                                 },
	{ EGL_NATIVE_VISUAL_TYPE,         GGL_PIXEL_FORMAT_RGB_565          },
	{ EGL_SAMPLES,                    0                                 },
	{ EGL_SAMPLE_BUFFERS,             0                                 },
	{ EGL_TRANSPARENT_TYPE,           EGL_NONE                          },
	{ EGL_TRANSPARENT_BLUE_VALUE,     0                                 },
	{ EGL_TRANSPARENT_GREEN_VALUE,    0                                 },
	{ EGL_TRANSPARENT_RED_VALUE,      0                                 },
	{ EGL_BIND_TO_TEXTURE_RGBA,       EGL_FALSE                         },
	{ EGL_BIND_TO_TEXTURE_RGB,        EGL_FALSE                         },
	{ EGL_MIN_SWAP_INTERVAL,          1                                 },
	{ EGL_MAX_SWAP_INTERVAL,          1                                 },
	{ EGL_LUMINANCE_SIZE,             0                                 },
	{ EGL_ALPHA_MASK_SIZE,            0                                 },
	{ EGL_COLOR_BUFFER_TYPE,          EGL_RGB_BUFFER                    },
	{ EGL_RENDERABLE_TYPE,            EGL_OPENGL_ES_BIT                 },
	{ EGL_CONFORMANT,                 0                                 }
};

// These configs can override the base attribute list
// NOTE: when adding a config here, don't forget to update eglCreate*Surface()

// RGB 565 configs
static FGLConfigPair const configAttributes0[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_CONFIG_ID,        0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

static FGLConfigPair const configAttributes1[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_CONFIG_ID,        1 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

// RGB 888 configs
static FGLConfigPair const configAttributes2[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_CONFIG_ID,        2 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

static FGLConfigPair const configAttributes3[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_CONFIG_ID,        3 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

// ARGB 8888 configs
static FGLConfigPair const configAttributes4[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_CONFIG_ID,        4 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

static FGLConfigPair const configAttributes5[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_CONFIG_ID,        5 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
	{ EGL_SURFACE_TYPE,     EGL_WINDOW_BIT|EGL_PBUFFER_BIT/*|EGL_PIXMAP_BIT*/ },
};

static FGLConfigs const gConfigs[] = {
	{ configAttributes0, NELEM(configAttributes0) },
	{ configAttributes1, NELEM(configAttributes1) },
	{ configAttributes2, NELEM(configAttributes2) },
	{ configAttributes3, NELEM(configAttributes3) },
	{ configAttributes4, NELEM(configAttributes4) },
	{ configAttributes5, NELEM(configAttributes5) },
};

static FGLConfigMatcher const gConfigManagement[] = {
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
	{ EGL_MAX_PBUFFER_HEIGHT,         FGLConfigMatcher::ignore   },
	{ EGL_MAX_PBUFFER_PIXELS,         FGLConfigMatcher::ignore   },
	{ EGL_MAX_PBUFFER_WIDTH,          FGLConfigMatcher::ignore   },
	{ EGL_NATIVE_RENDERABLE,          FGLConfigMatcher::exact   },
	{ EGL_NATIVE_VISUAL_ID,           FGLConfigMatcher::ignore   },
	{ EGL_NATIVE_VISUAL_TYPE,         FGLConfigMatcher::exact   },
	{ EGL_SAMPLES,                    FGLConfigMatcher::exact   },
	{ EGL_SAMPLE_BUFFERS,             FGLConfigMatcher::exact   },
	{ EGL_SURFACE_TYPE,               FGLConfigMatcher::mask    },
	{ EGL_TRANSPARENT_TYPE,           FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_BLUE_VALUE,     FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_GREEN_VALUE,    FGLConfigMatcher::exact   },
	{ EGL_TRANSPARENT_RED_VALUE,      FGLConfigMatcher::exact   },
	{ EGL_BIND_TO_TEXTURE_RGBA,       FGLConfigMatcher::exact   },
	{ EGL_BIND_TO_TEXTURE_RGB,        FGLConfigMatcher::exact   },
	{ EGL_MIN_SWAP_INTERVAL,          FGLConfigMatcher::exact   },
	{ EGL_MAX_SWAP_INTERVAL,          FGLConfigMatcher::exact   },
	{ EGL_LUMINANCE_SIZE,             FGLConfigMatcher::atLeast },
	{ EGL_ALPHA_MASK_SIZE,            FGLConfigMatcher::atLeast },
	{ EGL_COLOR_BUFFER_TYPE,          FGLConfigMatcher::exact   },
	{ EGL_RENDERABLE_TYPE,            FGLConfigMatcher::mask    },
	{ EGL_CONFORMANT,                 FGLConfigMatcher::mask    }
};


static FGLConfigPair const defaultConfigAttributes[] = {
// attributes that are not specified are simply ignored, if a particular
// one needs not be ignored, it must be specified here, eg:
// { EGL_SURFACE_TYPE, EGL_WINDOW_BIT },
};

// ----------------------------------------------------------------------------

static FGLint getConfigFormatInfo(EGLint configID,
	int32_t *pixelFormat, int32_t *depthFormat)
{
	switch(configID) {
	case 0:
		*pixelFormat = FGPF_COLOR_MODE_565;
		*depthFormat = 0;
		break;
	case 1:
		*pixelFormat = FGPF_COLOR_MODE_565;
		*depthFormat = (8 << 8) | 24;
		break;
	case 2:
		*pixelFormat = FGPF_COLOR_MODE_0888;
		*depthFormat = 0;
		break;
	case 3:
		*pixelFormat = FGPF_COLOR_MODE_0888;
		*depthFormat = (8 << 8) | 24;
		break;
	case 4:
		*pixelFormat = FGPF_COLOR_MODE_8888;
		*depthFormat = 0;
		break;
	case 5:
		*pixelFormat = FGPF_COLOR_MODE_8888;
		*depthFormat = (8 << 8) | 24;
		break;
	default:
		return NAME_NOT_FOUND;
	}

	return FGL_NO_ERROR;
}

static FGLint bppFromFormat(EGLint format)
{
	switch(format) {
	case FGPF_COLOR_MODE_565:
		return 2;
	case FGPF_COLOR_MODE_0888:
	case FGPF_COLOR_MODE_8888:
		return 4;
	default:
		return 0;
	}
}

// ----------------------------------------------------------------------------

template<typename T>
static int binarySearch(T const sortedArray[], int first, int last, EGLint key)
{
	while (first <= last) {
		int mid = (first + last) / 2;

		if (key > sortedArray[mid].key) {
			first = mid + 1;
		} else if (key < sortedArray[mid].key) {
			last = mid - 1;
		} else {
			return mid;
		}
	}

	return -1;
}

static int isAttributeMatching(int i, EGLint attr, EGLint val)
{
	// look for the attribute in all of our configs
	FGLConfigPair const* configFound = gConfigs[i].array;
	int index = binarySearch<FGLConfigPair>(
		gConfigs[i].array,
		0, gConfigs[i].size-1,
		attr);

	if (index < 0) {
		configFound = baseConfigAttributes;
		index = binarySearch<FGLConfigPair>(
			baseConfigAttributes,
			0, NELEM(baseConfigAttributes)-1,
			attr);
	}

	if (index >= 0) {
		// attribute found, check if this config could match
		int cfgMgtIndex = binarySearch<FGLConfigMatcher>(
			gConfigManagement,
			0, NELEM(gConfigManagement)-1,
			attr);

		if (cfgMgtIndex >= 0) {
			bool match = gConfigManagement[cfgMgtIndex].match(
				val, configFound[index].value);
			if (match) {
				// this config matches
				return 1;
			}
		} else {
		// attribute not found. this should NEVER happen.
		}
	} else {
		// error, this attribute doesn't exist
	}

	return 0;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
			EGLint config_size, EGLint *num_config)
{
	if(unlikely(!isDisplayValid(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if(unlikely(!isDisplayInitialized(dpy))) {
		setError(EGL_NOT_INITIALIZED);
		return EGL_FALSE;
	}

	if(unlikely(!num_config)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	EGLint num = NELEM(gConfigs);

	if(!configs) {
		*num_config = num;
		return EGL_TRUE;
	}

	num = min(num, config_size);

	EGLint i;
	for(i = 0; i < num; i++)
		*(configs)++ = (EGLConfig)i;

	*num_config = i;

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
			EGLConfig *configs, EGLint config_size,
			EGLint *num_config)
{
	if (unlikely(!isDisplayValid(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (unlikely(!num_config)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	if (unlikely(attrib_list==0)) {
		/*
		* A NULL attrib_list should be treated as though it was an empty
		* one (terminated with EGL_NONE) as defined in
		* section 3.4.1 "Querying Configurations" in the EGL specification.
		*/
		static const EGLint dummy = EGL_NONE;
		attrib_list = &dummy;
	}

	int numAttributes = 0;
	int numConfigs =  NELEM(gConfigs);
	uint32_t possibleMatch = (1<<numConfigs)-1;
	if(attrib_list) {
		while(possibleMatch && *attrib_list != EGL_NONE) {
			numAttributes++;
			EGLint attr = *attrib_list++;
			EGLint val  = *attrib_list++;
			for (int i=0 ; possibleMatch && i<numConfigs ; i++) {
				if (!(possibleMatch & (1<<i)))
					continue;
				if (isAttributeMatching(i, attr, val) == 0) {
					possibleMatch &= ~(1<<i);
				}
			}
		}
	}

	// now, handle the attributes which have a useful default value
	for (int j=0 ; possibleMatch && j<(int)NELEM(defaultConfigAttributes) ; j++) {
		// see if this attribute was specified, if not, apply its
		// default value
		if (binarySearch<FGLConfigPair>(
			(FGLConfigPair const*)attrib_list,
			0, numAttributes-1,
			defaultConfigAttributes[j].key) < 0)
		{
			for (int i=0 ; possibleMatch && i<numConfigs ; i++) {
				if (!(possibleMatch & (1<<i)))
					continue;
				if (isAttributeMatching(i,
					defaultConfigAttributes[j].key,
					defaultConfigAttributes[j].value) == 0)
				{
					possibleMatch &= ~(1<<i);
				}
			}
		}
	}

	// return the configurations found
	int n=0;
	if (possibleMatch) {
		if (configs) {
			for (int i=0 ; config_size && i<numConfigs ; i++) {
				if (possibleMatch & (1<<i)) {
					*configs++ = (EGLConfig)i;
					config_size--;
					n++;
				}
			}
		} else {
			for (int i=0 ; i<numConfigs ; i++) {
				if (possibleMatch & (1<<i)) {
					n++;
				}
			}
		}
	}

	*num_config = n;
	return EGL_TRUE;
}

static EGLBoolean getConfigAttrib(EGLDisplay dpy, EGLConfig config,
	EGLint attribute, EGLint *value)
{
	size_t numConfigs =  NELEM(gConfigs);
	int index = (int)config;

	if (uint32_t(index) >= numConfigs) {
		setError(EGL_BAD_CONFIG);
		return EGL_FALSE;
	}

	int attrIndex;
	attrIndex = binarySearch<FGLConfigPair>(
		gConfigs[index].array,
		0, gConfigs[index].size-1,
		attribute);

	if (attrIndex>=0) {
		*value = gConfigs[index].array[attrIndex].value;
		return EGL_TRUE;
	}

	attrIndex = binarySearch<FGLConfigPair>(
		baseConfigAttributes,
		0, NELEM(baseConfigAttributes)-1,
		attribute);

	if (attrIndex>=0) {
		*value = baseConfigAttributes[attrIndex].value;
		return EGL_TRUE;
	}

	setError(EGL_BAD_ATTRIBUTE);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
			EGLint attribute, EGLint *value)
{
	if (unlikely(!isDisplayValid(dpy))) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	return getConfigAttrib(dpy, config, attribute, value);
}

void fglSetColorBuffer(FGLContext *gl, FGLSurface *cbuf, unsigned int width,
		unsigned int height, unsigned int stride, unsigned int format)
{
	fimgSetFrameBufSize(gl->fimg, stride, height);
	fimgSetFrameBufParams(gl->fimg, 1, 0, 255, (fimgColorMode)format);
	fimgSetColorBufBaseAddr(gl->fimg, cbuf->paddr);
	gl->surface.draw = cbuf;
	gl->surface.width = width;
	gl->surface.stride = stride;
	gl->surface.height = height;
	gl->surface.format = format;
}

void fglSetDepthBuffer(FGLContext *gl, FGLSurface *zbuf, unsigned int format)
{
	if (!zbuf || !format) {
		gl->surface.depth = 0;
		gl->surface.depthFormat = 0;
		return;
	}

	fimgSetZBufBaseAddr(gl->fimg, zbuf->paddr);
	gl->surface.depth = zbuf;
	gl->surface.depthFormat = format;
}

void fglSetReadBuffer(FGLContext *gl, FGLSurface *rbuf)
{
	gl->surface.read = rbuf;
}

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

	virtual EGLBoolean	bindDrawSurface(FGLContext* gl) = 0;
	virtual EGLBoolean	bindReadSurface(FGLContext* gl) = 0;
	virtual EGLBoolean	connect() { return EGL_TRUE; }
	virtual void		disconnect() {}
	virtual EGLint		getWidth() const = 0;
	virtual EGLint		getHeight() const = 0;

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
	int			stride;
	int			height;
	int32_t			format;
};

FGLRenderSurface::FGLRenderSurface(EGLDisplay dpy,
	EGLConfig config, int32_t pixelFormat, int32_t depthFormat) :
magic(MAGIC), flags(0), dpy(dpy), config(config), ctx(0), color(0), depth(0),
depthFormat(depthFormat), format(pixelFormat)
{
}

FGLRenderSurface::~FGLRenderSurface()
{
	magic = 0;
	delete depth;
	delete color;
}

bool FGLRenderSurface::isValid() const {
	LOGE_IF(magic != MAGIC, "invalid EGLSurface (%p)", this);
	return magic == MAGIC;
}

void FGLRenderSurface::terminate() {
	flags |= TERMINATED;
}

bool FGLRenderSurface::isTerminated() const {
	return flags & TERMINATED;
}

EGLBoolean FGLRenderSurface::swapBuffers() {
	return EGL_FALSE;
}

EGLint FGLRenderSurface::getHorizontalResolution() const {
	return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
}

EGLint FGLRenderSurface::getVerticalResolution() const {
	return (0 * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
}

EGLint FGLRenderSurface::getRefreshRate() const {
	return (60 * EGL_DISPLAY_SCALING);
}

EGLint FGLRenderSurface::getSwapBehavior() const {
	return EGL_BUFFER_PRESERVED;
}

EGLBoolean FGLRenderSurface::setSwapRectangle(
	EGLint l, EGLint t, EGLint w, EGLint h)
{
	return EGL_FALSE;
}

EGLClientBuffer FGLRenderSurface::getRenderBuffer() const {
	return 0;
}

// ----------------------------------------------------------------------------

struct FGLWindowSurface : public FGLRenderSurface
{
	FGLWindowSurface(
		EGLDisplay dpy, EGLConfig config,
		int32_t depthFormat,
		android_native_window_t* window,
		int32_t pixelFormat);

	~FGLWindowSurface();

	virtual     bool        initCheck() const { return true; } // TODO: report failure if ctor fails
	virtual     EGLBoolean  swapBuffers();
	virtual     EGLBoolean  bindDrawSurface(FGLContext* gl);
	virtual     EGLBoolean  bindReadSurface(FGLContext* gl);
	virtual     EGLBoolean  connect();
	virtual     void        disconnect();
	virtual     EGLint      getWidth() const    { return width;  }
	virtual     EGLint      getHeight() const   { return height; }
	virtual     EGLint      getHorizontalResolution() const;
	virtual     EGLint      getVerticalResolution() const;
	virtual     EGLint      getRefreshRate() const;
	virtual     EGLint      getSwapBehavior() const;
	virtual     EGLBoolean  setSwapRectangle(EGLint l, EGLint t, EGLint w, EGLint h);
	virtual     EGLClientBuffer  getRenderBuffer() const;

private:
	FGLint lock(android_native_buffer_t* buf, int usage, void** vaddr);
	FGLint unlock(android_native_buffer_t* buf);
	android_native_window_t*	nativeWindow;
	android_native_buffer_t*	buffer;
	android_native_buffer_t*	previousBuffer;
	gralloc_module_t const*		module;
	copybit_device_t*		blitengine;
	void*			 	bits;

	struct Rect {
		inline Rect() { };
		inline Rect(int32_t w, int32_t h)
		: left(0), top(0), right(w), bottom(h) { }
		inline Rect(int32_t l, int32_t t, int32_t r, int32_t b)
		: left(l), top(t), right(r), bottom(b) { }
		Rect& andSelf(const Rect& r) {
		left   = max(left, r.left);
		top    = max(top, r.top);
		right  = min(right, r.right);
		bottom = min(bottom, r.bottom);
		return *this;
		}
		bool isEmpty() const {
		return (left>=right || top>=bottom);
		}
		void dump(char const* what) {
		LOGD("%s { %5d, %5d, w=%5d, h=%5d }",
			what, left, top, right-left, bottom-top);
		}

		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;
	};

	struct Region {
		inline Region() : count(0) { }
		typedef Rect const* const_iterator;
		const_iterator begin() const { return storage; }
		const_iterator end() const { return storage+count; }
		static Region subtract(const Rect& lhs, const Rect& rhs) {
			Region reg;
			Rect* storage = reg.storage;
			if (!lhs.isEmpty()) {
				if (lhs.top < rhs.top) { // top rect
					storage->left   = lhs.left;
					storage->top    = lhs.top;
					storage->right  = lhs.right;
					storage->bottom = rhs.top;
					storage++;
				}
				const int32_t top = max(lhs.top, rhs.top);
				const int32_t bot = min(lhs.bottom, rhs.bottom);
				if (top < bot) {
					if (lhs.left < rhs.left) { // left-side rect
						storage->left   = lhs.left;
						storage->top    = top;
						storage->right  = rhs.left;
						storage->bottom = bot;
						storage++;
					}
					if (lhs.right > rhs.right) { // right-side rect
						storage->left   = rhs.right;
						storage->top    = top;
						storage->right  = lhs.right;
						storage->bottom = bot;
						storage++;
					}
				}
				if (lhs.bottom > rhs.bottom) { // bottom rect
					storage->left   = lhs.left;
					storage->top    = rhs.bottom;
					storage->right  = lhs.right;
					storage->bottom = lhs.bottom;
					storage++;
				}
				reg.count = storage - reg.storage;
			}
			return reg;
		}
		bool isEmpty() const {
			return count<=0;
		}
	private:
		Rect storage[4];
		ssize_t count;
	};

	struct region_iterator : public copybit_region_t {
		region_iterator(const Region& region)
		: b(region.begin()), e(region.end()) {
			this->next = iterate;
		}
	private:
		static int iterate(copybit_region_t const * self, copybit_rect_t* rect) {
			region_iterator const* me = static_cast<region_iterator const*>(self);
			if (me->b != me->e) {
				*reinterpret_cast<Rect*>(rect) = *me->b++;
				return 1;
			}
			return 0;
		}
		mutable Region::const_iterator b;
		Region::const_iterator const e;
	};

	void copyBlt(
		android_native_buffer_t* dst, void* dst_vaddr,
		android_native_buffer_t* src, void const* src_vaddr,
		const Region& clip);

	Rect dirtyRegion;
	Rect oldDirtyRegion;
};

FGLWindowSurface::FGLWindowSurface(EGLDisplay dpy,
	EGLConfig config,
	int32_t depthFormat,
	android_native_window_t* window,
	int32_t pixelFormat)
	: FGLRenderSurface(dpy, config, pixelFormat, depthFormat),
	nativeWindow(window), buffer(0), previousBuffer(0), module(0),
	blitengine(0), bits(0)
{
	hw_module_t const* pModule;
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule);
	module = reinterpret_cast<gralloc_module_t const*>(pModule);

	//if (hw_get_module(COPYBIT_HARDWARE_MODULE_ID, &pModule) == 0) {
	//	copybit_open(pModule, &blitengine);
	//}

	// keep a reference on the window
	nativeWindow->common.incRef(&nativeWindow->common);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_WIDTH, &width);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_HEIGHT, &height);
}

FGLWindowSurface::~FGLWindowSurface()
{
	if (buffer) {
		buffer->common.decRef(&buffer->common);
	}
	if (previousBuffer) {
		previousBuffer->common.decRef(&previousBuffer->common);
	}
	nativeWindow->common.decRef(&nativeWindow->common);

	if (blitengine) {
		copybit_close(blitengine);
	}
}

EGLBoolean FGLWindowSurface::connect()
{
	// we're intending to do hardware rendering
	native_window_set_usage(nativeWindow,
		GRALLOC_USAGE_SW_READ_RARELY | GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER);

	// dequeue a buffer
	if (nativeWindow->dequeueBuffer(nativeWindow, &buffer) != FGL_NO_ERROR) {
		setError(EGL_BAD_ALLOC);
		return EGL_FALSE;
	}

	// allocate a corresponding depth-buffer
	width = buffer->width;
	height = buffer->height;
	stride = buffer->stride;
	if (depthFormat) {
		unsigned int size = stride * height * 4;

		depth = new FGLLocalSurface(size);
		if (!depth || !depth->isValid()) {
			setError(EGL_BAD_ALLOC);
			return EGL_FALSE;
		}
	}

	// keep a reference on the buffer
	buffer->common.incRef(&buffer->common);

	// Lock the buffer
	nativeWindow->lockBuffer(nativeWindow, buffer);

	// pin the buffer down
	if (lock(buffer, GRALLOC_USAGE_SW_READ_RARELY |
		GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER, &bits) != FGL_NO_ERROR) {
		LOGE("connect() failed to lock buffer %p (%ux%u)",
			buffer, buffer->width, buffer->height);
		setError(EGL_BAD_ACCESS);
		return EGL_FALSE;
		// FIXME:
	}

	delete color;
	color = new FGLExternalSurface(bits, fglGetBufferPhysicalAddress(buffer));

	return EGL_TRUE;
}

void FGLWindowSurface::disconnect()
{
	delete color;
	color = 0;

	if (buffer && bits) {
		bits = NULL;
		unlock(buffer);
	}

	if (buffer) {
		// enqueue the last frame
		nativeWindow->queueBuffer(nativeWindow, buffer);
		buffer->common.decRef(&buffer->common);
		buffer = 0;
	}

	if (previousBuffer) {
		previousBuffer->common.decRef(&previousBuffer->common);
		previousBuffer = 0;
	}
}

FGLint FGLWindowSurface::lock(
	android_native_buffer_t* buf, int usage, void** vaddr)
{
	int err;
	if (sw_gralloc_handle_t::validate(buf->handle) < 0) {
		err = module->lock(module, buf->handle,
			usage, 0, 0, buf->width, buf->height, vaddr);
	} else {
		sw_gralloc_handle_t const* hnd =
			reinterpret_cast<sw_gralloc_handle_t const*>(buf->handle);
		*vaddr = (void*)hnd->base;
		err = FGL_NO_ERROR;
	}
	return err;
}

FGLint FGLWindowSurface::unlock(android_native_buffer_t* buf)
{
	if (!buf) return BAD_VALUE;
	int err = FGL_NO_ERROR;
	if (sw_gralloc_handle_t::validate(buf->handle) < 0) {
		err = module->unlock(module, buf->handle);
	}
	return err;
}

static inline FGLint getBpp(int format)
{
	switch(format) {
	case PIXEL_FORMAT_RGBA_8888:
	case PIXEL_FORMAT_RGBX_8888:
		return 4;
	case PIXEL_FORMAT_RGB_565:
		return 2;
	default:
		return 0;
	}
}

void FGLWindowSurface::copyBlt(
	android_native_buffer_t* dst, void* dst_vaddr,
	android_native_buffer_t* src, void const* src_vaddr,
	const Region& clip)
{
	// NOTE: dst and src must be the same format
#if 0
	FGLint err = FGL_NO_ERROR;

	copybit_device_t* const copybit = blitengine;
	if (copybit)  {
		copybit_image_t simg;
		simg.w = src->stride;
		simg.h = src->height;
		simg.format = src->format;
		simg.handle = const_cast<native_handle_t*>(src->handle);

		copybit_image_t dimg;
		dimg.w = dst->stride;
		dimg.h = dst->height;
		dimg.format = dst->format;
		dimg.handle = const_cast<native_handle_t*>(dst->handle);

		copybit->set_parameter(copybit, COPYBIT_TRANSFORM, 0);
		copybit->set_parameter(copybit, COPYBIT_PLANE_ALPHA, 255);
		copybit->set_parameter(copybit, COPYBIT_DITHER, COPYBIT_DISABLE);
		region_iterator it(clip);

		err = copybit->blit(copybit, &dimg, &simg, &it);
		if (err == FGL_NO_ERROR)
			return;

		LOGE("copybit failed (%s)", strerror(err));
	}
#endif
	Region::const_iterator cur = clip.begin();
	Region::const_iterator end = clip.end();

	const size_t bpp = getBpp(src->format);
	const size_t dbpr = dst->stride * bpp;
	const size_t sbpr = src->stride * bpp;

	uint8_t const * const src_bits = (uint8_t const *)src_vaddr;
	uint8_t       * const dst_bits = (uint8_t       *)dst_vaddr;

	while (cur != end) {
		const Rect& r(*cur++);
		ssize_t w = r.right - r.left;
		ssize_t h = r.bottom - r.top;

		if (w <= 0 || h<=0) continue;

		size_t size = w * bpp;
		uint8_t const * s = src_bits + (r.left + src->stride * r.top) * bpp;
		uint8_t       * d = dst_bits + (r.left + dst->stride * r.top) * bpp;

		if (dbpr==sbpr && size==sbpr) {
			size *= h;
			h = 1;
		}

		do {
			memcpy(d, s, size);
			d += dbpr;
			s += sbpr;
		} while (--h > 0);
	}
}

EGLBoolean FGLWindowSurface::swapBuffers()
{
	if (!buffer) {
		setError(EGL_BAD_ACCESS);
		return EGL_FALSE;
	}

	/*
	* Handle eglSetSwapRectangleANDROID()
	* We copyback from the front buffer
	*/

	if (!dirtyRegion.isEmpty()) {
		dirtyRegion.andSelf(Rect(buffer->width, buffer->height));
		if (previousBuffer) {
			Region copyBack(Region::subtract(oldDirtyRegion, dirtyRegion));
			if (!copyBack.isEmpty()) {
				void* prevBits;
				if (lock(previousBuffer, GRALLOC_USAGE_SW_READ_OFTEN,
				&prevBits) == FGL_NO_ERROR) {
					// copy from previousBuffer to buffer
					copyBlt(buffer, bits, previousBuffer, prevBits, copyBack);
					unlock(previousBuffer);
				}
			}
		}
		oldDirtyRegion = dirtyRegion;
	}

	if (previousBuffer) {
		previousBuffer->common.decRef(&previousBuffer->common);
		previousBuffer = 0;
	}

	unlock(buffer);
	previousBuffer = buffer;
	nativeWindow->queueBuffer(nativeWindow, buffer);
	buffer = 0;

	// dequeue a new buffer
	nativeWindow->dequeueBuffer(nativeWindow, &buffer);

	// TODO: lockBuffer should rather be executed when the very first
	// direct rendering occurs.
	nativeWindow->lockBuffer(nativeWindow, buffer);

	// reallocate the depth-buffer if needed
	if ((width != buffer->width) || (height != buffer->height)
		|| (stride != buffer->stride))
	{
		// TODO: we probably should reset the swap rect here
		// if the window size has changed
		width = buffer->width;
		height = buffer->height;
		stride = buffer->stride;

		if (depthFormat) {
			unsigned int size = stride * height * 4;

			delete depth;

			depth = new FGLLocalSurface(size);
			if (!depth || !depth->isValid()) {
				setError(EGL_BAD_ALLOC);
				return EGL_FALSE;
			}
		}
#if 0
		fglSetClipper(0, 0, width, height);
#endif
	}

	// keep a reference on the buffer
	buffer->common.incRef(&buffer->common);

	// finally pin the buffer down
	if (lock(buffer, GRALLOC_USAGE_SW_READ_RARELY |
		GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER, &bits) != FGL_NO_ERROR) {
		LOGE("eglSwapBuffers() failed to lock buffer %p (%ux%u)",
			buffer, buffer->width, buffer->height);
		setError(EGL_BAD_ACCESS);
		return EGL_FALSE;
		// FIXME: we should make sure we're not accessing the buffer anymore
	}

	delete color;
	color = new FGLExternalSurface(bits, fglGetBufferPhysicalAddress(buffer));

	return EGL_TRUE;
}

EGLBoolean FGLWindowSurface::setSwapRectangle(
	EGLint l, EGLint t, EGLint w, EGLint h)
{
	dirtyRegion = Rect(l, t, l+w, t+h);
	return EGL_TRUE;
}

EGLClientBuffer FGLWindowSurface::getRenderBuffer() const
{
	return buffer;
}

EGLBoolean FGLWindowSurface::bindDrawSurface(FGLContext* gl)
{
	fglSetColorBuffer(gl, color, width, height, stride, format);
	fglSetDepthBuffer(gl, depth, depthFormat);

	return EGL_TRUE;
}

EGLBoolean FGLWindowSurface::bindReadSurface(FGLContext* gl)
{
	fglSetReadBuffer(gl, color);

	return EGL_TRUE;
}

EGLint FGLWindowSurface::getHorizontalResolution() const {
	return (nativeWindow->xdpi * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
}

EGLint FGLWindowSurface::getVerticalResolution() const {
	return (nativeWindow->ydpi * EGL_DISPLAY_SCALING) * (1.0f / 25.4f);
}

EGLint FGLWindowSurface::getRefreshRate() const {
	return (60 * EGL_DISPLAY_SCALING); // FIXME
}

EGLint FGLWindowSurface::getSwapBehavior() const
{
	/*
	* EGL_BUFFER_PRESERVED means that eglSwapBuffers() completely preserves
	* the content of the swapped buffer.
	*
	* EGL_BUFFER_DESTROYED means that the content of the buffer is lost.
	*
	* However when ANDROID_swap_retcangle is supported, EGL_BUFFER_DESTROYED
	* only applies to the area specified by eglSetSwapRectangleANDROID(), that
	* is, everything outside of this area is preserved.
	*
	* This implementation of EGL assumes the later case.
	*
	*/

	return EGL_BUFFER_DESTROYED;
}

// ----------------------------------------------------------------------------
#if 0
/* FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME */
struct FGLPixmapSurface : public FGLRenderSurface
{
	FGLPixmapSurface(
		EGLDisplay dpy, EGLConfig config,
		int32_t depthFormat,
		egl_native_pixmap_t const * pixmap);

	virtual ~FGLPixmapSurface() { }

	virtual     bool        initCheck() const { return !depth.format || depth.vaddr!=0; }
	virtual     EGLBoolean  bindDrawSurface(FGLContext* gl);
	virtual     EGLBoolean  bindReadSurface(FGLContext* gl);
	virtual     EGLint      getWidth() const    { return nativePixmap.width;  }
	virtual     EGLint      getHeight() const   { return nativePixmap.height; }
	private:
	egl_native_pixmap_t     nativePixmap;
};

FGLPixmapSurface::FGLPixmapSurface(EGLDisplay dpy,
	EGLConfig config,
	int32_t depthFormat,
	egl_native_pixmap_t const * pixmap)
	: FGLRenderSurface(dpy, config, depthFormat), nativePixmap(*pixmap)
{
	FUNC_UNIMPLEMENTED;

	if (depthFormat) {
		depth.width   = pixmap->width;
		depth.height  = pixmap->height;
		depth.stride  = depth.width; // use the width here
		depth.size    = depth.stride*depth.height*4;
		if (fglCreatePmemSurface(&depth)) {
			setError(EGL_BAD_ALLOC);
		}
	}
}

EGLBoolean FGLPixmapSurface::bindDrawSurface(FGLContext* gl)
{
	FGLSurface buffer;

	FUNC_UNIMPLEMENTED;

	buffer.width   = nativePixmap.width;
	buffer.height  = nativePixmap.height;
	buffer.stride  = nativePixmap.stride;
	buffer.vaddr   = nativePixmap.data;
	buffer.paddr   = 0;

	buffer.format  = nativePixmap.format;

	fglSetColorBuffer(gl, &buffer);
	fglSetDepthBuffer(gl, &depth);

	return EGL_TRUE;
}

EGLBoolean FGLPixmapSurface::bindReadSurface(FGLContext* gl)
{
	FGLSurface buffer;

	FUNC_UNIMPLEMENTED;

	buffer.width   = nativePixmap.width;
	buffer.height  = nativePixmap.height;
	buffer.stride  = nativePixmap.stride;
	buffer.vaddr   = nativePixmap.data;
	buffer.paddr   = 0;
	buffer.size    = 0;
	buffer.format  = nativePixmap.format;

	fglSetReadBuffer(gl, &buffer);

	return EGL_TRUE;
}
/* FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME */
#endif
// ----------------------------------------------------------------------------

struct FGLPbufferSurface : public FGLRenderSurface
{
	FGLPbufferSurface(
		EGLDisplay dpy, EGLConfig config, int32_t depthFormat,
		int32_t w, int32_t h, int32_t f);

	virtual ~FGLPbufferSurface();

	virtual     bool        initCheck() const
	{
		return color && color->isValid() && (!depth || depth->isValid());
	}
	virtual     EGLBoolean  bindDrawSurface(FGLContext* gl);
	virtual     EGLBoolean  bindReadSurface(FGLContext* gl);
	virtual     EGLint      getWidth() const    { return width;  }
	virtual     EGLint      getHeight() const   { return height; }
};

FGLPbufferSurface::FGLPbufferSurface(EGLDisplay dpy,
	EGLConfig config, int32_t depthFormat,
	int32_t w, int32_t h, int32_t f)
: FGLRenderSurface(dpy, config, f, depthFormat)
{
	unsigned int size = w * h * bppFromFormat(f);

	color = new FGLLocalSurface(size);
	if (!color || !color->isValid()) {
		setError(EGL_BAD_ALLOC);
		return;
	}

	width   = w;
	height  = h;
	stride  = w;

	if (depthFormat) {
		size = w * h * 4;

		depth = new FGLLocalSurface(size);
		if (!depth || !depth->isValid()) {
			setError(EGL_BAD_ALLOC);
			return;
		}
	}
}

FGLPbufferSurface::~FGLPbufferSurface()
{
}

EGLBoolean FGLPbufferSurface::bindDrawSurface(FGLContext* gl)
{
	fglSetColorBuffer(gl, color, width, height, stride, format);
	fglSetDepthBuffer(gl, depth, depthFormat);

	return EGL_TRUE;
}

EGLBoolean FGLPbufferSurface::bindReadSurface(FGLContext* gl)
{
	fglSetReadBuffer(gl, color);

	return EGL_TRUE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
				EGLNativeWindowType win,
				const EGLint *attrib_list)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	if (win == 0) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	EGLint surfaceType;
	if (getConfigAttrib(dpy, config, EGL_SURFACE_TYPE, &surfaceType) == EGL_FALSE)
		return EGL_NO_SURFACE;

	if (!(surfaceType & EGL_WINDOW_BIT)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	if (static_cast<android_native_window_t*>(win)->common.magic != ANDROID_NATIVE_WINDOW_MAGIC) {
		setError(EGL_BAD_NATIVE_WINDOW);
		return EGL_NO_SURFACE;
	}

	EGLint configID;
	if (getConfigAttrib(dpy, config, EGL_CONFIG_ID, &configID) == EGL_FALSE)
		return EGL_NO_SURFACE;

	int32_t depthFormat;
	int32_t pixelFormat;
	if (getConfigFormatInfo(configID, &pixelFormat, &depthFormat) != NO_ERROR) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	// FIXME: we don't have access to the pixelFormat here just yet.
	// (it's possible that the surface is not fully initialized)
	// maybe this should be done after the page-flip
	//if (EGLint(info.format) != pixelFormat)
	//    setError(EGL_BAD_MATCH);
	//    return EGL_NO_SURFACE;

	FGLRenderSurface* surface;
	surface = new FGLWindowSurface(dpy, config, depthFormat,
			static_cast<android_native_window_t*>(win), pixelFormat);

	if (surface == NULL) {
		setError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}

	if (!surface->initCheck()) {
		// there was a problem in the ctor, the error
		// flag has been set.
		delete surface;
		surface = 0;
	}
	return surface;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
				const EGLint *attrib_list)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	EGLint surfaceType;
	if (getConfigAttrib(dpy, config, EGL_SURFACE_TYPE, &surfaceType) == EGL_FALSE)
		return EGL_NO_SURFACE;

	if (!(surfaceType & EGL_PBUFFER_BIT)) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	EGLint configID;
	if (getConfigAttrib(dpy, config, EGL_CONFIG_ID, &configID) == EGL_FALSE)
		return EGL_NO_SURFACE;

	int32_t depthFormat;
	int32_t pixelFormat;
	if (getConfigFormatInfo(configID, &pixelFormat, &depthFormat) != NO_ERROR) {
		setError(EGL_BAD_MATCH);
		return EGL_NO_SURFACE;
	}

	int32_t w = 0;
	int32_t h = 0;
	if(attrib_list) {
		while (attrib_list[0]) {
			if (attrib_list[0] == EGL_WIDTH)  w = attrib_list[1];
			if (attrib_list[0] == EGL_HEIGHT) h = attrib_list[1];
			attrib_list+=2;
		}
	}

	FGLRenderSurface* surface;
	surface = new FGLPbufferSurface(dpy, config, depthFormat, w, h,
								pixelFormat);
	if (surface == NULL) {
		setError(EGL_BAD_ALLOC);
		return EGL_NO_SURFACE;
	}

	if (!surface->initCheck()) {
		// there was a problem in the ctor, the error
		// flag has been set.
		delete surface;
		surface = 0;
	}
	return surface;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
				EGLNativePixmapType pixmap,
				const EGLint *attrib_list)
{
	FUNC_UNIMPLEMENTED;
	return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (surface != EGL_NO_SURFACE) {
		FGLRenderSurface* fglSurface( static_cast<FGLRenderSurface*>(surface) );

		if (!fglSurface->isValid()) {
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}

		if (fglSurface->dpy != dpy) {
			setError(EGL_BAD_DISPLAY);
			return EGL_FALSE;
		}

		if (fglSurface->ctx) {
			// FIXME: this surface is current check what the spec says
			fglSurface->terminate();
			//fglSurface->disconnect();
			//fglSurface->ctx = 0;
			return EGL_TRUE;
		}

		delete fglSurface;
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
			EGLint attribute, EGLint *value)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface* fglSurface = static_cast<FGLRenderSurface*>(surface);

	if (!fglSurface->isValid()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if (fglSurface->dpy != dpy) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	EGLBoolean ret = EGL_TRUE;

	switch (attribute) {
		case EGL_CONFIG_ID:
			ret = getConfigAttrib(dpy, fglSurface->config, EGL_CONFIG_ID, value);
			break;
		case EGL_WIDTH:
			*value = fglSurface->getWidth();
			break;
		case EGL_HEIGHT:
			*value = fglSurface->getHeight();
			break;
		case EGL_LARGEST_PBUFFER:
			// not modified for a window or pixmap surface
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
			// TODO: return the real RENDER_BUFFER here
			*value = EGL_BACK_BUFFER;
			break;
		case EGL_HORIZONTAL_RESOLUTION:
			// pixel/mm * EGL_DISPLAY_SCALING
			*value = fglSurface->getHorizontalResolution();
			break;
		case EGL_VERTICAL_RESOLUTION:
			// pixel/mm * EGL_DISPLAY_SCALING
			*value = fglSurface->getVerticalResolution();
			break;
		case EGL_PIXEL_ASPECT_RATIO: {
			// w/h * EGL_DISPLAY_SCALING
			int wr = fglSurface->getHorizontalResolution();
			int hr = fglSurface->getVerticalResolution();
			*value = (wr * EGL_DISPLAY_SCALING) / hr;
			} break;
		case EGL_SWAP_BEHAVIOR:
			*value = fglSurface->getSwapBehavior();
			break;
		default:
			setError(EGL_BAD_ATTRIBUTE);
			ret = EGL_FALSE;
	}

	return ret;
}

/**
	Client APIs
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
	glFinish();
	return EGL_TRUE;
}

// TODO: Implement following functions
EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void)
{
	FUNC_UNIMPLEMENTED;
	return EGL_TRUE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferFromClientBuffer(
	EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
	EGLConfig config, const EGLint *attrib_list)
{
	FUNC_UNIMPLEMENTED;
	return EGL_NO_SURFACE;
}


EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
			EGLint attribute, EGLint value)
{
	FUNC_UNIMPLEMENTED;
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_SURFACE);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	FUNC_UNIMPLEMENTED;
	setError(EGL_BAD_SURFACE);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	FUNC_UNIMPLEMENTED;
	return EGL_FALSE;
}

extern FGLContext *fglCreateContext(void);
extern void fglDestroyContext(FGLContext *ctx);


EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay dpy, EGLConfig config,
			EGLContext share_context,
			const EGLint *attrib_list)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_SURFACE;
	}

	FGLContext* gl = fglCreateContext();
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

	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (getGlThreadSpecific() == c)
		eglMakeCurrent(dpy, EGL_NO_SURFACE,
					EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (c->egl.flags & FGL_IS_CURRENT)
		c->egl.flags |= FGL_TERMINATE;
	else
		fglDestroyContext(c);

	return EGL_TRUE;
}

static int fglMakeCurrent(FGLContext* gl)
{
	FGLContext* current = getGlThreadSpecific();

	if (gl) {
		if (gl->egl.flags & FGL_IS_CURRENT) {
			if (current != gl) {
				// it is an error to set a context current, if it's already
				// current to another thread
				return -1;
			}
		} else {
			if (current) {
				FGLRenderSurface *s(static_cast<FGLRenderSurface*>(current->egl.draw));
				// mark the current context as not current, and flush
				glFinish();
				current->egl.flags &= ~FGL_IS_CURRENT;
				if (current->egl.flags & FGL_TERMINATE) {
					if(s->isTerminated()) {
						s->disconnect();
						s->ctx = 0;
						delete s;
					}
					fglDestroyContext(current);
				}
			}
			// The context is not current, make it current!
			setGlThreadSpecific(gl);
			gl->egl.flags |= FGL_IS_CURRENT;
		}
	} else {
		if (current) {
			FGLRenderSurface *s(static_cast<FGLRenderSurface*>(current->egl.draw));
			// mark the current context as not current, and flush
			glFinish();
			current->egl.flags &= ~FGL_IS_CURRENT;
			if (current->egl.flags & FGL_TERMINATE) {
				if(s->isTerminated()) {
					s->disconnect();
					s->ctx = 0;
					delete s;
				}
				fglDestroyContext(current);
			}
		}
		// this thread has no context attached to it
		setGlThreadSpecific(0);
	}

	return 0;
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
			EGLSurface read, EGLContext ctx)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (draw) {
		FGLRenderSurface* s = (FGLRenderSurface*)draw;

		if (!s->isValid()) {
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}

		if (s->dpy != dpy) {
			setError(EGL_BAD_DISPLAY);
			return EGL_FALSE;
		}
		// TODO: check that draw is compatible with the context
	}

	if (read && read!=draw) {
		FGLRenderSurface* s = (FGLRenderSurface*)read;

		if (!s->isValid()) {
			setError(EGL_BAD_SURFACE);
			return EGL_FALSE;
		}

		if (s->dpy != dpy) {
			setError(EGL_BAD_DISPLAY);
			return EGL_FALSE;
		}
		// TODO: check that read is compatible with the context
	}

	EGLContext current_ctx = EGL_NO_CONTEXT;

	if ((read == EGL_NO_SURFACE && draw == EGL_NO_SURFACE) && (ctx != EGL_NO_CONTEXT)) {
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	if ((read != EGL_NO_SURFACE || draw != EGL_NO_SURFACE) && (ctx == EGL_NO_CONTEXT)) {
		setError(EGL_BAD_MATCH);
		return EGL_FALSE;
	}

	if (ctx == EGL_NO_CONTEXT) {
		// if we're detaching, we need the current context
		current_ctx = (EGLContext)getGlThreadSpecific();
	} else {
		FGLRenderSurface* d = (FGLRenderSurface*)draw;
		FGLRenderSurface* r = (FGLRenderSurface*)read;

		if ((d && d->ctx && d->ctx != ctx) ||
		(r && r->ctx && r->ctx != ctx)) {
			// one of the surface is bound to a context in another thread
			setError(EGL_BAD_ACCESS);
			return EGL_FALSE;
		}
	}

	FGLContext* gl = (FGLContext*)ctx;
	if (fglMakeCurrent(gl) == 0) {
		if (ctx) {
			FGLRenderSurface* d = (FGLRenderSurface*)draw;
			FGLRenderSurface* r = (FGLRenderSurface*)read;

			if (gl->egl.draw) {
				FGLRenderSurface* s = reinterpret_cast<FGLRenderSurface*>(gl->egl.draw);
				s->disconnect();
			}

			if (gl->egl.read) {
				// FIXME: unlock/disconnect the read surface too
			}

			gl->egl.draw = draw;
			gl->egl.read = read;

			if (d) {
				if (d->connect() == EGL_FALSE)
					return EGL_FALSE;

				d->ctx = ctx;
				d->bindDrawSurface(gl);
			}

			if (r) {
				// FIXME: lock/connect the read surface too
				r->ctx = ctx;
				r->bindReadSurface(gl);
			}

			if (gl->egl.flags & FGL_NEVER_CURRENT) {
				gl->egl.flags &= ~FGL_NEVER_CURRENT;
				GLint w = 0;
				GLint h = 0;

				if (draw) {
					w = d->getWidth();
					h = d->getHeight();
				}

				uint32_t depth = (gl->surface.depthFormat & 0xff) ? 1 : 0;
				uint32_t stencil = (gl->surface.depthFormat >> 8) ? 0xff : 0;

				fimgSetZBufWriteMask(gl->fimg, depth);
				fimgSetStencilBufWriteMask(gl->fimg, 0, stencil);
				fimgSetStencilBufWriteMask(gl->fimg, 1, stencil);
#if 0
				fglSetClipper(0, 0, w, h);
#endif
				glViewport(0, 0, w, h);
				glScissor(0, 0, w, h);
				glDisable(GL_SCISSOR_TEST);
			}
		} else {
			// if surfaces were bound to the context bound to this thread
			// mark then as unbound.
			if (current_ctx) {
				FGLContext* c = (FGLContext*)current_ctx;
				FGLRenderSurface* d = (FGLRenderSurface*)c->egl.draw;
				FGLRenderSurface* r = (FGLRenderSurface*)c->egl.read;

				if (d) {
					c->egl.draw = 0;
					d->ctx = EGL_NO_CONTEXT;
					d->disconnect();
				}

				if (r) {
					c->egl.read = 0;
					r->ctx = EGL_NO_CONTEXT;
					// FIXME: unlock/disconnect the read surface too
				}
			}
		}

		return EGL_TRUE;
	}

	setError(EGL_BAD_ACCESS);
	return EGL_FALSE;
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

	FGLContext* c = (FGLContext*)ctx;

	if (readdraw == EGL_READ)
		return c->egl.read;
	else if (readdraw == EGL_DRAW)
		return c->egl.draw;

	setError(EGL_BAD_ATTRIBUTE);
	return EGL_NO_SURFACE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void)
{
	EGLContext ctx = (EGLContext)getGlThreadSpecific();

	if (ctx == EGL_NO_CONTEXT)
		return EGL_NO_DISPLAY;

	FGLContext* c = (FGLContext*)ctx;
	return c->egl.dpy;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay dpy, EGLContext ctx,
			EGLint attribute, EGLint *value)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLContext* c = (FGLContext*)ctx;

	switch (attribute) {
	case EGL_CONFIG_ID:
		// Returns the ID of the EGL frame buffer configuration with
		// respect to which the context was created
		return getConfigAttrib(dpy, c->egl.config, EGL_CONFIG_ID, value);
	}

	setError(EGL_BAD_ATTRIBUTE);
	return EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglWaitGL(void)
{
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
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface* d = static_cast<FGLRenderSurface*>(surface);

	if (!d->isValid()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if (d->dpy != dpy) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	if (d->ctx != EGL_NO_CONTEXT) {
		FGLContext* c = (FGLContext*)d->ctx;
		if (c->egl.flags & FGL_IS_CURRENT)
			glFinish();
	}

	// post the surface
	d->swapBuffers();

	// if it's bound to a context, update the buffer
	if (d->ctx != EGL_NO_CONTEXT) {
		FGLContext* c = (FGLContext*)d->ctx;
		d->bindDrawSurface(c);
		// if this surface is also the read surface of the context
		// it is bound to, make sure to update the read buffer as well.
		// The EGL spec is a little unclear about this.

		if (c->egl.read == surface)
			d->bindReadSurface(c);
	}

	return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
			EGLNativePixmapType target)
{
	FUNC_UNIMPLEMENTED;
	return EGL_FALSE;
}

// ----------------------------------------------------------------------------
// ANDROID extensions
// ----------------------------------------------------------------------------

EGLBoolean eglSetSwapRectangleANDROID(EGLDisplay dpy, EGLSurface draw,
	EGLint left, EGLint top, EGLint width, EGLint height)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface* d = static_cast<FGLRenderSurface*>(draw);

	if (!d->isValid()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if (d->dpy != dpy) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	// post the surface
	d->setSwapRectangle(left, top, width, height);

	return EGL_TRUE;
}

EGLClientBuffer eglGetRenderBufferANDROID(EGLDisplay dpy, EGLSurface draw)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return (EGLClientBuffer)0;
	}

	FGLRenderSurface* d = static_cast<FGLRenderSurface*>(draw);

	if (!d->isValid()) {
		setError(EGL_BAD_SURFACE);
		return (EGLClientBuffer)0;
	}

	if (d->dpy != dpy) {
		setError(EGL_BAD_DISPLAY);
		return (EGLClientBuffer)0;
	}

	// post the surface
	return d->getRenderBuffer();
}

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
        EGLClientBuffer buffer, const EGLint *attrib_list)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_NO_IMAGE_KHR;
	}

	if (ctx != EGL_NO_CONTEXT) {
		setError(EGL_BAD_CONTEXT);
		return EGL_NO_IMAGE_KHR;
	}

	if (target != EGL_NATIVE_BUFFER_ANDROID) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	android_native_buffer_t* native_buffer = (android_native_buffer_t*)buffer;

	if (native_buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	if (native_buffer->common.version != sizeof(android_native_buffer_t)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	switch (native_buffer->format) {
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		//case HAL_PIXEL_FORMAT_RGB_888:
		case HAL_PIXEL_FORMAT_RGB_565:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		case HAL_PIXEL_FORMAT_RGBA_5551:
		case HAL_PIXEL_FORMAT_RGBA_4444:
			break;
		default:
			setError(EGL_BAD_PARAMETER);
			return EGL_NO_IMAGE_KHR;
	}

	native_buffer->common.incRef(&native_buffer->common);
	return (EGLImageKHR)native_buffer;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR img)
{
	if (!isDisplayValid(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	android_native_buffer_t* native_buffer = (android_native_buffer_t*)img;

	if (native_buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	if (native_buffer->common.version != sizeof(android_native_buffer_t)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	native_buffer->common.decRef(&native_buffer->common);

	return EGL_TRUE;
}

struct extention_map_t {
    const char * const name;
    __eglMustCastToProperFunctionPointerType address;
};

static const extention_map_t gExtentionMap[] = {
#if 0
	{ "glDrawTexsOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexsOES },
	{ "glDrawTexiOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexiOES },
	{ "glDrawTexfOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexfOES },
	{ "glDrawTexxOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexxOES },
	{ "glDrawTexsvOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexsvOES },
	{ "glDrawTexivOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexivOES },
	{ "glDrawTexfvOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexfvOES },
	{ "glDrawTexxvOES",
		(__eglMustCastToProperFunctionPointerType)&glDrawTexxvOES },
	{ "glQueryMatrixxOES",
		(__eglMustCastToProperFunctionPointerType)&glQueryMatrixxOES },
	{ "glEGLImageTargetTexture2DOES",
		(__eglMustCastToProperFunctionPointerType)&glEGLImageTargetTexture2DOES },
	{ "glEGLImageTargetRenderbufferStorageOES",
		(__eglMustCastToProperFunctionPointerType)&glEGLImageTargetRenderbufferStorageOES },
	{ "glClipPlanef",
		(__eglMustCastToProperFunctionPointerType)&glClipPlanef },
	{ "glClipPlanex",
		(__eglMustCastToProperFunctionPointerType)&glClipPlanex },
#endif
	{ "glBindBuffer",
		(__eglMustCastToProperFunctionPointerType)&glBindBuffer },
	{ "glBufferData",
		(__eglMustCastToProperFunctionPointerType)&glBufferData },
	{ "glBufferSubData",
		(__eglMustCastToProperFunctionPointerType)&glBufferSubData },
	{ "glDeleteBuffers",
		(__eglMustCastToProperFunctionPointerType)&glDeleteBuffers },
	{ "glGenBuffers",
		(__eglMustCastToProperFunctionPointerType)&glGenBuffers },
	{ "eglCreateImageKHR",
		(__eglMustCastToProperFunctionPointerType)&eglCreateImageKHR },
	{ "eglDestroyImageKHR",
		(__eglMustCastToProperFunctionPointerType)&eglDestroyImageKHR },
	{ "eglSetSwapRectangleANDROID",
		(__eglMustCastToProperFunctionPointerType)&eglSetSwapRectangleANDROID },
	{ "eglGetRenderBufferANDROID",
		(__eglMustCastToProperFunctionPointerType)&eglGetRenderBufferANDROID },
};

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY
eglGetProcAddress(const char *procname)
{
	extention_map_t const * const map = gExtentionMap;

	for (uint32_t i=0 ; i<NELEM(gExtentionMap) ; i++) {
		if (!strcmp(procname, map[i].name))
			return map[i].address;
	}

	//LOGE("Requested not implemented function %s address", procname);

	return NULL;
}
