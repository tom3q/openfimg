/* g3d/s3c-g3d.c
 *
 * Copyright (c) 2010 Tomasz Figa <tomasz.figa at gmail.com>
 *
 * Samsung S3C G3D driver
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

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/pm.h>
#include <plat/power-clock-domain.h>
#include <plat/reserved_mem.h>

#include "s3c_g3d.h"

#ifdef CONFIG_S3C64XX_DOMAIN_GATING
#define USE_G3D_DOMAIN_GATING
#endif

/* Driver information */
#define DRIVER_DESC		"S3C G3D Device Driver"
#define DRIVER_NAME		"s3c-g3d"

/* Various definitions */
#define G3D_MINOR  		249
#define G3D_SFR_BASE		0x72000000
#define G3D_SFR_SIZE		0x80000
#define G3D_IDLE_TIME_SECS	10
#define G3D_TIMEOUT		1000

/* Registers */
#define G3D_FGGB_PIPESTAT_REG		(0x00)
#define G3D_FGGB_CACHECTL_REG		(0x04)
#define G3D_FGGB_RESET_REG		(0x08)
#define G3D_FGGB_INTPENDING_REG		(0x40)
#define G3D_FGGB_INTMASK_REG		(0x44)
#define G3D_FGGB_PIPEMASK_REG		(0x48)
#define G3D_FGGB_PIPETGTSTATE_REG	(0x4c)

/* Valid bits of FGGB_PIPESTAT */
#define G3D_FGGB_PIPESTAT_MSK	(0x0005171f)
#define G3D_FGGB_FLUSH_MSK	(0x00000033)
#define G3D_FGGB_INVAL_MSK	(0x00001300)

struct g3d_context;

/* Driver data (shared) */
struct g3d_drvdata {
	void __iomem		*base;	// registers base address

	uint32_t		mask;
	struct mutex		mutex;	// mutex
	struct g3d_context	*owner; // current context
	struct completion	completion; // completion

	int			irq;	// interrupt number
	struct resource 	*mem;	// memory resource
	struct clk		*clock;	// device clock

#ifdef USE_G3D_DOMAIN_GATING
	struct hrtimer		timer;	// idle timer
	int			state;	// power state
#endif
};

static struct g3d_drvdata *drvdata;

struct g3d_context {
	struct g3d_drvdata	*data;
	/* More to come */
};

