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

#include <stdlib.h>
#include <string.h>
#include "fimg_private.h"

#define TEXTURE_OFFSET	0x60000

#define FGTU_TSTA(i)		(0x60000 + 0x50 * (i))
#define FGTU_USIZE(i)		(0x60004 + 0x50 * (i))
#define FGTU_VSIZE(i)		(0x60008 + 0x50 * (i))
#define FGTU_PSIZE(i)		(0x6000c + 0x50 * (i))
#define FGTU_TOFFS_L1(i)	(0x60010 + 0x50 * (i))
#define FGTU_TOFFS_L2(i)	(0x60014 + 0x50 * (i))
#define FGTU_TOFFS_L3(i)	(0x60018 + 0x50 * (i))
#define FGTU_TOFFS_L4(i)	(0x6001c + 0x50 * (i))
#define FGTU_TOFFS_L5(i)	(0x60020 + 0x50 * (i))
#define FGTU_TOFFS_L6(i)	(0x60024 + 0x50 * (i))
#define FGTU_TOFFS_L7(i)	(0x60028 + 0x50 * (i))
#define FGTU_TOFFS_L8(i)	(0x6002c + 0x50 * (i))
#define FGTU_TOFFS_L9(i)	(0x60030 + 0x50 * (i))
#define FGTU_TOFFS_L10(i)	(0x60034 + 0x50 * (i))
#define FGTU_TOFFS_L11(i)	(0x60038 + 0x50 * (i))
#define FGTU_T_MIN_L(i)		(0x6003c + 0x50 * (i))
#define FGTU_T_MAX_L(i)		(0x60040 + 0x50 * (i))
#define FGTU_TBADD(i)		(0x60044 + 0x50 * (i))
#define FGTU_CKEY(i)		(0x60280 + 4 * (i))
#define FGTU_CKYUV		(0x60288)
#define FGTU_CKMASK		(0x6028c)
#define FGTU_PALETTE_ADDR	(0x60290)
#define FGTU_PALETTE_IN		(0x60294)
#define FGTU_VTSTA(i)		(0x602c0 + 8 * (i))
#define FGTU_VTBADDR(i)		(0x602c4 + 8 * (i))

typedef union {
	unsigned int val;
	struct {
		unsigned b		:8;
		unsigned g		:8;
		unsigned r		:8;
		unsigned		:8;
	} bits;
} fimgTextureCKey;

typedef union {
	unsigned int val;
	struct {
		unsigned v		:8;
		unsigned u		:8;
		unsigned		:16;
	} bits;
} fimgTextureCKYUV;

/**
 * Creates a texture object.
 * @return Initialized texture object or NULL on error.
 */
fimgTexture *fimgCreateTexture(void)
{
	fimgTexture *texture;

	texture = malloc(sizeof(*texture));
	if(texture == NULL)
		return texture;

	memset(texture, 0, sizeof(*texture));

	texture->control.useMipmap = FGTU_TSTA_MIPMAP_LINEAR;
	texture->control.magFilter = FGTU_TSTA_FILTER_LINEAR;
	texture->control.alphaFmt = FGTU_TSTA_AFORMAT_RGBA;
	texture->control.type = FGTU_TSTA_TYPE_2D;

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
	texture->reserved2 = flags;
	texture->control.textureFmt = format;
	texture->control.alphaFmt = !!(flags & FGTU_TEX_RGBA);
	texture->baseAddr = addr;
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

	texture->offset[level - 1] = offset;
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

	return texture->offset[level - 1];
}

/**
 * Marks texture cache to be invalidated on next rendering.
 * @param ctx Hardware context.
 */
void fimgInvalidateTextureCache(fimgContext *ctx)
{
	ctx->invalTexCache = 1;
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
	volatile uint32_t *reg = (volatile uint32_t *)(ctx->base +FGTU_TSTA(unit));
	uint32_t *data = (uint32_t *)texture;
	unsigned count = sizeof(fimgTexture) / 4;

	asm volatile (
		"1:\n\t"
		"ldmia %1!, {r0-r3}\n\t"
		"stmia %0!, {r0-r3}\n\t"
		"subs %4, %4, $1\n\t"
		"bne 1b\n\t"
		: "=r"(reg), "=r"(data)
		: "0"(reg), "1"(data), "r"(count / 4)
		: "r0", "r1", "r2", "r3"
	);
}

/**
 * Sets available mipmap levels of texture object.
 * @param texture Texture object.
 * @param level Mipmap level.
 */
void fimgSetTexMipmapLevel(fimgTexture *texture, int level)
{
	texture->maxLevel = level;
}

/**
 * Sets texture base address of texture object.
 * @param texture Texture object.
 * @param addr Physical address of texture data.
 */
void fimgSetTexBaseAddr(fimgTexture *texture, unsigned int addr)
{
	texture->baseAddr = addr;
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
	texture->uSize = uSize;
	texture->vSize = vSize;
	texture->maxLevel = maxLevel;
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
	texture->uSize = uSize;
	texture->vSize = vSize;
	texture->pSize = pSize;
}

/**
 * Sets U addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexUAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.uAddrMode = mode;
}

/**
 * Sets V addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexVAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.vAddrMode = mode;
}

/**
 * Sets P addressing mode of texture.
 * @param texture Texture object.
 * @param mode Addressing mode.
 */
void fimgSetTexPAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.pAddrMode = mode;
}

/**
 * Sets texture minification filter.
 * @param texture Texture object.
 * @param mode Minification filter.
 */
void fimgSetTexMinFilter(fimgTexture *texture, unsigned mode)
{
	texture->control.minFilter = mode;
}

/**
 * Sets texture magnification filter.
 * @param texture Texture object.
 * @param mode Magnification filter.
 */
void fimgSetTexMagFilter(fimgTexture *texture, unsigned mode)
{
	texture->control.magFilter = mode;
}

/**
 * Controls mipmap enable state of texture.
 * @param texture Texture object.
 * @param mode Non-zero to enable mipmapping.
 */
void fimgSetTexMipmap(fimgTexture *texture, unsigned mode)
{
	texture->control.useMipmap = mode;
}

/**
 * Sets texture coordinate system.
 * @param texture Texture object.
 * @param mode Non-zero to use non-parametric coordinate system.
 */
void fimgSetTexCoordSys(fimgTexture *texture, unsigned mode)
{
	texture->control.texCoordSys = mode;
}
