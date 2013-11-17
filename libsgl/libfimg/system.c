/*
 * fimg/system.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE SYSTEM-DEVICE INTERFACE
 *
 * Copyrights:	2010 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <xf86drm.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fimg_private.h"

/**
 * Opens G3D device and maps GPU registers into application address space.
 * @param ctx Hardware context.
 * @return 0 on success, negative on error.
 */
int fimgDeviceOpen(fimgContext *ctx)
{
	ctx->fd = drmOpen("exynos", NULL);
	if(ctx->fd < 0) {
		LOGE("Couldn't open /dev/dri/card0 (%s).", strerror(errno));
		return -errno;
	}

	LOGD("Opened /dev/dri/card0 (%d).", ctx->fd);

	return 0;
}

/**
 * Unmaps GPU registers and closes G3D device.
 * @param ctx Hardware context.
 */
void fimgDeviceClose(fimgContext *ctx)
{
	close(ctx->fd);

	LOGD("fimg3D: Closed /dev/dri/card0 (%d).", ctx->fd);
}

/**
	Context management
*/

/**
 * Creates a hardware context.
 * @return A pointer to hardware context struct or NULL on error.
 */
fimgContext *fimgCreateContext(void)
{
	fimgContext *ctx;
	struct g3d_state_entry *queue;
	int ret;

	if ((ctx = calloc(1, sizeof(*ctx))) == NULL) {
		LOGE("failed to allocate context data");
		return NULL;
	}

	queue = calloc(G3D_NUM_REGISTERS + 1, sizeof(*queue));
	if (!queue) {
		LOGE("failed to allocate register queue");
		goto err_free_ctx;
	}
	ctx->queueLen = G3D_NUM_REGISTERS;
	ctx->queueStart = queue;
	ctx->queue = queue;

	if(fimgDeviceOpen(ctx)) {
		LOGE("failed to open device");
		goto err_free_queue;
	}

	ret = fimgCreateGEM(ctx, FIMG_RING_BUFFER_SIZE,
			FIMG_GEM_RING_BUFFER, &ctx->ringBufferHandle);
	if (ret < 0) {
		LOGE("failed to allocate ring buffer");
		goto err_close;
	}

	ctx->ringBufferStart = fimgMapGEM(ctx, ctx->ringBufferHandle,
						FIMG_RING_BUFFER_SIZE);
	if (!ctx->ringBufferStart) {
		LOGE("failed to map ring buffer");
		goto err_free_ring;
	}

	ctx->ringBufferPtr = ctx->ringBufferStart;
	ctx->ringBufferFree = FIMG_RING_BUFFER_SIZE;

	fimgCreateHostContext(ctx);
	fimgCreatePrimitiveContext(ctx);
	fimgCreateRasterizerContext(ctx);
	fimgCreateFragmentContext(ctx);
#ifdef FIMG_FIXED_PIPELINE
	fimgCreateCompatContext(ctx);
#endif

	return ctx;

err_free_ring:
	fimgDestroyGEMHandle(ctx, ctx->ringBufferHandle);
err_close:
	fimgDeviceClose(ctx);
err_free_queue:
	free(queue);
err_free_ctx:
	free(ctx);

	return NULL;
}

/**
 * Destroys a hardware context.
 * @param ctx Hardware context.
 */
void fimgDestroyContext(fimgContext *ctx)
{
	munmap(ctx->ringBufferStart, FIMG_RING_BUFFER_SIZE);
	fimgDestroyGEMHandle(ctx, ctx->ringBufferHandle);
	fimgDestroyHostContext(ctx);
#ifdef FIMG_FIXED_PIPELINE
	fimgDestroyCompatContext(ctx);
#endif
	fimgDeviceClose(ctx);
	free(ctx->queueStart);
	free(ctx);
}

