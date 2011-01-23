/*
 * fimg/compat.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE FIXED PIPELINE COMPATIBILITY FUNCTIONS
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

#include <string.h>
#include <stdio.h>
#include "fimg_private.h"
#include "shaders/vert.h"
#include "shaders/frag.h"

#define FGVS_INSTMEM_START	(0x10000)
#define FGVS_CFLOAT_START	(0x14000)
#define FGVS_CINT_START		(0x18000)
#define FGVS_CBOOL_START	(0x18400)

#define FGVS_CONFIG		(0x1c800)
#define FGVS_STATUS		(0x1c804)
#define FGVS_PCRANGE		(0x20000)
#define FGVS_ATTRIB_NUM		(0x20004)

#define FGVS_IN_ATTR_IDX(i)	(0x20008 + 4*(i))
#define FGVS_OUT_ATTR_IDX(i)	(0x20014 + 4*(i))

typedef union {
	uint32_t val;
	struct {
		unsigned copyPC		:1;
		unsigned clrStatus	:1;
		unsigned		:30;
	};
} fimgVShaderConfig;

typedef union {
	uint32_t val;
	struct {
		unsigned PCStart	:9;
		unsigned		:7;
		unsigned PCEnd		:9;
		unsigned		:6;
		unsigned ignorePCEnd	:1;
	};
} fimgVShaderPCRange;

typedef union {
	uint32_t val;
	struct {
		unsigned num		:4;
		unsigned		:4;
	} attrib[4];
} fimgVShaderAttrIdx;

#define FGPS_INSTMEM_START	(0x40000)
#define FGPS_CFLOAT_START	(0x44000)
#define FGPS_CINT_START		(0x48000)
#define FGPS_CBOOL_START	(0x48400)

#define FGPS_EXE_MODE		(0x4c800)
#define FGPS_PC_START		(0x4c804)
#define FGPS_PC_END		(0x4c808)
#define FGPS_PC_COPY		(0x4c80c)
#define FGPS_ATTRIB_NUM		(0x4c810)
#define FGPS_IBSTATUS		(0x4c814)

typedef union {
	uint32_t val;
	struct {
		unsigned PCEnd		:9;
		unsigned ignorePCEnd	:1;
		unsigned		:22;
	};
} fimgPShaderPCEnd;

/*****************************************************************************
 * FUNCTIONS:	fimgLoadMatrix
 * SYNOPSIS:	This function loads the specified matrix (4x4) into const float
 *		registers of vertex shader.
 * PARAMETERS:	[IN] matrix - which matrix to load (FGL_MATRIX_*)
 *		[IN] pData - pointer to matrix elements in column-major ordering
 *****************************************************************************/
void fimgLoadMatrix(fimgContext *ctx, uint32_t matrix, const float *pfData)
{
	ctx->compat.matrix[matrix] = pfData;
	ctx->compat.matrixDirty[matrix] = 1;
}

/*
 * SHADERS
 */

struct shaderBlock {
	const uint32_t *data;
	uint32_t len;
};
#define SHADER_BLOCK(blk)	{ blk, sizeof(blk) / 16 }

/* Vertex shader */

static const struct shaderBlock vertexConstFloat = SHADER_BLOCK(vert_cfloat);
static const struct shaderBlock vertexHeader = SHADER_BLOCK(vert_header);
static const struct shaderBlock vertexFooter = SHADER_BLOCK(vert_footer);

static const struct shaderBlock texcoordTransform[] = {
	SHADER_BLOCK(vert_texture0),
	SHADER_BLOCK(vert_texture1)
};

static const struct shaderBlock vertexClear = SHADER_BLOCK(vert_clear);

/* Pixel shader */

static const struct shaderBlock pixelConstFloat = SHADER_BLOCK(frag_cfloat);
static const struct shaderBlock pixelHeader = SHADER_BLOCK(frag_header);
static const struct shaderBlock pixelFooter = SHADER_BLOCK(frag_footer);

static const struct shaderBlock textureUnit[] = {
	SHADER_BLOCK(frag_texture0),
	SHADER_BLOCK(frag_texture1)
};

