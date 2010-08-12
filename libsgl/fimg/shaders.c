/*
* fimg/shaders.c
*
* SAMSUNG S3C6410 FIMG-3DSE SHADERS RELATED FUNCTIONS
*
* Copyrights:	2005 by Samsung Electronics Limited (original code)
*		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
*/

#include <stdlib.h>
#include "fimg.h"

/*
 * Vertex shader
 */

#define VSHADER_OFFSET		0x10000

#define FGVS_INSTMEM_START	VSHADER_ADDR(0x0000)
#define FGVS_INSTMEM(i)		(FGVS_INSTMEM_START + 4*(i))

#define FGVS_CFLOAT_START	VSHADER_ADDRF(0x4000)
#define FGVS_CFLOAT_X(i)	(0x00 + FGVS_CFLOAT_START + 4*(i))
#define FGVS_CFLOAT_Y(i)	(0x04 + FGVS_CFLOAT_START + 4*(i))
#define FGVS_CFLOAT_Z(i)	(0x08 + FGVS_CFLOAT_START + 4*(i))
#define FGVS_CFLOAT_W(i)	(0x0c + FGVS_CFLOAT_START + 4*(i))

#define FGVS_CINT_START		VSHADER_ADDR(0x8000)
#define FGVS_CINT(i)		(FGVS_CINT_START + (i))

#define FGVS_CBOOL_START	VSHADER_ADDR(0x8400)

#define FGVS_CONFIG		VSHADER_ADDR(0xc800)
#define FGVS_STATUS		VSHADER_ADDR(0xc804)
#define FGVS_PCRANGE		VSHADER_ADDR(0x10000)
#define FGVS_ATTRIB_NUM		VSHADER_ADDR(0x10004)

#define FGVS_IN_ATTR_IDX(i)	VSHADER_ADDR(0x10008 + 4*(i))
#define FGVS_OUT_ATTR_IDX(i)	VSHADER_ADDR(0x10014 + 4*(i))

#define VSHADER_OFFS(reg)	(VSHADER_OFFSET + (reg))
#define VSHADER_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + VSHADER_OFFS(reg)))
#define VSHADER_ADDRF(reg)	((volatile float *)((char *)fimgBase + VSHADER_OFFS(reg)))

typedef union {
	unsigned int val;
	struct {
		unsigned		:30;
		unsigned clrStatus	:1;
		unsigned copyPC		:1;
	} bits;
} fimgVShaderConfig;

typedef union {
	unsigned int val;
	struct {
		unsigned ignorePCEnd	:1;
		unsigned		:6;
		unsigned PCEnd		:9;
		unsigned		:7;
		unsigned PCStart	:9;
	} bits;
} fimgVShaderPCRange;

typedef union {
	unsigned int val;
	struct {
		unsigned		:4;
		unsigned num		:4;
	} attrib[4];
} fimgVShaderAttrIdx;

static inline void fimgVShaderWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline void fimgVShaderWriteF(float data, volatile float *reg)
{
	*reg = data;
}

static inline unsigned int fimgVShaderRead(volatile unsigned int *reg)
{
	return *reg;
}

/*
 * Pixel shader
 */

#define PSHADER_OFFSET		0x40000

#define FGPS_INSTMEM_START	PSHADER_ADDR(0x0000)
#define FGPS_INSTMEM(i)		(FGPS_INSTMEM_START + 4*(i))

#define FGPS_CFLOAT_START	PSHADER_ADDRF(0x4000)
#define FGPS_CFLOAT_X(i)	(0x00 + FGPS_CFLOAT_START + 4*(i))
#define FGPS_CFLOAT_Y(i)	(0x04 + FGPS_CFLOAT_START + 4*(i))
#define FGPS_CFLOAT_Z(i)	(0x08 + FGPS_CFLOAT_START + 4*(i))
#define FGPS_CFLOAT_W(i)	(0x0c + FGPS_CFLOAT_START + 4*(i))

