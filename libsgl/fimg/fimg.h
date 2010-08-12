/*
 * fimg/fimg.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _FIMG_H_
#define _FIMG_H_

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

extern volatile void *fimgBase;

#define _FIMG3DSE_VER_1_2       1
#define _FIMG3DSE_VER_1_2_1     2
#define _FIMG3DSE_VER_2_0	4

#define TARGET_FIMG_VERSION     _FIMG3DSE_VER_1_2_1

struct _fimgContext;
typedef struct _fimgContext fimgContext;

/*
 * Global block
 */

/* Type definitions */
typedef union {
	unsigned int val;
	struct {
		unsigned		:13;
		unsigned ccache0	:1;
		unsigned		:1;
		unsigned pf0		:1;
		unsigned		:3;
		unsigned ps0		:1;
		unsigned		:1;
		unsigned ra		:1;
		unsigned tse		:1;
		unsigned pe		:1;
		unsigned		:3;
		unsigned vs		:1;
		unsigned vc		:1;
		unsigned hvf		:1;
		unsigned hi		:1;
		unsigned host_fifo	:1;
	} bits;
} fimgPipelineStatus;

typedef union {
	unsigned int val;
	struct {
		unsigned major		:8;
		unsigned minor		:8;
		unsigned revision	:8;
		unsigned		:8;
	} bits;
} fimgVersion;

/* Functions */
fimgPipelineStatus fimgGetPipelineStatus(void);
int fimgFlush(fimgPipelineStatus pipelineFlags);
int fimgClearCache(unsigned int clearFlags);
void fimgSoftReset(void);
fimgVersion fimgGetVersion(void);
unsigned int fimgGetInterrupt(void);
void fimgClearInterrupt(void);
void fimgEnableInterrupt(void);
void fimgDisableInterrupt(void);
void fimgSetInterruptBlock(fimgPipelineStatus pipeMask);
void fimgSetInterruptState(fimgPipelineStatus status);
fimgPipelineStatus fimgGetInterruptState(void);

/*
 * Host interface
 */

#define FIMG_ATTRIB_NUM		10

/* Type definitions */
typedef union {
	unsigned int val;
	struct {
		unsigned envb		:1;
		unsigned		:5;
		unsigned idxtype	:2;
		unsigned		:7;
		unsigned autoinc	:1;
		unsigned		:11;
		unsigned envc		:1;
		unsigned numoutattrib	:4;
	} bits;
} fimgHInterface;

typedef struct {
	union {
		unsigned int val;
		struct {
			unsigned stride		:8;
			unsigned		:8;
			unsigned range		:16;
		} bits;
	} ctrl;
	union {
		unsigned int val;
		struct {
			unsigned		:16;
			unsigned addr		:16;
		} bits;
	} base;
} fimgVtxBufAttrib;

typedef union {
	struct {
		unsigned lastattr	:1;
		unsigned		:15;
		unsigned dt		:4;
		unsigned		:2;
		unsigned numcomp	:2;
		unsigned srcw		:2;
		unsigned srcz		:2;
		unsigned srcy		:2;
		unsigned srcx		:2;
	} bits;
	unsigned int val;
} fimgAttribute;

#define FGHI_NUMCOMP(i)		((i) - 1)

typedef enum {
	FGHI_ATTRIB_DT_BYTE = 0,
	FGHI_ATTRIB_DT_SHORT,
	FGHI_ATTRIB_DT_INT,
	FGHI_ATTRIB_DT_FIXED,
	FGHI_ATTRIB_DT_UBYTE,
	FGHI_ATTRIB_DT_USHORT,
	FGHI_ATTRIB_DT_UINT,
	FGHI_ATTRIB_DT_FLOAT,
	FGHI_ATTRIB_DT_NBYTE,
	FGHI_ATTRIB_DT_NSHORT,
	FGHI_ATTRIB_DT_NINT,
	FGHI_ATTRIB_DT_NFIXED,
	FGHI_ATTRIB_DT_NUBYTE,
	FGHI_ATTRIB_DT_NUSHORT,
	FGHI_ATTRIB_DT_NUINT,
	FGHI_ATTRIB_DT_HALF_FLOAT
} fimgHostDataType;

/* Functions */
unsigned int fimgGetNumEmptyFIFOSlots(void);
void fimgSendToFIFO(unsigned int bytes, void *buffer);
void fimgDrawNonIndexArrays(fimgContext *ctx,
			    unsigned int numVertices,
			    const void **ppvData,
			    unsigned int *pStride);
