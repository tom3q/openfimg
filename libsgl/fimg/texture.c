/*
* fimg/texture.c
*
* SAMSUNG S3C6410 FIMG-3DSE TEXTURE ENGINE RELATED FUNCTIONS
*
* Copyrights:	2006 by Samsung Electronics Limited (original code)
*		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
*/

#include "fimg_private.h"

#define TEXTURE_OFFSET	0x60000

#define FGTU_TSTA(i)		TEXTURE_ADDR(0x00 + 0x50 * (i))
#define FGTU_USIZE(i)		TEXTURE_ADDR(0x04 + 0x50 * (i))
#define FGTU_VSIZE(i)		TEXTURE_ADDR(0x08 + 0x50 * (i))
#define FGTU_PSIZE(i)		TEXTURE_ADDR(0x0c + 0x50 * (i))
#define FGTU_TOFFS_L1(i)	TEXTURE_ADDR(0x10 + 0x50 * (i))
#define FGTU_TOFFS_L2(i)	TEXTURE_ADDR(0x14 + 0x50 * (i))
#define FGTU_TOFFS_L3(i)	TEXTURE_ADDR(0x18 + 0x50 * (i))
#define FGTU_TOFFS_L4(i)	TEXTURE_ADDR(0x1c + 0x50 * (i))
#define FGTU_TOFFS_L5(i)	TEXTURE_ADDR(0x20 + 0x50 * (i))
#define FGTU_TOFFS_L6(i)	TEXTURE_ADDR(0x24 + 0x50 * (i))
#define FGTU_TOFFS_L7(i)	TEXTURE_ADDR(0x28 + 0x50 * (i))
#define FGTU_TOFFS_L8(i)	TEXTURE_ADDR(0x2c + 0x50 * (i))
#define FGTU_TOFFS_L9(i)	TEXTURE_ADDR(0x30 + 0x50 * (i))
#define FGTU_TOFFS_L10(i)	TEXTURE_ADDR(0x34 + 0x50 * (i))
#define FGTU_TOFFS_L11(i)	TEXTURE_ADDR(0x38 + 0x50 * (i))
#define FGTU_T_MIN_L(i)		TEXTURE_ADDR(0x3c + 0x50 * (i))
#define FGTU_T_MAX_L(i)		TEXTURE_ADDR(0x40 + 0x50 * (i))
#define FGTU_TBADD(i)		TEXTURE_ADDR(0x44 + 0x50 * (i))
#define FGTU_CKEY(i)		TEXTURE_ADDR(0x280 + 4 * (i))
#define FGTU_CKYUV		TEXTURE_ADDR(0x288)
#define FGTU_CKMASK		TEXTURE_ADDR(0x28c)
#define FGTU_PALETTE_ADDR	TEXTURE_ADDR(0x290)
#define FGTU_PALETTE_IN		TEXTURE_ADDR(0x294)
#define FGTU_VTSTA(i)		TEXTURE_ADDR(0x2c0 + 8 * (i))
#define FGTU_VTBADDR(i)		TEXTURE_ADDR(0x2c4 + 8 * (i))

#define TEXTURE_OFFS(reg)	(TEXTURE_OFFSET + (reg))
#define TEXTURE_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + TEXTURE_OFFS(reg)))

typedef union {
	unsigned int val;
	struct {
		unsigned		:8;
		unsigned r		:8;
		unsigned g		:8;
		unsigned b		:8;
	} bits;
} fimgTextureCKey;

typedef union {
	unsigned int val;
	struct {
		unsigned		:16;
		unsigned u		:8;
		unsigned v		:8;
	} bits;
} fimgTextureCKYUV;

static inline void fimgTextureWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline unsigned int fimgTextureRead(volatile unsigned int *reg)
{
	return *reg;
}

