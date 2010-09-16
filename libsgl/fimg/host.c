/*
 * fimg/host.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2005 by Samsung Electronics Limited (original code)
 *		2010 by Tomasz Figa <tomasz.figa@gmail.com> (new code)
 */

#include <stdio.h>
#include "fimg_private.h"

#define FGHI_FIFO_SIZE		32

#define FGHI_DWSPACE		0x8000
#define FGHI_FIFO_ENTRY		0xc000
#define FGHI_CONTROL		0x8008
#define FGHI_IDXOFFSET		0x800c
#define FGHI_VBADDR		0x8010
#define FGHI_VB_ENTRY		0xe000

#define ATTRIB_NUM		10
#define FGHI_ATTRIB(i)		(0x8040 + 4*(i))
#define FGHI_ATTRIB_VBCTRL(i)	(0x8080 + 4*(i))
#define FGHI_ATTRIB_VBBASE(i)	(0x80c0 + 4*(i))

typedef enum {
	FGHI_CONTROL_IDXTYPE_UINT = 0,
	FGHI_CONTROL_IDXTYPE_USHORT,
	FGHI_CONTROL_IDXTYPE_UBYTE = 3
} fimgHostIndexType;

/* TODO: Function inlining */

/*****************************************************************************
 * FUNCTIONS:	fimgGetNumEmptyFIFOSlots
 * SYNOPSIS:	This function obtains how many FIFO slots are empty in host interface
 * RETURNS:	the number of empty slots
 *****************************************************************************/
unsigned int fimgGetNumEmptyFIFOSlots(fimgContext *ctx)
{
	return fimgRead(ctx, FGHI_DWSPACE);
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
			unsigned int word = 0;
			unsigned char *bits = (unsigned char *)&word;

			for(n = 0; n <= ctx->host.attrib[j].bits.numcomp; n++)
				bits[n] = (ppData[j] + i*pStride[j])[n];

			fimgSendToFIFO(ctx, 4, &word);

			break; }
		// 2bytes
		case FGHI_ATTRIB_DT_SHORT:
		case FGHI_ATTRIB_DT_USHORT:
		case FGHI_ATTRIB_DT_NSHORT:
		case FGHI_ATTRIB_DT_NUSHORT: {
			unsigned int word[2] = {0, 0};
			unsigned short *bits = (unsigned short *)word;

			for(n = 0; n <= ctx->host.attrib[j].bits.numcomp; n++)
				bits[n] = ((unsigned short *)(ppData[j] + i*pStride[j]))[n];

			fimgSendToFIFO(ctx, ((ctx->host.attrib[j].bits.numcomp + 2) / 2) * 4, word);

			break; }
		// 4 bytes
		case FGHI_ATTRIB_DT_FIXED:
		case FGHI_ATTRIB_DT_NFIXED:
		case FGHI_ATTRIB_DT_FLOAT:
		case FGHI_ATTRIB_DT_INT:
		case FGHI_ATTRIB_DT_UINT:
		case FGHI_ATTRIB_DT_NINT:
		case FGHI_ATTRIB_DT_NUINT:
			fimgSendToFIFO(ctx, 4 * (ctx->host.attrib[j].bits.numcomp + 1), (const unsigned int *)(ppData[j] + i*pStride[j]));
			break;
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
void fimgDrawNonIndexArrays(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = numVertices;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=first; i<first+numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, i);
}

#ifdef FIMG_INTERPOLATION_WORKAROUND
void fimgDrawNonIndexArraysPoints(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 1;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=first; i<first+numVertices; i++) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i);
	}
}

void fimgDrawNonIndexArraysLines(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 2;
	words[1] = 0xffffffff;

	for(i=first; i<first+numVertices-1; i+=2) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i);
		fimgDrawVertex(ctx, ppData, pStride, i+1);
	}
}

void fimgDrawNonIndexArraysLineStrips(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 2;
	words[1] = 0xffffffff;

	for(i=first; i<first+numVertices-1; i++) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i);
		fimgDrawVertex(ctx, ppData, pStride, i+1);
	}
}

void fimgDrawNonIndexArraysLineLoops(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 2;
	words[1] = 0xffffffff;

	for(i=first; i<first+numVertices-1; i++) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i);
		fimgDrawVertex(ctx, ppData, pStride, i+1);
	}

	fimgSendToFIFO(ctx, 8, words);
	fimgDrawVertex(ctx, ppData, pStride, first+numVertices-1);
	fimgDrawVertex(ctx, ppData, pStride, first);
}

void fimgDrawNonIndexArraysTriangles(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 3;
	words[1] = 0xffffffff;

	for(i=first; i<first+numVertices-2; i+=3) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i);
		fimgDrawVertex(ctx, ppData, pStride, i+1);
		fimgDrawVertex(ctx, ppData, pStride, i+2);
	}
}

void fimgDrawNonIndexArraysTriangleFans(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 3;
	words[1] = 0xffffffff;

	for(i=first+1; i<first+numVertices-1; i++) {
		fimgFlush(ctx);
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, first);
		fimgDrawVertex(ctx, ppData, pStride, i);
		fimgDrawVertex(ctx, ppData, pStride, i+1);
	}
}

