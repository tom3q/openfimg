/*
 * libsgl/eglFramebuffer.cpp
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
#include <poll.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm/exynos_drm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "eglCommon.h"
#include "platform.h"
#include "fglrendersurface.h"
#include "fglimage.h"
#include "common.h"
#include "types.h"
#include "libfimg/fimg.h"
#include "fglsurface.h"

/*
 * Configurations available for direct rendering into frame buffer
 */

#define BUFFER_COUNT	2

/*
 * In the lists below, attributes names MUST be sorted.
 * Additionally, all configs must be sorted according to
 * the EGL specification.
 */

/*
 * These configs can override the base attribute list
 * NOTE: when adding a config here, don't forget to update eglCreate*Surface()
 */

/* RGB 565 configs */

/** RGB565, no depth, no stencil configuration */
static const FGLConfigPair configAttributes0[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** RGB565, no depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes1[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
};

/** RGB565, 24-bit depth, no stencil configuration */
static const FGLConfigPair configAttributes2[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** RGB565, 24-bit depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes3[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
};

/* RGB 888 configs */

/** XRGB8888, no depth, no stencil configuration */
static const FGLConfigPair configAttributes4[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** XRGB8888, no depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes5[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
};

/** XRGB8888, 24-bit depth, no stencil configuration */
static const FGLConfigPair configAttributes6[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** XRGB8888, 24-bit depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes7[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
};

/* ARGB 8888 configs */

/** ARGB8888, no depth, no stencil configuration */
static const FGLConfigPair configAttributes8[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** ARGB8888, no depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes9[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
};

/** ARGB8888, 24-bit depth, no stencil configuration */
static const FGLConfigPair configAttributes10[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
};

/** ARGB8888, 24-bit depth, 8-bit stencil configuration */
static const FGLConfigPair configAttributes11[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
};

/**
 * EGL configurations supported by direct framebuffer access.
 * Exported to platform-independent EGL code.
 */
const FGLConfigs gPlatformConfigs[] = {
	{ configAttributes0, NELEM(configAttributes0)   },
	{ configAttributes1, NELEM(configAttributes1)   },
	{ configAttributes2, NELEM(configAttributes2)   },
	{ configAttributes3, NELEM(configAttributes3)   },
	{ configAttributes4, NELEM(configAttributes4)   },
	{ configAttributes5, NELEM(configAttributes5)   },
	{ configAttributes6, NELEM(configAttributes6)   },
	{ configAttributes7, NELEM(configAttributes7)   },
	{ configAttributes8, NELEM(configAttributes8)   },
	{ configAttributes9, NELEM(configAttributes9)   },
	{ configAttributes10, NELEM(configAttributes10) },
	{ configAttributes11, NELEM(configAttributes11) },
};

/** Number of exported EGL configurations */
const int gPlatformConfigsNum = NELEM(gPlatformConfigs);

/**
 * Framebuffer window render surface.
 * Provides framebuffer for rendering operations directly from Linux
 * framebuffer device.
 */
class FGLFramebufferWindowSurface : public FGLRenderSurface {
	int fd;
	drmModeConnector *conn;
	drmModeEncoder *enc;
	FGLSurface *surfaces[BUFFER_COUNT];
	uint32_t handles[BUFFER_COUNT];
	uint32_t fb[BUFFER_COUNT];
	unsigned int frontIdx;
	unsigned int backIdx;
	int bytesPerPixel;
	unsigned long size;
	uint32_t kmsFormat;
	bool needsModeset;
	volatile bool pageFlipDone;

	drmModeConnector *findConnector(drmModeRes *res, uint32_t id)
	{
		drmModeConnector *conn = NULL;
		int i;

		/* iterate all connectors */
		for (i = 0; i < res->count_connectors; ++i) {
			conn = drmModeGetConnector(fd, res->connectors[i]);
			if (!conn)
				continue;

			if (conn->connector_id == id)
				break;

			drmModeFreeConnector(conn);
		}

		if (i == res->count_connectors)
			return NULL;

		return conn;
	}

	drmModeEncoder *findEncoder(drmModeRes *res, uint32_t id)
	{
		drmModeEncoder *enc = NULL;
		int i;

		/* iterate all connectors */
		for (i = 0; i < res->count_encoders; ++i) {
			enc = drmModeGetEncoder(fd, res->encoders[i]);
			if (!enc)
				continue;

			if (enc->encoder_id == id)
				break;

			drmModeFreeEncoder(enc);
		}

		if (i == res->count_encoders)
			return NULL;

		return enc;
	}

	int createBuffer(unsigned int *handle)
	{
		struct drm_exynos_gem_create req;
		int ret;

		req.size = size;
		req.flags = EXYNOS_BO_UNMAPPED | EXYNOS_BO_CACHABLE;
		req.handle = 0;

		ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_CREATE, &req);
		if (ret < 0) {
			LOGE("failed to create GEM (%d)", ret);
			return ret;
		}

		*handle = req.handle;
		return 0;
	}

	int exportBuffer(unsigned int handle)
	{
		int ret;
		int bufFd;

		ret = drmPrimeHandleToFD(fd, handle, 0, &bufFd);
		if (ret < 0) {
			LOGE("failed to export GEM (%d)", ret);
			return ret;
		}

		return bufFd;
	}

	void releaseBuffer(unsigned int handle)
	{
		struct drm_gem_close req;
		int ret;

		req.handle = handle;

		ret = ioctl(fd, DRM_IOCTL_GEM_CLOSE, &req);
		if (ret < 0)
			LOGE("failed to destroy GEM (%d)", ret);
	}

	bool allocateBuffers()
	{
		unsigned int i;

		for (i = 0; i < BUFFER_COUNT; ++i) {
			int fbFd, ret;
			uint32_t h[4], p[4], o[4];

			ret = createBuffer(&handles[i]);
			if (ret < 0) {
				LOGE("failed to create buffer");
				return false;
			}

			fbFd = exportBuffer(handles[i]);
			if (fd < 0) {
				LOGE("failed to export buffer");
				releaseBuffer(handles[i]);
				freeBuffers();
				return false;
			}

			h[0] = handles[i];
			p[0] = bytesPerPixel * width;
			o[0] = 0;

			ret = drmModeAddFB2(fd, width, height, kmsFormat,
							h, p, o, &fb[i], 0);
			if (ret < 0) {
				LOGE("failed to add framebuffer");
				close(fbFd);
				releaseBuffer(handles[i]);
				freeBuffers();
				return false;
			}

			surfaces[i] = new FGLExternalSurface(fbFd, 0, size);
			if (!surfaces[i] || !surfaces[i]->isValid()) {
				LOGE("failed to create surface");
				freeBuffers();
				return false;
			}
		}

		return true;
	}

	void freeBuffers()
	{
		unsigned int i;

		for (i = 0; i < BUFFER_COUNT; ++i) {
			if (!surfaces[i])
				break;

			delete surfaces[i];
			drmModeRmFB(fd, fb[i]);
			releaseBuffer(handles[i]);
		}
	}

	static void pageFlipHandler(int fd, unsigned int frame,
				unsigned int sec, unsigned int usec, void *data)
	{
		volatile bool *done = (bool *)data;

		*done = true;
	}

	bool waitForPageFlip(void)
	{
		drmEventContext evctx;
		struct pollfd pollFd;
		int ret;

		memset(&pollFd, 0 , sizeof(pollFd));
		pollFd.fd = fd;
		pollFd.events = POLLIN;

		memset (&evctx, 0, sizeof evctx);
		evctx.version = DRM_EVENT_CONTEXT_VERSION;
		evctx.page_flip_handler = pageFlipHandler;

		do {
			ret = poll(&pollFd, 1, 100);
			if (ret < 0)
				return false;

			ret = drmHandleEvent (fd, &evctx);
			if (ret < 0)
				return false;
		} while (!pageFlipDone);

		return true;
	}

public:
	/**
	 * Class constructor.
	 * Constructs a window surface from Linux framebuffer device.
	 * @param dpy EGL display to which the surface shall belong.
	 * @param config Index of EGL configuration to use.
	 * @param pixelFormat Requested color format. (See ::FGLPixelFormatEnum)
	 * @param depthFormat Requested depth format.
	 * @param fileDesc File descriptor of opened framebuffer device.
	 */
	FGLFramebufferWindowSurface(EGLDisplay dpy, uint32_t config,
				uint32_t pixelFormat, uint32_t depthFormat,
				uint32_t connector) :
		FGLRenderSurface(dpy, config, pixelFormat, depthFormat),
		backIdx(0),
		bytesPerPixel(0),
		needsModeset(true)
	{
		const FGLPixelFormat *pix;
		drmModeRes *res;

		pix = FGLPixelFormat::get(pixelFormat);
		if (!pix || !pix->fourCC) {
			LOGE("invalid pixel format (%u)", pixelFormat);
			return;
		}

		memset(surfaces, 0, sizeof(surfaces));

		fd = drmOpen("exynos", NULL);
		if (fd < 0) {
			LOGE("failed to open DRM device: %s", strerror(errno));
			setError(EGL_BAD_ACCESS);
			return;
		}

		/* retrieve resources */
		res = drmModeGetResources(fd);
		if (!res) {
			LOGE("could not retrieve DRM resources: %s",
							strerror(errno));
			setError(EGL_BAD_ACCESS);
			goto err_close;
		}

		conn = findConnector(res, connector);
		if (!conn) {
			LOGE("could not find requested connector (%u)",
								connector);
			setError(EGL_BAD_NATIVE_WINDOW);
			goto err_free_res;
		}

		/* check if there is at least one valid mode */
		if (conn->count_modes == 0) {
			LOGE("no valid mode for connector %u\n",
							conn->connector_id);
			setError(EGL_BAD_NATIVE_WINDOW);
			goto err_free_conn;
		}

		enc = findEncoder(res, conn->encoder_id);
		if (!enc) {
			LOGE("could not find encoder for connector");
			setError(EGL_BAD_NATIVE_WINDOW);
			goto err_free_conn;
		}

		width = conn->modes[0].hdisplay;
		height = conn->modes[0].vdisplay;
		bytesPerPixel = pix->pixelSize;
		size = width * height * bytesPerPixel;
		kmsFormat = pix->fourCC;

		if (!allocateBuffers()) {
			LOGE("failed to allocate frame buffers");
			setError(EGL_BAD_ALLOC);
			goto err_free_enc;
		}

		drmModeFreeResources(res);
		return;

		err_free_enc:
			drmModeFreeEncoder(enc);
		err_free_conn:
			drmModeFreeConnector(conn);
		err_free_res:
			drmModeFreeResources(res);
		err_close:
			close(fd);

		fd = -1;
	}

	/** Class destructor. */
	~FGLFramebufferWindowSurface()
	{
		if (!initCheck())
			return;

		freeBuffers();
		drmModeFreeEncoder(enc);
		drmModeFreeConnector(conn);
		close(fd);

		delete depth;
	}

	virtual bool swapBuffers()
	{
		FGLContext *ctx = this->ctx;

		unbindContext();

		if (needsModeset) {
			drmModeSetCrtc(fd, enc->crtc_id, fb[backIdx],
				0, 0, &conn->connector_id, 1, &conn->modes[0]);
			needsModeset = false;
		} else {
			pageFlipDone = false;
			drmModePageFlip(fd, enc->crtc_id, fb[backIdx],
				DRM_MODE_PAGE_FLIP_EVENT, (void *)&pageFlipDone);
			waitForPageFlip();
		}

		frontIdx = (frontIdx + 1) % BUFFER_COUNT;
		backIdx = (backIdx + 1) % BUFFER_COUNT;

		color = surfaces[backIdx];

		if (ctx && !bindContext(ctx))
			return false;

		return true;
	}

	virtual bool allocate(FGLContext *fgl)
	{
		if (depthFormat) {
			unsigned int size = width * height * 4;

			delete depth;
			depth = new FGLLocalSurface(size);
			if (!depth) {
				setError(EGL_BAD_ALLOC);
				return false;
			}
		}

		color = surfaces[backIdx];
		needsModeset = true;
		return true;
	}

	virtual bool initCheck() const
	{
		return fd >= 0;
	}

	virtual EGLint getSwapBehavior() const
	{
		return EGL_BUFFER_DESTROYED;
	}
};

/**
 * Creates native window surface based on user parameters.
 * This function is a glue between generic and platform-specific EGL parts.
 * @param dpy EGL display to which the surface shall belong.
 * @param config EGL configuration which shall be used.
 * @param pixelFormat Preferred pixel format.
 * @param depthFormat Requested depth format.
 * @param window EGL native window backing the surface.
 * @return Created render surface or NULL on error.
 */
FGLRenderSurface *platformCreateWindowSurface(EGLDisplay dpy,
		uint32_t config, uint32_t pixelFormat, uint32_t depthFormat,
		EGLNativeWindowType window)
{
	return new FGLFramebufferWindowSurface(dpy, config,
				pixelFormat, depthFormat, (uint32_t)window);
}

#define EGLFunc	__eglMustCastToProperFunctionPointerType

/** List of platform-specific EGL functions */
const FGLExtensionMap gPlatformExtensionMap[] = {
	{ NULL, NULL },
};
