/*
 * fimg/texture.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE TEXTURE ENGINE RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * Texture engine
 */

#define FGTU_MAX_MIPMAP_LEVEL	11

/* Type definitions */
enum {
	FGTU_TSTA_TYPE_2D = 1,
	FGTU_TSTA_TYPE_CUBE,
	FGTU_TSTA_TYPE_3D
};

enum {
	FGTU_TSTA_TEX_EXP_DUP = 0,
	FGTU_TSTA_TEX_EXP_ZERO
};

enum {
	FGTU_TSTA_AFORMAT_ARGB = 0,
	FGTU_TSTA_AFORMAT_RGBA
};

enum {
	FGTU_TSTA_PAL_TEX_FORMAT_1555 = 0,
	FGTU_TSTA_PAL_TEX_FORMAT_565,
	FGTU_TSTA_PAL_TEX_FORMAT_4444,
	FGTU_TSTA_PAL_TEX_FORMAT_8888
};

enum {
	FGTU_TSTA_TEXTURE_FORMAT_1555 = 0,
	FGTU_TSTA_TEXTURE_FORMAT_565,
	FGTU_TSTA_TEXTURE_FORMAT_4444,
	FGTU_TSTA_TEXTURE_FORMAT_DEPTHCOMP16,
	FGTU_TSTA_TEXTURE_FORMAT_88,
	FGTU_TSTA_TEXTURE_FORMAT_8,
	FGTU_TSTA_TEXTURE_FORMAT_8888,
	FGTU_TSTA_TEXTURE_FORMAT_1BPP,
	FGTU_TSTA_TEXTURE_FORMAT_2BPP,
	FGTU_TSTA_TEXTURE_FORMAT_4BPP,
	FGTU_TSTA_TEXTURE_FORMAT_8BPP,
	FGTU_TSTA_TEXTURE_FORMAT_S3TC,
	FGTU_TSTA_TEXTURE_FORMAT_Y1VY0U,
	FGTU_TSTA_TEXTURE_FORMAT_VY1UY0,
	FGTU_TSTA_TEXTURE_FORMAT_Y1UY0V,
	FGTU_TSTA_TEXTURE_FORMAT_UY1VY0
};

enum {
	FGTU_TSTA_ADDR_MODE_REPEAT = 0,
	FGTU_TSTA_ADDR_MODE_FLIP,
	FGTU_TSTA_ADDR_MODE_CLAMP
};

enum {
	FGTU_TSTA_TEX_COOR_PARAM = 0,
	FGTU_TSTA_TEX_COOR_NON_PARAM
};

enum {
	FGTU_TSTA_MIPMAP_DISABLED = 0,
	FGTU_TSTA_MIPMAP_NEAREST,
	FGTU_TSTA_MIPMAP_LINEAR
};

enum {
	FGTU_VTSTA_MOD_REPEAT = 0,
	FGTU_VTSTA_MOD_FLIP,
	FGTU_VTSTA_MOD_CLAMP
};

typedef union {
	unsigned int val;
	struct {
		unsigned		:3;
		unsigned type		:2;
		unsigned		:4;
		unsigned clrKeySel	:1;
		unsigned clrKeyEn	:1;
		unsigned texExp		:1;
		unsigned alphaFmt	:1;
		unsigned paletteFmt	:2;
		unsigned textureFmt	:5;
		unsigned uAddrMode	:2;
		unsigned vAddrMode	:2;
		unsigned pAddrMode	:2;
		unsigned		:1;
		unsigned texCoordSys	:1;
		unsigned magFilter	:1;
		unsigned minFilter	:1;
		unsigned useMipmap	:2;
	} bits;
} fimgTexControl;

typedef struct {
	fimgTexControl	ctrl;
	unsigned int	mipmapLvl;
	unsigned int 	uUSize;
	unsigned int 	uVSize;
	unsigned int 	uPSize;
} fimgTexUnitParams;

typedef union {
	unsigned int val;
	struct {
		unsigned		:20;
		unsigned umod		:2;
		unsigned vmod		:2;
		unsigned usize		:4;
		unsigned vsize		:4;
	} bits;
} fimgVtxTexControl;

/* Functions */
void fimgSetTexUnitParams(unsigned int unit, fimgTexUnitParams *params);
void fimgSetTexStatusParams(unsigned int unit, fimgTexControl params);
void fimgSetTexUSize(unsigned int unit, unsigned int uSize);
void fimgSetTexVSize(unsigned int unit, unsigned int vSize);
void fimgSetTexPSize(unsigned int unit, unsigned int pSize);
unsigned int fimgCalculateMipmapOffset(unsigned int unit,
				       unsigned int uSize,
				       unsigned int vSize,
				       unsigned int maxLevel);
unsigned int fimgCalculateMipmapOffsetYUV(unsigned int unit,
		unsigned int uSize,
		unsigned int vSize,
		unsigned int maxLevel);
unsigned int fimgCalculateMipmapOffsetS3TC(unsigned int unit,
		unsigned int uSize,
		unsigned int vSize,
		unsigned int maxLevel);
void fimgSetTexMipmapLevel(unsigned int unit, int min, int max);
void fimgSetTexBaseAddr(unsigned int unit, unsigned int addr);
void fimgSetTexColorKey(unsigned int unit,
			unsigned char r, unsigned char g, unsigned char b);
void fimgSetTexColorKeyYUV(unsigned char u, unsigned char v);
void fimgSetTexColorKeyMask(unsigned char bitsToMask);
void fimgSetTexPaletteAddr(unsigned char addr);
void fimgSetTexPaletteEntry(unsigned int entry);
void fimgSetVtxTexUnitParams(unsigned int unit, fimgVtxTexControl vts);
void fimgSetVtxTexBaseAddr(unsigned int unit, unsigned int addr);

#ifdef __cplusplus
}
#endif

#endif