static const struct shaderBlock textureFunc[] = {
	SHADER_BLOCK(frag_replace),
	SHADER_BLOCK(frag_modulate),
	SHADER_BLOCK(frag_decal),
	SHADER_BLOCK(frag_blend),
	SHADER_BLOCK(frag_add),
	SHADER_BLOCK(frag_combine)
};

static const struct shaderBlock combineFunc[] = {
	SHADER_BLOCK(frag_combine_replace),
	SHADER_BLOCK(frag_combine_modulate),
	SHADER_BLOCK(frag_combine_add),
	SHADER_BLOCK(frag_combine_adds),
	SHADER_BLOCK(frag_combine_interpolate),
	SHADER_BLOCK(frag_combine_subtract),
	SHADER_BLOCK(frag_combine_dot3),
	SHADER_BLOCK(frag_combine_dot3) // for RGBA mode
};

static const struct shaderBlock combineArg0[] = {
	SHADER_BLOCK(frag_combine_arg0tex),
	SHADER_BLOCK(frag_combine_arg0const),
	SHADER_BLOCK(frag_combine_arg0col),
	SHADER_BLOCK(frag_combine_arg0prev)
};

static const struct shaderBlock combineArg1[] = {
	SHADER_BLOCK(frag_combine_arg1tex),
	SHADER_BLOCK(frag_combine_arg1const),
	SHADER_BLOCK(frag_combine_arg1col),
	SHADER_BLOCK(frag_combine_arg1prev)
};

static const struct shaderBlock combineArg2[] = {
	SHADER_BLOCK(frag_combine_arg2tex),
	SHADER_BLOCK(frag_combine_arg2const),
	SHADER_BLOCK(frag_combine_arg2col),
	SHADER_BLOCK(frag_combine_arg2prev)
};

static const struct shaderBlock *combineArg[] = {
	combineArg0,
	combineArg1,
	combineArg2
};

static const struct shaderBlock combineArg0Mod[] = {
	SHADER_BLOCK(frag_combine_arg0sc),
	SHADER_BLOCK(frag_combine_arg0omsc),
	SHADER_BLOCK(frag_combine_arg0sa),
	SHADER_BLOCK(frag_combine_arg0omsa)
};

static const struct shaderBlock combineArg1Mod[] = {
	SHADER_BLOCK(frag_combine_arg1sc),
	SHADER_BLOCK(frag_combine_arg1omsc),
	SHADER_BLOCK(frag_combine_arg1sa),
	SHADER_BLOCK(frag_combine_arg1omsa)
};

static const struct shaderBlock combineArg2Mod[] = {
	SHADER_BLOCK(frag_combine_arg2sc),
	SHADER_BLOCK(frag_combine_arg2omsc),
	SHADER_BLOCK(frag_combine_arg2sa),
	SHADER_BLOCK(frag_combine_arg2omsa)
};

static const struct shaderBlock *combineArgMod[] = {
	combineArg0Mod,
	combineArg1Mod,
	combineArg2Mod
};

static const struct shaderBlock combine_c = SHADER_BLOCK(frag_combine_col);
static const struct shaderBlock combine_a = SHADER_BLOCK(frag_combine_a);
static const struct shaderBlock combine_u = SHADER_BLOCK(frag_combine_uni);

static const struct shaderBlock pixelClear = SHADER_BLOCK(frag_clear);

/* Shader functions */

//#define FIMG_DYNSHADER_DEBUG

static inline volatile void *vsInstAddr(fimgContext *ctx, unsigned int slot)
{
	return ctx->base + FGVS_INSTMEM_START + 16*slot;
}

static inline uint32_t vsInstLen(fimgContext *ctx, volatile void *vaddr)
{
	return ((volatile char *)vaddr - (ctx->base + FGVS_INSTMEM_START)) / 16;
}

static inline volatile void *psInstAddr(fimgContext *ctx, unsigned int slot)
{
	return ctx->base + FGPS_INSTMEM_START + 16*slot;
}

