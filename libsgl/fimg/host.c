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
static inline uint32_t fimgGetNumEmptyFIFOSlots(fimgContext *ctx)
{
	return fimgRead(ctx, FGHI_DWSPACE);
}

static inline void fimgSetVertexBuffer(fimgContext *ctx, int enable)
{
	ctx->host.control.envb = !!enable;

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

	fimgSetVertexBuffer(ctx, 0);

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

#define FGHI_VERTICES_PER_VB_ATTRIB	256
#define FGHI_MAX_BYTES_PER_VERTEX	16
#define FGHI_MAX_BYTES_PER_ATTRIB	(FGHI_VERTICES_PER_VB_ATTRIB)*(FGHI_MAX_BYTES_PER_VERTEX)
#define FGHI_VBADDR_ATTRIB(i)		((i)*(FGHI_MAX_BYTES_PER_ATTRIB))

/*****************************************************************************
 * FUNCTIONS:	fimgSetVtxBufferAddr
 * SYNOPSIS:	This function defines the starting address in vertex buffer,
 *		which are used to send data into vertex buffer
 * PARAMETERS:	[IN] address: the starting address
 *****************************************************************************/
static inline void fimgSetVtxBufferAddr(fimgContext *ctx, unsigned int addr)
{
	fimgWrite(ctx, addr, FGHI_VBADDR);
}

static inline size_t fimgFillVertexBuffer(fimgContext *ctx,
					fimgArray *a, uint32_t cnt)
{
	register uint32_t word;
	uint32_t written;

#if 1
	// Stride = 0 optimization
	cnt = 1;
#endif

	// Each vertex must be word aligned
	written  = ((a->width + 3) & ~3);
	// Count in words
	written /= 4;
	// For each vertex
	written *= cnt;

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
					fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
					len -= 4;
				}
			}

			break;
		}
	// halfwords
	case 2:
		// Check if vertices are halfword aligned
		if ((unsigned long)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len, num, srcpad = (a->stride - a->width) / 2;
			const uint16_t *data;

			while (cnt--) {
				data = (const uint16_t *)a->pointer;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgWrite(ctx, word, FGHI_VB_ENTRY);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
			}
			break;
		}
	// bytes
	default:
		// Fallback - no check required
		{
			uint32_t len, srcpad = a->stride - a->width;
			const uint8_t *data;

			while (cnt--) {
				data = (const uint8_t *)a->pointer;
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					fimgWrite(ctx, word, FGHI_VB_ENTRY);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgWrite(ctx, word, FGHI_VB_ENTRY);
				}
			}
			break;
		}
	}

	return written;
}

static inline size_t fimgPackToVertexBuffer(fimgContext *ctx,
				fimgArray *a, uint32_t pos, uint32_t cnt)
{
	register uint32_t word;
	uint32_t written;

	// Each vertex must be word aligned
	written  = ((a->width + 3) & ~3);
	// Count in words
	written /= 4;
	// For each vertex
	written *= cnt;

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
					fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
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
			uint32_t len, num, srcpad = (a->stride - a->width) / 2;
			const uint16_t *data = (const uint16_t *)((const uint8_t *)a->pointer + pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					fimgWrite(ctx, word, FGHI_VB_ENTRY);
					len -= 4;
				}

				// Single halfword left
				if (len)
					fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);

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
					fimgWrite(ctx, word, FGHI_VB_ENTRY);
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					fimgWrite(ctx, word, FGHI_VB_ENTRY);
				}

				data += srcpad;
			}
			break;
		}
	}

	return written;
}

static inline size_t fimgCopyToVertexBuffer(fimgContext *ctx,
				fimgArray *a, uint32_t pos, uint32_t cnt)
{
	uint32_t len;
	uint32_t written;
	const uint32_t *data = (const uint32_t *)((const uint8_t *)a->pointer + pos*a->stride);

	len = a->stride * cnt;
	len /= 4;
	written = len;

	// Try to copy in bursts of 4 words
	while (len >= 4) {
		fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
		fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
		fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
		fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);
		len -= 4;
	}

	while (len--)
		fimgWrite(ctx, *(data++), FGHI_VB_ENTRY);

	return written;
}

static inline size_t fimgLoadVertexBuffer(fimgContext *ctx, fimgArray *a,
				uint32_t pos, uint32_t cnt)
{
	if (a->stride == 0)
		// Fill the buffer with a constant
		return fimgFillVertexBuffer(ctx, a, cnt);
	else if (a->stride % 4 || a->stride > 16 || (unsigned long)a->pointer % 4)
		// Pack attribute data and store in the buffer
		return fimgPackToVertexBuffer(ctx, a, pos, cnt);
	else
		// Copy packed attribute data to the buffer
		return fimgCopyToVertexBuffer(ctx, a, pos, cnt);
}

static inline void fimgPadVertexBuffer(fimgContext *ctx, size_t words)
{
	size_t padding;

	if (words % 4) {
		padding = 4 - (words % 4);

		while (padding--)
			fimgWrite(ctx, 0, FGHI_VB_ENTRY);
	}
}

