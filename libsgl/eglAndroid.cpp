/*
 * libsgl/eglAndroid.cpp
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

#include <gralloc_priv.h>
#include <linux/android_pmem.h>
#include <ui/android_native_buffer.h>
#include <ui/PixelFormat.h>
#include <hardware/gralloc.h>
#include <linux/fb.h>

using namespace android;

/*
 * Configurations available on Android
 */

/*
 * In the lists below, attributes names MUST be sorted.
 * Additionally, all configs must be sorted according to
 * the EGL specification.
 */

/*
 * These configs can override the base attribute list
 */

/* RGB 565 configs */
static const FGLConfigPair configAttributes0[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
};

static const FGLConfigPair configAttributes1[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
};

static const FGLConfigPair configAttributes2[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
};

static const FGLConfigPair configAttributes3[] = {
	{ EGL_BUFFER_SIZE,     16 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        5 },
	{ EGL_GREEN_SIZE,       6 },
	{ EGL_RED_SIZE,         5 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
};

/* XBGR 888 configs */
static const FGLConfigPair configAttributes4[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

static const FGLConfigPair configAttributes5[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

static const FGLConfigPair configAttributes6[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

static const FGLConfigPair configAttributes7[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

/* ABGR 8888 configs */
static const FGLConfigPair configAttributes8[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

static const FGLConfigPair configAttributes9[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

static const FGLConfigPair configAttributes10[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

static const FGLConfigPair configAttributes11[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

/* ARGB 8888 configs */
static const FGLConfigPair configAttributes12[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

static const FGLConfigPair configAttributes13[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

static const FGLConfigPair configAttributes14[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

static const FGLConfigPair configAttributes15[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

/* Exported to platform-independent EGL code */
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
	{ configAttributes12, NELEM(configAttributes12) },
	{ configAttributes13, NELEM(configAttributes13) },
	{ configAttributes14, NELEM(configAttributes14) },
	{ configAttributes15, NELEM(configAttributes15) },
};

/* Exported to platform-independent EGL code */
const int gPlatformConfigsNum = NELEM(gPlatformConfigs);

/*
 * Android buffers
 * FIXME: Get framebuffer addresses properly.
 */

#define FB_DEVICE_NAME	"/dev/graphics/fb0"

static inline unsigned long getFramebufferAddress(void)
{
	static unsigned long address = 0;

	if (address != 0)
		return address;

	int fb_fd = open(FB_DEVICE_NAME, O_RDWR, 0);

	if (fb_fd == -1) {
		LOGE("EGL: GetFramebufferAddress: cannot open fb");
		return 0;
	}

	fb_fix_screeninfo finfo;
	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
		LOGE("EGL: Failed to get framebuffer address");
		close(fb_fd);
		return 0;
	}
	close(fb_fd);

	address = finfo.smem_start;
	return address;
}

static unsigned long fglGetBufferPhysicalAddress(android_native_buffer_t *buffer)
{
	const private_handle_t *hnd = (const private_handle_t *)buffer->handle;

	/* this pointer came from framebuffer */
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
		return getFramebufferAddress() + hnd->offset;

	/* this pointer came from pmem domain */
	pmem_region region;
	if (ioctl(hnd->fd, PMEM_GET_PHYS, &region) >= 0)
		return region.offset + hnd->offset;

	/* otherwise we can't do anything, but fail */
	LOGE("EGL: fglGetBufferPhysicalAddress failed");
	return 0;
}

/*
 * Android image surface
 */

class FGLImageSurface : public FGLSurface {
	const gralloc_module_t	*module;
	EGLImageKHR		image;
	android_native_buffer_t	*buffer;
	const private_handle_t	*handle;

public:
	FGLImageSurface(EGLClientBuffer img) :
		image(0)
	{
		android_native_buffer_t *b = (android_native_buffer_t *)img;

		if (!b || b->common.magic != ANDROID_NATIVE_BUFFER_MAGIC
		    || b->common.version != sizeof(android_native_buffer_t)) {
			LOGE("%s: Invalid EGLClientBuffer", __func__);
			return;
		}

		const private_handle_t *hnd =
				(const private_handle_t *)b->handle;
		if (!hnd) {
			LOGE("%s: Invalid buffer handle", __func__);
			return;
		}

		const hw_module_t *pModule;
		if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule)) {
			LOGE("%s: Could not get gralloc module", __func__);
			return;
		}

		module	= (const gralloc_module_t *)pModule;
		vaddr	= 0; /* Needs locking */
		paddr	= fglGetBufferPhysicalAddress(b);
		size	= hnd->size;;
		image	= img;
		buffer	= b;
		handle	= hnd;
	}

	virtual ~FGLImageSurface() {}

	virtual void flush(void)
	{
		struct pmem_region region;

		region.offset	= 0;
		region.len	= size;

		if (ioctl(handle->fd, PMEM_CACHE_FLUSH, &region) != 0)
			LOGW("Could not flush PMEM surface %d", handle->fd);
	}

	virtual int lock(int usage = 0)
	{
		return module->lock(module, handle, usage,
				0, 0, buffer->stride, buffer->height, &vaddr);
	}

	virtual int unlock(void)
	{
		return module->unlock(module, handle);
	}

	virtual bool isValid(void) { return image != 0; };
};

/*
 * Android native window render surface
 */

class FGLWindowSurface : public FGLRenderSurface {
	class Rect {
	public:
		int32_t left;
		int32_t top;
		int32_t right;
		int32_t bottom;

		inline Rect() {};

		inline Rect(int32_t w, int32_t h) :
			left(0),
			top(0),
			right(w),
			bottom(h) {}

		inline Rect(int32_t l, int32_t t, int32_t r, int32_t b) :
			left(l),
			top(t),
			right(r),
			bottom(b) {}

		Rect& andSelf(const Rect& r)
		{
			left	= max(left, r.left);
			top	= max(top, r.top);
			right	= min(right, r.right);
			bottom	= min(bottom, r.bottom);

			return *this;
		}

		bool isEmpty() const
		{
			return (left >= right || top >= bottom);
		}

		void dump(const char *what)
		{
			LOGD("%s { %5d, %5d, w=%5d, h=%5d }",
				what, left, top, right-left, bottom-top);
		}
	};

	class Region {
		Rect	storage[4];
		ssize_t	count;

	public:
		inline Region() :
			count(0) {}

		typedef const Rect *const_iterator;

		const_iterator begin() const { return storage; }
		const_iterator end() const { return storage+count; }

		static Region subtract(const Rect &lhs, const Rect &rhs)
		{
			Region reg;
			Rect *storage = reg.storage;

			if (lhs.isEmpty())
				return reg;

			if (lhs.top < rhs.top) {
				/* top rect */
				storage->left	= lhs.left;
				storage->top	= lhs.top;
				storage->right	= lhs.right;
				storage->bottom	= rhs.top;
				++storage;
			}

			const int32_t top = max(lhs.top, rhs.top);
			const int32_t bot = min(lhs.bottom, rhs.bottom);
			if (top < bot) {
				if (lhs.left < rhs.left) {
					/* left-side rect */
					storage->left	= lhs.left;
					storage->top	= top;
					storage->right	= rhs.left;
					storage->bottom	= bot;
					++storage;
				}

				if (lhs.right > rhs.right) {
					/* right-side rect */
					storage->left	= rhs.right;
					storage->top	= top;
					storage->right	= lhs.right;
					storage->bottom	= bot;
					++storage;
				}
			}

			if (lhs.bottom > rhs.bottom) {
				/* bottom rect */
				storage->left	= lhs.left;
				storage->top	= rhs.bottom;
				storage->right	= lhs.right;
				storage->bottom	= lhs.bottom;
				++storage;
			}

			reg.count = storage - reg.storage;
			return reg;
		}

		bool isEmpty() const { return count <= 0; }
	};

	android_native_window_t	*nativeWin;
	android_native_buffer_t	*buffer;
	android_native_buffer_t	*previousBuffer;
	const gralloc_module_t	*module;
	void			*bits;
	int			bytesPerPixel;
	Rect			dirtyRegion;
	Rect			oldDirtyRegion;

	int lock(android_native_buffer_t *buf, int usage, void **vaddr)
	{
		return module->lock(module, buf->handle,
				usage, 0, 0, buf->stride, buf->height, vaddr);
	}

	int unlock(android_native_buffer_t *buf)
	{
		if (!buf)
			return BAD_VALUE;

		return module->unlock(module, buf->handle);
	}

	inline int getBpp(int format)
	{
		switch(format) {
		case PIXEL_FORMAT_RGBA_8888:
		case PIXEL_FORMAT_RGBX_8888:
		case PIXEL_FORMAT_BGRA_8888:
			return 4;
		case PIXEL_FORMAT_RGB_565:
			return 2;
		default:
			return 0;
		}
	}

	void copyBlt(android_native_buffer_t *dst, void *dst_vaddr,
			android_native_buffer_t *src, const void *src_vaddr,
			const Region& clip)
	{
		/* NOTE: dst and src must be the same format */
		Region::const_iterator cur = clip.begin();
		Region::const_iterator end = clip.end();

		const size_t bpp = getBpp(src->format);
		const size_t dbpr = dst->stride * bpp;
		const size_t sbpr = src->stride * bpp;

		const uint8_t *src_bits = (const uint8_t *)src_vaddr;
		uint8_t *dst_bits = (uint8_t *)dst_vaddr;

		while (cur != end) {
			const Rect &r = *cur++;
			ssize_t w = r.right - r.left;
			ssize_t h = r.bottom - r.top;

			if (w <= 0 || h <= 0)
				continue;

			size_t size = w * bpp;
			size_t srcPos = r.left + src->stride * r.top;
			size_t dstPos = r.left + dst->stride * r.top;
			const uint8_t *s = src_bits + srcPos*bpp;
			uint8_t *d = dst_bits + dstPos*bpp;

			if (dbpr == sbpr && size == sbpr) {
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

	void doCopyBack()
	{
		dirtyRegion.andSelf(Rect(buffer->width, buffer->height));

		if (!previousBuffer)
			return;

		const Region copyBack =
				Region::subtract(oldDirtyRegion, dirtyRegion);
		if (copyBack.isEmpty())
			return;

		void *prevBits;
		int ret = lock(previousBuffer,
					GRALLOC_USAGE_SW_READ_OFTEN, &prevBits);
		if (!ret) {
			/* copy from previousBuffer to buffer */
			copyBlt(buffer, bits,
					previousBuffer, prevBits, copyBack);
			unlock(previousBuffer);
		}
	}

public:
	FGLWindowSurface(EGLDisplay dpy, uint32_t config,
				uint32_t colorFormat, uint32_t depthFormat,
				android_native_window_t *window) :
		FGLRenderSurface(dpy, config, colorFormat, depthFormat),
		nativeWin(window),
		buffer(0),
		previousBuffer(0),
		module(0),
		bits(0)
	{
		const hw_module_t *pModule;
		hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule);
		module = (const gralloc_module_t *)pModule;

		/* keep a reference on the window */
		nativeWin->common.incRef(&nativeWin->common);
		nativeWin->query(nativeWin,
					NATIVE_WINDOW_WIDTH, (int *)&width);
		nativeWin->query(nativeWin,
					NATIVE_WINDOW_HEIGHT, (int *)&height);

		/* store pixel size */
		const FGLPixelFormat *pix = FGLPixelFormat::get(colorFormat);
		bytesPerPixel = pix->pixelSize;
	}

	~FGLWindowSurface()
	{
		if (buffer)
			buffer->common.decRef(&buffer->common);

		if (previousBuffer)
			previousBuffer->common.decRef(&previousBuffer->common);

		nativeWin->common.decRef(&nativeWin->common);
	}

	virtual bool initCheck() const { return true; }

	virtual bool swapBuffers()
	{
		if (!buffer) {
			setError(EGL_BAD_ACCESS);
			return false;
		}

		if (!dirtyRegion.isEmpty()) {
			/*
			* Handle eglSetSwapRectangleANDROID()
			* We copyback from the front buffer
			*/
			doCopyBack();
			oldDirtyRegion = dirtyRegion;
		}

		if (previousBuffer) {
			previousBuffer->common.decRef(&previousBuffer->common);
			previousBuffer = 0;
		}

		unlock(buffer);
		previousBuffer = buffer;

		nativeWin->queueBuffer(nativeWin, buffer);
		buffer = 0;

		delete color;
		color = 0;

		/* dequeue a new buffer */
		if (nativeWin->dequeueBuffer(nativeWin, &buffer)) {
			setError(EGL_BAD_CURRENT_SURFACE);
			return false;
		}

		/*
		 * TODO: lockBuffer should rather be executed
		 * when the very first direct rendering occurs.
		 */
		nativeWin->lockBuffer(nativeWin, buffer);

		uint32_t oldSize = width*height;
		uint32_t newSize = buffer->stride*buffer->height;

		/* reallocate the depth-buffer if needed */
		if (depthFormat && oldSize != newSize) {
			delete depth;
			depth = new FGLLocalSurface(4 * newSize);
			if (!depth || !depth->isValid()) {
				delete depth;
				depth = 0;

				nativeWin->cancelBuffer(nativeWin, buffer);
				buffer = 0;

				setError(EGL_BAD_ALLOC);
				return false;
			}
		}

		width = buffer->stride;
		height = buffer->height;

		/* keep a reference on the buffer */
		buffer->common.incRef(&buffer->common);

		/* finally pin the buffer down */
		const int usage = GRALLOC_USAGE_SW_READ_RARELY
				| GRALLOC_USAGE_SW_WRITE_RARELY
				| GRALLOC_USAGE_HW_RENDER;
		if (lock(buffer, usage, &bits)) {
			LOGE("%s failed to lock buffer %p (%ux%u)",
					__func__, buffer, buffer->stride,
					buffer->height);

			nativeWin->cancelBuffer(nativeWin, buffer);
			buffer->common.decRef(&buffer->common);
			buffer = 0;

			delete depth;
			depth = 0;

			setError(EGL_BAD_ACCESS);
			return false;
		}

		color = new FGLExternalSurface(bits,
					fglGetBufferPhysicalAddress(buffer),
					width * height * bytesPerPixel);
		if (!color || !color->isValid()) {
			delete color;
			color = 0;

			unlock(buffer);
			nativeWin->cancelBuffer(nativeWin, buffer);
			buffer->common.decRef(&buffer->common);
			buffer = 0;

			delete depth;
			depth = 0;

			setError(EGL_BAD_ALLOC);
			return false;
		}

		return true;
	}

	virtual bool connect()
	{
		/* we're intending to do hardware rendering */
		const int usage = GRALLOC_USAGE_SW_READ_RARELY
				| GRALLOC_USAGE_SW_WRITE_RARELY
				| GRALLOC_USAGE_HW_RENDER;

		if (buffer)
			return false;

		native_window_set_usage(nativeWin, usage);

		/* dequeue a buffer */
		if (nativeWin->dequeueBuffer(nativeWin, &buffer)) {
			setError(EGL_BAD_ALLOC);
			return false;
		}

		width = buffer->stride;
		height = buffer->height;

		if (depthFormat) {
			/* allocate a corresponding depth-buffer */
			unsigned int size = width * height * 4;

			depth = new FGLLocalSurface(size);
			if (!depth || !depth->isValid()) {
				delete depth;
				depth = 0;

				nativeWin->cancelBuffer(nativeWin, buffer);
				buffer = 0;

				setError(EGL_BAD_ALLOC);
				return false;
			}
		}

		// keep a reference on the buffer
		buffer->common.incRef(&buffer->common);

		// Lock the buffer
		nativeWin->lockBuffer(nativeWin, buffer);

		// pin the buffer down
		if (lock(buffer, usage, &bits)) {
			LOGE("%s failed to lock buffer %p (%ux%u)",
					__func__, buffer, buffer->stride,
					buffer->height);

			nativeWin->cancelBuffer(nativeWin, buffer);
			buffer->common.decRef(&buffer->common);
			buffer = 0;

			setError(EGL_BAD_ACCESS);
			return false;
		}

		color = new FGLExternalSurface(bits,
					fglGetBufferPhysicalAddress(buffer),
					width * height * bytesPerPixel);
		if (!color || !color->isValid()) {
			delete color;
			color = 0;

			unlock(buffer);
			nativeWin->cancelBuffer(nativeWin, buffer);
			buffer->common.decRef(&buffer->common);
			buffer = 0;

			delete depth;
			depth = 0;

			setError(EGL_BAD_ALLOC);
			return false;
		}

		return true;
	}

	virtual void disconnect()
	{
		if (!buffer)
			return;

		delete color;
		color = 0;

		delete depth;
		depth = 0;

		if (bits) {
			bits = NULL;
			unlock(buffer);
		}

		/* enqueue the last frame */
		nativeWin->queueBuffer(nativeWin, buffer);
		buffer->common.decRef(&buffer->common);
		buffer = 0;

		if (previousBuffer) {
			previousBuffer->common.decRef(&previousBuffer->common);
			previousBuffer = 0;
		}
	}

	virtual EGLint getHorizontalResolution() const
	{
		return (nativeWin->xdpi * EGL_DISPLAY_SCALING)
							* (1.0f / 25.4f);
	}

	virtual EGLint getVerticalResolution() const
	{
		return (nativeWin->ydpi * EGL_DISPLAY_SCALING)
							* (1.0f / 25.4f);
	}

	virtual EGLint getRefreshRate() const
	{
		return (60 * EGL_DISPLAY_SCALING); /* FIXME */
	}

	virtual EGLint getSwapBehavior() const
	{
		/*
		 * EGL_BUFFER_PRESERVED means that eglSwapBuffers() completely
		 * preserves the content of the swapped buffer.
		 *
		 * EGL_BUFFER_DESTROYED means that the content of the buffer
		 * is lost.
		 *
		 * However when ANDROID_swap_retcangle is supported,
		 * EGL_BUFFER_DESTROYED only applies to the area specified
		 * by eglSetSwapRectangleANDROID(), that is, everything
		 * outside of this area is preserved.
		 *
		 * This implementation of EGL assumes the later case.
		 */
		return EGL_BUFFER_DESTROYED;
	}

	virtual bool setSwapRectangle(EGLint l, EGLint t, EGLint w, EGLint h)
	{
		dirtyRegion = Rect(l, t, l+w, t+h);
		return true;
	}

	virtual EGLClientBuffer getRenderBuffer() const { return buffer; }
};

/*
 * Platform glue
 */

static bool fglNativeToFGLPixelFormat(int format, uint32_t *fglFormat)
{
	uint32_t fmt;

	switch(format) {
	case HAL_PIXEL_FORMAT_BGRA_8888:
		fmt = FGL_PIXFMT_ARGB8888;
		break;
	case HAL_PIXEL_FORMAT_RGBA_8888:
		fmt = FGL_PIXFMT_ABGR8888;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		fmt = FGL_PIXFMT_XBGR8888;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		fmt = FGL_PIXFMT_RGB565;
		break;
	default:
		return false;
	}

	*fglFormat = fmt;
	return true;
}

FGLRenderSurface *platformCreateWindowSurface(EGLDisplay dpy,
		uint32_t config, uint32_t pixelFormat, uint32_t depthFormat,
		EGLNativeWindowType window)
{
	int format;
	android_native_window_t *win = (android_native_window_t *)window;

	if (win->common.magic != ANDROID_NATIVE_WINDOW_MAGIC) {
		setError(EGL_BAD_NATIVE_WINDOW);
		return NULL;
	}

	win->query(win, NATIVE_WINDOW_FORMAT, &format);

	if (!fglNativeToFGLPixelFormat(format, &pixelFormat)) {
		setError(EGL_BAD_MATCH);
		return NULL;
	}

	if (!fglEGLValidatePixelFormat(config, pixelFormat)) {
		setError(EGL_BAD_MATCH);
		return NULL;
	}

	return new FGLWindowSurface(dpy, config, pixelFormat, depthFormat, win);
}

/*
 * EGL extensions specific to Android
 */

EGLBoolean eglSetSwapRectangleANDROID(EGLDisplay dpy, EGLSurface draw,
			EGLint left, EGLint top, EGLint width, EGLint height)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLRenderSurface *d = (FGLRenderSurface *)draw;

	if (!d->isValid()) {
		setError(EGL_BAD_SURFACE);
		return EGL_FALSE;
	}

	if(!d->setSwapRectangle(left, top, width, height))
		return EGL_FALSE;

	return EGL_TRUE;
}

EGLClientBuffer eglGetRenderBufferANDROID(EGLDisplay dpy, EGLSurface draw)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return (EGLClientBuffer)0;
	}

	FGLRenderSurface *d = (FGLRenderSurface *)draw;

	if (!d->isValid() || d->isTerminated()) {
		setError(EGL_BAD_SURFACE);
		return (EGLClientBuffer)0;
	}

	return d->getRenderBuffer();
}

struct FGLAndroidImage : FGLImage {
	FGLAndroidImage(void *buf, uint32_t fmt)
	{
		pixelFormat	= fmt;
		buffer		= buf;

		android_native_buffer_t *native_buffer =
						(android_native_buffer_t *)buf;
		width = native_buffer->stride;
		height = native_buffer->height;

		native_buffer->common.incRef(&native_buffer->common);

		surface = new FGLImageSurface(native_buffer);
	}

	virtual ~FGLAndroidImage()
	{
		android_native_buffer_t *native_buffer =
					(android_native_buffer_t *)buffer;

		native_buffer->common.decRef(&native_buffer->common);

		delete surface;
	}
};

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx,
					EGLenum target, EGLClientBuffer buffer,
					const EGLint *attrib_list)
{
	if (!fglEGLValidateDisplay(dpy)) {
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

	android_native_buffer_t *native_buffer =
					(android_native_buffer_t *)buffer;

	if (native_buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	if (native_buffer->common.version != sizeof(android_native_buffer_t)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	uint32_t pixelFormat;

	switch (native_buffer->format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		pixelFormat = FGL_PIXFMT_ABGR8888;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		pixelFormat = FGL_PIXFMT_XBGR8888;
		break;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		pixelFormat = FGL_PIXFMT_ARGB8888;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		pixelFormat = FGL_PIXFMT_RGB565;
		break;
	case HAL_PIXEL_FORMAT_RGBA_5551:
		pixelFormat = FGL_PIXFMT_RGBA5551;
		break;
	case HAL_PIXEL_FORMAT_RGBA_4444:
		pixelFormat = FGL_PIXFMT_RGBA4444;
		break;
	default:
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	FGLImage *image = new FGLAndroidImage(native_buffer, pixelFormat);
	if (!image || !image->isValid()) {
		delete image;
		setError(EGL_BAD_ALLOC);
		return EGL_NO_IMAGE_KHR;
	}

	return (EGLImageKHR)image;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR img)
{
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return EGL_FALSE;
	}

	FGLImage *image = (FGLImage *)img;
	if (!image || !image->isValid()) {
		setError(EGL_BAD_PARAMETER);
		return EGL_FALSE;
	}

	image->terminate();

	return EGL_TRUE;
}

#define EGLFunc	__eglMustCastToProperFunctionPointerType

const FGLExtensionMap gPlatformExtensionMap[] = {
	{ "eglCreateImageKHR",
		(EGLFunc)&eglCreateImageKHR },
	{ "eglDestroyImageKHR",
		(EGLFunc)&eglDestroyImageKHR },
	{ "eglSetSwapRectangleANDROID",
		(EGLFunc)&eglSetSwapRectangleANDROID },
	{ "eglGetRenderBufferANDROID",
		(EGLFunc)&eglGetRenderBufferANDROID },
	{ NULL, NULL },
};
