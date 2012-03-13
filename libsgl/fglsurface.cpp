/*
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include <linux/android_pmem.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "eglCommon.h"
#include "platform.h"
#include "fglsurface.h"
#include "common.h"
#include "types.h"
#include "state.h"
#include "libfimg/fimg.h"

/*
 * Surfaces
 */

FGLLocalSurface::FGLLocalSurface(unsigned long req_size)
	: fd(-1)
{
	pmem_region region;
	unsigned long page_size = getpagesize();

	/* Round up to page size */
	size = (req_size + page_size - 1) & ~(page_size - 1);

	/* Create a buffer file (cached) */
	fd = open("/dev/pmem_gpu1", O_RDWR, 0);
	if(fd < 0) {
		LOGE("EGL: Could not open PMEM device (%s)", strerror(errno));
		return;
	}

	/* Allocate and map the memory */
	vaddr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
	if (vaddr == MAP_FAILED) {
		LOGE("EGL: PMEM buffer allocation failed (%s)", strerror(errno));
		goto err_mmap;
	}

	/* Get physical address */
	if (ioctl(fd, PMEM_GET_PHYS, &region) < 0) {
		LOGE("EGL: PMEM_GET_PHYS failed (%s)", strerror(errno));
		goto err_phys;
	}
	this->paddr = region.offset;

	/* Allocation succeeded */
	return;

err_phys:
	munmap(vaddr, size);
err_mmap:
	close(fd);
	/* Allocation failed */
	fd = -1;
}

FGLLocalSurface::~FGLLocalSurface()
{
	if (!isValid())
		return;

	munmap(vaddr, size);
	close(fd);
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

FGLExternalSurface::FGLExternalSurface(void *v, intptr_t p, size_t s)
{
	vaddr = v;
	paddr = p;
	size = s;
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
	__clear_cache((char *)vaddr, (char *)vaddr + size);
}