void fimgDrawNonIndexArraysTriangleStrips(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];
	const unsigned lookup[2][3] = { {0,1,2},{1,0,2} };

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = 3;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=first; i<first+numVertices-2; i++) {
		fimgSendToFIFO(ctx, 8, words);
		fimgDrawVertex(ctx, ppData, pStride, i + lookup[i & 1][0]);
		fimgDrawVertex(ctx, ppData, pStride, i + lookup[i & 1][1]);
		fimgDrawVertex(ctx, ppData, pStride, i + lookup[i & 1][2]);
	}
}
#else
#ifdef FIMG_CLIPPER_WORKAROUND
void fimgDrawNonIndexArraysTriangleFans(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = numVertices + 2;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	/* WORKAROUND */
	fimgDrawVertex(ctx, ppData, pStride, 0);
	fimgDrawVertex(ctx, ppData, pStride, 0);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, i);
}

void fimgDrawNonIndexArraysTriangleStrips(fimgContext *ctx, unsigned int first, unsigned int numVertices, const void **ppvData, unsigned int *pStride)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	// write the number of vertices
	words[0] = numVertices + 1;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, i);

	/* WORKAROUND */
	fimgDrawVertex(ctx, ppData, pStride, numVertices - 1);
}
#endif
#endif

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
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

	// write the number of vertices
	words[0] = numVertices;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, idx[i]);
}

void fimgDrawArraysUShortIndex(fimgContext *ctx, unsigned int numVertices, const void **ppvData, unsigned int *pStride, const unsigned short *idx)
{
	unsigned int i;
	const unsigned char **ppData = (const unsigned char **)ppvData;
	fimgAttribute last;
	unsigned int words[2];

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.bits.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

	// write the number of vertices
	words[0] = numVertices;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, idx[i]);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSendToFIFO
 * SYNOPSIS:	This function sends data to the 3D rendering pipeline
 * PARAMETERS:	[IN] buffer: pointer of input data
 *            	[IN] bytes: the total bytes count of input data
 *****************************************************************************/
/* TODO: Use vertex buffer */
inline void fimgSendToFIFO(fimgContext *ctx, unsigned int count, const unsigned int *ptr)
{
	unsigned int nEmptySpace = 0;

	// We're sending only full words
	count /= 4;

	// Transfer words to the FIFO
	while(count--) {
#if 0
		/* The CPU is ours! Don't let it go!!!!! */
		while(!nEmptySpace)
			nEmptySpace = fimgRead(ctx, FGHI_DWSPACE);
#else
		/* Be more system friendly and share the CPU. */
		if(!nEmptySpace) {
			nEmptySpace = fimgRead(ctx, FGHI_DWSPACE);
			if(!nEmptySpace) {
				fimgWaitForFlush(ctx, 1);
				nEmptySpace = fimgRead(ctx, FGHI_DWSPACE);
			}
		}
#endif
		fimgWrite(ctx, *(ptr++), FGHI_FIFO_ENTRY);
		nEmptySpace--;
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
	fimgWrite(ctx, HI.val, FGHI_CONTROL);
}

void fimgSetAttribCount(fimgContext *ctx, unsigned char count)
{
#ifdef FIMG_INTERPOLATION_WORKAROUND
	ctx->host.control.bits.numoutattrib = 9;
#else
	ctx->host.control.bits.numoutattrib = count;
#endif
	fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetIndexOffset
 * SYNOPSIS:	This function defines index offset which is used in the auto increment mode
 * PARAMETERS:	[IN] offset: index offset value
 *****************************************************************************/
void fimgSetIndexOffset(fimgContext *ctx, unsigned int offset)
{
	ctx->host.indexOffset = offset;
	fimgWrite(ctx, offset, FGHI_IDXOFFSET);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufferAddr
 * SYNOPSIS:	This function defines the starting address in vertex buffer,
 *		which are used to send data into vertex buffer
 * PARAMETERS:	[IN] address: the starting address
 *****************************************************************************/
void fimgSetVtxBufferAddr(fimgContext *ctx, unsigned int addr)
{
	fimgWrite(ctx, addr, FGHI_VBADDR);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSendToVtxBuffer
 * SYNOPSIS:	The function copies data into vertex buffer.
 * PARAMETERS:	[IN] data: data issued into vertex buffer
 *****************************************************************************/
void fimgSendToVtxBuffer(fimgContext *ctx, unsigned int data)
{
	fimgWrite(ctx, data, FGHI_VB_ENTRY);
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
	ctx->host.attrib[idx].bits.numcomp = FGHI_NUMCOMP(numComp);
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

void fimgCreateHostContext(fimgContext *ctx)
{
	int i;

	ctx->host.control.bits.autoinc = 1;
	ctx->host.indexOffset = 1;

	for(i = 0; i < FIMG_ATTRIB_NUM; i++) {
		ctx->host.attrib[i].bits.srcw = 3;
		ctx->host.attrib[i].bits.srcz = 2;
		ctx->host.attrib[i].bits.srcy = 1;
		ctx->host.attrib[i].bits.srcx = 0;
	}
}

void fimgRestoreHostState(fimgContext *ctx)
{
	fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
	fimgWrite(ctx, ctx->host.indexOffset, FGHI_IDXOFFSET);
}
