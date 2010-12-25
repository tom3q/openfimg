/*
 * fimg/shaders.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE SHADERS RELATED FUNCTIONS
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
#include "fimg_private.h"

/*
 * Vertex shader
 */

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
	unsigned int val;
	struct {
		unsigned copyPC		:1;
		unsigned clrStatus	:1;
		unsigned		:30;
	} bits;
} fimgVShaderConfig;

typedef union {
	unsigned int val;
	struct {
		unsigned PCStart	:9;
		unsigned		:7;
		unsigned PCEnd		:9;
		unsigned		:6;
		unsigned ignorePCEnd	:1;
	} bits;
} fimgVShaderPCRange;

typedef union {
	unsigned int val;
	struct {
		unsigned num		:4;
		unsigned		:4;
	} attrib[4];
} fimgVShaderAttrIdx;

/*
 * Pixel shader
 */

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
	unsigned int val;
	struct {
		unsigned PCEnd		:9;
		unsigned ignorePCEnd	:1;
		unsigned		:22;
	} bits;
} fimgPShaderPCEnd;

typedef enum {
	FGVS_ATRBDEF_POSITION  = 0x10,
	FGVS_ATRBDEF_NORMAL    = 0x20,
	FGVS_ATRBDEF_PCOLOR    = 0x40,
	FGVS_ATRBDEF_SCOLOR    = 0x41,
	FGVS_ATRBDEF_TEXTURE0  = 0x80,
	FGVS_ATRBDEF_TEXTURE1  = 0x81,
	FGVS_ATRBDEF_TEXTURE2  = 0x82,
	FGVS_ATRBDEF_TEXTURE3  = 0x83,
	FGVS_ATRBDEF_TEXTURE4  = 0x84,
	FGVS_ATRBDEF_TEXTURE5  = 0x85,
	FGVS_ATRBDEF_TEXTURE6  = 0x86,
	FGVS_ATRBDEF_TEXTURE7  = 0x87,
	FGVS_ATRBDEF_POINTSIZE = 0x1,
	FGVS_ATRBDEF_USERDEF0  = 0x2,
	FGVS_ATRBDEF_USERDEF1  = 0x3,
	FGVS_ATRBDEF_USERDEF2  = 0x4,
	FGVS_ATRBDEF_USERDEF3  = 0x5
} fimgDeclareAttrib;

static int _SearchAttribTable(unsigned int *pAttribTable,
			      unsigned int tableSize,
			      fimgDeclareAttrib dclAttribName);

// Vertex Shader Register-level API

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgVSSetIgnorePCEnd
 * SYNOPSIS:	This function specifies a start and end address of vetex shader
 *           	program.
 * PARAMETERS:	[IN] enable - specifies whether the end of PC range is ignored
 *                           or not.
 *****************************************************************************/