static inline uint32_t psInstLen(fimgContext *ctx, volatile void *vaddr)
{
	return ((volatile char *)vaddr - (ctx->base + FGPS_INSTMEM_START)) / 16;
}

static uint32_t loadShaderBlock(const struct shaderBlock *blk,
						volatile void *vaddr)
{
	uint32_t inst;
	const uint32_t *data = blk->data;
	volatile uint32_t *addr = (volatile uint32_t *)vaddr;

	for (inst = 0; inst < blk->len; inst++) {
#ifdef FIMG_DYNSHADER_DEBUG
		printf("%p: %08x %08x %08x %08x\n", addr,
					data[0], data[1], data[2], data[3]);
#endif
#if 0
		asm ( 	"ldmia %0!, {r0-r3}"
			"stmia %1!, {r0-r3}"
			: "=r"(data), "=r"(addr)
			: "0"(data), "1"(addr)
			: "r0", "r1", "r2", "r3");
#else
		*(addr++) = *(data++);
		*(addr++) = *(data++);
		*(addr++) = *(data++);
		*(addr++) = *(data++);
#endif
	}

	return 4*inst;
}

static inline void setVertexShaderAttribCount(fimgContext *ctx, uint32_t count)
{
	fimgWrite(ctx, count, FGVS_ATTRIB_NUM);
}

static inline void setVertexShaderRange(fimgContext *ctx,
					uint32_t start, uint32_t end)
{
	fimgVShaderPCRange PCRange;
	fimgVShaderConfig Config;

	PCRange.val = 0;
	PCRange.PCStart = start;
	PCRange.PCEnd = end;
	fimgWrite(ctx, PCRange.val, FGVS_PCRANGE);

	Config.val = 0;
	Config.copyPC = 1;
	Config.clrStatus = 1;
	fimgWrite(ctx, Config.val, FGVS_CONFIG);
}

static inline void setPixelShaderState(fimgContext *ctx, int state)
{
	fimgWrite(ctx, !!state, FGPS_EXE_MODE);
}

static inline void setPixelShaderAttribCount(fimgContext *ctx, uint32_t count)
{
	fimgWrite(ctx, count, FGPS_ATTRIB_NUM);

	while (fimgRead(ctx, FGPS_IBSTATUS) & 1);
}

static inline void setPixelShaderRange(fimgContext *ctx,
					uint32_t start, uint32_t end)
{
	fimgWrite(ctx, start, FGPS_PC_START);
	fimgWrite(ctx, end, FGPS_PC_END);
	fimgWrite(ctx, 1, FGPS_PC_COPY);
}

void fimgCompatLoadVertexShader(fimgContext *ctx)
{
	uint32_t unit;
	volatile uint32_t *addr;
	fimgTextureCompat *texture;

	texture = ctx->compat.texture;
	addr = vsInstAddr(ctx, 0);

	addr += loadShaderBlock(&vertexHeader, addr);

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++, texture++) {
		if (!texture->enabled)
			continue;

		addr += loadShaderBlock(&texcoordTransform[unit], addr);
	}

	addr += loadShaderBlock(&vertexFooter, addr);

	ctx->compat.vshaderEnd = vsInstLen(ctx, addr) - 1;

	setVertexShaderRange(ctx, 0, ctx->compat.vshaderEnd);

	loadShaderBlock(&vertexClear, vsInstAddr(ctx, 512 - vertexClear.len));

	addr = (volatile uint32_t *)(ctx->base + FGVS_CFLOAT_START);

	loadShaderBlock(&vertexConstFloat, addr);
}

