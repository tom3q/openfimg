/*
 * fimg/host.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2005 by Samsung Electronics Limited (original code)
 *		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
 */

#include "host.h"

#define FGHI_FIFO_SIZE		32

#define HOST_OFFSET		0x8000

#define FGHI_DWSPACE		HOST_ADDR(0x00)
#define FGHI_FIFO_ENTRY		HOST_ADDR(0x4000)
#define FGHI_CONTROL		HOST_ADDR(0x08)
#define FGHI_IDXOFFSET		HOST_ADDR(0x0c)
#define FGHI_VBADDR		HOST_ADDR(0x10)
#define FGHI_VB_ENTRY		HOST_ADDR(0x6000)

#define ATTRIB_NUM		10
#define FGHI_ATTRIB(i)		HOST_ADDR(0x40 + 4*(i))
#define FGHI_ATTRIB_VBCTRL(i)	HOST_ADDR(0x80 + 4*(i))
#define FGHI_ATTRIB_VBBASE(i)	HOST_ADDR(0xc0 + 4*(i))

#define HOST_OFFS(reg)		(HOST_OFFSET + (reg))
#define HOST_ADDR(reg)		((volatile unsigned int *)((char *)fimgBase + HOST_OFFS(reg)))

typedef enum {
	FGHI_CONTROL_IDXTYPE_UINT = 0,
	FGHI_CONTROL_IDXTYPE_USHORT,
	FGHI_CONTROL_IDXTYPE_UBYTE = 3
} fimgHostIndexType;

static inline void fimgHostWrite(unsigned int data, volatile unsigned int *reg)
{
	*reg = data;
}

static inline unsigned int fimgHostRead(volatile unsigned int *reg)
{
	return *reg;
}

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgGetNumEmptyFIFOSlots
 * SYNOPSIS:	This function obtains how many FIFO slots are empty in host interface
 * RETURNS:	the number of empty slots
 *****************************************************************************/
unsigned int fimgGetNumEmptyFIFOSlots(void)
{
	return fimgHostRead(FGHI_DWSPACE);
}

static inline void fimgDrawVertex(unsigned int numAttribs, fimgAttribute *pAttrib, void **ppvData, void **pConst, unsigned int *pStride, unsigned int i)
{
		for(j=0; j<numAttribs; j++) {
			switch(pAttrib[j].bits.dt) {
				// 1bytes
			case FGHI_ATTRIB_DT_BYTE:
			case FGHI_ATTRIB_DT_UBYTE:
			case FGHI_ATTRIB_DT_NBYTE:
			case FGHI_ATTRIB_DT_NUBYTE: {
				unsigned char bits[4] = {0, 0, 0, 0};

				for(n=0; n<pAttrib[j].bits.numcomp; n++) {
					if(ppData[j] != NULL)
						bits[n] = (ppData[j] + pStride[j] * i)[n];
					else
						bits[n] = ((unsigned char *)(pConst[j]))[n];
				}

				fimgSendToFIFO(4, bits);

				break;
			}
			// 2bytes
			case FGHI_ATTRIB_DT_SHORT:
			case FGHI_ATTRIB_DT_USHORT:
			case FGHI_ATTRIB_DT_NSHORT:
			case FGHI_ATTRIB_DT_NUSHORT: {
				unsigned short bits[4] = {0, 0, 0, 0};

				for(n=0; n<pAttrib[j].bits.numcomp; n++) {
					if(ppData[j] != NULL)
						bits[n] = ((unsigned short *)(ppData[j] + pStride[j] * i))[n];
					else
						bits[n] = ((unsigned short *)(pConst[j]))[n];
				}

				if(pAttrib[j].bits.numcomp > 2)
					fimgSendToFIFO(8, bits);
				else
					fimgSendToFIFO(4, bits);

				break;
			}
			// 4 bytes
			case FGHI_ATTRIB_DT_FIXED:
			case FGHI_ATTRIB_DT_NFIXED:
			case FGHI_ATTRIB_DT_FLOAT:
			case FGHI_ATTRIB_DT_INT:
			case FGHI_ATTRIB_DT_UINT:
			case FGHI_ATTRIB_DT_NINT:
			case FGHI_ATTRIB_DT_NUINT: {
				for(n=0; n<pAttrib[j].bits.numcomp; n++) {
					if(ppData[j] != NULL)
						fimgSendToFIFO(4, &((unsigned int *)(ppData[j] + pStride[j] * i))[n]);
					else
						fimgSendToFIFO(4, ((unsigned int *)(pConst[j]))[n])
				}

				break;
			}
			}
		}
}

/*****************************************************************************
 * FUNCTIONS:	fimgDrawNonIndexArrays
 * SYNOPSIS:	This function sends geometric data to rendering pipeline using non-index scheme.
 * PARAMETERS:	[IN] numAttribs: number of input attributes
 *		[IN] pAttrib: array of input attributes
 *		[IN] numVertices: number of vertices
 *		[IN] ppData: array of pointers of input data
 *		[IN] pConst: array of constant data
 *		[IN] stride: stride of input data
 *****************************************************************************/
void fimgDrawNonIndexArrays(unsigned int numAttribs, fimgAttribute *pAttrib, unsigned int numVertices, void **ppvData, void **pConst, unsigned int *pStride)
{
	unsigned int i, j, n;
	unsigned char **ppData;

	// write the property of input attributes
	for(i=0; i<numAttribs; i++)
		fimgSetAttribute(i, pAttrib[i]);

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(numAttribs, pAttrib, ppvData, pConst, pStride, i);
}

