/*
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

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "eglCommon.h"
#include "platform.h"
#include "fglrendersurface.h"
#include "fglimage.h"
#include "common.h"
#include "types.h"
#include "libfimg/fimg.h"
#include "eglMem.h"

#include "../libgralloc/gralloc_priv.h"
#include <linux/android_pmem.h>
#include <private/ui/sw_gralloc_handle.h>
#include <ui/android_native_buffer.h>
#include <ui/PixelFormat.h>
#include <hardware/copybit.h>
#include <hardware/gralloc.h>

using namespace android;

/*
 * Configurations available on Android
 */

/*
* In the lists below, attributes names MUST be sorted.
* Additionally, all configs must be sorted according to
* the EGL specification.
*/

// These configs can override the base attribute list
// NOTE: when adding a config here, don't forget to update eglCreate*Surface()

// RGB 565 configs
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
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGB_565 },
};

// RGB 888 configs
static const FGLConfigPair configAttributes2[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

static const FGLConfigPair configAttributes3[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       0 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBX_8888 },
};

// ARGB 8888 configs
static const FGLConfigPair configAttributes4[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

static const FGLConfigPair configAttributes5[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_RGBA_8888 },
};

static const FGLConfigPair configAttributes6[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,       0 },
	{ EGL_STENCIL_SIZE,     0 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

static const FGLConfigPair configAttributes7[] = {
	{ EGL_BUFFER_SIZE,     32 },
	{ EGL_ALPHA_SIZE,       8 },
	{ EGL_BLUE_SIZE,        8 },
	{ EGL_GREEN_SIZE,       8 },
	{ EGL_RED_SIZE,         8 },
	{ EGL_DEPTH_SIZE,      24 },
	{ EGL_STENCIL_SIZE,     8 },
	{ EGL_NATIVE_VISUAL_ID, GGL_PIXEL_FORMAT_BGRA_8888 },
};

const FGLConfigs gPlatformConfigs[] = {
	{ configAttributes0, NELEM(configAttributes0) },
	{ configAttributes1, NELEM(configAttributes1) },
	{ configAttributes2, NELEM(configAttributes2) },
	{ configAttributes3, NELEM(configAttributes3) },
	{ configAttributes4, NELEM(configAttributes4) },
	{ configAttributes5, NELEM(configAttributes5) },
	{ configAttributes6, NELEM(configAttributes6) },
	{ configAttributes7, NELEM(configAttributes7) },
};

const int gPlatformConfigsNum = NELEM(gPlatformConfigs);

/*
 * Android buffers
 */

#define FB_DEVICE_NAME "/dev/graphics/fb0"
static inline unsigned long getFramebufferAddress(void)
{
	static unsigned long address = 0;

	//LOGD("getFramebufferAddress");

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
	const private_handle_t* hnd =
			static_cast<const private_handle_t*>(buffer->handle);

	//LOGD("fglGetBufferPhysicalAddress");

	// this pointer came from framebuffer
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
		return getFramebufferAddress() + hnd->offset;

	// this pointer came from pmem domain
	pmem_region region;
	if (ioctl(hnd->fd, PMEM_GET_PHYS, &region) >= 0)
		return region.offset + hnd->offset;

	// otherwise we failed
	LOGE("EGL: fglGetBufferPhysicalAddress failed");
	return 0;
}

class FGLImageSurface : public FGLSurface {
	gralloc_module_t const* module;
	EGLImageKHR	image;
public:
			FGLImageSurface(EGLClientBuffer img);
	virtual		~FGLImageSurface();

	virtual void	flush(void);
	virtual int	lock(int usage = 0);
	virtual int	unlock(void);

	virtual bool	isValid(void) { return image != 0; };
};

FGLImageSurface::FGLImageSurface(EGLClientBuffer img)
	: image(0)
{
	android_native_buffer_t *buffer = (android_native_buffer_t *)img;

	if (buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC
	    || buffer->common.version != sizeof(android_native_buffer_t)) {
		LOGE("%s: Invalid EGLClientBuffer", __func__);
		return;
	}

	const private_handle_t* hnd =
			static_cast<const private_handle_t*>(buffer->handle);
	if (!hnd) {
		LOGE("%s: Invalid buffer handle", __func__);
		return;
	}

	hw_module_t const* pModule;
	if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule)) {
		LOGE("%s: Could not get gralloc module", __func__);
		return;
	}

	module = reinterpret_cast<gralloc_module_t const*>(pModule);

	vaddr = 0; /* Needs locking */
	paddr = fglGetBufferPhysicalAddress(buffer);
	size = hnd->size;;
	image = img;

	//LOGD("FGLImageSurface (vaddr = %p, paddr = %08x, size = %u)",
	//			vaddr, paddr, size);
}

