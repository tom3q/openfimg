/*
* Copyright (C) 2008 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#define LOG_TAG "copybit"

#include <cutils/log.h>

#include <linux/msm_mdp.h>
#include <linux/fb.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <hardware/copybit.h>

#include "../libgralloc/gralloc_priv.h"
#include "../modules/g2d/s3c_g2d.h"

//#define COPYBIT_DEBUG

#ifdef COPYBIT_DEBUG
#define DEBUG_ENTER()	LOGD("Entering %s", __func__); sleep(5)
#define DEBUG_LEAVE()	LOGD("Leaving %s", __func__); sleep(5)
#define DEBUG(args...)	LOGD(args)
#else
#define DEBUG_ENTER()
#define DEBUG_LEAVE()
#define DEBUG(args...)
#endif

/******************************************************************************/

#define MAX_SCALE_FACTOR    (256)
#define MAX_DIMENSION       (2040)

/******************************************************************************/

/** State information for each device instance */
struct copybit_context_t {
	struct copybit_device_t device;
	int s3c_g2d_fd;
	uint32_t transform;
};

/**
* Common hardware methods
*/

static int open_copybit(const struct hw_module_t* module, const char* name,
						struct hw_device_t** device);

static struct hw_module_methods_t copybit_module_methods = {
	open:	open_copybit
};

/*
* The COPYBIT Module
*/
struct copybit_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag:		HARDWARE_MODULE_TAG,
		version_major:	1,
		version_minor:	0,
		id:		COPYBIT_HARDWARE_MODULE_ID,
		name:		"S3C6410 COPYBIT Module",
		author:		"Tomasz Figa <tomasz.figa@gmail.com>",
		methods:	&copybit_module_methods
	}
};

/******************************************************************************/

/** min of int a, b */
static inline int min(int a, int b)
{
	return (a<b) ? a : b;
}

/** max of int a, b */
static inline int max(int a, int b)
{
	return (a>b) ? a : b;
}

/** scale each parameter by mul/div. Assume div isn't 0 */
static inline void MULDIV(uint32_t *a, uint32_t *b, int mul, int div)
{
	if (mul != div) {
		*a = (mul * *a) / div;
		*b = (mul * *b) / div;
	}
}

/** Determine the intersection of lhs & rhs store in out */
static void intersect(struct copybit_rect_t *out,
		const struct copybit_rect_t *lhs,
		const struct copybit_rect_t *rhs)
{
	out->l = max(lhs->l, rhs->l);
	out->t = max(lhs->t, rhs->t);
	out->r = min(lhs->r, rhs->r);
	out->b = min(lhs->b, rhs->b);
}

/** convert COPYBIT_FORMAT to G2D format */
static int get_format(int format)
{
	switch (format) {
	case COPYBIT_FORMAT_RGBA_5551:
		return G2D_RGBA16;
	case COPYBIT_FORMAT_RGB_565:
		return G2D_RGB16;
	case COPYBIT_FORMAT_RGBX_8888:
		return G2D_RGBX32;
	case COPYBIT_FORMAT_RGBA_8888:
		return G2D_RGBA32;
	case COPYBIT_FORMAT_BGRA_8888:
		return G2D_XRGB32;
	default:
		return -1;
	}
}