void fimgVSSetIgnorePCEnd(fimgContext *ctx, int enable)
{
	fimgVShaderPCRange reg;

	reg.val = fimgRead(ctx, FGVS_PCRANGE);
	reg.bits.ignorePCEnd = !!enable;
	fimgWrite(ctx, reg.val, FGVS_PCRANGE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgVSSetPCRange
 * SYNOPSIS:	This function specifies a start and end address of vetex shader
 *		program.
 * PARAMETERS:	[IN] start: the start program count of vertex shader program.
 *		[IN] end: the end program count of vertex shader program.
 *****************************************************************************/
void fimgVSSetPCRange(fimgContext *ctx, unsigned int start, unsigned int end)
{
	fimgVShaderPCRange PCRange;
	fimgVShaderConfig Config;

	PCRange.val = 0;
	PCRange.bits.PCStart = start;
	PCRange.bits.PCEnd = end;
	fimgWrite(ctx, PCRange.val, FGVS_PCRANGE);

	Config.val = 0;
	Config.bits.copyPC = 1;
	Config.bits.clrStatus = 1;
	fimgWrite(ctx, Config.val, FGVS_CONFIG);
}

/*****************************************************************************
 * FUNCTIONS:	fimgVSSetAttribNum
 * SYNOPSIS:	This function specifies the number of attributes of current
 *		vertex shader programs
 * PARAMETERS:	[in] intAttribNum: the number of attributes for the vertex
 *		shader input (0~8)
 *****************************************************************************/
void fimgVSSetAttribNum(fimgContext *ctx, unsigned int inAttribNum)
{
	fimgWrite(ctx, inAttribNum, FGVS_ATTRIB_NUM);
}

/*****************************************************************************
 * FUNCTIONS:	fimgMakeShaderAttribTable
 * SYNOPSIS:	to make a attribute table which is extracted from shader program.
 * PARAMETERS:	[in] pVertexShader 	- the pointer variable of a vertex Program.
 *		[in] pPixelShader  	- the pointer variable of a fragment program.
 *		[in] attribTable   	- the pointer variable pointing in/out
 *					  attributes table.
 * RETURNS:	 0 on success,
 *		-1 on error
 *****************************************************************************/
int fimgMakeShaderAttribTable(fimgContext *ctx,
			      const unsigned int *pVertexShader,
			      const unsigned int *pPixelShader,
			      fimgShaderAttribTable *attribTable)
{
	int ret = 0;
	fimgShaderHeader *pVShaderHeader;
	fimgShaderHeader *pPShaderHeader;
	unsigned int *pShaderBody;
	unsigned int *pVSOutTable;
	unsigned int *pPSInTable;
	unsigned int nTableSize;
	unsigned int i;

	if(pVertexShader == NULL || pPixelShader == NULL || attribTable == NULL)
		return -1;

	pVShaderHeader = (fimgShaderHeader*)pVertexShader;

	if((pVShaderHeader->Magic != VERTEX_SHADER_MAGIC) ||
			(pVShaderHeader->Version != SHADER_VERSION))
		return -1;

	nTableSize = pVShaderHeader->InTableSize;
	pShaderBody = (unsigned int *)(&pVShaderHeader[1]);
	pVSOutTable = (unsigned int *)(pShaderBody + nTableSize);
	nTableSize = pVShaderHeader->OutTableSize;

	if((nTableSize >= FGSP_MAX_ATTRIBTBL_SIZE) ||
			(pVShaderHeader->OutTableSize == 0))
		return -1;

	attribTable->outAttribTableSize = nTableSize;

	for(i=0; i<nTableSize; i++)
		attribTable->vsOutAttribTable[i] = *pVSOutTable++;

	pPShaderHeader = (fimgShaderHeader*)pPixelShader;

	if((pPShaderHeader->Magic != PIXEL_SHADER_MAGIC) ||
			(pPShaderHeader->Version != SHADER_VERSION))
		return -1;

	pPSInTable = (unsigned int*)(&pPShaderHeader[1]);

	nTableSize = pPShaderHeader->InTableSize;

	if((nTableSize >= FGSP_MAX_ATTRIBTBL_SIZE) ||
			(pPShaderHeader->InTableSize == 0))
		return -1;

	attribTable->inAttribTableSize = nTableSize;

	for(i=0; i<nTableSize; i++)
		attribTable->psInAttribTable[i] = *pPSInTable++;

	attribTable->validTableInfo = 1;

	return ret;
}

/*****************************************************************************
 * FUNCTIONS:	fimgRemapVShaderOutAttrib
 * SYNOPSIS:	This function remap the input attribute index registers for
 *		flexibility. The N-th input attribute from host is actually read
 *		from the position indicated by the index looked up from the
 *		AttribN corresponding to the register number of input register
 *		in the shader program.
 * PARAMETERS:	[in] pShaderAttribTable - the pointer variable pointing in/out
 *		     attribute table which is extracted from shader program.
 * RETURNS:	 0, if successful
 *		-1, otherwise.
 *****************************************************************************/
int fimgRemapVShaderOutAttrib(fimgContext *ctx,
			      fimgShaderAttribTable *pShaderAttribTable)
{
	unsigned int i;
	unsigned int nInAttribNum = 0;
	unsigned int nOutAttribNum = 0;
	unsigned int *pOutAttribTable;
	unsigned int *pInAttribTable;
	int uAttribIndex;
	unsigned int uIndexCount = 1;

	fimgVShaderAttrIdx OutAttribIndex[3] = { {.val = 0x03020100}, {.val = 0x07060503}, {.val = 0x0b0a0908} };

	if(pShaderAttribTable == NULL)
		// Invalid pointer
		return -1;

	if(!pShaderAttribTable->validTableInfo)
		// Invalid table info
		return -1;

	nInAttribNum = pShaderAttribTable->inAttribTableSize;
	nOutAttribNum = pShaderAttribTable->outAttribTableSize;

	if((nInAttribNum <= 0) || (nOutAttribNum <= 0) || (nOutAttribNum <= nInAttribNum))
		// Invalid attribute count
		return -1;

	pOutAttribTable = (unsigned int *)&pShaderAttribTable->vsOutAttribTable[0];
	pInAttribTable = (unsigned int *)&pShaderAttribTable->psInAttribTable[0];

	uAttribIndex = _SearchAttribTable(pOutAttribTable, nOutAttribNum, FGVS_ATRBDEF_POINTSIZE);

	if(uAttribIndex >= 0) {
		// Found point size in output table, skip one more output attribute
		uIndexCount++;
	}

	for(i = 0; i < nInAttribNum; i++) {
		uAttribIndex = _SearchAttribTable(pOutAttribTable, nOutAttribNum, (fimgDeclareAttrib)pInAttribTable[i]);

		if(uAttribIndex < 0)
			// Input attribute not found in output table
			return -1;

		OutAttribIndex[FGVS_ATTRIB_BANK(uIndexCount)].attrib[FGVS_ATTRIB(uIndexCount)].num = uAttribIndex;
		uIndexCount++;
	}

	for(i = 0; i < 3; i++)
		fimgWrite(ctx, OutAttribIndex[i].val, FGVS_OUT_ATTR_IDX(i));

	return 0;
}


/*****************************************************************************
 * FUNCTIONS:	fimgSetVShaderAttribTable
 * SYNOPSIS:	This function specifies in/out arrributes order of the vertex
 *		shader program which vertex shader will be used.
 * PARAMETERS:	[in] idx	- a index of the attribute table.
 *		[in] in		- 0, set output attribute table
 *				  otherwise, set input attribute table
 *		[in] value	- a value to order attributes.
 *****************************************************************************/
void fimgSetVShaderAttribTable(fimgContext *ctx,
			       unsigned int in, unsigned int idx,
			       unsigned int value)
{
	fimgWrite(ctx, value, (in) ? FGVS_IN_ATTR_IDX(idx) : FGVS_OUT_ATTR_IDX(idx));
}


// Fragment Shader Register-level API

int fimgPSInBufferStatusReady(fimgContext *ctx);
int fimgPSSetExecMode(fimgContext *ctx, int exec);

/*****************************************************************************
 * FUNCTIONS:	fimgPSSetPCRange
 * SYNOPSIS:	This function specifies the start and end address of the fragment
 *		shader program which fragment shader will be used.
 * PARAMETERS:	[in] start - a start address of the pixel shader program.
 *		[in] end - an end address of the pixel shader program.
 * RETURNS:	 0, if successful
 *		-1, on error
 *****************************************************************************/
inline void fimgPSSetPCRange(fimgContext *ctx,
			     unsigned int start, unsigned int end)
{
	fimgWrite(ctx, start, FGPS_PC_START);
	fimgWrite(ctx, end, FGPS_PC_END);
	fimgWrite(ctx, 1, FGPS_PC_COPY);
}

/*****************************************************************************
 * FUNCTIONS:	fimgPSSetAttributeNum
 * SYNOPSIS:	this function specifies the value ranged between 1 and 8 according to
 *		the number of semantics such as color and texture coordinate
 *		which are transferred to pixel shader.
 * PARAMETERS:	[in] attributeNum - the number of attribute for current context.
 * RETURNS:	 0, if successful
 *		-1, on error
 *****************************************************************************/
inline int fimgPSSetAttributeNum(fimgContext *ctx, unsigned int attributeNum)
{
	int ret;

	fimgWrite(ctx, attributeNum, FGPS_ATTRIB_NUM);

	do {
		// TODO: Add some sleep
	} while(!fimgPSInBufferStatusReady(ctx));

	return 0;
}

/*****************************************************************************
 * FUNCTIONS:	fimgPSGetInBufferStatus
 * SYNOPSIS:	this function read status register for monitoring fragment shader
 *		input buffer initialization status.
 * RETURNS:	1, if buffer is ready
 *		0, otherwise
 *****************************************************************************/
int fimgPSInBufferStatusReady(fimgContext *ctx)
{
	unsigned int uInBufStatus = fimgRead(ctx, FGPS_IBSTATUS);

	if(uInBufStatus) {
		//printf((DBG_ERROR, "The input buffer of pixel shader is not ready);
		return 0;
	}

	//printf((DBG_ERROR, "The input buffer of pixel shader is ready);
	return 1;
}


/*****************************************************************************
 * FUNCTIONS:	fimgLoadVShader
 * SYNOPSIS:	this function uploads a vertex shader program to shader memory
 *		such as constant integer, float and instruction.
 * PARAMETERS:	[in] pShaderCode - the pointer of vertex shader program.
 * RETURNS:	 0, if successful
 *		-1, invalid shader code
 *****************************************************************************/
int fimgLoadVShader(fimgContext *ctx,
		    const unsigned int *pShaderCode, unsigned int numAttribs)
{
	unsigned int i;
	unsigned int size;
	unsigned int offset = 0;
	fimgVShaderAttrIdx AttribIdx;
	unsigned int IdxVal = 0;
	volatile void *pAddr;
	void *pShaderData;

	fimgShaderHeader *pShaderHeader = (fimgShaderHeader*)pShaderCode;
	unsigned int *pShaderBody = (unsigned int*)(&pShaderHeader[1]);

	if((pShaderHeader->Magic != VERTEX_SHADER_MAGIC) || (pShaderHeader->Version != SHADER_VERSION))
		return -1;

	if(pShaderHeader->InstructSize) {
		// vertex shader instruction memory start addr.
		pAddr = ctx->base + FGVS_INSTMEM_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->InstructSize;
		offset += 4 * size;

		// Program counter start/end address setting
		fimgVSSetPCRange(ctx, 0, size - 1);

		memcpy((void *)pAddr, pShaderData, 16 * size);
	}

	if(pShaderHeader->ConstFloatSize) {
		// vertex shader float memory start addr.
		pAddr = ctx->base + FGVS_CFLOAT_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstFloatSize;
		offset += 4 * size;

		memcpy((void *)pAddr, pShaderData, 16 * size);
	}

	if(pShaderHeader->ConstIntSize) {
		// vertex shader integer memory start addr.
		pAddr = ctx->base + FGVS_CINT_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstIntSize;
		offset += size;

		memcpy((void *)pAddr, pShaderData, 4 * size);
	}

	if(pShaderHeader->ConstBoolSize) {
		pAddr = ctx->base + FGVS_CBOOL_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstBoolSize;
		offset += size;

		memcpy((void *)pAddr, pShaderData, 4 * size);
	}

	fimgWrite(ctx, numAttribs, FGVS_ATTRIB_NUM);

#if 0
	if(pShaderHeader->InTableSize && pShaderHeader->OutTableSize) {
		fimgShaderAttribNum num;
		num.val = 0;
		num.bits.in = pShaderHeader->InTableSize;
		num.bits.out = pShaderHeader->OutTableSize;
		fimgWrite(ctx, num.val, FGVS_ATTRIB_NUM);

		pAddr = FGVS_IN_ATTR_IDX(0);
		size = pShaderHeader->InTableSize;
		offset += size;

		while(size >= 4) {
			AttribIdx.val = 0;
			for(i = 0; i < 4; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;

			fimgWrite(ctx, AttribIdx.val, pAddr++);
			size -= 4;
		}

		if(size) {
			AttribIdx.val = 0;
			for(i=0; i<size; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;
			fimgWrite(ctx, AttribIdx.val, pAddr++);
		}

		pAddr = FGVS_OUT_ATTR_IDX(0);
		size = pShaderHeader->OutTableSize;
		offset += size;
		IdxVal = 0;

		while(size >= 4) {
			AttribIdx.val = 0;
			for(i = 0; i < 4; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;

			fimgWrite(ctx, AttribIdx.val, pAddr++);
			size -= 4;
		}

		if(size) {
			AttribIdx.val = 0;
			for(i=0; i<size; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;
			fimgWrite(ctx, AttribIdx.val, pAddr++);
		}
	}
#else
	fimgWrite(ctx, 0x03020100, FGVS_IN_ATTR_IDX(0));
	fimgWrite(ctx, 0x07060504, FGVS_IN_ATTR_IDX(1));
	fimgWrite(ctx, 0x0b0a0908, FGVS_IN_ATTR_IDX(2));

	fimgWrite(ctx, 0x03020100, FGVS_OUT_ATTR_IDX(0));
	fimgWrite(ctx, 0x07060504, FGVS_OUT_ATTR_IDX(1));
	fimgWrite(ctx, 0x0b0a0908, FGVS_OUT_ATTR_IDX(2));
#endif

#if 0
	if(pShaderHeader->UniformTableSize) {
		// Unused
		offset += pShaderHeader->UniformTableSize;
	}

	if(pShaderHeader->SamTableSize) {
		// Unused
		offset += pShaderHeader->SamTableSize;
	}
#endif

	return 0;
}

/*****************************************************************************
 * FUNCTIONS: fimgLoadPShader
 * SYNOPSIS: this function uploads a fragment shader program to shader memory
 *           such as constant integer, float and instruction.
 * PARAMETERS: [in] pShaderCode - the pointer of fragment shader program.
 * RETURNS: FGL_ERR_NO_ERROR, if successful
 *          FGL_ERR_INVALID_VALUE - the program count exceeds the range of 512 slots
 *          FGL_ERR_INVALID_SHADER_CODE - either magic number or shader version
 *                                        were not an accepted value.
 * ERRNO:   FGL_ERR_NO_ERROR                1
 *          FGL_ERR_INVALID_VALUE           7
 *          FGL_ERR_INVALID_SHADER_CODE     8
 *****************************************************************************/
int fimgLoadPShader(fimgContext *ctx,
		    const unsigned int *pShaderCode, unsigned int numAttribs)
{
	int ret;
	unsigned int size;
	unsigned int offset = 0;
	volatile void *pAddr;
	void *pShaderData;

	fimgShaderHeader *pShaderHeader = (fimgShaderHeader*)pShaderCode;
	unsigned int *pShaderBody = (unsigned int*)(&pShaderHeader[1]);

	if((pShaderHeader->Magic != PIXEL_SHADER_MAGIC) || (pShaderHeader->Version != SHADER_VERSION))
		return -1;

	if((ret = fimgPSSetExecMode(ctx, 0)) != 0)
		return ret;

	if(pShaderHeader->InstructSize) {
		// pixel shader instruction memory start addr.
		pAddr = ctx->base + FGPS_INSTMEM_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->InstructSize;
		offset += 4 * size;

		fimgPSSetPCRange(ctx, 0, size - 1);

		memcpy((void *)pAddr, pShaderData, 16 * size);
	}

	if(pShaderHeader->ConstFloatSize) {
		// pixel shader float memory start addr.
		pAddr = ctx->base + FGPS_CFLOAT_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstFloatSize;
		offset += 4 * size;

		memcpy((void *)pAddr, pShaderData, 16 * size);
	}

	if(pShaderHeader->ConstIntSize) {
		// pixel shader integer memory start addr.
		pAddr = ctx->base + FGPS_CINT_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstIntSize;
		offset += size;

		memcpy((void *)pAddr, pShaderData, 4 * size);
	}

	if(pShaderHeader->ConstBoolSize) {
		pAddr = ctx->base + FGPS_CBOOL_START;
		pShaderData = pShaderBody + offset;
		size = pShaderHeader->ConstBoolSize;
		offset += size;

		memcpy((void *)pAddr, pShaderData, 4 * size);
	}

	fimgPSSetAttributeNum(ctx, numAttribs);

#if 0
	if(pShaderHeader->InTableSize) {
		offset += pShaderHeader->InTableSize;
	}

	if(pShaderHeader->OutTableSize) {
		// Unused
		offset += pShaderHeader->OutTableSize;
	}

	if(pShaderHeader->UniformTableSize) {
		// Unused
		offset += pShaderHeader->UniformTableSize;
	}

	if(pShaderHeader->SamTableSize) {
		// Unused
		offset += pShaderHeader->SamTableSize;
	}
#endif

	if((ret = fimgPSSetExecMode(ctx, 1)) != 0)
		return ret;

	return 0;
}

int fimgPSSetExecMode(fimgContext *ctx, int exec)
{
	int target = !!exec;
	int status = fimgRead(ctx, FGPS_EXE_MODE);

	if(status == target)
		return 0;

	fimgWrite(ctx, target, FGPS_EXE_MODE);

	return 0;
}

/*****************************************************************************
  INTERNAL FUNCTIONS
 *****************************************************************************/

static int _SearchAttribTable(unsigned int *pAttribTable,
			      unsigned int tableSize,
			      fimgDeclareAttrib dclAttribName)
{
	unsigned int i;

	if(pAttribTable == NULL)
		return 0;

	for(i=0; i < tableSize; i++)
		if(pAttribTable[i] == dclAttribName)
			return i;

	return -1;
}

