/* g2d/s3c_g2d_driver.c
 *
 * Copyright (c)	2008 Samsung Electronics
 *			2010 Tomasz Figa
 *
 * Samsung S3C G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/fs.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/power-clock-domain.h>
#include <plat/pm.h>

#include "g2d_regs.h"
#include "s3c_g2d.h"

#ifdef CONFIG_S3C64XX_DOMAIN_GATING
//#define USE_G2D_DOMAIN_GATING
#endif

/* Driver information */
#define DRIVER_DESC		"S3C G2D Device Driver"
#define DRIVER_NAME		"s3c-g2d"

/* Various definitions */
#define G2D_FIFO_SIZE		32
#define G2D_SFR_SIZE		0x1000
#define G2D_TIMEOUT		100
#define G2D_RESET_TIMEOUT	1000
#define G2D_MINOR		220

/* Driver data (shared) */
struct g2d_drvdata {
	void __iomem		*base;	// registers base address
	
	struct mutex		mutex;	// mutex
	struct completion	completion; // completion
	struct work_struct	work;	// work
	wait_queue_head_t	waitq;	// poll waitqueue

	int			irq;	// interrupt number
	struct resource 	*mem;	// memory resource
	struct clk		*clock;	// device clock

#ifdef USE_G2D_DOMAIN_GATING
	struct hrtimer		timer;	// idle timer
#endif
};

static struct g2d_drvdata *drvdata;

/* Context data (unique) */
struct g2d_context
{
	struct g2d_drvdata	*data;	// driver data
	struct s3c_g2d_req	*req;	// blit request
	uint32_t		blend;	// blending mode
	uint32_t		alpha;	// plane alpha value
	uint32_t		rot;	// rotation mode
	uint32_t		rop;	// raster operation
	struct file		*srcf;	// source file (for PMEM)
	struct file		*dstf;	// destination file (for PMEM)
};

/* Logging */
//#define G2D_DEBUG
#ifdef G2D_DEBUG
#define DBG(format, args...) printk(KERN_DEBUG "%s: " format, DRIVER_NAME, ## args)
#else
#define DBG(format, args...)
#endif
#define ERR(format, args...) printk(KERN_ERR "%s: " format, DRIVER_NAME, ## args)
#define WARNING(format, args...) printk(KERN_WARNING "%s: " format, DRIVER_NAME, ## args)
#define INFO(format, args...) printk(KERN_INFO "%s: " format, DRIVER_NAME, ## args)

/**
	Register accessors
*/

static inline void g2d_write(struct g2d_drvdata *d, uint32_t b, uint32_t r)
{
	__raw_writel(b, d->base + r);
}

static inline uint32_t g2d_read(struct g2d_drvdata *d, uint32_t r)
{
	return __raw_readl(d->base + r);
}

/**
	Hardware operations
*/

static uint32_t g2d_check_fifo(struct g2d_drvdata *data, uint32_t needed)
{
	int i;
	u32 used;

	for (i = 0; i < G2D_TIMEOUT; i++) {
		used = (g2d_read(data, G2D_FIFO_STAT_REG) >> 1) & 0x3f;

		if ((G2D_FIFO_SIZE - used) >= needed)
			return 0;
	}

	/* timeout */
	return 1;
}

static void g2d_soft_reset(struct g2d_drvdata *data)
{
	int i;
	u32 reg;

	g2d_write(data, 1, G2D_CONTROL_REG);

	for(i = 0; i < G2D_RESET_TIMEOUT; i++) {
		reg = g2d_read(data, G2D_CONTROL_REG) & 1;

		if(reg == 0)
			break;

		udelay(1);
	}

	if(i == G2D_RESET_TIMEOUT)
		ERR("soft reset timeout.\n");
}