void fimgDrawArraysUByteIndex(fimgContext *ctx,
			      unsigned int numVertices,
			      const void **ppvData,
			      unsigned int *pStride,
			      const unsigned char *idx);
void fimgDrawArraysUShortIndex(fimgContext *ctx,
			       unsigned int numVertices,
			       const void **ppvData,
			       unsigned int *pStride,
			       const unsigned short *idx);
void fimgSetHInterface(fimgHInterface HI);
void fimgSetIndexOffset(unsigned int offset);
void fimgSetVtxBufferAddr(unsigned int addr);
void fimgSendToVtxBuffer(unsigned int data);
void fimgSetAttribute(fimgContext *ctx,
		      unsigned int idx,
		      unsigned int type,
		      unsigned int numComp);
void fimgSetVtxBufAttrib(unsigned char attribIdx,
			 fimgVtxBufAttrib AttribInfo);

/*
 * Primitive Engine
 */

/* Type definitions */
typedef enum {
	FGPE_TRIANGLES		= (1 << 7),
	FGPE_TRIANGLE_FAN	= (1 << 6),
	FGPE_TRIANGLE_STRIP	= (1 << 5),
	FGPE_LINES		= (1 << 4),
	FGPE_LINE_LOOP		= (1 << 3),
	FGPE_LINE_STRIP		= (1 << 2),
	FGPE_POINTS		= (1 << 1),
	FGPE_POINT_SPRITE	= (1 << 0)
} fimgPrimitiveType;

typedef union {
	unsigned int val;
	struct {
		unsigned intUse		:2;
		unsigned		:3;
		unsigned type		:8;
		unsigned pointSize	:1;
		unsigned		:4;
		unsigned vsOut		:4;
		unsigned flatShadeEn	:1;
		unsigned flatShadeSel	:9;
	} bits;
} fimgVertexContext;

/* Functions */
void fimgSetVertexContext(fimgContext *ctx,
			  unsigned int type,
			  unsigned int count);
void fimgSetViewportParams(/*int bYFlip,*/
			   float x0,
			   float y0,
			   float px,
			   float py/*,
			   float H*/);
void fimgSetDepthRange(float n, float f);

/*
 * Raster engine
 */

/* Type definitions */
typedef enum {
	FGRA_DEPTH_OFFSET_FACTOR,
	FGRA_DEPTH_OFFSET_UNITS,
	FGRA_DEPTH_OFFSET_R
} fimgDepthOffsetParam;

typedef enum {
	FGRA_BFCULL_FACE_BACK = 0,
	FGRA_BFCULL_FACE_FRONT,
	FGRA_BFCULL_FACE_BOTH = 3
} fimgCullingFace;

typedef union {
	unsigned int val;
	struct {
		unsigned		:8;
		struct {
			unsigned ddy	:1;
			unsigned ddx	:1;
			unsigned lod	:1;
		} coef[8];
	} bits;
} fimgLODControl;

/* Functions */
void fimgSetPixelSamplePos(int corner);
void fimgEnableDepthOffset(int enable);
int fimgSetDepthOffsetParam(fimgDepthOffsetParam param,
			    unsigned int value);
int fimgSetDepthOffsetParamF(fimgDepthOffsetParam param,
			     float value);
void fimgSetFaceCullControl(int enable,
			    int bCW,
			    fimgCullingFace face);
void fimgSetYClip(unsigned int ymin, unsigned int ymax);
void fimgSetLODControl(fimgLODControl ctl);
void fimgSetXClip(unsigned int xmin, unsigned int xmax);
void fimgSetPointWidth(float pWidth);
void fimgSetMinimumPointWidth(float pWidthMin);
void fimgSetMaximumPointWidth(float pWidthMax);
void fimgSetCoordReplace(unsigned int coordReplaceNum);
void fimgSetLineWidth(float lWidth);

/*
 * Shaders
 */

#define FGSP_MAX_ATTRIBTBL_SIZE 12
#define BUILD_SHADER_VERSION(major, minor)	(0xFFFF0000 | (((minor)&0xFF)<<8) | ((major) & 0xFF))
#define VERTEX_SHADER_MAGIC 			(((('V')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define PIXEL_SHADER_MAGIC 			(((('P')&0xFF)<<0)|((('S')&0xFF)<<8)|(((' ')&0xFF)<<16)|(((' ')&0xFF)<<24))
#define SHADER_VERSION 				BUILD_SHADER_VERSION(3,0)
#define FGVS_ATTRIB(i)				(3 - ((i) % 4))
#define FGVS_ATTRIB_BANK(i)			(((i) / 4) & 3)

