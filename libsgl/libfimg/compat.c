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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

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
static const struct shaderBlock tex_swap = SHADER_BLOCK(frag_tex_swap);
static const struct shaderBlock out_swap = SHADER_BLOCK(frag_out_swap);

/* Shader functions */

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
		LOGD("%p: %08x %08x %08x %08x", addr,
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

/*
 * Shader optimization code
 */

typedef struct __attribute__ ((__packed__)) _fimgShaderInstruction {
	struct {
		unsigned src2_regnum	:5;
		unsigned		:3;
		unsigned src2_regtype	:3;
		unsigned src2_ar	:1;
		unsigned		:2;
		unsigned src2_modifier	:2;
		unsigned src2_swizzle	:8;
		unsigned src1_regnum	:5;
		unsigned src1_pch	:2;
		unsigned src1_pa	:1;
	};
	struct {
		unsigned src1_regtype	:3;
		unsigned src1_ar	:1;
		unsigned src1_pn	:1;
		unsigned src1_p		:1;
		unsigned src1_modifier	:2;
		unsigned src1_swizzle	:8;
		unsigned src0_regnum	:5;
		unsigned src0_extnum	:3;
		unsigned src0_regtype	:3;
		unsigned src0_ar	:3;
		unsigned src0_modifier	:2;
	};
	union {
		struct {
			unsigned src0_swizzle	:8;
			unsigned dest_regnum	:5;
			unsigned dest_regtype	:3;
			unsigned dest_a		:1;
			unsigned dest_modifier	:2;
			unsigned dest_mask	:4;
			unsigned opcode		:6;
			unsigned next_3src	:1;
			unsigned		:2;
		};
		struct {
			unsigned 		:8;
			unsigned branch_offs	:8;
			unsigned branch_dir	:1;
			unsigned		:15;
		};
	};
	struct {
		uint32_t reserved;
	};
} fimgShaderInstruction;

typedef struct opcodeInfo {
	uint8_t type;
	uint8_t srcCount;
} fimgOpcodeInfo;

enum fimgOpcode {
	OP_NOP = 0,
	OP_MOV,
	OP_MOVA,
	OP_MOVC,
	OP_ADD,
	OP_MUL,
	OP_MUL_LIT,
	OP_DP3,
	OP_DP4,
	OP_DPH,
	OP_DST,
	OP_EXP,
	OP_EXP_LIT,
	OP_LOG,
	OP_LOG_LIT,
	OP_RCP,
	OP_RSQ,
	OP_DP2ADD,
	OP_RSVD_13,
	OP_MAX,
	OP_MIN,
	OP_SGE,
	OP_SLT,
	OP_SETP_EQ,
	OP_SETP_GE,
	OP_SETP_GT,
	OP_SETP_NE,
	OP_CMP,
	OP_MAD,
	OP_FRC,
	OP_RSVD_1F,
	OP_TEXLD,
	OP_CUBEDIR,
	OP_MAXCOMP,
	OP_TEXLDC,
	OP_RSVD_24,
	OP_RSVD_25,
	OP_RSVD_26,
	OP_TEXKILL,
	OP_MOVIPS,
	OP_ADDI,
	OP_B,
	OP_BF,
	OP_RSVD_32,
	OP_RSVD_33,
	OP_BP,
	OP_BFP,
	OP_BZP,
	OP_RSVD_37,
	OP_CALL,
	OP_CALLNZ,
	OP_RSVD_3A,
	OP_RSVD_3B,
	OP_RET
};

enum fimgOpcodeType {
	OP_TYPE_RESERVED = 0,
	OP_TYPE_FLOW,
	OP_TYPE_NORMAL,
	OP_TYPE_MOVE
};

static fimgOpcodeInfo opcodeMap[64] = {
	[OP_NOP] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_MOV] = {
		.type		= OP_TYPE_MOVE,
		.srcCount	= 1,
	},
	[OP_MOVA] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_MOVC] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_ADD] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_MUL] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_MUL_LIT] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_DP3] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_DP4] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_DPH] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_DST] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_EXP] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_EXP_LIT] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_LOG] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_LOG_LIT] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_RCP] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_RSQ] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_DP2ADD] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 3,
	},
	[OP_RSVD_13] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_MAX] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_MIN] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SGE] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SLT] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SETP_EQ] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SETP_GE] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SETP_GT] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_SETP_NE] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_CMP] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 3,
	},
	[OP_MAD] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 3,
	},
	[OP_FRC] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_RSVD_1F] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_TEXLD] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_CUBEDIR] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_MAXCOMP] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_TEXLDC] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 3,
	},
	[OP_RSVD_24] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_RSVD_25] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_RSVD_26] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_TEXKILL] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_MOVIPS] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 1,
	},
	[OP_ADDI] = {
		.type		= OP_TYPE_NORMAL,
		.srcCount	= 2,
	},
	[OP_B] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 0,
	},
	[OP_BF] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 1,
	},
	[OP_RSVD_32] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_RSVD_33] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_BP] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 0,
	},
	[OP_BFP] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 1,
	},
	[OP_BZP] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 1,
	},
	[OP_RSVD_37] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_CALL] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 0,
	},
	[OP_CALLNZ] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 1,
	},
	[OP_RSVD_3A] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_RSVD_3B] = {
		.type		= OP_TYPE_RESERVED,
		.srcCount	= 0,
	},
	[OP_RET] = {
		.type		= OP_TYPE_FLOW,
		.srcCount	= 0,
	}
};