static int g2d_do_blit(struct g2d_context *ctx)
{
	struct g2d_drvdata *data = ctx->data;
	struct s3c_g2d_req *req = ctx->req;
	uint32_t srcw, srch, dstw, dsth;
	uint32_t xincr, yincr;
	uint32_t xref, yref;
	uint32_t stretch;
	uint32_t blend;

	srcw = req->src.r - req->src.l + 1;
	srch = req->src.b - req->src.t + 1;
	dstw = req->dst.r - req->dst.l + 1;
	dsth = req->dst.b - req->dst.t + 1;

	stretch = (srcw != dstw) || (srch != dsth);

	/* NOTE: Theoretically this could by skipped */
	if (g2d_check_fifo(data, 32)) {
		ERR("timeout while waiting for FIFO\n");
		return -EBUSY;
	}

	if(stretch) {
		/* Compute X scaling factor */
		/* (division takes time, so do it now for pipelining */
		xincr = (srcw << 11) / dstw;
	}

	/* Configure source image */
	DBG("SRC %08x + %08x, %dx%d, fmt = %d\n", req->src.base, req->src.offs,
					req->src.w, req->src.h, req->src.fmt);

	g2d_write(data, req->src.base + req->src.offs, G2D_SRC_BASE_ADDR);
	g2d_write(data, req->src.w, G2D_SRC_HORI_REG);
	g2d_write(data, req->src.h, G2D_SRC_VERT_REG);
	/* NOTE: Is this neccessary? */
	//g2d_write(data, (req->src.h << 16) | req->src.w, G2D_SRC_RES_REG);
	g2d_write(data, req->src.fmt, G2D_SRC_FORMAT_REG);
	g2d_write(data, req->src.fmt == G2D_RGBA32, G2D_END_RDSIZE_REG);

	/* Configure destination image */
	DBG("DST %08x + %08x, %dx%d, fmt = %d\n", req->dst.base, req->dst.offs,
					req->dst.w, req->dst.h, req->dst.fmt);

	g2d_write(data, req->dst.base + req->dst.offs, G2D_DST_BASE_ADDR);
	g2d_write(data, req->dst.w, G2D_DST_HORI_REG);
	g2d_write(data, req->dst.h, G2D_DST_VERT_REG);
	/* NOTE: Is this neccessary? */
	//g2d_write(data, (req->dst.h << 16) | req->dst.w, G2D_DST_RES_REG);
	g2d_write(data, req->dst.fmt, G2D_DST_FORMAT_REG);

	/* Configure clipping window to destination size */
	DBG("CLIP (%d,%d) (%d,%d)\n", 0, 0, req->dst.w - 1, req->dst.h - 1);

	g2d_write(data, 0, G2D_CW_LT_X_REG);
	g2d_write(data, 0, G2D_CW_LT_Y_REG);
	g2d_write(data, req->dst.w - 1, G2D_CW_RB_X_REG);
	g2d_write(data, req->dst.h - 1, G2D_CW_RB_Y_REG);

	if(stretch) {
		/* Compute Y scaling factor */
		/* (division takes time, so do it now for pipelining */
		yincr = (srch << 11) / dsth;
	}

	/* Configure ROP and alpha blending */
	if(ctx->blend == G2D_PIXEL_ALPHA) {
		switch(req->src.fmt) {
		case G2D_ARGB16:
		case G2D_ARGB32:
		case G2D_RGBA32:
			blend = G2D_ROP_REG_ABM_SRC_BITMAP;
			break;
		default:
			blend = G2D_ROP_REG_ABM_REGISTER;
		}
	} else {
		blend = ctx->blend << 10;
	}
	blend |= ctx->rop;
	g2d_write(data, blend, G2D_ROP_REG);
	g2d_write(data, ctx->alpha, G2D_ALPHA_REG);

	/* Configure rotation */
	g2d_write(data, ctx->rot, G2D_ROTATE_REG);
	switch (ctx->rot) {
	case G2D_ROT_90:
		xref = req->src.b / 2;
		yref = req->src.b / 2;
		break;
	case G2D_ROT_180:
		xref = req->src.r / 2;
		yref = req->src.b / 2;
		break;
	case G2D_ROT_270:
		xref = -req->src.r / 2;
		yref =  req->src.r / 2;
		break;
	case G2D_ROT_FLIP_X:
		xref = 0;
		yref = (req->src.t + req->src.b) / 2;
		break;
	case G2D_ROT_FLIP_Y:
		xref = (req->src.l + req->src.r) / 2;
		yref = 0;
		break;
	default:
		xref = 0;
		yref = 0;
		break;
	}

	g2d_write(data, xref, G2D_ROT_OC_X_REG);
	g2d_write(data, yref, G2D_ROT_OC_Y_REG);

	DBG("BLEND %08x ROTATE %08x REF=(%d, %d)\n",
			blend, ctx->rot, xref, yref);

	/* Configure coordinates */
	DBG("BLIT (%d,%d) (%d,%d) => (%d,%d) (%d,%d)\n",
		req->src.l, req->src.t, req->src.r, req->src.b,
		req->dst.l, req->dst.t, req->dst.r, req->dst.b);

	g2d_write(data, req->src.l, G2D_COORD0_X_REG);
	g2d_write(data, req->src.t, G2D_COORD0_Y_REG);
	g2d_write(data, req->src.r, G2D_COORD1_X_REG);
	g2d_write(data, req->src.b, G2D_COORD1_Y_REG);

	g2d_write(data, req->dst.l, G2D_COORD2_X_REG);
	g2d_write(data, req->dst.t, G2D_COORD2_Y_REG);
	g2d_write(data, req->dst.r, G2D_COORD3_X_REG);
	g2d_write(data, req->dst.b, G2D_COORD3_Y_REG);

	/* Configure scaling factors */
	DBG("SCALE X_INCR = %08x, Y_INCR = %08x\n", xincr, yincr);
	g2d_write(data, xincr, G2D_X_INCR_REG);
	g2d_write(data, yincr, G2D_Y_INCR_REG);

	g2d_write(data, G2D_INTEN_REG_ACF, G2D_INTEN_REG);

	/* Start the operation */
	if(stretch)
		g2d_write(data, G2D_CMD1_REG_S, G2D_CMD1_REG);
	else
		g2d_write(data, G2D_CMD1_REG_N, G2D_CMD1_REG);

	return 0;
}

