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
static inline uint32_t fimgFIFOSlotsAvail(fimgContext *ctx)
{
	return fimgRead(ctx, FGHI_DWSPACE);
}

static inline void fimgSetHostInterface(fimgContext *ctx, int vb, int autoinc)
{
	ctx->host.control.envb		= !!vb;
	ctx->host.control.autoinc	= !!autoinc;

	fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
}

/*
 * UNBUFFERED
 */

/*****************************************************************************
 * FUNCTIONS:	fimgSendToFIFO
 * SYNOPSIS:	This function sends data to the 3D rendering pipeline
 * PARAMETERS:	[IN] buffer: pointer of input data
 *            	[IN] bytes: the total bytes count of input data
 *****************************************************************************/
static inline void fimgSendToFIFO(fimgContext *ctx, unsigned int count, const unsigned int *ptr)
{
	unsigned int nEmptySpace = 0;

	// We're sending only full words
	count /= 4;

	// Transfer words to the FIFO
	while(count--) {
#ifdef FIMG_FIFO_BUSY_WAIT
		/* The CPU is ours! Don't let it go!!!!! */
		while(!nEmptySpace)
			nEmptySpace = fimgFIFOSlotsAvail(ctx);
#else
		/* Be more system friendly and share the CPU. */
		if(!nEmptySpace) {
			nEmptySpace = fimgFIFOSlotsAvail(ctx);
			if(!nEmptySpace) {
				fimgWaitForFlush(ctx, 1);
				nEmptySpace = fimgFIFOSlotsAvail(ctx);
			}
		}
#endif
		fimgWrite(ctx, *(ptr++), FGHI_FIFO_ENTRY);
		nEmptySpace--;
	}
}