/**
	Power management
*/
static inline void get_abs_timeout(struct drm_exynos_timespec *tv, uint32_t ms)
{
	struct timespec t;
	uint32_t s = ms / 1000;
	clock_gettime(CLOCK_MONOTONIC, &t);
	tv->tv_sec = t.tv_sec + s;
	tv->tv_nsec = t.tv_nsec + ((ms - (s * 1000)) * 1000000);
}

int fimgWaitForFence(fimgContext *ctx, uint32_t cookie)
{
	struct drm_exynos_g3d_wait wait;
	int ret;

	wait.fence = cookie;
	get_abs_timeout(&wait.timeout, 5000);

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_WAIT, &wait);
	if (ret < 0) {
		LOGE("DRM_IOCTL_EXYNOS_G3D_WAIT failed (%d)", errno);
		return ret;
	}

	return 0;
}

/**
 * Waits for hardware to flush graphics pipeline.
 * @param ctx Hardware context.
 * @param target Bit mask of pipeline parts to be flushed.
 * @return 0 on success, negative on error.
 */
int fimgWaitForFlush(fimgContext *ctx)
{
	int ret;

	ret = fimgWaitForFence(ctx, ctx->lastFence);
	if (ret < 0) {
		LOGE("Fence wait failed");
		return ret;
	}

	return 0;
}

/**
 * Makes sure that any rendering that is in progress finishes and any data
 * in caches is saved to buffers in memory.
 * @param ctx Hardware context.
 */
void fimgFinish(fimgContext *ctx)
{
	fimgFlush(ctx);
	fimgWaitForFlush(ctx);
}

/* Register queue */
void fimgQueueFlush(fimgContext *ctx)
{
	uint32_t *req;

	if (!ctx->queueLen)
		return;

	req = fimgGetRequest(ctx, G3D_REQUEST_REGISTER_WRITE,
				ctx->queueLen * sizeof(*ctx->queueStart));

	/*
	 * The queue does not accept new elements if it already has
	 * the same size as the total number of G3D registers.
	 * In this case, the whole context needs to be restored, what
	 * is more efficient anyway.
	 */
	if (ctx->queueLen == G3D_NUM_REGISTERS) {
		const uint32_t *regs = (const uint32_t *)&ctx->hw;
		unsigned int i;

		for (i = 0; i < G3D_NUM_REGISTERS; ++i) {
			*(req++) = i;
			*(req++) = *(regs++);
		}
	} else {
		memcpy(req, &ctx->queueStart[1],
			ctx->queueLen * sizeof(*ctx->queueStart));
	}

	ctx->queueLen = 0;
	ctx->queue = ctx->queueStart;
	ctx->queue->reg = G3D_NUM_REGISTERS;
}

void fimgQueue(fimgContext *ctx, unsigned int data, enum g3d_register addr)
{
	if (ctx->queue->reg == addr) {
		ctx->queue->val = data;
		return;
	}

	/* Above the maximum length it's more effective to restore the whole
	 * context than just the changed registers */
	if (ctx->queueLen == G3D_NUM_REGISTERS)
		return;

	++ctx->queue;
	++ctx->queueLen;
	ctx->queue->reg = addr;
	ctx->queue->val = data;
}

/*
 * Request ring
 */

void fimgFlush(fimgContext *ctx)
{
	struct drm_exynos_g3d_submit submit;
	int ret;

	if (ctx->ringBufferPtr == ctx->ringBufferStart)
		return;

	submit.handle = ctx->ringBufferHandle;
	submit.offset = 0;
	submit.length = ctx->ringBufferPtr - ctx->ringBufferStart;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("DRM_IOCTL_EXYNOS_G3D_SUBMIT failed (%d)", ret);

	ctx->ringBufferPtr = ctx->ringBufferStart;
	ctx->ringBufferFree = FIMG_RING_BUFFER_SIZE;
	ctx->lastFence = submit.fence;
}

