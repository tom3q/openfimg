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
#include <sys/ioctl.h>
#include "fimg_private.h"
#include "shaders/vert.h"
#include "shaders/frag.h"

#define FGFP_TEXENV(unit)	(4 + 2*(unit))
#define FGFP_COMBSCALE(unit)	(5 + 2*(unit))

#define MAX_INSTR		(64)

struct shaderBlock {
	const uint32_t *data;
	uint32_t len;
};

#define SHADER_BLOCK(blk)	{ blk, sizeof(blk) / 16 }

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

static const struct shaderBlock vertexConstFloat = SHADER_BLOCK(vert_cfloat);
static const struct shaderBlock vertexHeader = SHADER_BLOCK(vert_header);
static const struct shaderBlock vertexFooter = SHADER_BLOCK(vert_footer);

static const struct shaderBlock texcoordTransform[] = {
	SHADER_BLOCK(vert_texture0),
	SHADER_BLOCK(vert_texture1)
};

static const struct shaderBlock pixelConstFloat = SHADER_BLOCK(frag_cfloat);
static const struct shaderBlock pixelHeader = SHADER_BLOCK(frag_header);
static const struct shaderBlock pixelFooter = SHADER_BLOCK(frag_footer);

static const struct shaderBlock textureUnit[] = {
	SHADER_BLOCK(frag_texture0),
	SHADER_BLOCK(frag_texture1)
};

static const struct shaderBlock textureFunc[] = {
	{ 0, 0 },
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

#ifndef FIMG_BYPASS_SHADER_OPTIMIZER
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
#endif

/*
 * Utility functions
 */

/**
 * Loads block of shader code into shader program memory.
 * @param blk Shader block descriptor.
 * @param vaddr Address of first instruction slot.
 * @return Loaded instruction count.
 */
static uint32_t loadShaderBlock(const struct shaderBlock *blk, void *vaddr)
{
#ifdef FIMG_DYNSHADER_DEBUG
	uint32_t inst;
	const uint32_t *data = blk->data;
	uint32_t *addr = (uint32_t *)vaddr;

	for (inst = 0; inst < blk->len; inst++, addr += 4, data += 4)
		LOGD("%p: %08x %08x %08x %08x", addr,
					data[0], data[1], data[2], data[3]);
#endif
	memcpy(vaddr, blk->data, 4 * sizeof(uint32_t) * blk->len);

	return 4 * blk->len;
}

/**
 * Loads vector into pixel shader const float slots.
 * @param ctx Hardware context.
 * @param pfData Pointer to vector data.
 * @param slot Number of first slot.
 */
static void loadPSConstFloat(fimgContext *ctx, const float *pfData,
								uint32_t slot)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	int ret;

	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_SHADER_DATA;
	req.data = (void *)pfData;
	req.length = 4 * sizeof(uint32_t);
	req.shader_data.unit = G3D_SHADER_PIXEL;
	req.shader_data.type = G3D_SHADER_DATA_FLOAT;
	req.shader_data.offset = 4 * slot;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_DATA failed (%d)", ret);
}

/**
 * Loads matrix into vertex shader const float slots.
 * @param ctx Hardware context.
 * @param pfData Pointer to matrix data.
 * @param slot Number of first slot.
 */
static void loadVSMatrix(fimgContext *ctx, const float *pfData, uint32_t slot)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	int ret;

	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_SHADER_DATA;
	req.data = (void *)pfData;
	req.length = 16 * sizeof(uint32_t);
	req.shader_data.unit = G3D_SHADER_VERTEX;
	req.shader_data.type = G3D_SHADER_DATA_FLOAT;
	req.shader_data.offset = 4 * slot;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_DATA failed (%d)", ret);
}

/*
 * Shader optimization code
 */

#ifndef FIMG_BYPASS_SHADER_OPTIMIZER
/**
 * Calculates composition of two swizzles.
 * @param a First swizzle.
 * @param b Second swizzle.
 * @return Resulting composition of two swizzles.
 */
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
#endif

/**
 * Performs low level optimization of shader program.
 * @param start Pointer to first instruction of shader program.
 * @param end Pointer to memory after last instruction of shader program.
 * @return Number of instructions in optimized shader program.
 */