enum fimgSrcRegType {
	REG_SRC_V = 0,
	REG_SRC_R,
	REG_SRC_C,
	REG_SRC_I,
	REG_SRC_AL,
	REG_SRC_B,
	REG_SRC_P,
	REG_SRC_S,
	REG_SRC_D,
	REG_SRC_VFACE,
	REG_SRC_VPOS
};

enum fimgDstRegType {
	REG_DST_O = 0,
	REG_DST_R,
	REG_DST_P,
	REG_DST_A0,
	REG_DST_AL
};

struct registerMap {
	union {
		struct {
			unsigned srcRegNum	:5;
			unsigned srcRegType	:3;
		};
		uint8_t srcReg;
	};
	uint8_t srcSwizzle;
	uint8_t movInstr;
	uint8_t flags;
};

#define MAP_FLAG_USED		(1 << 0)
#define MAP_FLAG_INVALID	(1 << 1)

#define SWIZZLE(a, b, c, d)	((a) | ((b) << 2) | ((c) << 4) | ((d) << 6))

static inline uint8_t mergeSwizzle(uint8_t a, uint8_t b)
{
	uint8_t swizzle = 0;

	swizzle |= ((a >> 2*(b & 3)) & 3) << 6;
	swizzle >>= 2;
	b >>= 2;
	swizzle |= ((a >> 2*(b & 3)) & 3) << 6;
	swizzle >>= 2;
	b >>= 2;
	swizzle |= ((a >> 2*(b & 3)) & 3) << 6;
	swizzle >>= 2;
	b >>= 2;
	swizzle |= ((a >> 2*(b & 3)) & 3) << 6;

	return swizzle;
}