/** convert from copybit image to g2d image structure */
static int set_image(s3c_g2d_image *img, const struct copybit_image_t *rhs)
{
	const private_handle_t* hnd =
		static_cast<const private_handle_t*>(rhs->handle);
	int format;

	format = get_format(rhs->format);
	if(format < 0)
		return -1;

	img->base = 0;
	if(hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
		img->base = hnd->smem_start;

	img->w		= (rhs->w + 1) & ~1;
	img->h		= rhs->h;
	img->offs	= hnd->offset;
	img->fd		= hnd->fd;
	img->fmt	= format;

	return 0;
}

/*****************************************************************************/

/** Set a parameter to value */
static int set_parameter_copybit(
	struct copybit_device_t *dev,
	int name,
	int value)
{
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;

	if (!ctx)
		return -EINVAL;

	switch (name) {
	case COPYBIT_ROTATION_DEG:
		switch (value) {
		case 0:
			ctx->transform = 0;
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_0);
			break;
		case 90:
			ctx->transform = COPYBIT_TRANSFORM_ROT_90;
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_90);
			break;
		case 180:
			ctx->transform = COPYBIT_TRANSFORM_ROT_180;
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_180);
			break;
		case 270:
			ctx->transform = COPYBIT_TRANSFORM_ROT_270;
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_270);
			break;
		default:
			LOGE("Invalid value for COPYBIT_ROTATION_DEG");
			return -EINVAL;
		}
		break;
	case COPYBIT_PLANE_ALPHA:
		if (value < 0)
			value = 0;
		if (value > ALPHA_VALUE_MAX)
			value = ALPHA_VALUE_MAX;
		ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_ALPHA_VAL, value);
		break;
	case COPYBIT_TRANSFORM:
		switch (value) {
		case 0:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_0);
			break;
		case COPYBIT_TRANSFORM_ROT_90:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_90);
			break;
		case COPYBIT_TRANSFORM_ROT_180:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_180);
			break;
		case COPYBIT_TRANSFORM_ROT_270:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_270);
			break;
		case COPYBIT_TRANSFORM_FLIP_H:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_FLIP_X);
			break;
		case COPYBIT_TRANSFORM_FLIP_V:
			ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_TRANSFORM,
								G2D_ROT_FLIP_Y);
			break;
		default:
			LOGE("Invalid value for COPYBIT_TRANSFORM");
			return -EINVAL;
		}
		ctx->transform = value;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/** Get a static info value */
static int get(struct copybit_device_t *dev, int name)
{
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;

	if (!ctx)
		return -EINVAL;

	switch (name) {
	case COPYBIT_MINIFICATION_LIMIT:
		return MAX_SCALE_FACTOR;
	case COPYBIT_MAGNIFICATION_LIMIT:
		return MAX_SCALE_FACTOR;
	case COPYBIT_SCALING_FRAC_BITS:
		return 11;
	case COPYBIT_ROTATION_STEP_DEG:
		return 90;
	}

	return -EINVAL;
}

/*****************************************************************************/

/**
 * Stretched BitBlit operation
 */

static inline void set_rects(struct copybit_context_t *dev,
		struct s3c_g2d_req *e,
		const struct copybit_rect_t *dst,
		const struct copybit_rect_t *src,
		const struct copybit_rect_t *scissor)
{
	struct copybit_rect_t clip;
	uint32_t H, W, h, w;

	intersect(&clip, scissor, dst);

	DEBUG("SRC (%d,%d) (%d,%d), DST (%d,%d), (%d,%d), "
		"CLIP (%d,%d) (%d,%d)", src->l, src->t, src->r,
		src->b, dst->l, dst->t, dst->r, dst->b,
		scissor->l, scissor->t, scissor->r, scissor->b);

	e->dst.l = clip.l;
	e->dst.t = clip.t;
	e->dst.r = clip.r - 1;
	e->dst.b = clip.b - 1;

	switch (dev->transform) {
	case COPYBIT_TRANSFORM_ROT_90:
		e->src.l = clip.t - dst->t + src->t;
		e->src.t = dst->r - clip.r + src->l;
		w = clip.b - clip.t;
		h = clip.r - clip.l;
		W = dst->b - dst->t;
		H = dst->r - dst->l;
		break;
	case COPYBIT_TRANSFORM_ROT_180:
		e->src.l = dst->r - clip.r + src->l;
		e->src.t = dst->b - clip.b + src->t;
		w = clip.r - clip.l;
		h = clip.b - clip.t;
		W = dst->r - dst->l;
		H = dst->b - dst->t;
		break;
	case COPYBIT_TRANSFORM_ROT_270:
		e->src.l = dst->b - clip.b + src->t;
		e->src.t = clip.l - dst->l + src->l;
		w = clip.b - clip.t;
		h = clip.r - clip.l;
		W = dst->b - dst->t;
		H = dst->r - dst->l;
		break;
	default:
		e->src.l = clip.l - dst->l + src->l;
		e->src.t = clip.t - dst->t + src->t;
		w = clip.r - clip.l;
		h = clip.b - clip.t;
		W = dst->r - dst->l;
		H = dst->b - dst->t;
	}

	MULDIV(&e->src.l, &w, src->r - src->l, W);
	MULDIV(&e->src.t, &h, src->b - src->t, H);
	e->src.r = e->src.l + w - 1;
	e->src.b = e->src.t + h - 1;

//	if (dev->transform & COPYBIT_TRANSFORM_FLIP_V)
//		e->src.t = e->src.h - (e->src.t + h);

//	if (dev->transform & COPYBIT_TRANSFORM_FLIP_H)
//		e->src.l = e->src.w  - (e->src.l + w);

	DEBUG("BLIT (%d,%d) (%d,%d) => (%d,%d) (%d,%d)",
		e->src.l, e->src.t, e->src.r, e->src.b,
		e->dst.l, e->dst.t, e->dst.r, e->dst.b);
}