static uint32_t optimizeShader(uint32_t *start, uint32_t *end)
{
#ifdef FIMG_BYPASS_SHADER_OPTIMIZER
	return (end - start) / 4;
#else /* FIMG_BYPASS_SHADER_OPTIMIZER */
	struct registerMap map[32];
	unsigned int deps[32];
	int reg;
	fimgShaderInstruction *instrStart = (fimgShaderInstruction *)start;
	fimgShaderInstruction *instrEnd = (fimgShaderInstruction *)end;
	fimgShaderInstruction *instr;
	fimgShaderInstruction *instrPtr;

	/* State initialization */
	memset(deps, 0, sizeof(deps));
	memset(map, 0, sizeof(map));

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
			)
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

			if (instr->src1_regtype == REG_SRC_R
			    && map[instr->src1_regnum].flags & MAP_FLAG_INVALID)
				map[instr->src1_regnum].flags = 0;

			if (instr->src1_regtype == REG_SRC_R
				&& map[instr->src1_regnum].flags
				&& map[instr->src1_regnum].srcRegType != REG_SRC_R
				&& map[instr->src1_regnum].srcRegType == instr->src0_regtype
			)
				map[instr->src1_regnum].flags = 0;

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
			)
				map[instr->src0_regnum].flags = 0;

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
			)
				map[instr->src1_regnum].flags = 0;

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
			)
				map[instr->src2_regnum].flags = 0;

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

			map[instr->dest_regnum].flags = 0;
		}

		depMask = deps[instr->dest_regnum];
		depReg = 0;
		while (depMask) {
			if (depMask & 1)
				map[depReg].flags |= MAP_FLAG_INVALID;
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
#endif /* FIMG_BYPASS_SHADER_OPTIMIZER */
}

/*
 * Shader generation code
 */

/**
 * Calculates pointer to selected slot of selected shader cache buffer.
 * @param buf Shader cache buffer.
 * @param slot Index of program slot.
 * @return Pointer to selected slot.
 */
static inline uint32_t *shaderSlotAddr(uint32_t *buf, uint32_t slot)
{
	return buf
		+ slot*MAX_INSTR*sizeof(fimgShaderInstruction)/sizeof(uint32_t);
}

/**
 * Builds vertex shader program according to current pipeline configuration
 * and stores it in selected slot of vertex shader cache.
 * @param ctx Hardware context.
 * @param slot Slot index.
 */
static void buildVertexShader(fimgContext *ctx, uint32_t slot)
{
	uint32_t unit;
	uint32_t *addr;
	uint32_t *start;

	if (!ctx->compat.vshaderBuf) {
		ctx->compat.vshaderBuf = malloc(VS_CACHE_SIZE * MAX_INSTR * sizeof(fimgShaderInstruction));
		if (!ctx->compat.vshaderBuf) {
			LOGE("Failed to allocate memory for shader buffer, terminating.");
			exit(1);
		}
	}
	start = addr = shaderSlotAddr(ctx->compat.vshaderBuf, slot);

	addr += loadShaderBlock(&vertexHeader, addr);

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++) {
		if (!FGFP_BITFIELD_GET_IDX(ctx->compat.vsState.vs, VS_TEX_EN, unit))
			continue;

		addr += loadShaderBlock(&texcoordTransform[unit], addr);
	}

	addr += loadShaderBlock(&vertexFooter, addr);

	FGFP_BITFIELD_SET(ctx->compat.vsState.vs, VS_INVALID, 0);

	ctx->compat.vertexShaders[slot].instrCount = (addr - start) / 4;
	ctx->compat.vertexShaders[slot].state = ctx->compat.vsState;
}

/**
 * Loads current vertex shader program and parameters into vertex shader memory.
 * @param ctx Hardware context.
 */
static void loadVertexShader(fimgContext *ctx)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	uint32_t slot = ctx->compat.curVsNum;
	struct fimgVertexShaderProgram *vs = &ctx->compat.vertexShaders[slot];
	int ret;
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading optimized shader");
#endif
	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_SHADER_PROGRAM;
	req.data = shaderSlotAddr(ctx->compat.vshaderBuf, slot);
	req.length = 4 * sizeof(uint32_t) * vs->instrCount;
	req.shader_program.unit = G3D_SHADER_VERTEX;
	req.shader_program.num_attrib = ctx->numAttribs;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_PROGRAM failed (%d)", ret);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading const float");
