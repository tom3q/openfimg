/*
 * fimg/shaders.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE SHADERS RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _SHADERS_H_
#define _SHADERS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

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

#ifdef __cplusplus
}
#endif

#endif