/* Logging */
//#define G3D_DEBUG
#ifdef G3D_DEBUG
#define DBG(format, args...) \
	printk(KERN_DEBUG "%s: " format, DRIVER_NAME, ## args)
#else
#define DBG(format, args...)
#endif
#define ERR(format, args...) \
	printk(KERN_ERR "%s: " format, DRIVER_NAME, ## args)
#define WARNING(format, args...) \
	printk(KERN_WARNING "%s: " format, DRIVER_NAME, ## args)
#define INFO(format, args...) \
	printk(KERN_INFO "%s: " format, DRIVER_NAME, ## args)

/*
	Register accessors
*/

static inline void g3d_write(struct g3d_drvdata *d, uint32_t b, uint32_t r)
{
	__raw_writel(b, d->base + r);
}

static inline uint32_t g3d_read(struct g3d_drvdata *d, uint32_t r)
{
	return __raw_readl(d->base + r);
}

/*
	Hardware operations
*/

static inline void g3d_soft_reset(struct g3d_drvdata *data)
{
	g3d_write(data, 1, G3D_FGGB_RESET_REG);
	udelay(1);
	g3d_write(data, 0, G3D_FGGB_RESET_REG);
}

static inline int g3d_flush(struct g3d_drvdata *data, unsigned int mask)
{
	int ret = 0;

	if((g3d_read(data, G3D_FGGB_PIPESTAT_REG) & mask) == 0)
		return 0;

	/* Setup the interrupt */
	data->mask = mask;
	init_completion(&data->completion);
	g3d_write(data, 0, G3D_FGGB_PIPEMASK_REG);
	g3d_write(data, 0, G3D_FGGB_PIPETGTSTATE_REG);
	g3d_write(data, mask, G3D_FGGB_PIPEMASK_REG);
	g3d_write(data, 1, G3D_FGGB_INTMASK_REG);

	/* Check if the condition isn't already met */
	if((g3d_read(data, G3D_FGGB_PIPESTAT_REG) & mask) == 0) {
		/* Disable the interrupt */
		g3d_write(data, 0, G3D_FGGB_INTMASK_REG);
		return 0;
	}

	if(!wait_for_completion_interruptible_timeout(&data->completion,
							G3D_TIMEOUT)) {
		ERR("Timeout while waiting for interrupt, resetting\n");
		g3d_soft_reset(data);
		ret = -EFAULT;
	}

	/* Disable the interrupt */
	g3d_write(data, 0, G3D_FGGB_INTMASK_REG);

	return ret;
}

static inline void g3d_flush_caches(struct g3d_drvdata *data)
{
	int timeout = 1000000000;
	g3d_write(data, G3D_FGGB_FLUSH_MSK, G3D_FGGB_CACHECTL_REG);

	do {
		if(!g3d_read(data, G3D_FGGB_CACHECTL_REG))
			break;
	} while (--timeout);
}

static inline void g3d_invalidate_caches(struct g3d_drvdata *data)
{
	int timeout = 1000000000;
	g3d_write(data, G3D_FGGB_INVAL_MSK, G3D_FGGB_CACHECTL_REG);

	do {
		if(!g3d_read(data, G3D_FGGB_CACHECTL_REG))
			break;
	} while (--timeout);
}

/*
	State processing
*/

static irqreturn_t g3d_handle_irq(int irq, void *dev_id)
{
	struct g3d_drvdata *data = (struct g3d_drvdata *)dev_id;
	uint32_t stat;

	g3d_write(data, 0, G3D_FGGB_INTPENDING_REG);
	stat = g3d_read(data, G3D_FGGB_PIPESTAT_REG) & data->mask;

	if(!stat)
		complete(&data->completion);

	return IRQ_HANDLED;
}

/*
	Power management
*/

static inline int g3d_do_power_up(struct g3d_drvdata *data)
{
#ifdef CONFIG_S3C64XX_DOMAIN_GATING
	s3c_set_normal_cfg(S3C64XX_DOMAIN_G, S3C64XX_ACTIVE_MODE, S3C64XX_3D);

	if (s3c_wait_blk_pwr_ready(S3C64XX_BLK_G)) {
		return -1;
	}
#endif
	clk_enable(data->clock);
	g3d_soft_reset(data);

	return 1;
}

static inline void g3d_do_power_down(struct g3d_drvdata *data)
{
	clk_disable(data->clock);
#ifdef CONFIG_S3C64XX_DOMAIN_GATING
	s3c_set_normal_cfg(S3C64XX_DOMAIN_G, S3C64XX_LP_MODE, S3C64XX_3D);
#endif
}

#ifdef USE_G3D_DOMAIN_GATING
/* Called with mutex locked */
static inline int g3d_power_up(struct g3d_drvdata *data)
{
	int ret;
	
	if (data->state)
		return 0;

	INFO("Requesting power up.\n");
	if((ret = g3d_do_power_up(data)) > 0)
		data->state = 1;

	return ret;
}

/* Called with mutex locked */
static inline void g3d_power_down(struct g3d_drvdata *data)
{
	if(!data->state)
		return;

	INFO("Requesting power down.\n");

	g3d_flush(data, G3D_FGGB_PIPESTAT_MSK);
	g3d_flush_caches(data);
	g3d_do_power_down(data);
	data->state = 0;
}

/* Called with mutex locked */
static enum hrtimer_restart g3d_idle_func(struct hrtimer *t)
{
	struct g3d_drvdata *data = container_of(t, struct g3d_drvdata, timer);

	g3d_power_down(data);

	return HRTIMER_NORESTART;
}
#endif

/*
	File operations
*/

static int s3c_g3d_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	struct g3d_context *ctx = file->private_data;
	struct g3d_drvdata *data = ctx->data;
	int ret = 0;

	switch(cmd) {
	/* Prepare and lock the hardware */
	case S3C_G3D_LOCK:
		mutex_lock(&data->mutex);
		DBG("Hardware lock acquired by %p\n", ctx);
#ifdef USE_G3D_DOMAIN_GATING
		if(!hrtimer_cancel(&data->timer)) {
			ret = g3d_power_up(data);
			if (ret < 0) {
				ERR("Timeout while waiting for G3D power up\n");
				mutex_unlock(&data->mutex);
				return -EFAULT;
			}
		}
#endif /* USE_G3D_DOMAIN_GATING */
		if (data->owner != ctx) {
			ret |= 1;
			g3d_flush(data, G3D_FGGB_PIPESTAT_MSK);
			g3d_flush_caches(data);
			g3d_invalidate_caches(data);
			data->owner = ctx;
		}
		return ret;
	/* Unlock the hardware and start idle timer */
	case S3C_G3D_UNLOCK:
#ifdef USE_G3D_DOMAIN_GATING
		hrtimer_start(&data->timer, ktime_set(G3D_IDLE_TIME_SECS, 0),
							HRTIMER_MODE_REL);
#endif /* USE_G3D_DOMAIN_GATING */
		mutex_unlock(&data->mutex);
		DBG("Hardware lock released by %p\n", ctx);
		return 0;
	/* Wait for the hardware to finish its work */
	case S3C_G3D_FLUSH:
		if(!mutex_is_locked(&data->mutex) || data->owner != ctx) {
			ERR("Tried to flush the hardware without locking\n");
			return -EINVAL;
		}
		return g3d_flush(data, arg & G3D_FGGB_PIPESTAT_MSK);
	default:
		return -EINVAL;
	}
}

static int s3c_g3d_open(struct inode *inode, struct file *file)
{
	struct g3d_context *ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);

	ctx->data = drvdata;
	file->private_data = ctx;
	DBG("device opened\n");

	return 0;
}

static int s3c_g3d_release(struct inode *inode, struct file *file)
{
	struct g3d_context *ctx = file->private_data;
	struct g3d_drvdata *data = ctx->data;
	unsigned long flags;
	int unlock = 0;

	/* Do this atomically */
	local_irq_save(flags);
	if(mutex_is_locked(&data->mutex) && data->owner == ctx)
		unlock = 1;
	local_irq_restore(flags);

	/* Unlock if we have the lock */
	if(unlock)
		s3c_g3d_ioctl(inode, file, S3C_G3D_UNLOCK, 0);

	kfree(ctx);
	DBG("device released\n");

	return 0;
}

int s3c_g3d_mmap(struct file* file, struct vm_area_struct *vma)
{
	unsigned long pfn;
	size_t size = vma->vm_end - vma->vm_start;

	pfn = __phys_to_pfn(G3D_SFR_BASE);

	if(size > G3D_SFR_SIZE) {
		ERR("mmap size bigger than G3D SFR block\n");
		return -EINVAL;
	}

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		ERR("mmap of G3D SFR block must be shared\n");
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		ERR("remap_pfn range failed\n");
		return -EINVAL;
	}

	DBG("Hardware mapped by %p\n", ctx);

	return 0;
}

static struct file_operations s3c_g3d_fops = {
	.owner 	= THIS_MODULE,
	.ioctl 	= s3c_g3d_ioctl,
	.open 	= s3c_g3d_open,
	.release = s3c_g3d_release,
	.mmap	= s3c_g3d_mmap,
};

static struct miscdevice s3c_g3d_dev = {
	.minor		= G3D_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &s3c_g3d_fops,
};

/*
	Platform device operations
*/

int s3c_g3d_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct g3d_drvdata *data;
	int ret;

	data = kmalloc(sizeof(struct g3d_drvdata), GFP_KERNEL);
	if(data == NULL) {
		ERR("failed to allocate driver data.\n");
		return -ENOMEM;
	}

	data->clock = clk_get(&pdev->dev, "hclk_g3d");
	if (data->clock == NULL) {
		ERR("failed to find g3d clock source\n");
		ret = -ENOENT;
		goto err_clock;
	}

	/* get the memory region for the post processor driver */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) {
		ERR("failed to get memory region resource.\n");
		ret = -ENOENT;
		goto err_mem;
	}

	/* reserve the memory */
	data->mem = request_mem_region(res->start, resource_size(res),
								pdev->name);
	if (data->mem == NULL) {
		ERR("failed to reserve memory region\n");
		ret = -ENOENT;
		goto err_mem;
	}

	/* map the memory */
	data->base = ioremap(data->mem->start, resource_size(data->mem));
	if (data->base == NULL) {
		ERR("ioremap failed\n");
		ret = -ENOENT;
		goto err_ioremap;
	}

	/* get the IRQ */
	data->irq = platform_get_irq(pdev, 0);
	if (data->irq <= 0) {
		ERR("failed to get irq resource (%d).\n", data->irq);
		ret = data->irq;
		goto err_irq;
	}

	/* request the IRQ */
	ret = request_irq(data->irq, g3d_handle_irq, 0, pdev->name, data);
	if (ret) {
		ERR("request_irq failed (%d).\n", ret);
		goto err_irq;
	}

	data->owner = NULL;
	mutex_init(&data->mutex);
	init_completion(&data->completion);