#endif
	req.type = G3D_REQUEST_SHADER_DATA_INIT;
	req.data = (void *)vertexConstFloat.data;
	req.length = 4 * sizeof(uint32_t) * vertexConstFloat.len;
	req.shader_data_init.unit = G3D_SHADER_VERTEX;
	req.shader_data_init.data_count[G3D_SHADER_DATA_FLOAT] =
						4 * vertexConstFloat.len;
	req.shader_data_init.data_count[G3D_SHADER_DATA_INT] = 0;
	req.shader_data_init.data_count[G3D_SHADER_DATA_BOOL] = 0;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_DATA_INIT failed (%d)", ret);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loaded vertex shader");
#endif
}

/**
 * Builds pixel shader program according to current pipeline configuration
 * and stores it in selected slot of pixel shader cache.
 * @param ctx Hardware context.
 * @param slot Slot index.
 */
static void buildPixelShader(fimgContext *ctx, uint32_t slot)
{
	uint32_t unit, arg;
	uint32_t *addr;
	uint32_t *start;
	uint32_t instrCount;
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading pixel shader");
#endif
	if (!ctx->compat.pshaderBuf) {
		ctx->compat.pshaderBuf = malloc(PS_CACHE_SIZE * MAX_INSTR * sizeof(fimgShaderInstruction));
		if (!ctx->compat.pshaderBuf) {
			LOGE("Failed to allocate memory for shader buffer, terminating.");
			exit(1);
		}
	}
	start = addr = shaderSlotAddr(ctx->compat.pshaderBuf, slot);

#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Generating basic shader code");
#endif
	addr += loadShaderBlock(&pixelHeader, addr);

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++) {
		uint32_t reg = ctx->compat.psState.tex[unit];
		if (!FGFP_BITFIELD_GET(reg, TEX_MODE))
			continue;

		addr += loadShaderBlock(&textureUnit[unit], addr);
		if (FGFP_BITFIELD_GET(reg, TEX_SWAP))
			addr += loadShaderBlock(&tex_swap, addr);
		addr += loadShaderBlock(&textureFunc[FGFP_BITFIELD_GET(reg, TEX_MODE)], addr);

		if (FGFP_BITFIELD_GET(reg, TEX_MODE) != FGFP_TEXFUNC_COMBINE)
			continue;

		for (arg = 0; arg < 3; arg++) {
			addr += loadShaderBlock(&combineArg[arg]
					[FGFP_BITFIELD_GET_IDX(reg, TEX_COMBC_SRC, arg)], addr);
			addr += loadShaderBlock(&combineArgMod[arg]
					[FGFP_BITFIELD_GET_IDX(reg, TEX_COMBC_MOD, arg)], addr);
		}

		addr += loadShaderBlock(&combineFunc[FGFP_BITFIELD_GET(reg, TEX_COMBC_FUNC)],
									addr);

		if (FGFP_BITFIELD_GET(reg, TEX_COMBC_FUNC) == FGFP_COMBFUNC_DOT3_RGBA) {
			addr += loadShaderBlock(&combine_u, addr);
			continue;
		}

		addr += loadShaderBlock(&combine_c, addr);

		for (arg = 0; arg < 3; arg++) {
			addr += loadShaderBlock(&combineArg[arg]
					[FGFP_BITFIELD_GET_IDX(reg, TEX_COMBA_SRC, arg)], addr);
			addr += loadShaderBlock(&combineArgMod[arg]
					[FGFP_BITFIELD_GET_IDX(reg, TEX_COMBA_MOD, arg) | 0x2], addr);
		}

		addr += loadShaderBlock(&combineFunc[FGFP_BITFIELD_GET(reg, TEX_COMBA_FUNC)],
									addr);
		addr += loadShaderBlock(&combine_a, addr);
	}

	if (FGFP_BITFIELD_GET(ctx->compat.psState.ps, PS_SWAP))
		addr += loadShaderBlock(&out_swap, addr);

	addr += loadShaderBlock(&pixelFooter, addr);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Optimizing pixel shader");