void fimgCompatLoadPixelShader(fimgContext *ctx)
{
	uint32_t unit, arg;
	volatile uint32_t *addr;
	fimgTextureCompat *texture;

	texture = ctx->compat.texture;
	addr = psInstAddr(ctx, 0);

	addr += loadShaderBlock(&pixelHeader, addr);

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++, texture++) {
		if (!texture->enabled)
			continue;

		addr += loadShaderBlock(&textureUnit[unit], addr);
		addr += loadShaderBlock(&textureFunc[texture->func], addr);

		if (texture->func != FGFP_TEXFUNC_COMBINE)
			continue;

		for (arg = 0; arg < 3; arg++) {
			addr += loadShaderBlock(&combineArg[arg]
					[texture->combc.arg[arg].src], addr);
			addr += loadShaderBlock(&combineArgMod[arg]
					[texture->combc.arg[arg].mod], addr);
		}

		addr += loadShaderBlock(&combineFunc[texture->combc.func],
									addr);
#if 0
		if (texture->combc.func == texture->comba.func) {
			addr += loadShaderBlock(&combine_u, addr);
			continue;
		}
#endif
		if (texture->combc.func == FGFP_COMBFUNC_DOT3_RGBA) {
			addr += loadShaderBlock(&combine_u, addr);
			continue;
		}

		addr += loadShaderBlock(&combine_c, addr);

		for (arg = 0; arg < 3; arg++) {
			addr += loadShaderBlock(&combineArg[arg]
					[texture->comba.arg[arg].src], addr);
			addr += loadShaderBlock(&combineArgMod[arg]
					[texture->comba.arg[arg].mod], addr);
		}

		addr += loadShaderBlock(&combineFunc[texture->comba.func],
									addr);
		addr += loadShaderBlock(&combine_a, addr);
	}

	addr += loadShaderBlock(&pixelFooter, addr);

	ctx->compat.pshaderEnd = psInstLen(ctx, addr) - 1;
	setPixelShaderRange(ctx, 0, ctx->compat.pshaderEnd);

	loadShaderBlock(&pixelClear, psInstAddr(ctx, 512 - pixelClear.len));

	addr = (volatile uint32_t *)(ctx->base + FGPS_CFLOAT_START);

	loadShaderBlock(&pixelConstFloat, addr);
}

void fimgCompatSetTextureEnable(fimgContext *ctx, uint32_t unit, int enable)
{
	if (ctx->compat.texture[unit].enabled == !!enable)
		return;

	ctx->compat.texture[unit].enabled = !!enable;
	ctx->compat.vsDirty = 1;
	ctx->compat.psDirty = 1;
}

void fimgCompatSetTextureFunc(fimgContext *ctx, uint32_t unit, fimgTexFunc func)
{
	if (ctx->compat.texture[unit].func == func)
		return;

	ctx->compat.texture[unit].func = func;

	if (ctx->compat.texture[unit].enabled)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetColorCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func)
{
	if (ctx->compat.texture[unit].combc.func == func)
		return;

	ctx->compat.texture[unit].combc.func = func;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetAlphaCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func)
{
	if (ctx->compat.texture[unit].comba.func == func)
		return;

	ctx->compat.texture[unit].comba.func = func;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetColorCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src)
{
	if (ctx->compat.texture[unit].combc.arg[arg].src == src)
		return;

	ctx->compat.texture[unit].combc.arg[arg].src = src;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetColorCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod mod)
{
	if (ctx->compat.texture[unit].combc.arg[arg].mod == mod)
		return;

	ctx->compat.texture[unit].combc.arg[arg].mod = mod;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetAlphaCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src)
{
	if (ctx->compat.texture[unit].comba.arg[arg].src == src)
		return;

	ctx->compat.texture[unit].comba.arg[arg].src = src;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetAlphaCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod mod)
{
	if (ctx->compat.texture[unit].comba.arg[arg].mod == mod)
		return;

	ctx->compat.texture[unit].comba.arg[arg].mod = mod;

	if (ctx->compat.texture[unit].enabled
	&& ctx->compat.texture[unit].func == FGFP_TEXFUNC_COMBINE)
		ctx->compat.psDirty = 1;
}

void fimgCompatSetColorScale(fimgContext *ctx, uint32_t unit, float scale)
{
	ctx->compat.texture[unit].scale[0] = scale;
	ctx->compat.texture[unit].scale[1] = scale;
	ctx->compat.texture[unit].scale[2] = scale;

	ctx->compat.texture[unit].dirty = 1;
}

void fimgCompatSetAlphaScale(fimgContext *ctx, uint32_t unit, float scale)
{
	ctx->compat.texture[unit].scale[3] = scale;

	ctx->compat.texture[unit].dirty = 1;
}