static inline void fimgSetupAttributes(fimgContext *ctx, fimgArray *arrays)
{
	fimgVtxBufAttrib vbattr;
	fimgAttribute last;
	fimgArray *a;
	unsigned int i;

	vbattr.val = 0;
	vbattr.range = 256;

	// write attribute configuration
	for (a = arrays, i = 0; i < ctx->numAttribs - 1; i++, a++) {
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));

		if (a->stride == 0)
			vbattr.stride = 0;
		else if (a->stride % 4 || a->stride > 16 || (unsigned long)a->pointer % 4)
			vbattr.stride = (a->width + 3) & ~3;
		else
			vbattr.stride = a->stride;

		fimgWrite(ctx, vbattr.val, FGHI_ATTRIB_VBCTRL(i));
	}

	// write the last one
	last = ctx->host.attrib[i];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));
}

static inline void fimgDrawAutoinc(fimgContext *ctx,
				uint32_t first, uint32_t count)
{
	fimgSetVertexBuffer(ctx, 1);
	fimgWrite(ctx, count, FGHI_FIFO_ENTRY);
	fimgWrite(ctx, first, FGHI_FIFO_ENTRY);
}

/* For points, lines and triangles */
void fimgDrawArraysBufferedSeparate(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = 0;
	size_t written;
	fimgArray *a;

	fimgSetupAttributes(ctx, arrays);

	while (count >= FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Points don't need to repeat anything
		count -= FGHI_VERTICES_PER_VB_ATTRIB;
		pos += FGHI_VERTICES_PER_VB_ATTRIB;
	}

	if (count) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, count);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, count);
	}
}

/* For line strips */
void fimgDrawArraysBufferedRepeatLast(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = 0;
	size_t written;
	fimgArray *a;

	fimgSetupAttributes(ctx, arrays);

	count--;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 1) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 1;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 1;
	}

	if (count) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, 1 + count);
	}
}

/* For line loops */
void fimgDrawArraysBufferedRepeatLastLoop(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = 0;
	size_t written;
	fimgArray *a;

	fimgSetupAttributes(ctx, arrays);

	count--;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 1) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 1;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 1;
	}

	if (count) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, 1 + count);
	}

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSelectiveFlush(ctx, 3);

		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
		written = fimgLoadVertexBuffer(ctx, a, 0, 1);
		written += fimgLoadVertexBuffer(ctx, a, count - 1, 1);
		fimgPadVertexBuffer(ctx, written);
	}

	fimgDrawAutoinc(ctx, 0, 2);
}

/* For triangle strips */
void fimgDrawArraysBufferedRepeatLastTwo(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = 0;
	size_t written;
	fimgArray *a;

	fimgSetupAttributes(ctx, arrays);

	count -= 2;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 2) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos - (pos % 2), 1);
			written += fimgLoadVertexBuffer(ctx, a, pos - 1 + (pos % 2), 1);
			written += fimgLoadVertexBuffer(ctx, a, pos + 2, FGHI_VERTICES_PER_VB_ATTRIB - 2);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat last two vertices per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 2;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 2;
	}

	if (count) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, pos - (pos % 2), 1);
			written += fimgLoadVertexBuffer(ctx, a, pos - 1 + (pos % 2), 1);
			written += fimgLoadVertexBuffer(ctx, a, pos + 2, count);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, 2 + count);
	}
}

/* For triangle fans */
void fimgDrawArraysBufferedRepeatFirstLast(fimgContext *ctx, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, attrib, pos = 1;
	size_t written;
	fimgArray *a;

	fimgSetupAttributes(ctx, arrays);

	count -= 2;
	while (count >= FGHI_VERTICES_PER_VB_ATTRIB - 2) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, 0, 1);
			written += fimgLoadVertexBuffer(ctx, a, pos, FGHI_VERTICES_PER_VB_ATTRIB - 1);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, FGHI_VERTICES_PER_VB_ATTRIB);

		// Repeat first and last vertex per group
		count -= FGHI_VERTICES_PER_VB_ATTRIB - 2;
		pos += FGHI_VERTICES_PER_VB_ATTRIB - 2;
	}

	if (count) {
		fimgSelectiveFlush(ctx, 3);

		for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
			fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i));
			written = fimgLoadVertexBuffer(ctx, a, 0, 1);
			written += fimgLoadVertexBuffer(ctx, a, pos, 1 + count);
			fimgPadVertexBuffer(ctx, written);
		}

		fimgDrawAutoinc(ctx, 0, 1 + count);
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
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(0), FGHI_ATTRIB_VBBASE(0));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(1), FGHI_ATTRIB_VBBASE(1));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(2), FGHI_ATTRIB_VBBASE(2));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(3), FGHI_ATTRIB_VBBASE(3));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(4), FGHI_ATTRIB_VBBASE(4));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(5), FGHI_ATTRIB_VBBASE(5));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(6), FGHI_ATTRIB_VBBASE(6));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(7), FGHI_ATTRIB_VBBASE(7));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(8), FGHI_ATTRIB_VBBASE(8));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(9), FGHI_ATTRIB_VBBASE(9));
#endif
}