FGLImageSurface::~FGLImageSurface()
{
	//LOGD("~FGLImageSurface (vaddr = %p, paddr = %08x, size = %u)",
	//			vaddr, paddr, size);
}

int FGLImageSurface::lock(int usage)
{
	android_native_buffer_t *buf = (android_native_buffer_t *)image;
	int err;

	if (sw_gralloc_handle_t::validate(buf->handle) < 0) {
		err = module->lock(module, buf->handle,
			usage, 0, 0, buf->width, buf->height, &vaddr);
	} else {
		sw_gralloc_handle_t const* hnd =
			reinterpret_cast<sw_gralloc_handle_t const*>(buf->handle);
		vaddr = (void*)hnd->base;
		err = FGL_NO_ERROR;
	}

	return err;
}

int FGLImageSurface::unlock(void)
{
	int err = 0;
	android_native_buffer_t *buf = (android_native_buffer_t *)image;

	if (!buf)
		return BAD_VALUE;

	if (sw_gralloc_handle_t::validate(buf->handle) < 0)
		err = module->unlock(module, buf->handle);

	return err;
}

void FGLImageSurface::flush(void)
{
	android_native_buffer_t *buffer = (android_native_buffer_t *)image;
	const private_handle_t* hnd =
			static_cast<const private_handle_t*>(buffer->handle);
	struct pmem_region region;

	region.offset = 0;
	region.len = size;

	if (ioctl(hnd->fd, PMEM_CACHE_FLUSH, &region) != 0)
		LOGW("Could not flush PMEM surface %d", hnd->fd);
}

/*
 * Android native window surface
 */

struct FGLWindowSurface : public FGLRenderSurface
{
	FGLWindowSurface(
		EGLDisplay dpy, EGLConfig config,
		int32_t depthFormat,
		android_native_window_t *window,
		int32_t pixelFormat);

	~FGLWindowSurface();

	virtual     bool        initCheck() const { return true; } // TODO: report failure if ctor fails
	virtual     EGLBoolean  swapBuffers();
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
	FGLint lock(android_native_buffer_t *buf, int usage, void **vaddr);
	FGLint unlock(android_native_buffer_t *buf);
	android_native_window_t	*nativeWindow;
	android_native_buffer_t	*buffer;
	android_native_buffer_t	*previousBuffer;
	const gralloc_module_t	*module;
	copybit_device_t	*blitengine;
	void			*bits;
	int			bytesPerPixel;

	struct Rect {
		inline Rect() { };

		inline Rect(int32_t w, int32_t h)
		: left(0), top(0), right(w), bottom(h) { }

		inline Rect(int32_t l, int32_t t, int32_t r, int32_t b)
		: left(l), top(t), right(r), bottom(b) { }

		Rect& andSelf(const Rect& r)
		{
			left   = max(left, r.left);
			top    = max(top, r.top);
			right  = min(right, r.right);
			bottom = min(bottom, r.bottom);
			return *this;
		}

		bool isEmpty() const
		{
			return (left>=right || top>=bottom);
		}

		void dump(const char *what)
		{
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

		typedef const Rect *const_iterator;

		const_iterator begin() const { return storage; }
		const_iterator end() const { return storage+count; }

		static Region subtract(const Rect& lhs, const Rect& rhs)
		{
			Region reg;
			Rect *storage = reg.storage;
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

		bool isEmpty() const
		{
			return count<=0;
		}
	private:
		Rect storage[4];
		ssize_t count;
	};

	struct region_iterator : public copybit_region_t {
		region_iterator(const Region& region)
		: b(region.begin()), e(region.end())
		{
			this->next = iterate;
		}
	private:
		static int iterate(const copybit_region_t *self, copybit_rect_t *rect)
		{
			const region_iterator *me = static_cast<const region_iterator *>(self);
			if (me->b != me->e) {
				*reinterpret_cast<Rect *>(rect) = *me->b++;
				return 1;
			}
			return 0;
		}

		mutable Region::const_iterator b;
		Region::const_iterator const e;
	};

	void copyBlt(
		android_native_buffer_t *dst, void *dst_vaddr,
		android_native_buffer_t *src, const void *src_vaddr,
		const Region& clip);

	Rect dirtyRegion;
	Rect oldDirtyRegion;
};

FGLWindowSurface::FGLWindowSurface(EGLDisplay dpy,
	EGLConfig config,
	int32_t depthFormat,
	android_native_window_t *window,
	int32_t pixelFormat)
	: FGLRenderSurface(dpy, config, pixelFormat, depthFormat),
	nativeWindow(window), buffer(0), previousBuffer(0), module(0),
	blitengine(0), bits(0)
{
	const hw_module_t *pModule;
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule);
	module = reinterpret_cast<const gralloc_module_t *>(pModule);
	int format;