void fimgCompatSetEnvColor(fimgContext *ctx, uint32_t unit,
					float r, float g, float b, float a)
{
	ctx->compat.texture[unit].env[0] = r;
	ctx->compat.texture[unit].env[1] = g;
	ctx->compat.texture[unit].env[2] = b;
	ctx->compat.texture[unit].env[3] = a;

	ctx->compat.texture[unit].dirty = 1;
}

void fimgCompatSetupTexture(fimgContext *ctx, fimgTexture *tex,
						uint32_t unit, int swap)
{
	ctx->compat.texture[unit].texture = tex;
	ctx->compat.texture[unit].swap = swap;
}

void fimgCreateCompatContext(fimgContext *ctx)
{
	uint32_t unit;
	fimgTextureCompat *texture;

	texture = ctx->compat.texture;

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++, texture++) {
		texture->func = FGFP_TEXFUNC_MODULATE;

		texture->combc.func = FGFP_COMBFUNC_MODULATE;
		texture->combc.arg[0].src = FGFP_COMBARG_TEX;
		texture->combc.arg[0].mod = FGFP_COMBARG_SRC_COLOR;
		texture->combc.arg[1].src = FGFP_COMBARG_PREV;
		texture->combc.arg[1].mod = FGFP_COMBARG_SRC_COLOR;
		texture->combc.arg[2].src = FGFP_COMBARG_CONST;
		texture->combc.arg[2].mod = FGFP_COMBARG_SRC_COLOR;

		texture->combc.func = FGFP_COMBFUNC_MODULATE;
		texture->comba.arg[0].src = FGFP_COMBARG_TEX;
		texture->comba.arg[0].mod = FGFP_COMBARG_SRC_ALPHA;
		texture->comba.arg[1].src = FGFP_COMBARG_PREV;
		texture->comba.arg[1].mod = FGFP_COMBARG_SRC_ALPHA;
		texture->comba.arg[2].src = FGFP_COMBARG_CONST;
		texture->comba.arg[2].mod = FGFP_COMBARG_SRC_ALPHA;

		texture->scale[0] = 1.0;
		texture->scale[1] = 1.0;
		texture->scale[2] = 1.0;
		texture->scale[3] = 1.0;

		texture->swap = 0;
	}

	ctx->compat.vsDirty = 1;
	ctx->compat.psDirty = 1;

	ctx->clear.depth = 1.0;
}

#define FGFP_TEXENV(unit)	(4 + 2*(unit))
#define FGFP_COMBSCALE(unit)	(5 + 2*(unit))

static void setPSConstBool(fimgContext *ctx, int val, uint32_t slot)
{
	uint32_t reg;

	reg = fimgRead(ctx, FGPS_CBOOL_START);
	reg &= ~(!val << slot);
	reg |= !!val << slot;
	fimgWrite(ctx, reg, FGPS_CBOOL_START);
}

static void loadPSConstFloat(fimgContext *ctx, const float *pfData,
								uint32_t slot)
{
	const uint32_t *data = (const uint32_t *)pfData;
	volatile uint32_t *reg = (volatile uint32_t *)(ctx->base
						+ FGPS_CFLOAT_START + 16*slot);
#if 0
	asm ( 	"ldmia %0!, {r0-r3}"
		"stmia %1!, {r0-r3}"
		: "=r"(data), "=r"(reg)
		: "0"(data), "1"(reg)
		: "r0", "r1", "r2", "r3");
#else
	*(reg++) = *(data++);
	*(reg++) = *(data++);
	*(reg++) = *(data++);
	*(reg++) = *(data++);
#endif
}

static void loadVSMatrix(fimgContext *ctx, const float *pfData, uint32_t slot)
{
	uint32_t i;
	const uint32_t *data = (const uint32_t *)pfData;
	volatile uint32_t *reg = (volatile uint32_t *)(ctx->base
						+ FGVS_CFLOAT_START + 16*slot);
	for (i = 0; i < 4; i++) {
#if 0
		asm ( 	"ldmia %0!, {r0-r3}"
			"stmia %1!, {r0-r3}"
			: "=r"(data), "=r"(reg)
			: "0"(data), "1"(reg)
			: "r0", "r1", "r2", "r3");
#else
		*(reg++) = *(data++);
		*(reg++) = *(data++);
		*(reg++) = *(data++);
		*(reg++) = *(data++);
#endif
	}
}