/* Type definitions */
typedef struct {
	unsigned int	Magic;
	unsigned int	Version;
	unsigned int	HeaderSize;
	unsigned int	InTableSize;
	unsigned int	OutTableSize;
	unsigned int	SamTableSize;
	unsigned int	InstructSize;
	unsigned int	ConstFloatSize;
	unsigned int	ConstIntSize;
	unsigned int	ConstBoolSize;
	unsigned int	reserved[6];
} fimgShaderHeader;

typedef struct {
	int		validTableInfo;
	unsigned int	outAttribTableSize;
	unsigned int	inAttribTableSize;
	unsigned int	vsOutAttribTable[12];
	unsigned int	psInAttribTable[12];
} fimgShaderAttribTable;

typedef union {
	unsigned int val;
	struct {
		unsigned		:12;
		unsigned out		:4;
		unsigned		:12;
		unsigned in		:4;
	} bits;
} fimgShaderAttribNum;

/* Vertex shader functions */
int fimgLoadVShader(const unsigned int *pShaderCode);
void fimgVSSetIgnorePCEnd(int enable);
void fimgVSSetPCRange(unsigned int start, unsigned int end);
void fimgVSSetAttribNum(unsigned int inAttribNum);
int fimgMakeShaderAttribTable(const unsigned int *pVertexShader,
			      const unsigned int *pPixelShader,
			      fimgShaderAttribTable *attribTable);
int fimgRemapVShaderOutAttrib(fimgShaderAttribTable *pShaderAttribTable);
void fimgSetVShaderAttribTable(unsigned int in, unsigned int idx, unsigned int value);

/* Pixel shader functions */
int fimgLoadPShader(const unsigned int *pShaderCode);
void fimgPSSetPCRange(unsigned int start, unsigned int end);
void fimgPSSetIgnorePCEnd(int enable);
int fimgPSSetAttributeNum(unsigned int attributeNum);
int fimgPSInBufferStatusReady(void);
int fimgPSSetExecMode(int exec);

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

/*
 * OpenGL 1.1 compatibility
 */

void fimgLoadMatrix(unsigned int matrix, float *pData);

/*
 * Per-fragment unit
 */

/* Type definitions */
typedef enum {
	FGPF_TEST_MODE_NEVER = 0,
	FGPF_TEST_MODE_ALWAYS,
	FGPF_TEST_MODE_LESS,
	FGPF_TEST_MODE_LEQUAL,
	FGPF_TEST_MODE_EQUAL,
	FGPF_TEST_MODE_GREATER,
	FGPF_TEST_MODE_GEQUAL,
	FGPF_TEST_MODE_NOTEQUAL
} fimgTestMode;

/**
 *	WORKAROUND
 *	Inconsistent with fimgTestMode due to hardware bug
 */
typedef enum {
	FGPF_STENCIL_MODE_NEVER = 0,
	FGPF_STENCIL_MODE_ALWAYS,
	FGPF_STENCIL_MODE_GREATER,
	FGPF_STENCIL_MODE_GEQUAL,
	FGPF_STENCIL_MODE_EQUAL,
	FGPF_STENCIL_MODE_LESS,
	FGPF_STENCIL_MODE_LEQUAL,
	FGPF_STENCIL_MODE_NOTEQUAL
} fimgStencilMode;

typedef enum {
	FGPF_TEST_ACTION_KEEP = 0,
	FGPF_TEST_ACTION_ZERO,
	FGPF_TEST_ACTION_REPLACE,
	FGPF_TEST_ACTION_INCR,
	FGPF_TEST_ACTION_DECR,
	FGPF_TEST_ACTION_INVERT,
	FGPF_TEST_ACTION_INCR_WRAP,
	FGPF_TEST_ACTION_DECR_WRAP
} fimgTestAction;

typedef enum {
	FGPF_BLEND_EQUATION_ADD = 0,
	FGPF_BLEND_EQUATION_SUB,
	FGPF_BLEND_EQUATION_REVSUB,
	FGPF_BLEND_EQUATION_MIN,
	FGPF_BLEND_EQUATION_MAX
} fimgBlendEquation;