#endif
	instrCount = optimizeShader(start, addr);

	FGFP_BITFIELD_SET(ctx->compat.psState.ps, PS_INVALID, 0);

	ctx->compat.pixelShaders[slot].instrCount = instrCount;
	ctx->compat.pixelShaders[slot].state = ctx->compat.psState;
}

/**
 * Loads current pixel shader program and parameters into pixel shader memory.
 * @param ctx Hardware context.
 */
static void loadPixelShader(fimgContext *ctx)
{
	struct drm_exynos_g3d_submit submit;
	struct drm_exynos_g3d_request req;
	uint32_t slot = ctx->compat.curPsNum;
	struct fimgPixelShaderProgram *ps = &ctx->compat.pixelShaders[slot];
	int ret;
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading optimized shader");
#endif
	submit.requests = &req;
	submit.nr_requests = 1;

	req.type = G3D_REQUEST_SHADER_PROGRAM;
	req.data = shaderSlotAddr(ctx->compat.pshaderBuf, slot);
	req.length = 4 * sizeof(uint32_t) * ps->instrCount;
	req.shader_program.unit = G3D_SHADER_PIXEL;
	req.shader_program.num_attrib = FIMG_ATTRIB_NUM - 1;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_PROGRAM failed (%d)", ret);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loading const float");
#endif
	req.type = G3D_REQUEST_SHADER_DATA_INIT;
	req.data = (void *)pixelConstFloat.data;
	req.length = 4 * sizeof(uint32_t) * pixelConstFloat.len;
	req.shader_data_init.unit = G3D_SHADER_PIXEL;
	req.shader_data_init.data_count[G3D_SHADER_DATA_FLOAT] =
						4 * pixelConstFloat.len;
	req.shader_data_init.data_count[G3D_SHADER_DATA_INT] = 0;
	req.shader_data_init.data_count[G3D_SHADER_DATA_BOOL] = 0;

	ret = ioctl(ctx->fd, DRM_IOCTL_EXYNOS_G3D_SUBMIT, &submit);
	if (ret < 0)
		LOGE("G3D_REQUEST_SHADER_DATA_INIT failed (%d)", ret);
#ifdef FIMG_DYNSHADER_DEBUG
	LOGD("Loaded pixel shader");
#endif
}

/**
 * Compares two vertex shader configurations.
 * @param ctx Hardware context.
 * @param a First vertex shader configuration.
 * @param b Second vertex shader configuration.
 * @return 0 if equal, non-zero otherwise.
 */
static int compareVertexShaders(fimgContext *ctx,
			fimgVertexShaderState *a, fimgVertexShaderState *b)
{
	return a->val[0] != b->val[0];
}

/**
 * Compares two pixel shader configurations.
 * @param ctx Hardware context.
 * @param a First pixel shader configuration.
 * @param b Second pixel shader configuration.
 * @return 0 if equal, non-zero otherwise.
 */
static int comparePixelShaders(fimgContext *ctx,
			fimgPixelShaderState *a, fimgPixelShaderState *b)
{
	unsigned int i;

	for (i = 0; i < NELEM(a->val); ++i)
		if ((a->val[i] ^ b->val[i]) & ctx->compat.psMask[i])
			return 1;

	return 0;
}

/**
 * Validates vertex shader program and rebuilds it if needed.
 * @param ctx Hardware context.
 */