#define FGPS_CINT_START		PSHADER_ADDR(0x8000)
#define FGPS_CINT(i)		(FGPS_CINT_START + (i))

#define FGPS_CBOOL_START	PSHADER_ADDR(0x8400)

#define FGPS_EXE_MODE		PSHADER_ADDR(0xc800)
#define FGPS_PC_START		PSHADER_ADDR(0xc804)
#define FGPS_PC_END		PSHADER_ADDR(0xc808)
#define FGPS_PC_COPY		PSHADER_ADDR(0xc80c)
#define FGPS_ATTRIB_NUM		PSHADER_ADDR(0xc810)
#define FGPS_IBSTATUS		PSHADER_ADDR(0xc814)

#define PSHADER_OFFS(reg)	(PSHADER_OFFSET + (reg))
#define PSHADER_ADDR(reg)	((volatile unsigned int *)((char *)fimgBase + PSHADER_OFFS(reg)))
#define PSHADER_ADDRF(reg)	((volatile float *)((char *)fimgBase + PSHADER_OFFS(reg)))

typedef union {
	unsigned int val;
	struct {
		unsigned		:22;
		unsigned ignorePCEnd	:1;
		unsigned PCEnd		:9;
	} bits;
} fimgPShaderPCEnd;

static inline void fimgPShaderWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline void fimgPShaderWriteF(float data, volatile float *reg)
{
	*reg = data;
}

static inline unsigned int fimgPShaderRead(volatile unsigned int *reg)
{
	return *reg;
}

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
void fimgVSSetIgnorePCEnd(int enable)
{
	fimgVShaderPCRange reg;

	reg.val = fimgVShaderRead(FGVS_PCRANGE);
	reg.bits.ignorePCEnd = !!enable;
	fimgVShaderWrite(reg.val, FGVS_PCRANGE);
}

/*****************************************************************************
 * FUNCTIONS:	fimgVSSetPCRange
 * SYNOPSIS:	This function specifies a start and end address of vetex shader
 *		program.
 * PARAMETERS:	[IN] start: the start program count of vertex shader program.
 *		[IN] end: the end program count of vertex shader program.
 *****************************************************************************/
void fimgVSSetPCRange(unsigned int start, unsigned int end)
{
	fimgVShaderPCRange PCRange;
	fimgVShaderConfig Config;

	PCRange.val = 0;
	PCRange.bits.PCStart = start;
	PCRange.bits.PCEnd = end;
	fimgVShaderWrite(PCRange.val, FGVS_PCRANGE);

	Config.val = 0;
	Config.bits.copyPC = 1;
	fimgVShaderWrite(Config.val, FGVS_CONFIG);
}

/*****************************************************************************
 * FUNCTIONS:	fimgVSSetAttribNum
 * SYNOPSIS:	This function specifies the number of attributes of current
 *		vertex shader programs
 * PARAMETERS:	[in] intAttribNum: the number of attributes for the vertex
 *		shader input (0~8)
 *****************************************************************************/