	//if (hw_get_module(COPYBIT_HARDWARE_MODULE_ID, &pModule) == 0) {
	//	copybit_open(pModule, &blitengine);
	//}

	// keep a reference on the window
	nativeWindow->common.incRef(&nativeWindow->common);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_WIDTH, &width);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_HEIGHT, &height);
	nativeWindow->query(nativeWindow, NATIVE_WINDOW_FORMAT, &format);

	switch(format) {
	case HAL_PIXEL_FORMAT_BGRA_8888:
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_RGBX_8888:
		bytesPerPixel = 4;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
	case HAL_PIXEL_FORMAT_RGBA_4444:
	case HAL_PIXEL_FORMAT_RGBA_5551:
		bytesPerPixel = 2;
		break;
	default:
		bytesPerPixel = 0;
	}
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
	native_window_set_usage(nativeWindow, GRALLOC_USAGE_SW_READ_RARELY
		| GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER);

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
	if (lock(buffer, GRALLOC_USAGE_SW_READ_RARELY
		| GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER,
		&bits) != FGL_NO_ERROR)
	{
		LOGE("connect() failed to lock buffer %p (%ux%u)",
			buffer, buffer->width, buffer->height);
		setError(EGL_BAD_ACCESS);
		return EGL_FALSE;
		// FIXME:
	}

	delete color;
	color = new FGLExternalSurface(bits,
			fglGetBufferPhysicalAddress(buffer),
			stride * height * bytesPerPixel);

	return EGL_TRUE;
}

void FGLWindowSurface::disconnect()
{
	delete color;
	color = 0;

	if (depthFormat) {
		delete depth;
		depth = 0;
	}

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
	android_native_buffer_t *buf, int usage, void **vaddr)
{
	int err;
	if (sw_gralloc_handle_t::validate(buf->handle) < 0) {
		err = module->lock(module, buf->handle,
			usage, 0, 0, buf->width, buf->height, vaddr);
	} else {
		const sw_gralloc_handle_t *hnd =
			reinterpret_cast<const sw_gralloc_handle_t *>(buf->handle);
		*vaddr = (void *)hnd->base;
		err = FGL_NO_ERROR;
	}
	return err;
}

FGLint FGLWindowSurface::unlock(android_native_buffer_t *buf)
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
	case PIXEL_FORMAT_BGRA_8888:
		return 4;
	case PIXEL_FORMAT_RGB_565:
		return 2;
	default:
		return 0;
	}
}

