/*
 * fimg/host.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2005 by Samsung Electronics Limited (original code)
 *		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
 */

#include "fimg_private.h"

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

static inline void fimgDrawVertex(fimgContext *ctx, const unsigned char **ppData, unsigned int *pStride, unsigned int i)
{
	unsigned int j, n;

	for(j = 0; j < ctx->numAttribs; j++) {
		switch(ctx->host.attrib[j].bits.dt) {
			// 1bytes
		case FGHI_ATTRIB_DT_BYTE:
		case FGHI_ATTRIB_DT_UBYTE:
		case FGHI_ATTRIB_DT_NBYTE:
		case FGHI_ATTRIB_DT_NUBYTE: {
			unsigned char bits[4] = {0, 0, 0, 0};

			for(n = 0; n < ctx->host.attrib[j].bits.numcomp; n++)
				bits[n] = (ppData[j] + pStride[j] * i)[n];

			fimgSendToFIFO(4, bits);

			break;
		}
		// 2bytes
		case FGHI_ATTRIB_DT_SHORT:
		case FGHI_ATTRIB_DT_USHORT:
		case FGHI_ATTRIB_DT_NSHORT:
		case FGHI_ATTRIB_DT_NUSHORT: {
			unsigned short bits[4] = {0, 0, 0, 0};

			for(n = 0; n < ctx->host.attrib[j].bits.numcomp; n++)
				bits[n] = ((unsigned short *)(ppData[j] + pStride[j] * i))[n];

			if(ctx->host.attrib[j].bits.numcomp > 2)
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
			for(n = 0; n < ctx->host.attrib[j].bits.numcomp; n++)
				fimgSendToFIFO(4, &((unsigned int *)(ppData[j] + pStride[j] * i))[n]);

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
void fimgDrawNonIndexArrays(fimgContext *ctx, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgHostWrite(ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.bits.lastattr = 1;
	fimgHostWrite(last.val, FGHI_ATTRIB(i));

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, i);
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
void fimgDrawArraysUByteIndex(fimgContext *ctx, unsigned int numVertices, const void **ppvData, unsigned int *pStride, const unsigned char *idx)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgHostWrite(ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.bits.lastattr = 1;
	fimgHostWrite(last.val, FGHI_ATTRIB(i));

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, idx[i]);
}

void fimgDrawArraysUShortIndex(fimgContext *ctx, unsigned int numVertices, const void **ppvData, unsigned int *pStride, const unsigned short *idx)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgHostWrite(ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.bits.lastattr = 1;
	fimgHostWrite(last.val, FGHI_ATTRIB(i));

	// write the number of vertices
	fimgHostWrite(numVertices, FGHI_FIFO_ENTRY);
	fimgHostWrite(0xFFFFFFFF, FGHI_FIFO_ENTRY);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, idx[i]);
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
	unsigned int i;

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
void fimgSetHInterface(fimgContext *ctx, fimgHInterface HI)
{
	ctx->host.control = HI;
	fimgHostWrite(HI.val, FGHI_CONTROL);
}

void fimgSetAttribCount(fimgContext *ctx, unsigned char count)
{
	ctx->host.control.bits.numoutattrib = count;
	fimgHostWrite(ctx->host.control.val, FGHI_CONTROL);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetIndexOffset
 * SYNOPSIS:	This function defines index offset which is used in the auto increment mode
 * PARAMETERS:	[IN] offset: index offset value
 *****************************************************************************/
void fimgSetIndexOffset(fimgContext *ctx, unsigned int offset)
{
	ctx->host.indexOffset = offset;
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
void fimgSetAttribute(fimgContext *ctx, unsigned int idx, unsigned int type, unsigned int numComp)
{
	ctx->host.attrib[idx].bits.dt = type;
	ctx->host.attrib[idx].bits.numcomp = numComp;
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufAttrib
 * SYNOPSIS:	This function defines the property of attribute in vertex buffer
 * PARAMETERS:	[IN] attribIdx: the index of attribute
 *            	[IN] pAttribInfo: fimgVtxBufAttrib
 *****************************************************************************/
void fimgSetVtxBufAttrib(fimgContext *ctx, unsigned char idx, unsigned short base, unsigned char stride, unsigned short range)
{
	ctx->host.bufAttrib[idx].base = base;
	ctx->host.bufAttrib[idx].ctrl.bits.stride = stride;
	ctx->host.bufAttrib[idx].ctrl.bits.range = range;
}

void fimgRestoreHostState(fimgContext *ctx)
{
	fimgHostWrite(ctx->host.control.val, FGHI_CONTROL);
	fimgHostWrite(ctx->host.indexOffset, FGHI_IDXOFFSET);
}
