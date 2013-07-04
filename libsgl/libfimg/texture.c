/*
 * fimg/texture.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE TEXTURE ENGINE RELATED FUNCTIONS
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

#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include "fimg_private.h"

/**
 * Creates a texture object.
 * @return Initialized texture object or NULL on error.
 */
fimgTexture *fimgCreateTexture(void)
{
	fimgTexture *texture;
	fimgTexControl *ctl;

	texture = malloc(sizeof(*texture));
	if(texture == NULL)
		return texture;

	memset(texture, 0, sizeof(*texture));

	ctl = (fimgTexControl *)&texture->hw.control;
	ctl->val = 0;
	ctl->useMipmap = FGTU_TSTA_MIPMAP_LINEAR;
	ctl->magFilter = FGTU_TSTA_FILTER_LINEAR;
	ctl->alphaFmt = FGTU_TSTA_AFORMAT_RGBA;
	ctl->type = FGTU_TSTA_TYPE_2D;

	return texture;
}

/**
 * Destroys a texture object.
 * @param texture Texture object to destroy.
 */
void fimgDestroyTexture(fimgTexture *texture)
{
	free(texture);
}

/**
 * Initializes basic settings of texture object.
 * @param texture Texture object.
 * @param flags Extra texture flags.
 * @param format Pixel format.
 * @param addr Physical address of texture memory.
 */
void fimgInitTexture(fimgTexture *texture, unsigned int flags,
				unsigned int format, unsigned long addr)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	texture->flags = flags;
	ctl->textureFmt = format;
	ctl->alphaFmt = !!(flags & FGTU_TEX_RGBA);
	texture->gem = addr;
}

/**
 * Sets offset of selected mipmap level.
 * @param texture Texture object.
 * @param level Mipmap level.
 * @param offset Offset in pixels.
 */
void fimgSetTexMipmapOffset(fimgTexture *texture, unsigned int level,
						unsigned int offset)
{
	if(level < 1 || level > FGTU_MAX_MIPMAP_LEVEL)
		return;

	texture->hw.offset[level - 1] = offset;
}

/**
 * Gets offset of selected mipmap level.
 * @param texture Texture object.
 * @param level Mipmap level.
 * @return Offset in pixels.
 */
unsigned int fimgGetTexMipmapOffset(fimgTexture *texture, unsigned level)
{
	if(level < 1 || level > FGTU_MAX_MIPMAP_LEVEL)
		return 0;

	return texture->hw.offset[level - 1];
}

/**
 * Configures selected texture unit to selected texture object.
 * (Must be called with hardware locked.)
 * @param ctx Hardware context.
 * @param texture Texture object.
 * @param unit Texture unit index.
 */
void fimgSetupTexture(fimgContext *ctx, fimgTexture *texture, unsigned unit)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	int ret;

	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_TEXTURE_SETUP;
	req.data = &texture->hw;
	req.length = sizeof(texture->hw);
	req.texture.flags = texture->flags;
	req.texture.unit = unit;
	req.texture.handle = texture->gem;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_TEXTURE_SETUP failed (%d)", ret);

	texture->flags &= ~G3D_TEXTURE_DIRTY;
}

/**
 * Sets available mipmap levels of texture object.
 * @param texture Texture object.
 * @param level Mipmap level.
 */
void fimgSetTexMipmapLevel(fimgTexture *texture, int level)
{
	texture->hw.max_level = level;
}

/**
 * Sets 2D texture size.
 * @param texture Texture object.
 * @param uSize Texture width.
 * @param vSize Texture height.
 * @param maxLevel Maximum mipmap level.
 */
void fimgSetTex2DSize(fimgTexture *texture,
		unsigned int uSize, unsigned int vSize, unsigned int maxLevel)
{
	texture->hw.width = uSize;
	texture->hw.height = vSize;
	texture->hw.max_level = maxLevel;
}

/**
 * Sets 3D texture size.
 * @param texture Texture object.
 * @param vSize Texture height.
 * @param uSize Texture width.
 * @param pSize Texture depth.
 */
void fimgSetTex3DSize(fimgTexture *texture, unsigned int vSize,
				unsigned int uSize, unsigned int pSize)
{
	texture->hw.width = uSize;
	texture->hw.height = vSize;
	texture->hw.depth = pSize;
}

/**
 * Sets U addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexUAddrMode(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->uAddrMode = mode;
}

/**
 * Sets V addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexVAddrMode(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->vAddrMode = mode;
}

/**
 * Sets P addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexPAddrMode(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->pAddrMode = mode;
}

/**
 * Sets texture minification filter.
 * @param texture Texture object.
 * @param mode Minification filter.
 */
void fimgSetTexMinFilter(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->minFilter = mode;
}

/**
 * Sets texture magnification filter.
 * @param texture Texture object.
 * @param mode Magnification filter.
 */
void fimgSetTexMagFilter(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->magFilter = mode;
}

/**
 * Controls mipmap enable state of texture.
 * @param texture Texture object.
 * @param mode Non-zero to enable mipmapping.
 */
void fimgSetTexMipmap(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->useMipmap = mode;
}

/**
 * Sets texture coordinate system.
 * @param texture Texture object.
 * @param mode Non-zero to use non-parametric coordinate system.
 */
void fimgSetTexCoordSys(fimgTexture *texture, unsigned mode)
{
	fimgTexControl *ctl = (fimgTexControl *)&texture->hw.control;

	ctl->texCoordSys = mode;
}