#ifdef USE_G3D_DOMAIN_GATING
	hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	data->timer.function = g3d_idle_func;
	data->state = 1;
#endif
	if(g3d_do_power_up(data) < 0) {
		ERR("G3D power up failed\n");
		ret = -EFAULT;
		goto err_pm;
	}
#ifdef USE_G3D_DOMAIN_GATING
	hrtimer_start(&data->timer, ktime_set(G3D_IDLE_TIME_SECS, 0),
							HRTIMER_MODE_REL);
#endif

	platform_set_drvdata(pdev, data);
	drvdata = data;

	ret = misc_register(&s3c_g3d_dev);
	if (ret < 0) {
		printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
				G3D_MINOR, ret);
		goto err_misc_register;
	}

	INFO("Driver loaded succesfully\n");

	return 0;

err_misc_register:
#ifndef USE_G3D_DOMAIN_GATING
	g3d_do_power_down(data);
#else
	if(!hrtimer_cancel(&data->timer))
		g3d_power_down(data);
#endif
err_pm:
	free_irq(data->irq, pdev);
err_irq:
	iounmap(data->base);
err_ioremap:
        release_resource(data->mem);
err_mem:
err_clock:
	kfree(data);

	return ret;
}

static int s3c_g3d_remove(struct platform_device *pdev)
{
	struct g3d_drvdata *data = platform_get_drvdata(pdev);

#ifdef USE_G3D_DOMAIN_GATING
	if(!hrtimer_cancel(&data->timer))
		g3d_power_down(data);
#else
	g3d_flush(data, G3D_FGGB_PIPESTAT_MSK);
	g3d_flush_caches(data);
	g3d_do_power_down(data);
#endif

	misc_deregister(&s3c_g3d_dev);
	free_irq(data->irq, data);
	iounmap(data->base);
	release_resource(data->mem);
	kfree(data);

	INFO("Driver unloaded succesfully.\n");

	return 0;
}

