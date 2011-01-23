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

/* TODO: Consider inlining some of the functions */

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

void fimgDestroyTexture(fimgTexture *texture)
{
	free(texture);
}

void fimgInitTexture(fimgTexture *texture, unsigned int format,
			unsigned int maxLevel, unsigned long addr)
{
	texture->control.textureFmt = format & 0xf;
	texture->control.alphaFmt = !(format >> 4);
	texture->maxLevel = maxLevel;
	texture->baseAddr = addr;
}

void fimgSetTexMipmapOffset(fimgTexture *texture, unsigned int level,
						unsigned int offset)
{
	if(level < 1 || level > FGTU_MAX_MIPMAP_LEVEL)
		return;

	texture->offset[level - 1] = offset;
}

unsigned int fimgGetTexMipmapOffset(fimgTexture *texture, unsigned level)
{
	if(level < 1 || level > FGTU_MAX_MIPMAP_LEVEL)
		return 0;

	return texture->offset[level - 1];
}

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

	count %= 4;

	if (count--)
		*(reg++) = *(data++);

	if (count--)
		*(reg++) = *(data++);

	if (count--)
		*(reg++) = *(data++);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexMipmapLevel
* SYNOPSIS:	This function sets texture mipmap level.
* PARAMETERS:	[IN] unsigned int level: max level (<12)
*****************************************************************************/
void fimgSetTexMipmapLevel(fimgTexture *texture, int level)
{
	texture->maxLevel = level;
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexBaseAddr
* SYNOPSIS:	This function sets texture base address
* 		for selected texture unit.
* PARAMETERS:	[IN] unsigned int addr: base address
*****************************************************************************/
void fimgSetTexBaseAddr(fimgTexture *texture, unsigned int addr)
{
	texture->baseAddr = addr;
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexUSize
* SYNOPSIS:	This function sets a texture u size
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int uSize: texture u size (0~2047)
*****************************************************************************/
void fimgSetTex2DSize(fimgTexture *texture,
	unsigned int uSize, unsigned int vSize)
{
	texture->uSize = uSize;
	texture->vSize = vSize;
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexVSize
* SYNOPSIS:	This function sets a texture v size
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int vSize: texture v size (0~2047)
*****************************************************************************/
void fimgSetTex3DSize(fimgTexture *texture, unsigned int vSize,
				unsigned int uSize, unsigned int pSize)
{
	texture->uSize = uSize;
	texture->vSize = vSize;
	texture->pSize = pSize;
}

void fimgSetTexUAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.uAddrMode = mode;
}

void fimgSetTexVAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.vAddrMode = mode;
}

void fimgSetTexPAddrMode(fimgTexture *texture, unsigned mode)
{
	texture->control.pAddrMode = mode;
}

void fimgSetTexMinFilter(fimgTexture *texture, unsigned mode)
{
	texture->control.minFilter = mode;
}

void fimgSetTexMagFilter(fimgTexture *texture, unsigned mode)
{
	texture->control.magFilter = mode;
}

void fimgSetTexMipmap(fimgTexture *texture, unsigned mode)
{
	texture->control.useMipmap = mode;
}

void fimgSetTexCoordSys(fimgTexture *texture, unsigned mode)
{
	texture->control.texCoordSys = mode;
}

#if 0
/*****************************************************************************
* FUNCTIONS:	fimgSetTexUnitParams
* SYNOPSIS:	This function sets various texture parameters
* 		for selected texture unit.
*		(type, texture half decimation factor control,
*		texture decimation factor, color key selection,
*		texture value expansion method, bilinear filter register,
*		MIPMAP control register, texture format,
*		u address mode, v address mode, u size, v size)
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	FGL_TexUnitParams params: texture parameter structure
*****************************************************************************/
void fimgSetTexUnitParams(fimgContext *ctx,
			  unsigned int unit, fimgTexUnitParams *params)
{
	unsigned int TexRegs[4] = {0, 0, 0, 0};
	unsigned int regAddr;
	unsigned int *curReg;

	if(!params->mipmapLvl && params->ctrl.bits.useMipmap)
		params->ctrl.bits.useMipmap = FGTU_TSTA_MIPMAP_DISABLED;

	curReg = TexRegs;
	*(curReg++) = params->ctrl.val; //FGTU_TSTAx
	*(curReg++) = params->uUSize;   //FGTU_USIZEx
	*(curReg++) = params->uVSize;   //FGTU_VSIZEx
	*(curReg++) = params->uPSize;   //FGTU_PSIZEx

	curReg = TexRegs;
	regAddr = FGTU_TSTA(unit);
	fimgWrite(ctx, *(curReg++), regAddr);
	regAddr += 4;
	fimgWrite(ctx, *(curReg++), regAddr);
	regAddr += 4;
	fimgWrite(ctx, *(curReg++), regAddr);
	regAddr += 4;
	fimgWrite(ctx, *(curReg++), regAddr);

	if(params->ctrl.bits.useMipmap) {
		switch(params->ctrl.bits.textureFmt) {
		case FGTU_TSTA_TEXTURE_FORMAT_S3TC:
			params->mipmapLvl = fimgCalculateMipmapOffsetS3TC(ctx, unit, params->uUSize, params->uVSize, params->mipmapLvl);
			break;
		case FGTU_TSTA_TEXTURE_FORMAT_UY1VY0:
		case FGTU_TSTA_TEXTURE_FORMAT_VY1UY0:
		case FGTU_TSTA_TEXTURE_FORMAT_Y1UY0V:
		case FGTU_TSTA_TEXTURE_FORMAT_Y1VY0U:
			params->mipmapLvl = fimgCalculateMipmapOffsetYUV(ctx, unit, params->uUSize, params->uVSize, params->mipmapLvl);
			break;
		default:
			params->mipmapLvl = fimgCalculateMipmapOffset(ctx, unit, params->uUSize, params->uVSize, params->mipmapLvl);
		}
	}
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexStatusParams
* SYNOPSIS:	This function sets various texture parameters
* 		for selected texture unit.
* 		(type, color key selection, texture msb expansion method,
* 		palette format, texture format,
* 		u address mode, v address mode,
* 		parametric or nonparametric addressing,
* 		and mag., min. and mipmap filter)
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	fimgTexControl params: texture parameter structure
*****************************************************************************/
void fimgSetTexStatusParams(fimgContext *ctx,
			    unsigned int unit, fimgTexControl params)
{
	fimgWrite(ctx, params.val, FGTU_TSTA(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgCalculateMipmapOffset
* SYNOPSIS:	This function sets a texture mipmap offset and finds maximum level
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int uSize: texture u size (0~2047)
*		[IN]	unsigned int vSize: texture v size (0~2047)
*		[IN]	unsigned int maxLevel: max. mipmap level (0~11)
* RETURNS:	Maximum level of texture mipmap
*****************************************************************************/
unsigned int fimgCalculateMipmapOffset(fimgContext *ctx, unsigned int unit,
				       unsigned int uUSize, unsigned int uVSize,
				       unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	unsigned int regAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		fimgWrite(ctx, uOffset, regAddr);
		regAddr += 4;

		if(++uMipMapLevel == maxLevel)
			break;

		if(uUSize >= 2) {
			uMipMapSize /= 2;
			uUSize /= 2;
		}

		if(uVSize >= 2) {
			uMipMapSize /= 2;
			uVSize /= 2;
		}

		uCheckSize /= 2;
		uOffset += uMipMapSize;
	}

	fimgWrite(ctx, 0, FGTU_T_MIN_L(unit));
	fimgWrite(ctx, uMipMapLevel, FGTU_T_MAX_L(unit));

	return uMipMapLevel;
}

/*****************************************************************************
* FUNCTIONS:	fimgCalculateMipmapOffsetYUV
* SYNOPSIS:	This function sets a texture mipmap offset and finds maximum level
* 		for selected texture unit. (YUV variant)
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int uSize: texture u size (0~2047)
*		[IN]	unsigned int vSize: texture v size (0~2047)
*		[IN]	unsigned int maxLevel: max. mipmap level (0~11)
* RETURNS:	Maximum level of texture mipmap
*****************************************************************************/
unsigned int fimgCalculateMipmapOffsetYUV(fimgContext *ctx, unsigned int unit,
					  unsigned int uUSize, unsigned int uVSize,
					  unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	unsigned int regAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		fimgWrite(ctx, uOffset, regAddr);
		regAddr += 4;

		if(++uMipMapLevel == maxLevel)
			break;

		if(uUSize >= 2) {
			uMipMapSize /= 2;
			uUSize /= 2;
		}

		if(uVSize >= 2) {
			uMipMapSize /= 2;
			uVSize /= 2;
		}

		uCheckSize /= 2;
		uOffset += uMipMapSize;

		/* Round up to whole block (2 pixels) */
		uOffset += uMipMapSize % 2;
	}

	fimgWrite(ctx, 0, FGTU_T_MIN_L(unit));
	fimgWrite(ctx, uMipMapLevel, FGTU_T_MAX_L(unit));

	return uMipMapLevel;
}

/*****************************************************************************
* FUNCTIONS:	fimgCalculateMipmapOffsetS3TC
* SYNOPSIS:	This function sets a texture mipmap offset and finds maximum level
* 		for selected texture unit. (S3TC variant)
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int uSize: texture u size (0~2047)
*		[IN]	unsigned int vSize: texture v size (0~2047)
*		[IN]	unsigned int maxLevel: max. mipmap level (0~11)
* RETURNS:	Maximum level of texture mipmap
*****************************************************************************/
unsigned int fimgCalculateMipmapOffsetS3TC(fimgContext *ctx, unsigned int unit,
					   unsigned int uUSize, unsigned int uVSize,
					   unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	unsigned int regAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		fimgWrite(ctx, uOffset, regAddr);
		regAddr += 4;

		if(++uMipMapLevel == maxLevel)
			break;

		if(uUSize >= 2) {
			uMipMapSize /= 2;
			uUSize /= 2;
		}

		if(uVSize >= 2) {
			uMipMapSize /= 2;
			uVSize /= 2;
		}

		uCheckSize /= 2;
		uOffset += uMipMapSize;

		/* Round up to whole block (16 pixels) */
		uOffset += (16 - (uMipMapSize % 16)) % 16;
	}

	fimgWrite(ctx, 0, FGTU_T_MIN_L(unit));
	fimgWrite(ctx, uMipMapLevel, FGTU_T_MAX_L(unit));

	return uMipMapLevel;
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKey
* SYNOPSIS:	This function sets 3D color key 1 or 2 register
* PARAMETERS:	[IN] unsigned int unit: 3D color key register (0~1)
*		[IN] unsigned char r: red value
*		[IN] unsigned char g: green value
*		[IN] unsigned char b: blue value
*****************************************************************************/
void fimgSetTexColorKey(fimgContext *ctx, unsigned int unit,
			unsigned char r, unsigned char g, unsigned char b)
{
	fimgTextureCKey reg;

	reg.val = 0;
	reg.bits.r = r;
	reg.bits.g = g;
	reg.bits.b = b;

	fimgWrite(ctx, reg.val, FGTU_CKEY(!!unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKeyYUV
* SYNOPSIS:	This function sets 3D color key YUV register
* PARAMETERS:	[IN] unsigned char u: color key U value
* 		[IN] unsigned char v: color key V value
*****************************************************************************/
void fimgSetTexColorKeyYUV(fimgContext *ctx, unsigned char u, unsigned char v)
{
	fimgTextureCKYUV reg;

	reg.val = 0;
	reg.bits.u = u;
	reg.bits.v = v;

	fimgWrite(ctx, reg.val, FGTU_CKYUV);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKeyMask
* SYNOPSIS:	This function sets 3D color key mask register
* PARAMETERS:	[IN] unsigned char bitsToMask: 3D color key mask value (0~7)
*****************************************************************************/
void fimgSetTexColorKeyMask(fimgContext *ctx, unsigned char bitsToMask)
{
	fimgWrite(ctx, bitsToMask, FGTU_CKMASK);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexPaletteAddr
* SYNOPSIS:	This function sets palette address for texturing
* PARAMETERS:	[IN] unsigned int addr: 8-bit palette address (0~255)
*****************************************************************************/
void fimgSetTexPaletteAddr(fimgContext *ctx, unsigned char addr)
{
	fimgWrite(ctx, addr, FGTU_PALETTE_ADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexPaletteEntry
* SYNOPSIS:	This function sets palette entry for texturing
* PARAMETERS:	[IN] unsigned int entry: palette data in
*****************************************************************************/
void fimgSetTexPaletteEntry(fimgContext *ctx, unsigned int entry)
{
	fimgWrite(ctx, entry, FGTU_PALETTE_IN);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetVtxTexUnitParams
* SYNOPSIS:	This function sets vertex texture status register
* 		for selected vertex texture unit.
* PARAMETERS:	[IN] unsigned int unit: vertex texture unit ( 0~3 )
*		[IN] FGL_VtxTexStatusReg vts: vertex texture status structure
*****************************************************************************/
void fimgSetVtxTexUnitParams(fimgContext *ctx, unsigned int unit,
			     fimgVtxTexControl vts)
{
	fimgWrite(ctx, vts.val, FGTU_VTSTA(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetVtxTexBaseAddr
* SYNOPSIS:	This function sets vertex texture base address
* 		for selected vertex texture unit.
* PARAMETERS:	[IN] unsigned int unit: vertex texture unit ( 0~3 )
* 		[IN] unsigned int addr: vertex texture base address
*****************************************************************************/
void fimgSetVtxTexBaseAddr(fimgContext *ctx, unsigned int unit,
			   unsigned int addr)
{
	fimgWrite(ctx, addr, FGTU_VTBADDR(unit));
}

#endif