void *fimgGetRequest(fimgContext *ctx, uint8_t type, uint32_t length)
{
	uint32_t *ptr;
	uint32_t pkt_length;

	length = (length + 3) & ~3;
	pkt_length = length + sizeof(*ptr);

	if (ctx->ringBufferFree < pkt_length)
		fimgFlush(ctx);

	ptr = ctx->ringBufferPtr;
	ptr[0] = (type << 24) | (length & 0xffffff);
	ctx->ringBufferPtr += pkt_length;
	ctx->ringBufferFree -= pkt_length;

	return &ptr[1];
}

/*
 * Memory management (GEM)
 */

static const unsigned int fimgGemFlags[] = {
	[FIMG_GEM_SURFACE] = EXYNOS_BO_CACHABLE | EXYNOS_BO_UNMAPPED,
	[FIMG_GEM_RING_BUFFER] = EXYNOS_BO_CACHABLE | EXYNOS_BO_NONCONTIG,
	[FIMG_GEM_DRAW_BUFFER] = EXYNOS_BO_CACHABLE | EXYNOS_BO_NONCONTIG,
	[FIMG_GEM_SHADER_BUFFER] = EXYNOS_BO_CACHABLE | EXYNOS_BO_NONCONTIG,
};

int fimgCreateGEM(fimgContext *ctx, unsigned long size, unsigned int usage,
		  unsigned int *handle)
{
	struct drm_exynos_gem_create req;
	int ret;

	if (usage >= NELEM(fimgGemFlags)) {
		LOGE("invalid GEM usage (%d)", usage);
		return -EINVAL;
	}

	req.size = size;
	req.flags = fimgGemFlags[usage];
	req.handle = 0;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_GEM_CREATE, &req);
	if (ret < 0) {
		LOGE("failed to create GEM (%d)", ret);
		return ret;
	}

	*handle = req.handle;
	return 0;
}

void fimgDestroyGEMHandle(fimgContext *ctx, unsigned int handle)
{
	struct drm_gem_close req;
	int ret;

	req.handle = handle;

	ret = ioctl(ctx->fd, DRM_IOCTL_GEM_CLOSE, &req);
	if (ret < 0)
		LOGE("failed to destroy GEM (%d)", ret);
}

void *fimgMapGEM(fimgContext *ctx, unsigned int handle, unsigned long size)
{
	struct drm_exynos_gem_mmap req;
	int ret;

	req.handle = handle;
	req.size = size;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_GEM_MMAP, &req);
	if (ret < 0) {
		LOGE("failed to map GEM (%d)", ret);
		return NULL;
	}

	return (void *)(unsigned long)req.mapped;
}

int fimgExportGEM(fimgContext *ctx, unsigned int handle)
{
	int ret;
	int fd;

	ret = drmPrimeHandleToFD(ctx->fd, handle, 0, &fd);
	if (ret < 0) {
		LOGE("failed to export GEM (%d)", ret);
		return ret;
	}

	return fd;
}

int fimgImportGEM(fimgContext *ctx, int fd, unsigned int *handle)
{
	int ret;

	ret = drmPrimeFDToHandle(ctx->fd, fd, handle);
	if (ret < 0) {
		LOGE("failed to import GEM (%d)", ret);
		return ret;
	}

	return 0;
}

void *fimgCreateAndMapGEM(fimgContext *ctx, unsigned long size,
			  unsigned int usage, unsigned int *handle)
{
	void *addr;
	int ret;

	ret = fimgCreateGEM(ctx, size, usage, handle);
	if (ret < 0)
		return NULL;

	addr = fimgMapGEM(ctx, *handle, size);
	if (!addr)
		fimgDestroyGEMHandle(ctx, *handle);

	return addr;
}

void fimgUnmapAndDestroyGEMHandle(fimgContext *ctx, unsigned int handle,
				  void *addr, unsigned long size)
{
	if (addr)
		munmap(addr, size);
	fimgDestroyGEMHandle(ctx, handle);
}