static int s3c_g3d_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct g3d_drvdata *data = dev_get_drvdata(&pdev->dev);

#ifdef USE_G3D_DOMAIN_GATING
	if(!hrtimer_cancel(&data->timer))
		g3d_power_down(data);
#else
	g3d_flush(data, G3D_FGGB_PIPESTAT_MSK);
	g3d_flush_caches(data);
	g3d_do_power_down(data);
#endif
	return 0;
}

static int s3c_g3d_resume(struct platform_device *pdev)
{
#ifndef USE_G3D_DOMAIN_GATING
	struct g3d_drvdata *data = dev_get_drvdata(&pdev->dev);

	if(g3d_do_power_up(data) < 0) {
		ERR("G3D power up failed\n");
		return -EFAULT;
	}
#endif
	return 0;
}

static struct platform_driver s3c_g3d_driver = {
	.probe          = s3c_g3d_probe,
	.remove         = s3c_g3d_remove,
	.suspend        = s3c_g3d_suspend,
	.resume         = s3c_g3d_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= DRIVER_NAME,
	},
};

/*
	Module operations
*/

int __init  s3c_g3d_init(void)
{
	int ret;

	if ((ret = platform_driver_register(&s3c_g3d_driver)) != 0) {
		ERR("Platform device register failed (%d).\n", ret);
		return ret;
	}

	INFO("Module initialized.\n");

	return 0;
}

void  s3c_g3d_exit(void)
{
	platform_driver_unregister(&s3c_g3d_driver);

	INFO("Module exited.\n");
}

module_init(s3c_g3d_init);
module_exit(s3c_g3d_exit);

MODULE_AUTHOR("Tomasz Figa <tomasz.figa@gmail.com>");
MODULE_DESCRIPTION("S3C G3D Device Driver");
MODULE_LICENSE("GPL");