static int stretch_copybit(
	struct copybit_device_t *dev,
	struct copybit_image_t const *dst,
	struct copybit_image_t const *src,
	struct copybit_rect_t const *dst_rect,
	struct copybit_rect_t const *src_rect,
	struct copybit_region_t const *region)
{
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
	int status = 0;

	if (!ctx)
		return -EINVAL;

	struct s3c_g2d_req req;
	memset(&req, 0, sizeof(req));

	DEBUG("SRC %dx%d fmt %d, DST %dx%d fmt %d",
		src->w, src->h, src->format, dst->w, dst->h, dst->format);

	if (src_rect->l < 0 || src_rect->r > src->w ||
			src_rect->t < 0 || src_rect->b > src->h) {
		// this is always invalid
		return -EINVAL;
	}

	if (src->w > MAX_DIMENSION || src->h > MAX_DIMENSION)
		return -EINVAL;

	if (dst->w > MAX_DIMENSION || dst->h > MAX_DIMENSION)
		return -EINVAL;

	if(set_image(&req.dst, dst))
		return -EINVAL;

	if(set_image(&req.src, src))
		return -EINVAL;

	struct copybit_rect_t clip;
	while (region->next(region, &clip)) {
		set_rects(ctx, &req, dst_rect, src_rect, &clip);

		status = ioctl(ctx->s3c_g2d_fd, S3C_G2D_BITBLT, &req);
		if (status < 0) {
			LOGE("copyBits failed (%s)", strerror(errno));
			return -errno;
		}
	}

	return 0;
}

/**
 * 1:1 BitBlit operation
 */

static inline void set_rects_noscale(struct copybit_context_t *dev,
		struct s3c_g2d_req *e,
		const struct copybit_rect_t *dst,
		const struct copybit_rect_t *src,
		const struct copybit_rect_t *scissor)
{
	struct copybit_rect_t clip;
	uint32_t h, w;

	intersect(&clip, scissor, dst);

	DEBUG("SRC (%d,%d) (%d,%d), DST (%d,%d), (%d,%d), "
		"CLIP (%d,%d) (%d,%d)", src->l, src->t, src->r,
		src->b, dst->l, dst->t, dst->r, dst->b,
		scissor->l, scissor->t, scissor->r, scissor->b);

	e->dst.l = clip.l;
	e->dst.t = clip.t;
	e->dst.r = clip.r - 1;
	e->dst.b = clip.b - 1;

	switch (dev->transform) {
	case COPYBIT_TRANSFORM_ROT_90:
		e->src.l = clip.t - dst->t + src->t;
		e->src.t = dst->r - clip.r + src->l;
		w = clip.b - clip.t;
		h = clip.r - clip.l;
		break;
	case COPYBIT_TRANSFORM_ROT_180:
		e->src.l = dst->r - clip.r + src->l;
		e->src.t = dst->b - clip.b + src->t;
		w = clip.r - clip.l;
		h = clip.b - clip.t;
		break;
	case COPYBIT_TRANSFORM_ROT_270:
		e->src.l = dst->b - clip.b + src->t;
		e->src.t = clip.l - dst->l + src->l;
		w = clip.b - clip.t;
		h = clip.r - clip.l;
		break;
	default:
		e->src.l = clip.l - dst->l + src->l;
		e->src.t = clip.t - dst->t + src->t;
		w = clip.r - clip.l;
		h = clip.b - clip.t;
	}

	e->src.r = e->src.l + w - 1;
	e->src.b = e->src.t + h - 1;

//	if (dev->transform & COPYBIT_TRANSFORM_FLIP_V)
//		e->src.t = e->src.h - (e->src.t + h);

//	if (dev->transform & COPYBIT_TRANSFORM_FLIP_H)
//		e->src.l = e->src.w  - (e->src.l + w);

	DEBUG("BLIT (%d,%d) (%d,%d) => (%d,%d) (%d,%d)",
		e->src.l, e->src.t, e->src.r, e->src.b,
		e->dst.l, e->dst.t, e->dst.r, e->dst.b);
}