static void validateVertexShader(fimgContext *ctx)
{
	unsigned int i;
	int ret;
#ifdef FIMG_SHADER_CACHE_STATS
	if (++ctx->compat.vsStatsCounter == 128) {
		LOGD("Vertex shader cache stats:");
		LOGD("Same hits: %d\n", ctx->compat.vsSameHits);
		LOGD("Cache hits: %d\n", ctx->compat.vsCacheHits);
		LOGD("Misses: %d\n", ctx->compat.vsMisses);
		ctx->compat.vsSameHits = 0;
		ctx->compat.vsCacheHits = 0;
		ctx->compat.vsMisses = 0;
		ctx->compat.vsStatsCounter = 0;
	}
#endif
	ret = compareVertexShaders(ctx, &ctx->compat.vsState,
			&ctx->compat.vertexShaders[ctx->compat.curVsNum].state);
	if (!ret) {
#ifdef FIMG_SHADER_CACHE_STATS
		++ctx->compat.vsSameHits;
#endif
		return;
	}

	ctx->compat.vshaderLoaded = 0;

	for (i = 0; i < VS_CACHE_SIZE; ++i) {
		if (i == ctx->compat.curVsNum)
			continue;
		ret = compareVertexShaders(ctx, &ctx->compat.vsState,
					&ctx->compat.vertexShaders[i].state);
		if (!ret) {
#ifdef FIMG_SHADER_CACHE_STATS
			++ctx->compat.vsCacheHits;
#endif
			ctx->compat.curVsNum = i;
			return;
		}
	}
#ifdef FIMG_SHADER_CACHE_STATS
	++ctx->compat.vsMisses;
#endif
	i = ctx->compat.vsEvictCounter++;
	ctx->compat.vsEvictCounter %= VS_CACHE_SIZE;

	buildVertexShader(ctx, i);
	ctx->compat.curVsNum = i;
}

/**
 * Validates pixel shader program and rebuilds it if needed.
 * @param ctx Hardware context.
 */
static void validatePixelShader(fimgContext *ctx)
{
	unsigned int i;
	int ret;
#ifdef FIMG_SHADER_CACHE_STATS
	if (++ctx->compat.psStatsCounter == 128) {
		LOGD("Pixel shader cache stats:");
		LOGD("Same hits: %d\n", ctx->compat.psSameHits);
		LOGD("Cache hits: %d\n", ctx->compat.psCacheHits);
		LOGD("Misses: %d\n", ctx->compat.psMisses);
		ctx->compat.psSameHits = 0;
		ctx->compat.psCacheHits = 0;
		ctx->compat.psMisses = 0;
		ctx->compat.psStatsCounter = 0;
	}
#endif
	ret = comparePixelShaders(ctx, &ctx->compat.psState,
			&ctx->compat.pixelShaders[ctx->compat.curPsNum].state);
	if (!ret) {
#ifdef FIMG_SHADER_CACHE_STATS
		++ctx->compat.psSameHits;
#endif
		return;
	}

	ctx->compat.pshaderLoaded = 0;

	for (i = 0; i < PS_CACHE_SIZE; ++i) {
		if (i == ctx->compat.curPsNum)
			continue;
		ret = comparePixelShaders(ctx, &ctx->compat.psState,
					&ctx->compat.pixelShaders[i].state);
		if (!ret) {
#ifdef FIMG_SHADER_CACHE_STATS
			++ctx->compat.psCacheHits;
#endif
			ctx->compat.curPsNum = i;
			return;
		}
	}
#ifdef FIMG_SHADER_CACHE_STATS
	++ctx->compat.psMisses;
#endif
	i = ctx->compat.psEvictCounter++;
	ctx->compat.psEvictCounter %= PS_CACHE_SIZE;

	buildPixelShader(ctx, i);
	ctx->compat.curPsNum = i;
}

/*
 * Public functions
 */

/**
 * Sets texture function of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param func Texture function.
 */
void fimgCompatSetTextureFunc(fimgContext *ctx, uint32_t unit, fimgTexFunc func)
{
	FGFP_BITFIELD_SET(ctx->compat.psState.tex[unit], TEX_MODE, func);
	FGFP_BITFIELD_SET_IDX(ctx->compat.vsState.vs, VS_TEX_EN,
					unit, func != FGFP_TEXFUNC_NONE);

	switch (func) {
	case FGFP_TEXFUNC_NONE:
		ctx->compat.psMask[unit] = FGFP_TEX_MODE_MASK;
		break;
	case FGFP_TEXFUNC_COMBINE:
		ctx->compat.psMask[unit] = ~0UL;
		break;
	default:
		ctx->compat.psMask[unit] =
				FGFP_TEX_MODE_MASK | FGFP_TEX_SWAP_MASK;
	}
}

/**
 * Sets color combiner function of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param func Color combiner function.
 */
void fimgCompatSetColorCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func)
{
	FGFP_BITFIELD_SET(ctx->compat.psState.tex[unit], TEX_COMBC_FUNC, func);
}

/**
 * Sets alpha combiner function of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param func Alpha combiner function.
 */