void fimgVSSetAttribNum(unsigned int inAttribNum)
{
	fimgVShaderWrite(inAttribNum, FGVS_ATTRIB_NUM);
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
int fimgMakeShaderAttribTable(const unsigned int *pVertexShader,
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
int fimgRemapVShaderOutAttrib(fimgShaderAttribTable *pShaderAttribTable)
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
		fimgVShaderWrite(OutAttribIndex[i].val, FGVS_OUT_ATTR_IDX(i));

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
void fimgSetVShaderAttribTable(unsigned int in, unsigned int idx,
			       unsigned int value)
{
	fimgVShaderWrite(value, (in) ? FGVS_IN_ATTR_IDX(idx) : FGVS_OUT_ATTR_IDX(idx));
}


// Fragment Shader Register-level API

/*****************************************************************************
 * FUNCTIONS:	fimgPSSetPCRange
 * SYNOPSIS:	This function specifies the start and end address of the fragment
 *		shader program which fragment shader will be used.
 * PARAMETERS:	[in] start - a start address of the pixel shader program.
 *		[in] end - an end address of the pixel shader program.
 * RETURNS:	 0, if successful
 *		-1, on error
 *****************************************************************************/
void fimgPSSetPCRange(unsigned int start, unsigned int end)
{
	fimgPShaderWrite(start, FGPS_PC_START);
	fimgPShaderWrite(end, FGPS_PC_END);
	fimgPShaderWrite(1, FGPS_PC_COPY);
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
int fimgPSSetAttributeNum(unsigned int attributeNum)
{
	int ret;

	if((ret = fimgPSSetExecMode(0)))
		return ret;

	fimgPShaderWrite(attributeNum, FGPS_ATTRIB_NUM);

	do {
		// TODO: Add some sleep
	} while(!fimgPSInBufferStatusReady());

	if((ret = fimgPSSetExecMode(1)))
		return ret;

	return 0;
}

/*****************************************************************************
 * FUNCTIONS:	fimgPSGetInBufferStatus
 * SYNOPSIS:	this function read status register for monitoring fragment shader
 *		input buffer initialization status.
 * RETURNS:	1, if buffer is ready
 *		0, otherwise
 *****************************************************************************/
int fimgPSInBufferStatusReady(void)
{
	unsigned int uInBufStatus = fimgPShaderRead(FGPS_IBSTATUS);

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
int fimgLoadVShader(const unsigned int *pShaderCode)
{
	unsigned int i;
	unsigned int size;
	unsigned int offset = 0;
	fimgVShaderAttrIdx AttribIdx;
	unsigned int IdxVal = 0;
	volatile unsigned int *pAddr;
	volatile float *pfAddr;
	unsigned int *pShaderData;
	float *pfShaderData;

	fimgShaderHeader *pShaderHeader = (fimgShaderHeader*)pShaderCode;
	unsigned int *pShaderBody = (unsigned int*)(&pShaderHeader[1]);

	if((pShaderHeader->Magic != VERTEX_SHADER_MAGIC) || (pShaderHeader->Version != SHADER_VERSION))
		return -1;

	if(pShaderHeader->InTableSize && pShaderHeader->OutTableSize) {
		fimgShaderAttribNum num;
		num.val = 0;
		num.bits.in = pShaderHeader->InTableSize;
		num.bits.out = pShaderHeader->OutTableSize;
		fimgVShaderWrite(num.val, FGVS_ATTRIB_NUM);

		pAddr = FGVS_IN_ATTR_IDX(0);
		size = pShaderHeader->InTableSize;
		offset += size;

		while(size >= 4) {
			AttribIdx.val = 0;
			for(i = 0; i < 4; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;

			fimgVShaderWrite(AttribIdx.val, pAddr++);
			size -= 4;
		}

		if(size) {
			AttribIdx.val = 0;
			for(i=0; i<size; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;
			fimgVShaderWrite(AttribIdx.val, pAddr++);
		}

		pAddr = FGVS_OUT_ATTR_IDX(0);
		size = pShaderHeader->OutTableSize;
		offset += size;
		IdxVal = 0;

		while(size >= 4) {
			AttribIdx.val = 0;
			for(i = 0; i < 4; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;

			fimgVShaderWrite(AttribIdx.val, pAddr++);
			size -= 4;
		}

		if(size) {
			AttribIdx.val = 0;
			for(i=0; i<size; i++)
				AttribIdx.attrib[FGVS_ATTRIB(i)].num = IdxVal++;
			fimgVShaderWrite(AttribIdx.val, pAddr++);
		}
	}

	if(pShaderHeader->SamTableSize) {
		// Unused
		offset += pShaderHeader->SamTableSize;
	}

	if(pShaderHeader->InstructSize) {
		// vertex shader instruction memory start addr.
		pAddr = FGVS_INSTMEM_START;
		pShaderData =(unsigned int *)(pShaderBody + offset);
		size = pShaderHeader->InstructSize;
		offset += size;

		// Program counter start/end address setting
		fimgVSSetPCRange(0, (size / 4) - 1);

		do {
			fimgVShaderWrite(*pShaderData++, pAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstFloatSize) {
		// vertex shader float memory start addr.
		pfAddr = FGVS_CFLOAT_START;
		pfShaderData =(float *)(pShaderBody + offset);
		size = pShaderHeader->ConstFloatSize;
		offset += size;

		do {
			fimgVShaderWriteF(*pfShaderData++, pfAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstIntSize) {
		// vertex shader integer memory start addr.
		pAddr = (volatile unsigned int *)FGVS_CINT_START;
		pShaderData =(unsigned int *)(pShaderBody + offset);
		size = pShaderHeader->ConstIntSize;
		offset += size;

		do {
			fimgVShaderWrite(*pShaderData++, pAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstBoolSize) {
		pShaderData =(unsigned int *)(pShaderBody + offset);
		fimgVShaderWrite(*pShaderData, FGVS_CBOOL_START);
	}

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
int fimgLoadPShader(const unsigned int *pShaderCode)
{
	int ret;
	unsigned int size;
	unsigned int offset = 0;
	volatile unsigned int *pAddr;
	unsigned int *pShaderData;

	fimgShaderHeader *pShaderHeader = (fimgShaderHeader*)pShaderCode;
	unsigned int *pShaderBody = (unsigned int*)(&pShaderHeader[1]);

	if((pShaderHeader->Magic != PIXEL_SHADER_MAGIC) || (pShaderHeader->Version != SHADER_VERSION))
		return -1;

	if(pShaderHeader->InTableSize) {
		if((ret = fimgPSSetAttributeNum(pShaderHeader->InTableSize)) != 0)
			return ret;

		offset += pShaderHeader->InTableSize;
	}

	if(pShaderHeader->OutTableSize != 0) {
		// Unused
		offset += pShaderHeader->OutTableSize;
	}

	if(pShaderHeader->SamTableSize != 0) {
		// Unused
		offset += pShaderHeader->SamTableSize;
	}

	if(pShaderHeader->InstructSize != 0) {
		// pixel shader instruction memory start addr.
		pAddr = FGPS_INSTMEM_START;
		pShaderData =(unsigned int *)(pShaderBody + offset);
		size = pShaderHeader->InstructSize;
		offset += size;

		if((ret = fimgPSSetExecMode(0)) != 0)
			return ret;

		fimgPSSetPCRange(0, (size / 4) - 1);

		if((ret = fimgPSSetExecMode(1)) != 0)
			return ret;

		do {
			fimgPShaderWrite(*pShaderData++, pAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstFloatSize) {
		// pixel shader float memory start addr.
		pAddr = (volatile unsigned int *)FGPS_CFLOAT_START;
		pShaderData =(unsigned int *)(pShaderBody + offset);
		size = pShaderHeader->ConstFloatSize;
		offset += size;

		do {
			fimgPShaderWrite(*pShaderData++, pAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstIntSize) {
		// pixel shader integer memory start addr.
		pAddr = (volatile unsigned int *)FGPS_CINT_START;
		pShaderData =(unsigned int *)(pShaderBody + offset);
		size = pShaderHeader->ConstIntSize;
		offset += size;

		do {
			fimgPShaderWrite(*pShaderData++, pAddr++);
		} while(--size);
	}

	if(pShaderHeader->ConstBoolSize) {
		pShaderData =(unsigned int *)(pShaderBody + offset);
		fimgPShaderWrite(*pShaderData, FGPS_CBOOL_START);
	}

	return 0;
}

int fimgPSSetExecMode(int exec)
{
	int target = !!exec;
	int status = fimgPShaderRead(FGPS_EXE_MODE);

	if(status == target)
		return 0;

	fimgPShaderWrite(target, FGPS_EXE_MODE);

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