/*****************************************************************************
 * FUNCTIONS:	fimgDrawNonIndexArrays
 * SYNOPSIS:	This function sends geometric data to rendering pipeline using non-index scheme.
 * PARAMETERS:	[IN] numAttribs: number of input attributes
 *		[IN] pAttrib: array of input attributes
 *		[IN] numVertices: number of vertices
 *		[IN] ppData: array of pointers of input data
 *		[IN] pConst: array of constant data
 *		[IN] stride: stride of input data
 *****************************************************************************/
void fimgDrawArraysUByteIndex(unsigned int numAttribs, fimgAttribute *pAttrib, unsigned int numVertices, void **ppvData, void **pConst, unsigned int *pStride, const unsigned char *idx)
{
	unsigned int i, j, n;
	unsigned char **ppData;

	// write the property of input attributes
	for(i=0; i<numAttribs; i++)
		fimgSetAttribute(i, pAttrib[i]);

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(numAttribs, pAttrib, ppvData, pConst, pStride, idx[i]);
}

void fimgDrawArraysUShortIndex(unsigned int numAttribs, fimgAttribute *pAttrib, unsigned int numVertices, void **ppvData, void **pConst, unsigned int *pStride, const unsigned short *idx)
{
	unsigned int i, j, n;
	unsigned char **ppData;

	// write the property of input attributes
	for(i=0; i<numAttribs; i++)
		fimgSetAttribute(i, pAttrib[i]);

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(numAttribs, pAttrib, ppvData, pConst, pStride, idx[i]);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSendToFIFO
 * SYNOPSIS:	This function sends data to the 3D rendering pipeline
 * PARAMETERS:	[IN] buffer: pointer of input data
 *            	[IN] bytes: the total bytes of input data
 *****************************************************************************/
/* TODO: Use vertex buffer and interrupts */
void fimgSendToFIFO(unsigned int bytes, void *buffer)
{
	unsigned int nEmptySpace = 0;
	unsigned char *ptr = (unsigned char *)buffer;
	unsigned char bits[4] = {0, 0, 0, 0};
	int i;

	while(bytes >= 4) {
		while(!nEmptySpace)
			nEmptySpace = fimgHostRead(FGHI_DWSPACE);

		fimgHostWrite(*(ptr++), FGHI_FIFO_ENTRY);
		nEmptySpace--;
		bytes--;
	}

	if(bytes) {
		while(!nEmptySpace)
			nEmptySpace = fimgHostRead(FGHI_DWSPACE);

		for(i = 0; i < bytes; i++)
			bits[i] = *(ptr++);

		fimgHostWrite(*((unsigned int *)bits), FGHI_FIFO_ENTRY);
	}
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetHInterface
 * SYNOPSIS:	This function defines whether vertex buffer, vertex cache and
 *		auto increment index scheme is active or not.
 *		It also defines data type of transfered index and
 *		the number of output attributes of vertex shader.
 * PARAMETERS:	[IN] HI: fimgHInterface
 *****************************************************************************/
void fimgSetHInterface(fimgHInterface HI)
{
	fimgHostWrite(HI.val, FGHI_CONTROL);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetIndexOffset
 * SYNOPSIS:	This function defines index offset which is used in the auto increment mode
 * PARAMETERS:	[IN] offset: index offset value
 *****************************************************************************/
void fimgSetIndexOffset(unsigned int offset)
{
	fimgHostWrite(offset, FGHI_IDXOFFSET);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufferAddr
 * SYNOPSIS:	This function defines the starting address in vertex buffer,
 *		which are used to send data into vertex buffer
 * PARAMETERS:	[IN] address: the starting address
 *****************************************************************************/
void fimgSetVtxBufferAddr(unsigned int addr)
{
	fimgHostWrite(addr, FGHI_VBADDR);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSendToVtxBuffer
 * SYNOPSIS:	The function copies data into vertex buffer.
 * PARAMETERS:	[IN] data: data issued into vertex buffer
 *****************************************************************************/
void fimgSendToVtxBuffer(unsigned int data)
{
	fimgHostWrite(data, FGHI_VB_ENTRY);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetAttribute
 * SYNOPSIS:	This function specifies the property of attribute
 * PARAMETERS:	[IN] attribIdx: the index of attribute, which is in [0-15]
 *		[IN] pAttribInfo: fimgAttribute
 *****************************************************************************/
void fimgSetAttribute(unsigned char attribIdx, fimgAttribute AttribInfo)
{
	fimgHostWrite(AttribInfo.val, FGHI_ATTRIB(attribIdx));
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufAttrib
 * SYNOPSIS:	This function defines the property of attribute in vertex buffer
 * PARAMETERS:	[IN] attribIdx: the index of attribute
 *            	[IN] pAttribInfo: fimgVtxBufAttrib
 *****************************************************************************/
void fimgSetVtxBufAttrib(unsigned char attribIdx, fimgVtxBufAttrib AttribInfo)
{
	fimgHostWrite(AttribInfo.base.val, FGHI_ATTRIB_VBBASE(attribIdx));
	fimgHostWrite(AttribInfo.ctrl.val, FGHI_ATTRIB_VBCTRL(attribIdx));
}