/**
	PMEM support
*/

static inline int get_img(struct s3c_g2d_image *img, struct file** filep)
{
	unsigned long vstart, start, len;

#ifdef CONFIG_ANDROID_PMEM
	if(!get_pmem_file(img->fd, &start, &vstart, &len, filep)) {
		img->base = start;
		return 0;
	}
#endif

	return -1;
}

static inline void put_img(struct file *file)
{
#ifdef CONFIG_ANDROID_PMEM
	if (file)
		put_pmem_file(file);
#endif
}

static inline u32 get_pixel_size(uint32_t fmt)
{
	switch(fmt) {
	case G2D_RGBA32:
	case G2D_ARGB32:
	case G2D_XRGB32:
	case G2D_RGBX32:
		return 4;
	default:
		return 2;
	}
}

static inline void flush_img(struct s3c_g2d_image *img, struct file *file)
{
#ifdef CONFIG_ANDROID_PMEM
	u32 len;

	if(file) {
		/* flush image to memory before blit operation */
		len = img->w * img->h * get_pixel_size(img->fmt);
		flush_pmem_file(file, img->offs, len);
	}
#endif
}

/**
	State processing
*/

static irqreturn_t g2d_handle_irq(int irq, void *dev_id)
{
	struct g2d_drvdata *data = (struct g2d_drvdata *)dev_id;
	u32 stat;

	stat = g2d_read(data, G2D_INTC_PEND_REG);
	if (stat & G2D_PEND_REG_INTP_ALL_FIN) {
		g2d_write(data, G2D_PEND_REG_INTP_ALL_FIN, G2D_INTC_PEND_REG);
		complete(&data->completion);
	}

	return IRQ_HANDLED;
}