static uint32_t optimizeShader(uint32_t *start, uint32_t *end)
{
	struct registerMap map[32];
	unsigned int deps[32];
	int reg;
	fimgShaderInstruction *instrStart = (fimgShaderInstruction *)start;
	fimgShaderInstruction *instrEnd = (fimgShaderInstruction *)end;
	fimgShaderInstruction *instr;
	fimgShaderInstruction *instrPtr;

	/* State initialization */
	memset(deps, 0, sizeof(deps));
	for (reg = 0; reg < 32; ++reg) {
		map[reg].srcReg = reg;
		map[reg].srcRegType = 1;
		map[reg].srcSwizzle = SWIZZLE(0, 1, 2, 3);
		map[reg].movInstr = 0;
		map[reg].flags = 0;
	}

	/* Optimization pass */
	for (instr = instrStart; instr < instrEnd; ++instr) {
		fimgOpcodeInfo *info = &opcodeMap[instr->opcode];
		uint32_t depMask;
		uint32_t depReg;

		switch (info->srcCount) {
		case 1:
			if (instr->src0_regtype == REG_SRC_R
			    && map[instr->src0_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src0_regnum].flags = 0;

			if (instr->src0_regtype == REG_SRC_R
			    && map[instr->src0_regnum].flags
			) {
				instr->src0_swizzle =
					mergeSwizzle(map[instr->src0_regnum].srcSwizzle,
								instr->src0_swizzle);
				instr->src0_regtype = map[instr->src0_regnum].srcRegType;
				instr->src0_regnum = map[instr->src0_regnum].srcRegNum;
			}

			break;
		case 2:
			if (instr->src0_regtype == REG_SRC_R
			    && map[instr->src0_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src0_regnum].flags = 0;

			if (instr->src0_regtype == REG_SRC_R
				&& map[instr->src0_regnum].flags
				&& map[instr->src0_regnum].srcRegType != REG_SRC_R
				&& map[instr->src0_regnum].srcRegType == instr->src1_regtype
			) {
				if (map[instr->src0_regnum].srcRegType == REG_SRC_R)
					deps[map[instr->src0_regnum].srcRegNum] &= ~(1 << instr->src0_regnum);

				map[instr->src0_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[instr->src0_regnum].srcRegType = REG_SRC_R;
				map[instr->src0_regnum].srcRegNum = instr->src0_regnum;
				map[instr->src0_regnum].flags = 0;
			}

			if (instr->src0_regtype == REG_SRC_R
				&& map[instr->src0_regnum].flags
			) {
				instr->src0_swizzle =
					mergeSwizzle(map[instr->src0_regnum].srcSwizzle,
								instr->src0_swizzle);
				instr->src0_regtype = map[instr->src0_regnum].srcRegType;
				instr->src0_regnum = map[instr->src0_regnum].srcRegNum;
			}

			if (instr->src1_regtype == REG_SRC_R
			    && map[instr->src1_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src1_regnum].flags = 0;

			if (instr->src1_regtype == REG_SRC_R
				&& map[instr->src1_regnum].flags
				&& map[instr->src1_regnum].srcRegType != REG_SRC_R
				&& map[instr->src1_regnum].srcRegType == instr->src0_regtype
			) {
				if (map[instr->src1_regnum].srcRegType == REG_SRC_R)
					deps[map[instr->src1_regnum].srcRegNum] &= ~(1 << instr->src1_regnum);

				map[instr->src1_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[instr->src1_regnum].srcRegType = REG_SRC_R;
				map[instr->src1_regnum].srcRegNum = instr->src1_regnum;
				map[instr->src1_regnum].flags = 0;
			}

			if (instr->src1_regtype == REG_SRC_R
				&& map[instr->src1_regnum].flags
			) {
				instr->src1_swizzle =
					mergeSwizzle(map[instr->src1_regnum].srcSwizzle,
								instr->src1_swizzle);
				instr->src1_regtype = map[instr->src1_regnum].srcRegType;
				instr->src1_regnum = map[instr->src1_regnum].srcRegNum;
			}

			break;
		case 3:
			if (instr->src0_regtype == REG_SRC_R
			    && map[instr->src0_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src0_regnum].flags = 0;

			if (instr->src0_regtype == REG_SRC_R
				&& map[instr->src0_regnum].flags
				&& map[instr->src0_regnum].srcRegType != REG_SRC_R
				&& (map[instr->src0_regnum].srcRegType == instr->src1_regtype
				|| map[instr->src0_regnum].srcRegType == instr->src2_regtype)
			) {
				if (map[instr->src0_regnum].srcRegType == REG_SRC_R)
					deps[map[instr->src0_regnum].srcRegNum] &= ~(1 << instr->src0_regnum);

				map[instr->src0_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[instr->src0_regnum].srcRegType = REG_SRC_R;
				map[instr->src0_regnum].srcRegNum = instr->src0_regnum;
				map[instr->src0_regnum].flags = 0;
			}

			if (instr->src0_regtype == REG_SRC_R
			    && map[instr->src0_regnum].flags) {
				instr->src0_swizzle =
					mergeSwizzle(map[instr->src0_regnum].srcSwizzle,
								instr->src0_swizzle);
				instr->src0_regtype = map[instr->src0_regnum].srcRegType;
				instr->src0_regnum = map[instr->src0_regnum].srcRegNum;
			}

			if (instr->src1_regtype == REG_SRC_R
			    && map[instr->src1_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src1_regnum].flags = 0;

			if (instr->src1_regtype == REG_SRC_R
				&& map[instr->src1_regnum].flags
				&& map[instr->src1_regnum].srcRegType != REG_SRC_R
				&& (map[instr->src1_regnum].srcRegType == instr->src0_regtype
				|| map[instr->src1_regnum].srcRegType == instr->src2_regtype)
			) {
				if (map[instr->src1_regnum].srcRegType == REG_SRC_R)
					deps[map[instr->src1_regnum].srcRegNum] &= ~(1 << instr->src1_regnum);

				map[instr->src1_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[instr->src1_regnum].srcRegType = REG_SRC_R;
				map[instr->src1_regnum].srcRegNum = instr->src1_regnum;
				map[instr->src1_regnum].flags = 0;
			}

			if (instr->src1_regtype == REG_SRC_R
			    && map[instr->src1_regnum].flags) {
				instr->src1_swizzle =
					mergeSwizzle(map[instr->src1_regnum].srcSwizzle,
								instr->src1_swizzle);
				instr->src1_regtype = map[instr->src1_regnum].srcRegType;
				instr->src1_regnum = map[instr->src1_regnum].srcRegNum;
			}

			if (instr->src2_regtype == REG_SRC_R
			    && map[instr->src2_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src2_regnum].flags = 0;

			if (instr->src2_regtype == REG_SRC_R
				&& map[instr->src2_regnum].flags
				&& map[instr->src2_regnum].srcRegType != REG_SRC_R
				&& (map[instr->src2_regnum].srcRegType == instr->src0_regtype
				|| map[instr->src2_regnum].srcRegType == instr->src1_regtype)
			) {
				if (map[instr->src2_regnum].srcRegType == REG_SRC_R)
					deps[map[instr->src2_regnum].srcRegNum] &= ~(1 << instr->src2_regnum);

				map[instr->src2_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[instr->src2_regnum].srcRegType = REG_SRC_R;
				map[instr->src2_regnum].srcRegNum = instr->src2_regnum;
				map[instr->src2_regnum].flags = 0;
			}

			if (instr->src2_regtype == REG_SRC_R
			    && map[instr->src2_regnum].flags) {
				instr->src2_swizzle =
					mergeSwizzle(map[instr->src2_regnum].srcSwizzle,
								instr->src2_swizzle);
				instr->src2_regtype = map[instr->src2_regnum].srcRegType;
				instr->src2_regnum = map[instr->src2_regnum].srcRegNum;
			}
			break;
		default:
			break;
		}

		if (info->type <= OP_TYPE_FLOW || instr->dest_regtype != REG_DST_R)
			continue;

		if (map[instr->dest_regnum].flags & MAP_FLAG_USED) {
			fimgShaderInstruction *mov =
				instrStart + map[instr->dest_regnum].movInstr;

			/* Instructions with non-zero here will be removed */
			mov->reserved = 0xdeadc0de;

			if (map[instr->dest_regnum].srcRegType == REG_SRC_R)
				deps[map[instr->dest_regnum].srcRegNum] &= ~(1 << instr->dest_regnum);

			map[instr->dest_regnum].srcSwizzle = SWIZZLE(0, 1, 2, 3);
			map[instr->dest_regnum].srcRegType = REG_SRC_R;
			map[instr->dest_regnum].srcRegNum = instr->dest_regnum;
			map[instr->dest_regnum].flags = 0;
		}

		depMask = deps[instr->dest_regnum];
		depReg = 0;
		while (depMask) {
			if (depMask & 1) {
				map[depReg].srcSwizzle = SWIZZLE(0, 1, 2, 3);
				map[depReg].srcRegType = REG_SRC_R;
				map[depReg].srcRegNum = depReg;
				map[depReg].flags |= MAP_FLAG_INVALID;
			}
			depMask >>= 1;
			++depReg;
		}
		deps[instr->dest_regnum] = 0;

		if (info->type != OP_TYPE_MOVE)
			continue;

		if (instr->dest_mask != 0xf || instr->dest_modifier
		    || instr->src0_modifier)
			continue;

		map[instr->dest_regnum].srcRegNum = instr->src0_regnum;
		map[instr->dest_regnum].srcRegType = instr->src0_regtype;
		map[instr->dest_regnum].srcSwizzle = instr->src0_swizzle;
		map[instr->dest_regnum].movInstr = instr - instrStart;
		map[instr->dest_regnum].flags = MAP_FLAG_USED;

		if (instr->src0_regtype == REG_SRC_R)
			deps[instr->src0_regnum] |= (1 << instr->dest_regnum);
	}

	/* Mark all remaining mapped mov instructions to be removed */
	for (reg = 0; reg < 32; ++reg) {
		if (!map[reg].flags)
			continue;
		fimgShaderInstruction *mov = instrStart + map[reg].movInstr;
		mov->reserved = 0xdeadc0de;
	}

	/* Done, remove unused instructions and return */
	instrPtr = instrStart;
	for (instr = instrStart; instr < instrEnd; ++instr) {
		if (instr->reserved == 0xdeadc0de)
			continue;
		if (opcodeMap[instr->opcode].srcCount == 3)
			(instr - 1)->next_3src = 1;
		*(instrPtr++) = *instr;
	}

	return instrPtr - instrStart;
}

/*
 * Shader generation code
 */

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

	addr = (volatile uint32_t *)(ctx->base + FGVS_CFLOAT_START);

	loadShaderBlock(&vertexConstFloat, addr);
}

void fimgCompatLoadPixelShader(fimgContext *ctx)
{
	uint32_t unit, arg;
	uint32_t *addr;
	fimgTextureCompat *texture;
	uint32_t instrCount;
	volatile uint32_t *reg;
	struct shaderBlock blk;
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading pixel shader");
#endif
	texture = ctx->compat.texture;

	if (!ctx->compat.shaderBuf) {
		addr = malloc(64 * sizeof(fimgShaderInstruction));
		if (!addr) {
			LOGE("Failed to allocate memory for shader buffer, terminating.");
			exit(1);
		}
		ctx->compat.shaderBuf = addr;
	}

	addr = ctx->compat.shaderBuf;
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Generating basic shader code");
#endif
	addr += loadShaderBlock(&pixelHeader, addr);

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++, texture++) {
		if (!texture->enabled)
			continue;

		addr += loadShaderBlock(&textureUnit[unit], addr);
		if (texture->swap)
			addr += loadShaderBlock(&tex_swap, addr);
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

	if (ctx->fbFlags & FGPF_COLOR_MODE_BGR)
		addr += loadShaderBlock(&out_swap, addr);

	addr += loadShaderBlock(&pixelFooter, addr);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Optimizing pixel shader");
#endif
	instrCount = optimizeShader(ctx->compat.shaderBuf, addr);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading optimized shader");
#endif
	reg = psInstAddr(ctx, 0);
	blk.data = ctx->compat.shaderBuf;
	blk.len = instrCount;
	loadShaderBlock(&blk, reg);

	ctx->compat.pshaderEnd = instrCount - 1;
	setPixelShaderRange(ctx, 0, ctx->compat.pshaderEnd);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading const float");
#endif
	reg = (volatile uint32_t *)(ctx->base + FGPS_CFLOAT_START);
	loadShaderBlock(&pixelConstFloat, reg);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loaded pixel shader");
#endif
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

void fimgCompatSetupTexture(fimgContext *ctx, fimgTexture *tex, uint32_t unit)
{
	ctx->compat.texture[unit].texture = tex;
	if (tex) {
		if (ctx->compat.texture[unit].swap != !!(tex->reserved2 & FGTU_TEX_BGR))
			ctx->compat.psDirty = 1;
		ctx->compat.texture[unit].swap =
					!!(tex->reserved2 & FGTU_TEX_BGR);
	}
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
		texture->combc.arg[2].mod = FGFP_COMBARG_SRC_ALPHA;

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
	setPixelShaderAttribCount(ctx, FIMG_ATTRIB_NUM - 1);

	for (i = 0; i < FIMG_NUM_TEXTURE_UNITS; i++) {
		if (ctx->compat.texture[i].texture == NULL)
			continue;

		if (!ctx->compat.texture[i].enabled)
			continue;

		fimgSetupTexture(ctx, ctx->compat.texture[i].texture, i);

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
}
