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

	if ((ctx = calloc(1, sizeof(*ctx))) == NULL)
		return NULL;

	queue = calloc(FIMG_MAX_QUEUE_LEN + 1, sizeof(*queue));
	if (!queue) {
		free(ctx);
		return NULL;
	}
	ctx->queueLen = FIMG_MAX_QUEUE_LEN;
	ctx->queueStart = queue;
	ctx->queue = queue;

	ctx->requestDataBuffer = malloc(FIMG_REQUEST_DATA_BUF_SIZE);
	if (!ctx->requestDataBuffer) {
		free(queue);
		free(ctx);
		return NULL;
	}
	ctx->requestData = ctx->requestDataBuffer;
	ctx->freeRequestData = FIMG_REQUEST_DATA_BUF_SIZE;

	if(fimgDeviceOpen(ctx)) {
		free(queue);
		free(ctx);
		return NULL;
	}

	fimgCreateHostContext(ctx);
	fimgCreatePrimitiveContext(ctx);
	fimgCreateRasterizerContext(ctx);
	fimgCreateFragmentContext(ctx);
#ifdef FIMG_FIXED_PIPELINE
	fimgCreateCompatContext(ctx);
#endif

	return ctx;
}

/**
 * Destroys a hardware context.
 * @param ctx Hardware context.
 */
void fimgDestroyContext(fimgContext *ctx)
{
	fimgDeviceClose(ctx);
	free(ctx->queueStart);
	free(ctx->vertexData);
	free(ctx->requestDataBuffer);
#ifdef FIMG_FIXED_PIPELINE
	free(ctx->compat.vshaderBuf);
	free(ctx->compat.pshaderBuf);
#endif
	free(ctx);
}

/**
	Power management
*/

/**
 * Waits for hardware to flush graphics pipeline.
 * @param ctx Hardware context.
 * @param target Bit mask of pipeline parts to be flushed.
 * @return 0 on success, negative on error.
 */
int fimgWaitForFlush(fimgContext *ctx, uint32_t target)
{
	struct drm_exynos_g3d_request *req;
	int ret;

	req = fimgGetRequest(ctx, 0);

	req->type = G3D_REQUEST_FENCE;
	req->fence.flags = G3D_FENCE_FLUSH;

	fimgFlush(ctx);

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_WAIT);
	if (ret < 0) {
		LOGE("DRM_IOCTL_EXYNOS_G3D_WAIT failed (%d)", ret);
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
	fimgWaitForFlush(ctx, FGHI_PIPELINE_ALL);
}

/* Register queue */
void fimgQueueFlush(fimgContext *ctx)
{
	struct drm_exynos_g3d_request *req;

	if (!ctx->queueLen)
		return;

	/*
	 * Above the maximum length it's more efficient to restore the whole
	 * context than just the changed registers
	 */
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN) {
		req = fimgGetRequest(ctx, sizeof(ctx->hw));

		memcpy(req->data, &ctx->hw, sizeof(ctx->hw));

		req->type = G3D_REQUEST_STATE_INIT;
		req->length = sizeof(ctx->hw);
	} else {
		req = fimgGetRequest(ctx,
				ctx->queueLen * sizeof(*ctx->queueStart));

		memcpy(req->data, &ctx->queueStart[1],
			ctx->queueLen * sizeof(*ctx->queueStart));

		req->type = G3D_REQUEST_STATE_BUFFER;
		req->length = ctx->queueLen * sizeof(*ctx->queueStart);
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
	if (ctx->queueLen == FIMG_MAX_QUEUE_LEN)
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

	if (!ctx->numRequests)
		return;

	submit.requests = ctx->requests;
	submit.nr_requests = ctx->numRequests;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("DRM_IOCTL_EXYNOS_G3D_SUBMIT failed (%d)", ret);

	ctx->numRequests = 0;
	ctx->requestData = ctx->requestDataBuffer;
	ctx->freeRequestData = FIMG_REQUEST_DATA_BUF_SIZE;
}

struct drm_exynos_g3d_request* fimgGetRequest(fimgContext *ctx,
							uint32_t dataSize)
{
	struct drm_exynos_g3d_request *req;

	if (ctx->numRequests == FIMG_MAX_REQUESTS
	    || ctx->freeRequestData < dataSize)
		fimgFlush(ctx);

	req = &ctx->requests[ctx->numRequests++];

	req->data = ctx->requestData;
	dataSize = (dataSize + 7) & ~7;
	ctx->requestData += dataSize;
	ctx->freeRequestData -= dataSize;

	return req;
}

/*
 * Memory management (GEM)
 */

int fimgCreateGEM(fimgContext *ctx, unsigned long size, unsigned int *handle)
{
	struct drm_exynos_gem_create req;
	int ret;

	req.size = size;
	req.flags = EXYNOS_BO_UNMAPPED | EXYNOS_BO_CACHABLE;
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