static void g2d_workfunc(struct work_struct *work)
{
	struct g2d_context *ctx =
		(struct g2d_context *)atomic_long_read(&work->data);
	struct g2d_drvdata *data = ctx->data;
	
	if (wait_for_completion_interruptible_timeout(
			&data->completion, G2D_TIMEOUT) == 0) {
		ERR("timeout while waiting for interrupt, resetting\n");
		g2d_soft_reset(data);
	}

#ifdef USE_G2D_DOMAIN_GATING
	hrtimer_start(&data->timer, ktime_set(G2D_IDLE_TIME_SECS, 0),
							HRTIMER_MODE_REL);
#endif
	put_img(ctx->srcf);
	put_img(ctx->dstf);
	mutex_unlock(&data->mutex);
	wake_up_interruptible(&data->waitq);
}

#ifdef USE_G2D_DOMAIN_GATING
/**
	Power management
*/

static int g2d_clk_enable(struct g2d_drvdata *data)
{
	INFO("Requesting power up.\n");
	
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_ACTIVE_MODE, S3C64XX_2D);

	if (s3c_wait_blk_pwr_ready(S3C64XX_BLK_P)) {
		return -1;
	}

	clk_enable(data->clock);

	return s3c_g2d_init_regs();
}

static int g2d_clk_disable(struct g2d_drvdata *data)
{
	INFO("Requesting power down.\n");

	clk_disable(data->clock);
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_LP_MODE, S3C64XX_2D);

	return 0;
}

static enum hrtimer_restart g2d_idle_func(struct hrtimer *t)
{
	g2d_clk_disable();
	
	return HRTIMER_NORESTART;
}
#endif /* USE_G2D_DOMAIN_GATING */

/**
	File operations
*/

static int s3c_g2d_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct g2d_context *ctx = (struct g2d_context *)file->private_data;
	struct g2d_drvdata *data = ctx->data;
	struct file *srcf = 0, *dstf = 0;
	struct s3c_g2d_req req;
	int ret = 0;

	switch (cmd) {
	/* Proceed to the operation */
	case S3C_G2D_BITBLT:
		break;
	/* Set the parameter and return */
	case S3C_G2D_SET_TRANSFORM:
		ctx->rot = arg;
		return 0;
	case S3C_G2D_SET_ALPHA_VAL:
		ctx->alpha = (arg > ALPHA_VALUE_MAX) ? 255 : arg;
		return 0;
	case S3C_G2D_SET_RASTER_OP:
		ctx->rop = arg & 0xff;
		return 0;
	case S3C_G2D_SET_BLENDING:
		ctx->blend = arg;
		return 0;
	/* Invalid IOCTL call */
	default:
		return -EINVAL;
	}

	if(!mutex_trylock(&data->mutex)) {
		if(file->f_flags & O_NONBLOCK)
			return -EWOULDBLOCK;
		else
			mutex_lock(&data->mutex);
	}

	if (unlikely(copy_from_user(&req, (struct s3c_g2d_req*)arg,
					sizeof(struct s3c_g2d_req)))) {
		ERR("copy_from_user failed\n");
		ret = -EFAULT;
		goto err_noput;
	}

	if (unlikely((req.src.w <= 1) || (req.src.h <= 1))) {
		ERR("invalid source resolution\n");
		ret = -EINVAL;
		goto err_noput;
	}

	if (unlikely((req.dst.w <= 1) || (req.dst.h <= 1))) {
		ERR("invalid destination resolution\n");
		ret = -EINVAL;
		goto err_noput;
	}

	/* do this first so that if this fails, the caller can always
	 * safely call put_img */
	if (likely(req.src.base == 0) && unlikely(get_img(&req.src, &srcf))) {
		ERR("could not retrieve src image from memory\n");
		ret = -EINVAL;
		goto err_noput;
	}

	if (unlikely(req.dst.base == 0) && unlikely(get_img(&req.dst, &dstf))) {
		ERR("could not retrieve dst image from memory\n");
		put_img(srcf);
		ret = -EINVAL;
		goto err_noput;
	}

	ctx->srcf = srcf;
	ctx->dstf = dstf;
	ctx->req = &req; // becomes invalid after leaving this function!