static int blit_copybit(
	struct copybit_device_t *dev,
	struct copybit_image_t const *dst,
	struct copybit_image_t const *src,
	struct copybit_region_t const *region)
{
	struct copybit_rect_t dr = { 0, 0, dst->w, dst->h };
	struct copybit_rect_t sr = { 0, 0, src->w, src->h };
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;
	int status = 0;

	if (!ctx)
		return -EINVAL;

	struct s3c_g2d_req req;
	memset(&req, 0, sizeof(req));

	DEBUG("SRC %dx%d fmt %d, DST %dx%d fmt %d",
		src->w, src->h, src->format, dst->w, dst->h, dst->format);

	if (src->w > MAX_DIMENSION || src->h > MAX_DIMENSION)
		return -EINVAL;

	if (dst->w > MAX_DIMENSION || dst->h > MAX_DIMENSION)
		return -EINVAL;

	if(set_image(&req.dst, dst))
		return -EINVAL;

	if(set_image(&req.src, src))
		return -EINVAL;

	struct copybit_rect_t clip;
	while (region->next(region, &clip)) {
		set_rects_noscale(ctx, &req, &dr, &sr, &clip);

		status = ioctl(ctx->s3c_g2d_fd, S3C_G2D_BITBLT, &req);
		if (status < 0) {
			LOGE("copyBits failed (%s)", strerror(errno));
			return -errno;
		}
	}

	return 0;
}

/*****************************************************************************/

/** Close the copybit device */
static int close_copybit(struct hw_device_t *dev)
{
	struct copybit_context_t* ctx = (struct copybit_context_t*)dev;

	if (ctx) {
		close(ctx->s3c_g2d_fd);
		free(ctx);
	}

	return 0;
}

/** Open a new instance of a copybit device using name */
static int open_copybit(const struct hw_module_t* module, const char* name,
			struct hw_device_t** device)
{
	int status = 0;
	copybit_context_t *ctx;

	ctx = (copybit_context_t *)malloc(sizeof(copybit_context_t));
	if(!ctx) {
		LOGE("Failed to allocate context data");
		return -ENOMEM;
	}

	memset(ctx, 0, sizeof(*ctx));

	ctx->device.common.tag = HARDWARE_DEVICE_TAG;
	ctx->device.common.version = 1;
	ctx->device.common.module = const_cast<hw_module_t*>(module);
	ctx->device.common.close = close_copybit;
	ctx->device.set_parameter = set_parameter_copybit;
	ctx->device.get = get;
	ctx->device.blit = blit_copybit;
	ctx->device.stretch = stretch_copybit;

	ctx->s3c_g2d_fd = open("/dev/s3c-g2d", O_RDWR, 0);
	if (ctx->s3c_g2d_fd < 0) {
		status = errno;
		LOGE("Error opening g2d device, errno=%d (%s)", status,
							strerror(status));
		free(ctx);
		return -status;
	}

	ioctl(ctx->s3c_g2d_fd, S3C_G2D_SET_BLENDING, G2D_PIXEL_ALPHA);

	*device = &ctx->device.common;
	return 0;
}