static inline void fimgDrawVertex(fimgContext *ctx, const unsigned char **ppData, unsigned int *pStride, unsigned int i)
{
	unsigned int j, n;

	for(j = 0; j < ctx->numAttribs; j++) {
		switch(ctx->host.attrib[j].dt) {
			// 1bytes
		case FGHI_ATTRIB_DT_BYTE:
		case FGHI_ATTRIB_DT_UBYTE:
		case FGHI_ATTRIB_DT_NBYTE:
		case FGHI_ATTRIB_DT_NUBYTE: {
			unsigned int word = 0;
			unsigned char *bits = (unsigned char *)&word;

			for(n = 0; n <= ctx->host.attrib[j].numcomp; n++)
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

			for(n = 0; n <= ctx->host.attrib[j].numcomp; n++)
				bits[n] = ((unsigned short *)(ppData[j] + i*pStride[j]))[n];

			fimgSendToFIFO(ctx, ((ctx->host.attrib[j].numcomp + 2) / 2) * 4, word);

			break; }
		// 4 bytes
		case FGHI_ATTRIB_DT_FIXED:
		case FGHI_ATTRIB_DT_NFIXED:
		case FGHI_ATTRIB_DT_FLOAT:
		case FGHI_ATTRIB_DT_INT:
		case FGHI_ATTRIB_DT_UINT:
		case FGHI_ATTRIB_DT_NINT:
		case FGHI_ATTRIB_DT_NUINT:
			fimgSendToFIFO(ctx, 4 * (ctx->host.attrib[j].numcomp + 1), (const unsigned int *)(ppData[j] + i*pStride[j]));
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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

 fimgSetHostInterface(ctx, 0, 1);

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
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

 fimgSetHostInterface(ctx, 0, 1);

	// write the number of vertices
	words[0] = numVertices;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 8, words);

	for(i=0; i<numVertices; i++)
		fimgDrawVertex(ctx, ppData, pStride, idx[i]);
}

/*
 * BUFFERED
 */

#define FGHI_MAX_ATTRIBS		8
#define FGHI_VERTICES_PER_VB_ATTRIB	16
#define FGHI_MAX_BYTES_PER_VERTEX	16
#define FGHI_MAX_BYTES_PER_ATTRIB	(FGHI_VERTICES_PER_VB_ATTRIB)*(FGHI_MAX_BYTES_PER_VERTEX)
#define FGHI_VBADDR_ATTRIB(attrib, buf)	((2*attrib + ((buf)&1))*(FGHI_MAX_BYTES_PER_ATTRIB))

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufferAddr
 * SYNOPSIS:	This function defines the starting address in vertex buffer,
 *		which are used to send data into vertex buffer
 * PARAMETERS:	[IN] address: the starting address
 *****************************************************************************/
static inline void fimgSetVtxBufferAddr(fimgContext *ctx, uint32_t addr)
{
//	printf("< %08x\n", addr);
	fimgWrite(ctx, addr, FGHI_VBADDR);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSendToVtxBuffer
 * SYNOPSIS:	The function copies data into vertex buffer.
 * PARAMETERS:	[IN] data: data issued into vertex buffer
 *****************************************************************************/
static inline void fimgSendToVtxBuffer(fimgContext *ctx, uint32_t data)
{
//	printf("%08x\n", data);
//	printf("> %08x\n", fimgRead(ctx, FGHI_VBADDR));
	fimgWrite(ctx, data, FGHI_VB_ENTRY);
}

static inline void fimgPadVertexBuffer(fimgContext *ctx)
{
	uint32_t val;

	val = (fimgRead(ctx, FGHI_VBADDR) % 16) / 4;

	if (val) {
		val = 4 - val;

		while (val--)
			fimgSendToVtxBuffer(ctx, 0);
	}
}

static inline void fimgSetupAttributes(fimgContext *ctx, fimgArray *arrays)
{
	fimgVtxBufAttrib vbattr;
	fimgAttribute last;
	fimgArray *a;
	unsigned int i;

	vbattr.val = 0;
	vbattr.range = 2*FGHI_VERTICES_PER_VB_ATTRIB;

	// write attribute configuration
	for (a = arrays, i = 0; i < ctx->numAttribs - 1; i++, a++) {
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));

		if (a->stride == 0)
#ifdef FIMG_USE_STRIDE_0_CONSTANTS
			// Stride = 0 optimization
			vbattr.stride = 0;
#else
			vbattr.stride = (a->width + 3) & ~3;
#endif
		else if ((a->stride % 4) || (a->stride > 16) || ((unsigned long)a->pointer % 4))
			vbattr.stride = (a->width + 3) & ~3;
		else
			vbattr.stride = a->stride;

		fimgWrite(ctx, vbattr.val, FGHI_ATTRIB_VBCTRL(i));
	}

	// write the last one
	last = ctx->host.attrib[i];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

	if (a->stride == 0)
#ifdef FIMG_USE_STRIDE_0_CONSTANTS
		// Stride = 0 optimization
		vbattr.stride = 0;
#else
		vbattr.stride = (a->width + 3) & ~3;
#endif
	else if ((a->stride % 4) || (a->stride > 16) || ((unsigned long)a->pointer % 4))
		vbattr.stride = (a->width + 3) & ~3;
	else
		vbattr.stride = a->stride;

	fimgWrite(ctx, vbattr.val, FGHI_ATTRIB_VBCTRL(i));
}

/*
 * AUTOINDEXED
 */

static inline void fimgFillVertexBuffer(fimgContext *ctx,
					fimgArray *a, uint32_t cnt)
{
	register uint32_t word;

//	printf("%s\n", __func__);

#ifdef FIMG_USE_STRIDE_0_CONSTANTS
	// Stride = 0 optimization
	cnt = 1;
#endif

	switch(a->width % 4) {
	// words
	case 0:
		// Check if vertices are word aligned
		if ((unsigned long)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			uint32_t len;
			const uint32_t *data;

			while (cnt--) {
				data = (const uint32_t *)a->pointer;
				len = a->width;

				while (len) {
					fimgSendToVtxBuffer(ctx, *(data++));
					len -= 4;
				}
			}

			break;
		}
	// halfwords
	case 2:
		// Check if vertices are halfword aligned
		if ((unsigned long)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len;
			const uint16_t *data;

			while (cnt--) {
				data = (const uint16_t *)a->pointer;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgSendToVtxBuffer(ctx, *(data++));
			}
			break;
		}
	// bytes
	default:
		// Fallback - no check required
		{
			uint32_t len;
			const uint8_t *data;

			while (cnt--) {
				data = (const uint8_t *)a->pointer;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgSendToVtxBuffer(ctx, word);
				}
			}
			break;
		}
	}
}