#ifdef USE_G2D_DOMAIN_GATING
	hrtimer_cancel(&data->timer);
	ret = g2d_clk_enable(data);
	if (ret != 0) {
		ERR("Timeout while waiting for g2d domain power on\n");
		ret = -EFAULT;
		goto err_cmd;
	}
#endif /* USE_G2D_DOMAIN_GATING */

	flush_img(&req.src, srcf);
	flush_img(&req.dst, dstf);

	ret = g2d_do_blit(ctx);
	if(ret != 0) {
		ERR("Failed to start G2D operation (%d)\n", ret);
		g2d_soft_reset(data);
		goto err_cmd;
	}

	// block mode
	if (file->f_flags & O_NONBLOCK) {
		atomic_long_set(&data->work.data, (long)ctx);
		schedule_work(&data->work);
		return 0;
	} else if (!wait_for_completion_interruptible_timeout(&data->completion,
								G2D_TIMEOUT)) {
		ERR("Timeout while waiting for interrupt, resetting\n");
		g2d_soft_reset(data);
		ret = -EFAULT;
	}

err_cmd:
#ifdef USE_G2D_DOMAIN_GATING
	hrtimer_start(&data->timer, ktime_set(G2D_IDLE_TIME_SECS, 0),
							HRTIMER_MODE_REL);
#endif /* USE_G2D_DOMAIN_GATING */
	put_img(srcf);
	put_img(dstf);
err_noput:
	mutex_unlock(&data->mutex);

	return ret;
}

static unsigned int s3c_g2d_poll(struct file *file, poll_table *wait)
{
	struct g2d_context *ctx = (struct g2d_context *)file->private_data;
	struct g2d_drvdata *data = ctx->data;
	unsigned int mask = 0;

	poll_wait(file, &data->waitq, wait);

	if(!mutex_is_locked(&data->mutex))
		mask = POLLOUT|POLLWRNORM;

	return mask;
}

static int s3c_g2d_open(struct inode *inode, struct file *file)
{
	struct g2d_context *ctx;
	
	ctx = kmalloc(sizeof(struct g2d_context), GFP_KERNEL);
	if (ctx == NULL) {
		ERR("Context allocation failed\n");
		return -ENOMEM;
	}

	memset(ctx, 0, sizeof(struct g2d_context));
	ctx->data	= drvdata;
	ctx->rot	= G2D_ROT_0;
	ctx->alpha	= ALPHA_VALUE_MAX;
	ctx->rop	= G2D_ROP_SRC_ONLY;

	file->private_data = ctx;

	DBG("device opened\n");

	return 0;
}

static int s3c_g2d_release(struct inode *inode, struct file *file)
{
	struct g2d_context *ctx = (struct g2d_context *)file->private_data;

	kfree(ctx);

	DBG("device released\n");

	return 0;
}

static struct file_operations s3c_g2d_fops = {
	.owner		= THIS_MODULE,
	.open		= s3c_g2d_open,
	.release	= s3c_g2d_release,
	.ioctl		= s3c_g2d_ioctl,
	.poll		= s3c_g2d_poll,
};

static struct miscdevice s3c_g2d_dev = {
	.minor		= G2D_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &s3c_g2d_fops,
};

/**
	Platform device operations
*/

