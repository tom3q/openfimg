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

bool FGLSurface::bindContext(FGLContext *ctx)
{
	int ret;

	if (this->ctx)
		return false;

	if (fd < 0) {
		if (!allocate(ctx)) {
			LOGE("failed to allocate backing storage");
			return false;
		}
	} else {
		ret = fimgImportGEM(ctx->fimg, fd, &handle);
		if (ret) {
			LOGE("failed to import GEM");
			return false;
		}
	}

	if (!vaddr) {
		vaddr = fimgMapGEM(ctx->fimg, handle, size);
		if (!vaddr) {
			LOGE("failed to map GEM");
			return false;
		}
	}

	this->ctx = ctx;
	return true;
}

void FGLSurface::unbindContext(void)
{
	if (!ctx)
		return;

	fimgDestroyGEMHandle(ctx->fimg, handle);

	ctx = 0;
	handle = 0;
}

FGLSurface::~FGLSurface(void)
{
	unbindContext();
	if (vaddr)
		munmap(vaddr, size);
	if (fd >= 0)
		close(fd);
}

FGLLocalSurface::FGLLocalSurface(unsigned long req_size)
{
	size = req_size;
}

bool FGLLocalSurface::allocate(FGLContext *ctx)
{
	int ret;

	ret = fimgCreateGEM(ctx->fimg, size, &handle);
	if (ret < 0)
		return false;

	ret = fimgExportGEM(ctx->fimg, handle);
	if (ret < 0) {
		fimgDestroyGEMHandle(ctx->fimg, handle);
		return false;
	}

	fd = ret;
	return true;
}