void fimgCompatSetAlphaCombiner(fimgContext *ctx, uint32_t unit,
							fimgCombFunc func)
{
	FGFP_BITFIELD_SET(ctx->compat.psState.tex[unit], TEX_COMBA_FUNC, func);
}

/**
 * Sets source of selected color combiner argument of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param arg Index of argument.
 * @param func Combiner argument source.
 */
void fimgCompatSetColorCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src)
{
	FGFP_BITFIELD_SET_IDX(ctx->compat.psState.tex[unit], TEX_COMBC_SRC, arg, src);
}

/**
 * Sets mode of selected color combiner argument of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param arg Index of argument.
 * @param mod Combiner argument mode.
 */
void fimgCompatSetColorCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod mod)
{
	FGFP_BITFIELD_SET_IDX(ctx->compat.psState.tex[unit], TEX_COMBC_MOD, arg, mod);
}

/**
 * Sets source of selected alpha combiner argument of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param arg Index of argument.
 * @param func Combiner argument source.
 */
void fimgCompatSetAlphaCombineArgSrc(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgSrc src)
{
	FGFP_BITFIELD_SET_IDX(ctx->compat.psState.tex[unit], TEX_COMBA_SRC, arg, src);
}

/**
 * Sets mode of selected alpha combiner argument of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param arg Index of argument.
 * @param mod Combiner argument source.
 */
void fimgCompatSetAlphaCombineArgMod(fimgContext *ctx, uint32_t unit,
					uint32_t arg, fimgCombArgMod mod)
{
	FGFP_BITFIELD_SET_IDX(ctx->compat.psState.tex[unit], TEX_COMBA_MOD, arg, mod & 1);
}

/**
 * Sets color scale factor of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param scale Color scale factor.
 */
void fimgCompatSetColorScale(fimgContext *ctx, uint32_t unit, float scale)
{
	ctx->compat.texture[unit].scale[0] = scale;
	ctx->compat.texture[unit].scale[1] = scale;
	ctx->compat.texture[unit].scale[2] = scale;

	ctx->compat.texture[unit].dirty = 1;
}

/**
 * Sets alpha scale factor of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param scale Alpha scale factor.
 */
void fimgCompatSetAlphaScale(fimgContext *ctx, uint32_t unit, float scale)
{
	ctx->compat.texture[unit].scale[3] = scale;

	ctx->compat.texture[unit].dirty = 1;
}

/**
 * Sets environment color of selected texture unit.
 * @param ctx Hardware context.
 * @param unit Index of texture unit.
 * @param r Red component of environment color.
 * @param g Green component of environment color.
 * @param b Blue component of environment color.
 * @param a Alpha component of environment color.
 */
void fimgCompatSetEnvColor(fimgContext *ctx, uint32_t unit,
					float r, float g, float b, float a)
{
	ctx->compat.texture[unit].env[0] = r;
	ctx->compat.texture[unit].env[1] = g;
	ctx->compat.texture[unit].env[2] = b;
	ctx->compat.texture[unit].env[3] = a;

	ctx->compat.texture[unit].dirty = 1;
}

/**
 * Binds pointed texture object to selected texture unit.
 * @param ctx Hardware context.
 * @param tex Texture object.
 * @param unit Index of texture unit.
 */
void fimgCompatSetupTexture(fimgContext *ctx, fimgTexture *tex,
					uint32_t unit, int dirty)
{
	ctx->compat.texture[unit].texture = tex;
	if (tex) {
		FGFP_BITFIELD_SET(ctx->compat.psState.tex[unit],
				TEX_SWAP, !!(tex->flags & FGTU_TEX_BGR));
		if (dirty)
			tex->flags |= G3D_TEXTURE_DIRTY;
	}
}

/**
 * Initializes hardware context of fixed pipeline emulation block.
 * @param ctx Hardware context.
 */