static int s3c_g2d_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct g2d_drvdata *data;
	int ret;

	data = kmalloc(sizeof(struct g2d_drvdata), GFP_KERNEL);
	if(data == NULL) {
		ERR("failed to allocate driver data.\n");
		return -ENOMEM;
	}

	/* find the IRQ */
	data->irq = platform_get_irq(pdev, 0);
	if (data->irq <= 0) {
		ERR("failed to get irq resouce (%d).\n", data->irq);
		return data->irq;
	}

	/* request the IRQ */
	ret = request_irq(data->irq, g2d_handle_irq, IRQF_DISABLED, pdev->name, data);
	if (ret) {
		ERR("request_irq failed (%d).\n", ret);
		return ret;
	}

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		ERR("failed to get memory region resouce.\n");
		return -ENOENT;
	}

	/* reserve the memory */
	data->mem = request_mem_region(res->start, res->end-res->start+1, pdev->name);
	if (data->mem == NULL) {
		ERR("failed to reserve memory region\n");
		return -ENOENT;
	}

	/* map the memory */
	data->base = ioremap(data->mem->start, data->mem->end - res->start + 1);
	if (data->base == NULL) {
		ERR("ioremap failed\n");
		return -ENOENT;
	}

	data->clock = clk_get(&pdev->dev, "hclk_g2d");
	if (IS_ERR(data->clock)) {
		ERR("failed to find g2d clock source\n");
		return -ENOENT;
	}

	mutex_init(&data->mutex);
	init_completion(&data->completion);
	INIT_WORK(&data->work, g2d_workfunc);
	init_waitqueue_head(&data->waitq);

#ifdef USE_G2D_DOMAIN_GATING
	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->timer.function = g2d_idle_func;
#else
#ifdef CONFIG_S3C64XX_DOMAIN_GATING
	s3c_set_normal_cfg(S3C64XX_DOMAIN_P, S3C64XX_ACTIVE_MODE, S3C64XX_2D);
	if (s3c_wait_blk_pwr_ready(S3C64XX_BLK_P)) {
		ERR("timeout waiting for G2D power up\n");
		return -EFAULT;
	}
#endif
	clk_enable(data->clock);
	g2d_soft_reset(data);
#endif

	dev_set_drvdata(&pdev->dev, data);
	drvdata = data;

	ret = misc_register(&s3c_g2d_dev);
	if (ret) {
		printk (KERN_ERR "%s: cannot register miscdev on minor=%d (%d)\n", DRIVER_NAME,
			G2D_MINOR, ret);
		return ret;
	}

	INFO("Driver loaded succesfully\n");

	return 0;
}

static int s3c_g2d_remove(struct platform_device *pdev)
{
	struct g2d_drvdata *data = dev_get_drvdata(&pdev->dev);

#ifdef USE_G2D_DOMAIN_GATING
	if(!hrtimer_cancel(&data->timer))
		g2d_clk_disable(data);
#endif

	free_irq(data->irq, data);
	iounmap(data->base);
	release_resource(data->mem);
	misc_deregister(&s3c_g2d_dev);
	kfree(data);

	INFO("Driver unloaded succesfully.\n");

	return 0;
}

static int s3c_g2d_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef USE_G2D_DOMAIN_GATING
	struct g2d_drvdata *data = dev_get_drvdata(&pdev->dev);

	if(!hrtimer_cancel(&data->timer))
		g2d_clk_disable(data);
#endif

	return 0;
}

static int s3c_g2d_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver s3c_g2d_driver = {
	.probe          = s3c_g2d_probe,
	.remove         = s3c_g2d_remove,
	.suspend        = s3c_g2d_suspend,
	.resume         = s3c_g2d_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DRIVER_NAME,
	},
};

/**
	Module operations
*/

int __init  s3c_g2d_init(void)
{
	int ret;

	if ((ret = platform_driver_register(&s3c_g2d_driver)) != 0) {
		ERR("Platform device register failed (%d).\n", ret);
		return ret;
	}

	INFO("Module initialized.\n");

	return 0;
}

void  s3c_g2d_exit(void)
{
	platform_driver_unregister(&s3c_g2d_driver);

	INFO("Module exited.\n");
}

module_init(s3c_g2d_init);
module_exit(s3c_g2d_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