void FGLWindowSurface::copyBlt(
	android_native_buffer_t *dst, void *dst_vaddr,
	android_native_buffer_t *src, const void *src_vaddr,
	const Region& clip)
{
	// NOTE: dst and src must be the same format
#if 0
	FGLint err = FGL_NO_ERROR;

	copybit_device_t *const copybit = blitengine;
	if (copybit)  {
		copybit_image_t simg;
		simg.w = src->stride;
		simg.h = src->height;
		simg.format = src->format;
		simg.handle = const_cast<native_handle_t *>(src->handle);

		copybit_image_t dimg;
		dimg.w = dst->stride;
		dimg.h = dst->height;
		dimg.format = dst->format;
		dimg.handle = const_cast<native_handle_t *>(dst->handle);

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

	const uint8_t *const src_bits = (const uint8_t *)src_vaddr;
	uint8_t       *const dst_bits = (uint8_t       *)dst_vaddr;

	while (cur != end) {
		const Rect& r(*cur++);
		ssize_t w = r.right - r.left;
		ssize_t h = r.bottom - r.top;

		if (w <= 0 || h<=0) continue;

		size_t size = w * bpp;
		const uint8_t *s = src_bits + (r.left + src->stride * r.top) * bpp;
		uint8_t       *d = dst_bits + (r.left + dst->stride * r.top) * bpp;

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
			const Region copyBack(Region::subtract(oldDirtyRegion, dirtyRegion));
			if (!copyBack.isEmpty()) {
				void *prevBits;
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
	if (nativeWindow->dequeueBuffer(nativeWindow, &buffer)) {
		delete color;
		color = 0;
		setError(EGL_BAD_CURRENT_SURFACE);
		return EGL_FALSE;
	}

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
	}

	// keep a reference on the buffer
	buffer->common.incRef(&buffer->common);

	// finally pin the buffer down
	if (lock(buffer, GRALLOC_USAGE_SW_READ_RARELY
		| GRALLOC_USAGE_SW_WRITE_RARELY | GRALLOC_USAGE_HW_RENDER,
		&bits) != FGL_NO_ERROR)
	{
		LOGE("eglSwapBuffers() failed to lock buffer %p (%ux%u)",
			buffer, buffer->width, buffer->height);
		setError(EGL_BAD_ACCESS);
		return EGL_FALSE;
		// FIXME: we should make sure we're not accessing the buffer anymore
	}

	delete color;
	color = new FGLExternalSurface(bits,
			fglGetBufferPhysicalAddress(buffer),
			stride * height * bytesPerPixel);

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

FGLRenderSurface *platformCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
	int32_t depthFormat, EGLNativeWindowType window, int32_t pixelFormat)
{
	android_native_window_t *win = 
				static_cast<android_native_window_t *>(window);

	if (win->common.magic != ANDROID_NATIVE_WINDOW_MAGIC) {
		setError(EGL_BAD_NATIVE_WINDOW);
		return NULL;
	}

	return new FGLWindowSurface(dpy, config, depthFormat, win, pixelFormat);
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

	FGLRenderSurface *d = static_cast<FGLRenderSurface *>(draw);

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
	if (!fglEGLValidateDisplay(dpy)) {
		setError(EGL_BAD_DISPLAY);
		return (EGLClientBuffer)0;
	}

	FGLRenderSurface *d = static_cast<FGLRenderSurface *>(draw);

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

struct FGLAndroidImage : FGLImage {
#define FGL_IMAGE_MAGIC 0x474d4941
	uint32_t magic;

	FGLAndroidImage(void *buf, uint32_t fmt, bool swap, bool argb) :
		magic(FGL_IMAGE_MAGIC)
	{
		pixelFormat	= fmt;
		buffer		= buf;
		swapNeeded	= swap;
		isARGB		= argb;

		android_native_buffer_t *native_buffer =
						(android_native_buffer_t *)buf;
		stride = native_buffer->stride;
		height = native_buffer->height;

		surface = new FGLImageSurface(native_buffer);
	}

	virtual ~FGLAndroidImage()
	{
		if (!isValid())
			return;

		delete surface;
	}

	virtual bool isValid()
	{
		return magic == FGL_IMAGE_MAGIC && surface != NULL;
	}
};

EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
        EGLClientBuffer buffer, const EGLint *attrib_list)
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

	android_native_buffer_t *native_buffer = (android_native_buffer_t *)buffer;

	if (native_buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	if (native_buffer->common.version != sizeof(android_native_buffer_t)) {
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	bool swapNeeded = false;
	bool isARGB = false;
	uint32_t pixelFormat;

	switch (native_buffer->format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		swapNeeded = true;
		pixelFormat = FGPF_COLOR_MODE_8888;
		break;
	case HAL_PIXEL_FORMAT_RGBX_8888:
		swapNeeded = true;
		pixelFormat = FGPF_COLOR_MODE_0888;
		break;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		isARGB = true;
		pixelFormat = FGPF_COLOR_MODE_8888;
		break;
	case HAL_PIXEL_FORMAT_RGB_565:
		pixelFormat = FGPF_COLOR_MODE_565;
		break;
	case HAL_PIXEL_FORMAT_RGBA_5551:
		pixelFormat = FGPF_COLOR_MODE_1555;
		break;
	case HAL_PIXEL_FORMAT_RGBA_4444:
		pixelFormat = FGPF_COLOR_MODE_4444;
		break;
	default:
		setError(EGL_BAD_PARAMETER);
		return EGL_NO_IMAGE_KHR;
	}

	FGLImage *image = new FGLAndroidImage(native_buffer, pixelFormat,
							swapNeeded, isARGB);
	if (!image || !image->isValid()) {
		delete image;
		setError(EGL_BAD_ALLOC);
		return EGL_NO_IMAGE_KHR;
	}

	native_buffer->common.incRef(&native_buffer->common);
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

	android_native_buffer_t *native_buffer =
				(android_native_buffer_t *)image->buffer;

	native_buffer->common.decRef(&native_buffer->common);
	image->terminate();

	return EGL_TRUE;
}

#define EGLFunc	__eglMustCastToProperFunctionPointerType

const FGLExtensionMap gPlatformExtensionMap[] = {
	{ "eglCreateImageKHR", (EGLFunc)&eglCreateImageKHR },
	{ "eglDestroyImageKHR", (EGLFunc)&eglDestroyImageKHR },
	{ "eglSetSwapRectangleANDROID", (EGLFunc)&eglSetSwapRectangleANDROID },
	{ "eglGetRenderBufferANDROID", (EGLFunc)&eglGetRenderBufferANDROID },
	{ NULL, NULL },
};