static inline void fimgPackToVertexBuffer(fimgContext *ctx,
				fimgArray *a, uint32_t pos, uint32_t cnt)
{
	register uint32_t word;

//	printf("%s\n", __func__);

	switch(a->width % 4) {
	// words
	case 0:
		// Check if vertices are word aligned
		if ((unsigned long)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			uint32_t len, srcpad = (a->stride - a->width) / 4;
			const uint32_t *data = (const uint32_t *)((const uint8_t *)a->pointer + pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len) {
					fimgSendToVtxBuffer(ctx, *(data++));
					len -= 4;
				}

				data += srcpad;
			}

			break;
		}
	// halfwords
	case 2:
		// Check if vertices are halfword aligned
		if ((unsigned long)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len, srcpad = (a->stride - a->width) / 2;
			const uint16_t *data = (const uint16_t *)((const uint8_t *)a->pointer + pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgSendToVtxBuffer(ctx, *(data++));

				data += srcpad;
			}
			break;
		}
	// bytes
	default:
		// Fallback - no check required
		{
			uint32_t len, srcpad = a->stride - a->width;
			const uint8_t *data = (const uint8_t *)a->pointer + pos*a->stride;

			while (cnt--) {
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgSendToVtxBuffer(ctx, word);
				}

				data += srcpad;
			}
			break;
		}
	}
}

static inline void fimgCopyToVertexBuffer(fimgContext *ctx,
				fimgArray *a, uint32_t pos, uint32_t cnt)
{
	uint32_t len;
	const uint32_t *data = (const uint32_t *)((const uint8_t *)a->pointer + pos*a->stride);

//	printf("%s\n", __func__);

	len = a->stride * cnt;
	len /= 4;

	// Try to copy in bursts of 4 words
	while (len >= 4) {
		fimgSendToVtxBuffer(ctx, *(data++));
		fimgSendToVtxBuffer(ctx, *(data++));
		fimgSendToVtxBuffer(ctx, *(data++));
		fimgSendToVtxBuffer(ctx, *(data++));
		len -= 4;
	}

	while (len--)
		fimgSendToVtxBuffer(ctx, *(data++));
}

static inline void fimgLoadVertexBuffer(fimgContext *ctx, fimgArray *a,
				uint32_t pos, uint32_t cnt)
{
	if (a->stride == 0)
		// Fill the buffer with a constant
		fimgFillVertexBuffer(ctx, a, cnt);
	else if ((a->stride % 4) || (a->stride > 16) || ((unsigned long)a->pointer % 4))
		// Pack attribute data and store in the buffer
		fimgPackToVertexBuffer(ctx, a, pos, cnt);
	else
		// Copy packed attribute data to the buffer
		fimgCopyToVertexBuffer(ctx, a, pos, cnt);
}

static inline void fimgDrawAutoinc(fimgContext *ctx,
				uint32_t first, uint32_t count)
{
	unsigned int words[2];

	words[0] = count;
	words[1] = first;

	fimgSendToFIFO(ctx, 8, words);
}

#if 0
/* For points, lines and triangles */
void fimgDrawArraysBufferedSeparate(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = first;
	fimgArray *a;
	uint32_t buf = 0;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetupAttributes(ctx, arrays);

	while (count >= FGHI_VERTICES_PER_VB_ATTRIB) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Points don't need to repeat anything
		count -= FGHI_VERTICES_PER_VB_ATTRIB;
		pos += FGHI_VERTICES_PER_VB_ATTRIB;

		// Switch the buffer
		buf ^= 1;
	}

	if (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, count);
			fimgPadVertexBuffer(ctx);
		}

		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, count);
	}
}

/* For line strips */
void fimgDrawArraysBufferedRepeatLast(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = first;
	fimgArray *a;
	uint32_t buf = 0;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetupAttributes(ctx, arrays);

	count--;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 1) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 1;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 1;

		// Switch the buffer
		buf ^= 1;
	}

	if (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx);
		}

		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, 1 + count);
	}
}

/* For line loops */
void fimgDrawArraysBufferedRepeatLastLoop(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = first;
	fimgArray *a;
	uint32_t buf = 0;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetupAttributes(ctx, arrays);

	count--;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 1) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 1;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 1;

		// Switch the buffer
		buf ^= 1;
	}

	if (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, 1 + count);

		buf ^= 1;
	}

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
		fimgLoadVertexBuffer(ctx, a, 0, 1);
		fimgLoadVertexBuffer(ctx, a, count - 1, 1);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, 2);
}