/* TODO: Function inlining */

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
void fimgSetTexUnitParams(unsigned int unit, fimgTexUnitParams *params)
{
	unsigned int uUSize = params->uUSize;
	unsigned int uVSize = params->uVSize;
	unsigned int uPSize = params->uPSize;
	unsigned int uIndex = 0;
	volatile unsigned int *pTexRegBaseAddr;
	unsigned int TexRegs[4] = {0, 0, 0, 0};
	unsigned int *pTexRegVal = (unsigned int *)TexRegs;

	pTexRegBaseAddr = FGTU_TSTA(unit);

	if(!params->mipmapLvl && params->ctrl.bits.useMipmap)
		params->ctrl.bits.useMipmap = FGTU_TSTA_MIPMAP_DISABLED;

	//FGTU_TSTAx
	TexRegs[uIndex++] = params->ctrl.val;
	//FGTU_USIZEx
	TexRegs[uIndex++] = uUSize;
	//FGTU_VSIZEx
	TexRegs[uIndex++] = uVSize;
	//FGTU_PSIZEx
	TexRegs[uIndex++] = uPSize;

	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);

	if(params->ctrl.bits.useMipmap) {
		switch(params->ctrl.bits.textureFmt) {
		case FGTU_TSTA_TEXTURE_FORMAT_S3TC:
			params->mipmapLvl = fimgCalculateMipmapOffsetS3TC(unit, params->uUSize, params->uVSize, params->mipmapLvl);
			break;
		case FGTU_TSTA_TEXTURE_FORMAT_UY1VY0:
		case FGTU_TSTA_TEXTURE_FORMAT_VY1UY0:
		case FGTU_TSTA_TEXTURE_FORMAT_Y1UY0V:
		case FGTU_TSTA_TEXTURE_FORMAT_Y1VY0U:
			params->mipmapLvl = fimgCalculateMipmapOffsetYUV(unit, params->uUSize, params->uVSize, params->mipmapLvl);
			break;
		default:
			params->mipmapLvl = fimgCalculateMipmapOffset(unit, params->uUSize, params->uVSize, params->mipmapLvl);
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
void fimgSetTexStatusParams(unsigned int unit, fimgTexControl params)
{
	fimgTextureWrite(params.val, FGTU_TSTA(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexUSize
* SYNOPSIS:	This function sets a texture u size
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int uSize: texture u size (0~2047)
*****************************************************************************/
void fimgSetTexUSize(unsigned int unit, unsigned int uSize)
{
	fimgTextureWrite(uSize, FGTU_USIZE(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexVSize
* SYNOPSIS:	This function sets a texture v size
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int vSize: texture v size (0~2047)
*****************************************************************************/
void fimgSetTexVSize(unsigned int unit, unsigned int vSize)
{
	fimgTextureWrite(vSize, FGTU_VSIZE(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexPSize
* SYNOPSIS:	This function sets a texture p size
* 		for selected texture unit.
* PARAMETERS:	[IN]	unsigned int unit: texture unit (0~7)
*		[IN]	unsigned int pSize: texture p size (0~2047)
*****************************************************************************/
void fimgSetTexPSize(unsigned int unit, unsigned int pSize)
{
	fimgTextureWrite(pSize, FGTU_PSIZE(unit));
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
unsigned int fimgCalculateMipmapOffset(unsigned int unit,
				       unsigned int uUSize,
				       unsigned int uVSize,
				       unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	volatile unsigned int *pTexRegBaseAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		fimgTextureWrite(uOffset, pTexRegBaseAddr++);

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

	fimgTextureWrite(0, FGTU_T_MIN_L(unit));
	fimgTextureWrite(uMipMapLevel, FGTU_T_MAX_L(unit));

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
unsigned int fimgCalculateMipmapOffsetYUV(unsigned int unit,
		unsigned int uUSize,
		unsigned int uVSize,
		unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	unsigned int TexRegs[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int *pTexRegVal = (unsigned int *)TexRegs;
	volatile unsigned int *pTexRegBaseAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		*(pTexRegVal++) = uOffset;

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

	TexRegs[12] = uMipMapLevel;

	pTexRegVal = (unsigned int *)TexRegs;
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);

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
unsigned int fimgCalculateMipmapOffsetS3TC(unsigned int unit,
		unsigned int uUSize,
		unsigned int uVSize,
		unsigned int maxLevel)
{
	unsigned int uMipMapSize;
	unsigned int uOffset;
	unsigned int uMipMapLevel = 0;
	unsigned int uCheckSize;
	unsigned int TexRegs[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int *pTexRegVal = (unsigned int *)TexRegs;
	volatile unsigned int *pTexRegBaseAddr = FGTU_TOFFS_L1(unit);

	if(maxLevel > FGTU_MAX_MIPMAP_LEVEL)
		maxLevel = FGTU_MAX_MIPMAP_LEVEL;

	uMipMapSize = uUSize * uVSize;
	uOffset = uMipMapSize;
	uCheckSize = uUSize > uVSize ? uUSize : uVSize;
	uCheckSize /= 2;
	while(uCheckSize > 0) {
		*(pTexRegVal++) = uOffset;

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

	TexRegs[12] = uMipMapLevel;

	pTexRegVal = (unsigned int *)TexRegs;
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);
	fimgTextureWrite(*(pTexRegVal++), pTexRegBaseAddr++);

	return uMipMapLevel;
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexMipmapLevel
* SYNOPSIS:	This function sets texture base level and/or max. level
*		of mipmap for selected texture unit.
* PARAMETERS:	[IN] unsigned int unit: texture unit (0~7)
*		[IN] int min: base level or negative to keep current value (<12)
* 		[IN] int max: max level or negative to keep current value (<12)
*****************************************************************************/
void fimgSetTexMipmapLevel(unsigned int unit, int min, int max)
{
	if(min >= 0)
		fimgTextureWrite(min, FGTU_T_MIN_L(unit));
	if(max >= 0)
		fimgTextureWrite(max, FGTU_T_MAX_L(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexBaseAddr
* SYNOPSIS:	This function sets texture base address
* 		for selected texture unit.
* PARAMETERS:	[IN] unsigned int unit: texture unit (0~7)
* 		[IN] unsigned int addr: base address
*****************************************************************************/
void fimgSetTexBaseAddr(unsigned int unit, unsigned int addr)
{
	fimgTextureWrite(addr, FGTU_TBADD(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKey
* SYNOPSIS:	This function sets 3D color key 1 or 2 register
* PARAMETERS:	[IN] unsigned int unit: 3D color key register (0~1)
*		[IN] unsigned char r: red value
*		[IN] unsigned char g: green value
*		[IN] unsigned char b: blue value
*****************************************************************************/
void fimgSetTexColorKey(unsigned int unit,
			unsigned char r, unsigned char g, unsigned char b)
{
	fimgTextureCKey reg;

	reg.val = 0;
	reg.bits.r = r;
	reg.bits.g = g;
	reg.bits.b = b;

	fimgTextureWrite(reg.val, FGTU_CKEY(!!unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKeyYUV
* SYNOPSIS:	This function sets 3D color key YUV register
* PARAMETERS:	[IN] unsigned char u: color key U value
* 		[IN] unsigned char v: color key V value
*****************************************************************************/
void fimgSetTexColorKeyYUV(unsigned char u, unsigned char v)
{
	fimgTextureCKYUV reg;

	reg.val = 0;
	reg.bits.u = u;
	reg.bits.v = v;

	fimgTextureWrite(reg.val, FGTU_CKYUV);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexColorKeyMask
* SYNOPSIS:	This function sets 3D color key mask register
* PARAMETERS:	[IN] unsigned char bitsToMask: 3D color key mask value (0~7)
*****************************************************************************/
void fimgSetTexColorKeyMask(unsigned char bitsToMask)
{
	fimgTextureWrite(bitsToMask, FGTU_CKMASK);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexPaletteAddr
* SYNOPSIS:	This function sets palette address for texturing
* PARAMETERS:	[IN] unsigned int addr: 8-bit palette address (0~255)
*****************************************************************************/
void fimgSetTexPaletteAddr(unsigned char addr)
{
	fimgTextureWrite(addr, FGTU_PALETTE_ADDR);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetTexPaletteEntry
* SYNOPSIS:	This function sets palette entry for texturing
* PARAMETERS:	[IN] unsigned int entry: palette data in
*****************************************************************************/
void fimgSetTexPaletteEntry(unsigned int entry)
{
	fimgTextureWrite(entry, FGTU_PALETTE_IN);
}

/*****************************************************************************
* FUNCTIONS:	fimgSetVtxTexUnitParams
* SYNOPSIS:	This function sets vertex texture status register
* 		for selected vertex texture unit.
* PARAMETERS:	[IN] unsigned int unit: vertex texture unit ( 0~3 )
*		[IN] FGL_VtxTexStatusReg vts: vertex texture status structure
*****************************************************************************/
void fimgSetVtxTexUnitParams(unsigned int unit, fimgVtxTexControl vts)
{
	fimgTextureWrite(vts.val, FGTU_VTSTA(unit));
}

/*****************************************************************************
* FUNCTIONS:	fimgSetVtxTexBaseAddr
* SYNOPSIS:	This function sets vertex texture base address
* 		for selected vertex texture unit.
* PARAMETERS:	[IN] unsigned int unit: vertex texture unit ( 0~3 )
* 		[IN] unsigned int addr: vertex texture base address
*****************************************************************************/
void fimgSetVtxTexBaseAddr(unsigned int unit, unsigned int addr)
{
	fimgTextureWrite(addr, FGTU_VTBADDR(unit));
}
