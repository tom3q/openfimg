/**
 * libsgl/eglMem.cpp
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>

#include "eglMem.h"

#include <private/ui/sw_gralloc_handle.h>
#include <linux/android_pmem.h>
#include <linux/fb.h>

#include "fglsurface.h"
#include "common.h"
#include "types.h"
#include "state.h"
#include "libfimg/fimg.h"

/* HACK ALERT */
#include "../../../../hardware/libhardware/modules/gralloc/gralloc_priv.h"

using namespace android;

/**
	Surfaces
*/

#define FB_DEVICE_NAME "/dev/graphics/fb0"
static inline unsigned long getFramebufferAddress(void)
{
	static unsigned long address = 0;

//	LOGD("getFramebufferAddress");

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

unsigned long fglGetBufferPhysicalAddress(android_native_buffer_t *buffer)
{
	const private_handle_t* hnd = static_cast<const private_handle_t*>(buffer->handle);

//	LOGD("fglGetBufferPhysicalAddress");

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

/*
 * NEW SURFACE SUBSYSTEM
 */

FGLLocalSurface::FGLLocalSurface(unsigned long size)
	: fd(-1)
{
	int fd;
	void *vaddr;
	pmem_region region;

	// create a buffer file (cached)
	fd = open("/dev/pmem_gpu1", O_RDWR, 0);
	if(fd < 0) {
		LOGE("EGL: Could not open PMEM device (%s)", strerror(errno));
		return;
	}

	// allocate and map the memory
	if ((vaddr = mmap(NULL, size, PROT_WRITE | PROT_READ,
				MAP_SHARED, fd, NULL)) == MAP_FAILED) {
		LOGE("EGL: PMEM buffer allocation failed (%s)", strerror(errno));
		goto err_mmap;
	}

	if (ioctl(fd, PMEM_GET_PHYS, &region) < 0) {
		LOGE("EGL: PMEM_GET_PHYS failed (%s)", strerror(errno));
		goto err_phys;
	}

	/* Clear the buffer (NOTE: Is it needed?) */
	memset((char*)vaddr, 0, size);

	/* Setup surface struct */
	this->size	= size;
	this->fd	= fd;
	this->vaddr	= vaddr;
	this->paddr	= region.offset;

	LOGD("Created PMEM surface. fd = %d, vaddr = %p, paddr = %08x",
				fd, vaddr, (unsigned int)region.offset);

	flush();

	return;

err_phys:
	munmap(vaddr, size);
err_mmap:
	close(fd);
}

FGLLocalSurface::~FGLLocalSurface()
{
	if (!isValid())
		return;

	munmap(vaddr, size);
	close(fd);

	LOGD("Destroyed PMEM surface. fd = %d, vaddr = %p, paddr = %08x",
				fd, vaddr, (unsigned int)paddr);
}

int FGLLocalSurface::lock(int usage)
{
	return 0;
}

int FGLLocalSurface::unlock(void)
{
	return 0;
}

void FGLLocalSurface::flush(void)
{
	struct pmem_region region;

	region.offset = 0;
	region.len = size;

	if (ioctl(fd, PMEM_CACHE_FLUSH, &region) != 0)
		LOGW("Could not flush PMEM surface %d", fd);
}

FGLExternalSurface::FGLExternalSurface(void *v, unsigned long p)
{
	vaddr = v;
	paddr = p;
}

FGLExternalSurface::~FGLExternalSurface()
{

}

int FGLExternalSurface::lock(int usage)
{
	return 0;
}

int FGLExternalSurface::unlock(void)
{
	return 0;
}

void FGLExternalSurface::flush(void)
{

}

FGLImageSurface::FGLImageSurface(EGLImageKHR img)
	: image(0)
{
	android_native_buffer_t *buffer = (android_native_buffer_t *)img;

	if (buffer->common.magic != ANDROID_NATIVE_BUFFER_MAGIC)
		return;

	if (buffer->common.version != sizeof(android_native_buffer_t))
		return;

	const private_handle_t* hnd =
			static_cast<const private_handle_t*>(buffer->handle);

	hw_module_t const* pModule;
	if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule))
		return;

	module = reinterpret_cast<gralloc_module_t const*>(pModule);

	vaddr = 0; /* Needs locking */
	paddr = fglGetBufferPhysicalAddress(buffer);
	size = hnd->size;;
	image = img;
}

FGLImageSurface::~FGLImageSurface()
{

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

}