/* For triangle strips */
void fimgDrawArraysBufferedRepeatLastTwo(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = first;
	fimgArray *a;
	uint32_t buf = 0;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetupAttributes(ctx, arrays);

	count -= 2;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 2) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos - (pos % 2), 1);
			fimgLoadVertexBuffer(ctx, a, pos - 1 + (pos % 2), 1);
			fimgLoadVertexBuffer(ctx, a, pos + 2, FGHI_VERTICES_PER_VB_ATTRIB - 2);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last two vertices per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 2;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 2;

		// Switch the buffer
		buf ^= 1;
	}

	if (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos - (pos % 2), 1);
			fimgLoadVertexBuffer(ctx, a, pos - 1 + (pos % 2), 1);
			fimgLoadVertexBuffer(ctx, a, pos + 2, count);
			fimgPadVertexBuffer(ctx);
		}

		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, 2 + count);
	}
}

/* For triangle fans */
void fimgDrawArraysBufferedRepeatFirstLast(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = first + 1;
	fimgArray *a;
	uint32_t buf = 0;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetupAttributes(ctx, arrays);

	count -= 2;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 2) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, first, 1);
			fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB - 1);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat first and last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 2;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 2;

		// Switch the buffer
		buf ^= 1;
	}

	if (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, first, 1);
			fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx);
		}

		fimgDrawAutoinc(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, 2 + count);
	}
}
#endif

static inline void fimgDrawArraysBufferedAutoinc(fimgContext *ctx,
		fimgArray *arrays, unsigned int first, unsigned int count)
{
	unsigned i, pos = first;
	fimgArray *a;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetupAttributes(ctx, arrays);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBuffer(ctx, a, pos, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

/*
 * INDEXED BY CPU
 */

static inline void fimgSendIndices(fimgContext *ctx,
					uint32_t first, uint32_t count)
{
	fimgWrite(ctx, count, FGHI_FIFO_ENTRY);

	while (count--)
		fimgWrite(ctx, first++, FGHI_FIFO_ENTRY);
}

void fimgDrawArraysBuffered(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, pos = first;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawArraysBufferedAutoinc(ctx, arrays, first, count);
		return;
	}

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetupAttributes(ctx, arrays);

	alignment = count % FGHI_VERTICES_PER_VB_ATTRIB;

	if (alignment) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
			fimgLoadVertexBuffer(ctx, a, pos, alignment);
			fimgPadVertexBuffer(ctx);
		}

		fimgSendIndices(ctx, 0, alignment);

		count -= alignment;
		pos += alignment;

		// Switch the buffer
		buf ^= 1;
	}

	while (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgSendIndices(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		count -= FGHI_VERTICES_PER_VB_ATTRIB;
		pos += FGHI_VERTICES_PER_VB_ATTRIB;

		// Switch the buffer
		buf ^= 1;
	}
}

/*
 * DRAW ELEMENTS
 */

static inline void fimgPackToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	register uint32_t word;

//	printf("%s\n", __func__);

	switch(a->width % 4) {
	// words
	case 0:
		// Check if vertices are word aligned
		if ((unsigned long)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			uint32_t len;
			const uint32_t *data;

			while (cnt--) {
				data = (const uint32_t *)((const uint8_t *)a->pointer + *(idx++)*a->stride);
				len = a->width;

				while (len) {
					fimgSendToVtxBuffer(ctx, *(data++));
					len -= 4;
				}
			}

			break;
		}
	// halfwords
	case 2:
		// Check if vertices are halfword aligned
		if ((unsigned long)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len;
			const uint16_t *data;

			while (cnt--) {
				data = (const uint16_t *)((const uint8_t *)a->pointer + *(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgSendToVtxBuffer(ctx, *(data++));
			}
			break;
		}
	// bytes
	default:
		// Fallback - no check required
		{
			uint32_t len;
			const uint8_t *data;

			while (cnt--) {
				data = (const uint8_t *)a->pointer + *(idx++)*a->stride;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgSendToVtxBuffer(ctx, word);
				}
			}
			break;
		}
	}
}

static inline void fimgCopy4ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		cnt -= 4;
	}

	while (cnt--)
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
}

static inline void fimgCopy8ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
	}
}

static inline void fimgCopy12ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
	}
}

static inline void fimgCopy16ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
	}
}