void fimgCreateCompatContext(fimgContext *ctx)
{
	uint32_t unit;
	fimgTextureCompat *texture;

	texture = ctx->compat.texture;

	for (unit = 0; unit < FIMG_NUM_TEXTURE_UNITS; unit++, texture++) {
		uint32_t reg = 0;

		FGFP_BITFIELD_SET(reg, TEX_COMBC_FUNC, FGFP_COMBFUNC_MODULATE);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_SRC, 0, FGFP_COMBARG_TEX);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_MOD, 0, FGFP_COMBARG_SRC_COLOR);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_SRC, 1, FGFP_COMBARG_PREV);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_MOD, 1, FGFP_COMBARG_SRC_COLOR);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_SRC, 2, FGFP_COMBARG_CONST);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBC_MOD, 2, FGFP_COMBARG_SRC_ALPHA);

		FGFP_BITFIELD_SET(reg, TEX_COMBA_FUNC, FGFP_COMBFUNC_MODULATE);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_SRC, 0, FGFP_COMBARG_TEX);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_MOD, 0, FGFP_COMBARG_SRC_ALPHA & 1);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_SRC, 1, FGFP_COMBARG_PREV);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_MOD, 1, FGFP_COMBARG_SRC_ALPHA & 1);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_SRC, 2, FGFP_COMBARG_CONST);
		FGFP_BITFIELD_SET_IDX(reg, TEX_COMBA_MOD, 2, FGFP_COMBARG_SRC_ALPHA & 1);

		ctx->compat.texture[unit].scale[0] = 1.0;
		ctx->compat.texture[unit].scale[1] = 1.0;
		ctx->compat.texture[unit].scale[2] = 1.0;
		ctx->compat.texture[unit].scale[3] = 1.0;

		ctx->compat.psState.tex[unit] = reg;
	}

	for (unit = 0; unit < VS_CACHE_SIZE; ++unit)
		FGFP_BITFIELD_SET(ctx->compat.vertexShaders[unit].state.vs,
								VS_INVALID, 1);

	for (unit = 0; unit < PS_CACHE_SIZE; ++unit)
		FGFP_BITFIELD_SET(ctx->compat.pixelShaders[unit].state.ps,
								PS_INVALID, 1);

	ctx->compat.psMask[FIMG_NUM_TEXTURE_UNITS] = 0xffffffff;
}

/**
 * Validates fixed pipeline emulation setup and rebuilds it if needed.
 * @param ctx Hardware context.
 */
void fimgCompatFlush(fimgContext *ctx)
{
	uint32_t i;

	validateVertexShader(ctx);
	if (!ctx->compat.vshaderLoaded) {
		loadVertexShader(ctx);
		ctx->compat.vshaderLoaded = 1;
	}

	for (i = 0; i < 2 + FIMG_NUM_TEXTURE_UNITS; i++) {
		if (!ctx->compat.matrixDirty[i] || ctx->compat.matrix[i] == NULL)
			continue;

		loadVSMatrix(ctx, ctx->compat.matrix[i], 4*i);
		ctx->compat.matrixDirty[i] = 0;
	}

	validatePixelShader(ctx);
	if (!ctx->compat.pshaderLoaded) {
		loadPixelShader(ctx);
		ctx->compat.pshaderLoaded = 1;
	}

	for (i = 0; i < FIMG_NUM_TEXTURE_UNITS; i++) {
		if (ctx->compat.texture[i].texture == NULL)
			continue;

		if (!FGFP_BITFIELD_GET(ctx->compat.psState.tex[i], TEX_MODE))
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
}

/**
 * Sets data pointer of selected matrix and marks it to be reloaded.
 * @param ctx Hardware context.
 * @param matrix Which matrix to load (FGL_MATRIX_*).
 * @param pData Pointer to matrix elements in column-major ordering.
 */
void fimgLoadMatrix(fimgContext *ctx, uint32_t matrix, const float *pfData)
{
	ctx->compat.matrix[matrix] = pfData;
	ctx->compat.matrixDirty[matrix] = 1;
}

/**
 * Restores fixed pipeline compatibility block context.
 * @param ctx Hardware context.
 */
void fimgRestoreCompatState(fimgContext *ctx)
{
	uint32_t i;

	for (i = 0; i < 2 + FIMG_NUM_TEXTURE_UNITS; i++)
		ctx->compat.matrixDirty[i] = 1;

	for (i = 0; i < FIMG_NUM_TEXTURE_UNITS; i++)
		ctx->compat.texture[i].dirty = 1;

	ctx->compat.vshaderLoaded = 0;
	ctx->compat.pshaderLoaded = 0;
}