typedef enum {
	FGPF_BLEND_FUNC_ZERO = 0,
	FGPF_BLEND_FUNC_ONE,
	FGPF_BLEND_FUNC_SRC_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_SRC_COLOR,
	FGPF_BLEND_FUNC_DST_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_DST_COLOR,
	FGPF_BLEND_FUNC_SRC_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
	FGPF_BLEND_FUNC_DST_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_DST_ALPHA,
	FGPF_BLEND_FUNC_CONST_COLOR,
	FGPF_BLEND_FUNC_ONE_MINUS_CONST_COLOR,
	FGPF_BLEND_FUNC_CONST_ALPHA,
	FGPF_BLEND_FUNC_ONE_MINUS_CONST_ALPHA,
	FGPF_BLEND_FUNC_SRC_ALPHA_SATURATE
} fimgBlendFunction;

typedef enum {
	FGPF_LOGOP_CLEAR = 0,
	FGPF_LOGOP_AND,
	FGPF_LOGOP_AND_REVERSE,
	FGPF_LOGOP_COPY,
	FGPF_LOGOP_AND_INVERTED,
	FGPF_LOGOP_NOOP,
	FGPF_LOGOP_XOR,
	FGPF_LOGOP_OR,
	FGPF_LOGOP_NOR,
	FGPF_LOGOP_EQUIV,
	FGPF_LOGOP_INVERT,
	FGPF_LOGOP_OR_REVERSE,
	FGPF_LOGOP_COPY_INVERTED,
	FGPF_LOGOP_OR_INVERTED,
	FGPF_LOGOP_NAND,
	FGPF_LOGOP_SET
} fimgLogicalOperation;

typedef enum {
	FGPF_COLOR_MODE_555 = 0,
	FGPF_COLOR_MODE_565,
	FGPF_COLOR_MODE_4444,
	FGPF_COLOR_MODE_1555,
	FGPF_COLOR_MODE_0888,
	FGPF_COLOR_MODE_8888
} fimgColorMode;

/* Functions */
void fimgSetScissorParams(unsigned int xMax, unsigned int xMin,
			  unsigned int yMax, unsigned int yMin);
void fimgSetScissorEnable(int enable);
void fimgSetAlphaParams(unsigned int refAlpha, fimgTestMode mode);
void fimgSetAlphaEnable(int enable);
void fimgSetFrontStencilFunc(fimgStencilMode mode, unsigned char ref,
			     unsigned char mask);
void fimgSetFrontStencilOp(fimgTestAction sfail, fimgTestAction dpfail,
			   fimgTestAction dppass);
void fimgSetBackStencilFunc(fimgStencilMode mode, unsigned char ref,
			    unsigned char mask);
void fimgSetBackStencilOp(fimgTestAction sfail, fimgTestAction dpfail,
			  fimgTestAction dppass);
void fimgSetStencilEnable(int enable);
void fimgSetDepthParams(fimgTestMode mode);
void fimgSetDepthEnable(int enable);
void fimgSetBlendEquation(fimgBlendEquation alpha, fimgBlendEquation color);
void fimgSetBlendFunc(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
		      fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendFuncRGB565(fimgBlendFunction srcAlpha, fimgBlendFunction srcColor,
			    fimgBlendFunction dstAlpha, fimgBlendFunction dstColor);
void fimgSetBlendEnable(int enable);
void fimgSetBlendColor(unsigned int blendColor);
void fimgSetDitherEnable(int enable);
void fimgSetLogicalOpParams(fimgLogicalOperation alpha, fimgLogicalOperation color);
void fimgSetLogicalOpEnable(int enable);
void fimgSetColorBufWriteMask(int r, int g, int b, int a);
#if TARGET_FIMG_VERSION != _FIMG3DSE_VER_1_2
void fimgSetStencilBufWriteMask(int back, unsigned int mask);
#endif
void fimgSetZBufWriteMask(int enable);
void fimgSetFrameBufParams(int opaqueAlpha, unsigned int thresholdAlpha,
			   unsigned int constAlpha, fimgColorMode format);
void fimgSetZBufBaseAddr(unsigned int addr);
void fimgSetColorBufBaseAddr(unsigned int addr);
void fimgSetFrameBufWidth(unsigned int width);

/*
 * Hardware context
 */

/*typedef */struct _fimgContext {
	fimgAttribute attrib[FIMG_ATTRIB_NUM];
	unsigned int numAttribs;
}/*fimgContext*/;

//=============================================================================

#ifdef __cplusplus
}
#endif

#endif /* _FIMG_H_ */