static inline void fimgLoadVertexBufferUByteIdx(fimgContext *ctx, fimgArray *a,
				const uint8_t *indices, uint32_t cnt)
{
	switch (a->stride) {
	case 0:
		fimgFillVertexBuffer(ctx, a, cnt);
		return;
	case 4:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy4ToVertexBufferUByteIdx(ctx, a, indices, cnt);
		return;
	case 8:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy8ToVertexBufferUByteIdx(ctx, a, indices, cnt);
		return;
	case 12:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy12ToVertexBufferUByteIdx(ctx, a, indices, cnt);
		return;
	case 16:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy16ToVertexBufferUByteIdx(ctx, a, indices, cnt);
		return;
	}

	fimgPackToVertexBufferUByteIdx(ctx, a, indices, cnt);
}

static inline void fimgDrawElementsBufferedAutoincUByteIdx(fimgContext *ctx,
		fimgArray *arrays, unsigned int count, const uint8_t *indices)
{
	unsigned i;
	fimgArray *a;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetupAttributes(ctx, arrays);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBufferUByteIdx(ctx, a, indices, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

void fimgDrawElementsBufferedUByteIdx(fimgContext *ctx, fimgArray *arrays,
				unsigned int count, const uint8_t *indices)
{
	unsigned i;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawElementsBufferedAutoincUByteIdx(ctx, arrays,
							count, indices);
		return;
	}

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetupAttributes(ctx, arrays);

	alignment = count % FGHI_VERTICES_PER_VB_ATTRIB;

	if (alignment) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
			fimgLoadVertexBufferUByteIdx(ctx, a, indices, alignment);
			fimgPadVertexBuffer(ctx);
		}

		fimgSendIndices(ctx, 0, alignment);

		count -= alignment;
		indices += alignment;

		// Switch the buffer
		buf ^= 1;
	}

	while (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBufferUByteIdx(ctx, a, indices, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgSendIndices(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		count -= FGHI_VERTICES_PER_VB_ATTRIB;
		indices += FGHI_VERTICES_PER_VB_ATTRIB;

		// Switch the buffer
		buf ^= 1;
	}
}

static inline void fimgPackToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	register uint32_t word;

//	printf("%s\n", __func__);

	switch(a->width % 4) {
	// words
	case 0:
		// Check if vertices are word aligned
		if ((unsigned long)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			uint32_t len;
			const uint32_t *data;

			while (cnt--) {
				data = (const uint32_t *)((const uint8_t *)a->pointer + *(idx++)*a->stride);
				len = a->width;

				while (len) {
					fimgSendToVtxBuffer(ctx, *(data++));
					len -= 4;
				}
			}

			break;
		}
	// halfwords
	case 2:
		// Check if vertices are halfword aligned
		if ((unsigned long)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len;
			const uint16_t *data;

			while (cnt--) {
				data = (const uint16_t *)((const uint8_t *)a->pointer + *(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgSendToVtxBuffer(ctx, *(data++));
			}
			break;
		}
	// bytes
	default:
		// Fallback - no check required
		{
			uint32_t len;
			const uint8_t *data;

			while (cnt--) {
				data = (const uint8_t *)a->pointer + *(idx++)*a->stride;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					fimgSendToVtxBuffer(ctx, word);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgSendToVtxBuffer(ctx, word);
				}
			}
			break;
		}
	}
}

static inline void fimgCopy4ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
		cnt -= 4;
	}

	while (cnt--)
		fimgSendToVtxBuffer(ctx, data[*(idx++)]);
}

static inline void fimgCopy8ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 1]);
	}
}

static inline void fimgCopy12ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 2]);
	}
}

static inline void fimgCopy16ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const uint32_t *data = (const uint32_t *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 1]);
		fimgSendToVtxBuffer(ctx, data[*(idx) + 2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++) + 3]);
	}
}

static inline void fimgLoadVertexBufferUShortIdx(fimgContext *ctx, fimgArray *a,
				const uint16_t *indices, uint32_t cnt)
{
	switch (a->stride) {
	case 0:
		fimgFillVertexBuffer(ctx, a, cnt);
		return;
	case 4:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy4ToVertexBufferUShortIdx(ctx, a, indices, cnt);
		return;
	case 8:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy8ToVertexBufferUShortIdx(ctx, a, indices, cnt);
		return;
	case 12:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy12ToVertexBufferUShortIdx(ctx, a, indices, cnt);
		return;
	case 16:
		if ((unsigned long)a->pointer % 4)
			break;
		fimgCopy16ToVertexBufferUShortIdx(ctx, a, indices, cnt);
		return;
	}

	fimgPackToVertexBufferUShortIdx(ctx, a, indices, cnt);
}