void fimgCompatFlush(fimgContext *ctx)
{
	uint32_t i;

	if (ctx->compat.vsDirty) {
		fimgCompatLoadVertexShader(ctx);
		ctx->compat.vsDirty = 0;
	}
	setVertexShaderAttribCount(ctx, ctx->numAttribs);

	for (i = 0; i < 2 + FIMG_NUM_TEXTURE_UNITS; i++) {
		if (!ctx->compat.matrixDirty[i] || ctx->compat.matrix[i] == NULL)
			continue;

		loadVSMatrix(ctx, ctx->compat.matrix[i], 4*i);
		ctx->compat.matrixDirty[i] = 0;
	}

	setPixelShaderState(ctx, 0);

	if (ctx->compat.psDirty) {
		fimgCompatLoadPixelShader(ctx);
		ctx->compat.psDirty = 0;
	}
	setPixelShaderAttribCount(ctx, 8);

	for (i = 0; i < FIMG_NUM_TEXTURE_UNITS; i++) {
		if (ctx->compat.texture[i].texture == NULL)
			continue;

		if (!ctx->compat.texture[i].enabled)
			continue;

		fimgSetupTexture(ctx, ctx->compat.texture[i].texture, i);
		setPSConstBool(ctx, ctx->compat.texture[i].swap, i);

		if (!ctx->compat.texture[i].dirty)
			continue;

		loadPSConstFloat(ctx, ctx->compat.texture[i].env,
							FGFP_TEXENV(i));
		loadPSConstFloat(ctx, ctx->compat.texture[i].scale,
							FGFP_COMBSCALE(i));

		ctx->compat.texture[i].dirty = 0;
	}

	setPixelShaderState(ctx, 1);
}

void fimgRestoreCompatState(fimgContext *ctx)
{
	uint32_t i;

	for (i = 0; i < 2 + FIMG_NUM_TEXTURE_UNITS; i++)
		ctx->compat.matrixDirty[i] = 1;

	for (i = 0; i < FIMG_NUM_TEXTURE_UNITS; i++)
		ctx->compat.texture[i].dirty = 1;

	ctx->compat.vsDirty = 1;
	ctx->compat.psDirty = 1;

	fimgCompatFlush(ctx);
}

/*
 * Clear
 */

#define FGHI_ATTRIB(i)		(0x8040 + 4*(i))
#define FGHI_ATTRIB_VBCTRL(i)	(0x8080 + 4*(i))
#define FGRA_BFCULL		(0x38014)
#define FGRA_PWIDTH		(0x3801c)
#define FGPF_ALPHAT		(0x70008)
#define FGPF_FRONTST		(0x7000c)
#define FGPF_DEPTHT		(0x70014)
#define FGPF_BLEND		(0x7001c)
#define FGPF_LOGOP		(0x70020)
#define FGPF_CBMSK		(0x70024)

void fimgClear(fimgContext *ctx, uint32_t mode)
{
	fimgArray array;
	fimgAttribute pos;
	fimgVtxBufAttrib vbattr;
	fimgStencilTestData stencil;
	fimgDepthTestData depth;

	// prepare hardware
	fimgGetHardware(ctx);
	fimgFlush(ctx);

	// load clear vertex shader
	setVertexShaderAttribCount(ctx, 1);
	setVertexShaderRange(ctx, 512 - vertexClear.len, 511);

	// load clear pixel shader
	setPixelShaderState(ctx, 0);
	setPixelShaderAttribCount(ctx, 1);
	setPixelShaderRange(ctx, 512 - pixelClear.len, 511);
	loadPSConstFloat(ctx, ctx->clear.color, 255);
	setPixelShaderState(ctx, 1);

	// setup rasterizer
	fimgWrite(ctx, 0, FGRA_BFCULL);
	fimgWriteF(ctx, 2048.0f, FGRA_PWIDTH);

	// setup per-fragment
	fimgWrite(ctx, 0, FGPF_ALPHAT);
	fimgWrite(ctx, 0, FGPF_BLEND);
	fimgWrite(ctx, 0, FGPF_LOGOP);

	// setup stencil test
	stencil.val = 0;
	if (mode & FGFP_CLEAR_STENCIL) {
		stencil.dppass = FGPF_TEST_ACTION_REPLACE;
		stencil.mask = 0xff;
		stencil.ref = ctx->clear.stencil;
		stencil.mode = FGPF_STENCIL_MODE_ALWAYS;
		stencil.enable = 1;
	}
	fimgWrite(ctx, stencil.val, FGPF_FRONTST);

	// setup depth test
	depth.val = 0;
	if (mode & FGFP_CLEAR_DEPTH) {
		depth.mode = FGPF_TEST_MODE_ALWAYS;
		depth.enable = 1;
	}
	fimgWrite(ctx, depth.val, FGPF_DEPTHT);

	if ((mode & FGFP_CLEAR_COLOR) == 0)
		fimgWrite(ctx, 0xF, FGPF_CBMSK);

	// setup context
	fimgSetAttribCount(ctx, 1);
	fimgSetVertexContext(ctx, FGPE_POINTS);

	// setup attribute
	pos.val = 0;
	pos.lastattr = 1;
	pos.dt = FGHI_ATTRIB_DT_FLOAT;
	pos.numcomp = FGHI_NUMCOMP(1);
	pos.srcw = 3;
	pos.srcz = 0;
	pos.srcy = 1;
	pos.srcx = 2;
	fimgWrite(ctx, pos.val, FGHI_ATTRIB(0));

	// setup vtxbuf attrib
	vbattr.val = 0;
	vbattr.range = 1;
	vbattr.stride = 0;
	fimgWrite(ctx, vbattr.val, FGHI_ATTRIB_VBCTRL(0));

	// setup vertex array
	array.pointer = &ctx->clear.depth;
	array.stride = 0;
	array.width = sizeof(float);

	// start drawing
	fimgDrawArraysBufferedAutoinc(ctx, &array, 0, 1);

	// wait for hardware
	fimgFlush(ctx);
	fimgInvalidateFlushCache(ctx, 0, 0, 1, 1);

	// restore rasterizer
	fimgWrite(ctx, ctx->rasterizer.cull.val, FGRA_BFCULL);
	fimgWriteF(ctx, ctx->rasterizer.pointWidth, FGRA_PWIDTH);

	// restore per-fragment
	fimgWrite(ctx, ctx->fragment.alpha.val, FGPF_ALPHAT);
	fimgWrite(ctx, ctx->fragment.stFront.val, FGPF_FRONTST);
	fimgWrite(ctx, ctx->fragment.depth.val, FGPF_DEPTHT);
	fimgWrite(ctx, ctx->fragment.blend.val, FGPF_BLEND);
	fimgWrite(ctx, ctx->fragment.logop.val, FGPF_LOGOP);
	fimgWrite(ctx, ctx->fragment.mask.val, FGPF_CBMSK);

	// restore vertex shader
	setVertexShaderRange(ctx, 0, ctx->compat.vshaderEnd);

	// restore pixel shader
	setPixelShaderState(ctx, 0);
	setPixelShaderRange(ctx, 0, ctx->compat.pshaderEnd);
	setPixelShaderState(ctx, 1);

	// release hardware
	fimgPutHardware(ctx);
}

void fimgSetClearColor(fimgContext *ctx, float red, float green,
						float blue, float alpha)
{
	ctx->clear.color[0] = red;
	ctx->clear.color[1] = green;
	ctx->clear.color[2] = blue;
	ctx->clear.color[3] = alpha;
}

void fimgSetClearDepth(fimgContext *ctx, float depth)
{
	ctx->clear.depth = depth;
}

void fimgSetClearStencil(fimgContext *ctx, uint8_t stencil)
{
	ctx->clear.stencil = stencil;
}