static inline void fimgDrawElementsBufferedAutoincUShortIdx(fimgContext *ctx,
		fimgArray *arrays, unsigned int count, const uint16_t *indices)
{
	unsigned i;
	fimgArray *a;

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetupAttributes(ctx, arrays);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBufferUShortIdx(ctx, a, indices, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

void fimgDrawElementsBufferedUShortIdx(fimgContext *ctx, fimgArray *arrays,
				unsigned int count, const uint16_t *indices)
{
	unsigned i;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawElementsBufferedAutoincUShortIdx(ctx, arrays,
							count, indices);
		return;
	}

	//fimgSelectiveFlush(ctx, 3);

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetupAttributes(ctx, arrays);

	alignment = count % FGHI_VERTICES_PER_VB_ATTRIB;

	if (alignment) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
			fimgLoadVertexBufferUShortIdx(ctx, a, indices, alignment);
			fimgPadVertexBuffer(ctx);
		}

		fimgSendIndices(ctx, 0, alignment);

		count -= alignment;
		indices += alignment;

		// Switch the buffer
		buf ^= 1;
	}

	while (count) {
		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, buf));
			fimgLoadVertexBufferUShortIdx(ctx, a, indices, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx);
		}

		fimgSelectiveFlush(ctx, 3);
		fimgSendIndices(ctx, (buf) ? FGHI_VERTICES_PER_VB_ATTRIB : 0, FGHI_VERTICES_PER_VB_ATTRIB);

		count -= FGHI_VERTICES_PER_VB_ATTRIB;
		indices += FGHI_VERTICES_PER_VB_ATTRIB;

		// Switch the buffer
		buf ^= 1;
	}
}

/*
 * UTILS
 */

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
	ctx->host.control.numoutattrib = 9;
#else
	ctx->host.control.numoutattrib = count;
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
 * FUNCTIONS:	fimgSetAttribute
 * SYNOPSIS:	This function specifies the property of attribute
 * PARAMETERS:	[IN] attribIdx: the index of attribute, which is in [0-15]
 *		[IN] pAttribInfo: fimgAttribute
 *****************************************************************************/
void fimgSetAttribute(fimgContext *ctx, unsigned int idx, unsigned int type, unsigned int numComp)
{
	ctx->host.attrib[idx].dt = type;
	ctx->host.attrib[idx].numcomp = FGHI_NUMCOMP(numComp);
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufAttrib
 * SYNOPSIS:	This function defines the property of attribute in vertex buffer
 * PARAMETERS:	[IN] attribIdx: the index of attribute
 *            	[IN] pAttribInfo: fimgVtxBufAttrib
 *****************************************************************************/
void fimgSetVtxBufAttrib(fimgContext *ctx, unsigned char idx, unsigned short base, unsigned char stride, unsigned short range)
{
	ctx->host.vbbase[idx] = base;
	ctx->host.vbctrl[idx].stride = stride;
	ctx->host.vbctrl[idx].range = range;
}

void fimgCreateHostContext(fimgContext *ctx)
{
	int i;

	ctx->host.control.autoinc = 1;
	ctx->host.indexOffset = 1;
#ifdef FIMG_USE_VERTEX_BUFFER
	ctx->host.control.envb = 1;
#endif

	for(i = 0; i < FIMG_ATTRIB_NUM; i++) {
		ctx->host.attrib[i].srcw = 3;
		ctx->host.attrib[i].srcz = 2;
		ctx->host.attrib[i].srcy = 1;
		ctx->host.attrib[i].srcx = 0;
	}
}

void fimgRestoreHostState(fimgContext *ctx)
{
	fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
	fimgWrite(ctx, ctx->host.indexOffset, FGHI_IDXOFFSET);
#ifdef FIMG_USE_VERTEX_BUFFER
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(0, 0), FGHI_ATTRIB_VBBASE(0));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(1, 0), FGHI_ATTRIB_VBBASE(1));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(2, 0), FGHI_ATTRIB_VBBASE(2));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(3, 0), FGHI_ATTRIB_VBBASE(3));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(4, 0), FGHI_ATTRIB_VBBASE(4));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(5, 0), FGHI_ATTRIB_VBBASE(5));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(6, 0), FGHI_ATTRIB_VBBASE(6));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(7, 0), FGHI_ATTRIB_VBBASE(7));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(8, 0), FGHI_ATTRIB_VBBASE(8));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(9, 0), FGHI_ATTRIB_VBBASE(9));
#endif
}
