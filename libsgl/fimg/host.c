/*
 * fimg/host.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
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
#include <stdio.h>
#include <errno.h>
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

inline void fimgGetHardware(fimgContext *ctx)
{
	int ret;

	if(unlikely((ret = fimgAcquireHardwareLock(ctx)) != 0)) {
		if(likely(ret > 0)) {
			fimgFlush(ctx);
			fimgRestoreContext(ctx);
			fimgInvalidateFlushCache(ctx, 1, 1, 0, 0);
			return;
		} else {
			fprintf(stderr, "FIMG: Could not acquire hardware lock");
			exit(EBUSY);
		}
	}
}

inline void fimgFlushContext(fimgContext *ctx)
{
	fimgQueueFlush(ctx);
	fimgCompatFlush(ctx);
}

inline void fimgPutHardware(fimgContext *ctx)
{
	fimgReleaseHardwareLock(ctx);
}

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

/*****************************************************************************
 * FUNCTIONS:	fimgSetIndexOffset
 * SYNOPSIS:	This function defines index offset which is used in the auto increment mode
 * PARAMETERS:	[IN] offset: index offset value
 *****************************************************************************/
static inline void fimgSetIndexOffset(fimgContext *ctx, unsigned int offset)
{
	fimgWrite(ctx, offset, FGHI_IDXOFFSET);
}

/*
 * UNBUFFERED
 */

/*****************************************************************************
 * FUNCTIONS:	fimgSendToFIFO
 * SYNOPSIS:	This function sends data to the 3D rendering pipeline
 * PARAMETERS:	[IN] count: number of words to send
 *            	[IN] ptr: buffer containing the data
 *****************************************************************************/
static inline void fimgSendToFIFO(fimgContext *ctx, unsigned int count, const unsigned int *ptr)
{
	unsigned int nEmptySpace = 0;

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

static inline void fimgDrawVertex(fimgContext *ctx, fimgArray *arrays, unsigned int i)
{
	uint32_t j, n;

	for(j = 0; j < ctx->numAttribs; j++) {
		switch(ctx->host.attrib[j].dt) {
		// 1bytes
		case FGHI_ATTRIB_DT_BYTE:
		case FGHI_ATTRIB_DT_UBYTE:
		case FGHI_ATTRIB_DT_NBYTE:
		case FGHI_ATTRIB_DT_NUBYTE: {
			uint32_t word = 0;
			uint8_t *bytes = (uint8_t *)&word;
			const uint8_t *data = (const uint8_t *)
				((const uint8_t *)arrays[j].pointer +
				i*arrays[j].stride);

			for(n = 0; n <= ctx->host.attrib[j].numcomp; n++)
				bytes[n] = data[n];

			fimgSendToFIFO(ctx, 1, &word);
			break; }
		// 2bytes
		case FGHI_ATTRIB_DT_SHORT:
		case FGHI_ATTRIB_DT_USHORT:
		case FGHI_ATTRIB_DT_NSHORT:
		case FGHI_ATTRIB_DT_NUSHORT: {
			uint32_t words[2] = {0, 0};
			uint16_t *halfwords = (uint16_t *)words;
			const uint16_t *data = (const uint16_t *)
				((const uint8_t *)arrays[j].pointer +
				i*arrays[j].stride);
			uint32_t cnt = (ctx->host.attrib[j].numcomp + 2) / 2;

			for(n = 0; n <= ctx->host.attrib[j].numcomp; n++)
				halfwords[n] = data[n];

			fimgSendToFIFO(ctx, cnt, words);
			break; }
		// 4 bytes
		case FGHI_ATTRIB_DT_FIXED:
		case FGHI_ATTRIB_DT_NFIXED:
		case FGHI_ATTRIB_DT_FLOAT:
		case FGHI_ATTRIB_DT_INT:
		case FGHI_ATTRIB_DT_UINT:
		case FGHI_ATTRIB_DT_NINT:
		case FGHI_ATTRIB_DT_NUINT: {
			const uint32_t *data = (const uint32_t *)
				((const uint8_t *)arrays[j].pointer +
				i*arrays[j].stride);
			uint32_t cnt = ctx->host.attrib[j].numcomp + 1;

			fimgSendToFIFO(ctx, cnt, data);
			break; }
		}
	}
}

/*****************************************************************************
 * FUNCTIONS:	fimgDrawArrays
 * SYNOPSIS:	This function sends geometry data to rendering pipeline
 * 		using FIFO.
 * PARAMETERS:	[IN] arrays: description of geometry layout
 * 		[IN] first: index of first vertex
 *		[IN] count: number of vertices
 *****************************************************************************/
void fimgDrawArrays(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned int i;
	fimgAttribute last;
	unsigned int words[2];

	// Get hardware lock
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[ctx->numAttribs - 1];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(ctx->numAttribs - 1));

	fimgSetHostInterface(ctx, 0, 1);

	// write the number of vertices
	words[0] = count;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 2, words);

	for(i=first; i<first+count; i++)
		fimgDrawVertex(ctx, arrays, i);

	// Free hardware lock
	fimgPutHardware(ctx);
}

/*****************************************************************************
 * FUNCTIONS:	fimgDrawElementsUByteIdx
 * SYNOPSIS:	This function sends indexed geometry data to rendering pipeline
 * 		using FIFO.
 * PARAMETERS:	[IN] arrays: description of geometry layout
 *		[IN] count: number of vertices
 *		[IN] idx: array of ubyte indices
 *****************************************************************************/
void fimgDrawElementsUByteIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
			unsigned int count, const unsigned char *idx)
{
	unsigned int i;
	fimgAttribute last;
	unsigned int words[2];

	// Get hardware lock
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

	fimgSetHostInterface(ctx, 0, 1);

	// write the number of vertices
	words[0] = count;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 2, words);

	for(i=0; i<count; i++)
		fimgDrawVertex(ctx, arrays, idx[i]);

	// Free hardware lock
	fimgPutHardware(ctx);
}

/*****************************************************************************
 * FUNCTIONS:	fimgDrawElementsUShortIdx
 * SYNOPSIS:	This function sends indexed geometry data to rendering pipeline
 * 		using FIFO.
 * PARAMETERS:	[IN] arrays: description of geometry layout
 *		[IN] count: number of vertices
 *		[IN] idx: array of ushort indices
 *****************************************************************************/
void fimgDrawElementsUShortIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
			unsigned int count, const unsigned short *idx)
{
	unsigned int i;
	fimgAttribute last;
	unsigned int words[2];

	// Get hardware lock
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	// write attribute configuration
	for(i = 0; i < ctx->numAttribs - 1; i++)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));
	// write the last one
	last = ctx->host.attrib[i];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));

	fimgSetHostInterface(ctx, 0, 1);

	// write the number of vertices
	words[0] = count;
	words[1] = 0xffffffff;
	fimgSendToFIFO(ctx, 2, words);

	for(i=0; i<count; i++)
		fimgDrawVertex(ctx, arrays, idx[i]);

	// Free hardware lock
	fimgPutHardware(ctx);
}

/*
 * BUFFERED
 */

#define FGHI_MAX_ATTRIBS		8
#define FGHI_VERTICES_PER_VB_ATTRIB	16
#define FGHI_MAX_BYTES_PER_VERTEX	16
#define FGHI_MAX_BYTES_PER_ATTRIB	(FGHI_VERTICES_PER_VB_ATTRIB)*(FGHI_MAX_BYTES_PER_VERTEX)
#define FGHI_VBADDR_ATTRIB(attrib, buf)	((2*attrib + ((buf)&1))*(FGHI_MAX_BYTES_PER_ATTRIB))

/* Utils */

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

static inline void fimgDrawAutoinc(fimgContext *ctx,
				uint32_t first, uint32_t count)
{
	fimgWrite(ctx, count, FGHI_FIFO_ENTRY);
	fimgWrite(ctx, first, FGHI_FIFO_ENTRY);
}

static inline void fimgSendIndices(fimgContext *ctx,
					uint32_t first, uint32_t count)
{
	fimgWrite(ctx, count, FGHI_FIFO_ENTRY);

	while (count--)
		fimgWrite(ctx, first++, FGHI_FIFO_ENTRY);
}

static void fimgSetupAttributes(fimgContext *ctx, fimgArray *arrays)
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

/* Draw arrays */

static void fimgFillVertexBuffer(fimgContext *ctx,
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
		if ((unsigned long)a->pointer % 4 == 0) {
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
		if ((unsigned long)a->pointer % 2 == 0) {
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

static void fimgPackToVertexBuffer(fimgContext *ctx,
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

static void fimgCopyToVertexBuffer(fimgContext *ctx,
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

static inline void fimgDrawArraysBufferedAutoinc(fimgContext *ctx,
		fimgArray *arrays, unsigned int first, unsigned int count)
{
	unsigned i;
	fimgArray *a;

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetIndexOffset(ctx, 1);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBuffer(ctx, a, first, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

void fimgDrawArraysBuffered(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
					unsigned int first, unsigned int count)
{
	unsigned i, pos = first;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	// Get hardware lock
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);

	fimgSetVertexContext(ctx, mode);
	fimgSetupAttributes(ctx, arrays);

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawArraysBufferedAutoinc(ctx, arrays, first, count);
		fimgPutHardware(ctx);
		return;
	}

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetIndexOffset(ctx, 0);

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

	fimgPutHardware(ctx);
}

/* Draw elements */

static void fimgPackToVertexBufferUByteIdx(fimgContext *ctx,
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

typedef uint32_t fglVertex8[2];
typedef uint32_t fglVertex12[3];
typedef uint32_t fglVertex16[4];

static void fimgCopy4ToVertexBufferUByteIdx(fimgContext *ctx,
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

static void fimgCopy8ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const fglVertex8 *data = (const fglVertex8 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
	}
}

static void fimgCopy12ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const fglVertex12 *data = (const fglVertex12 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
	}
}

static void fimgCopy16ToVertexBufferUByteIdx(fimgContext *ctx,
				fimgArray *a, const uint8_t *idx, uint32_t cnt)
{
	const fglVertex16 *data = (const fglVertex16 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
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

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetIndexOffset(ctx, 1);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBufferUByteIdx(ctx, a, indices, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

void fimgDrawElementsBufferedUByteIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint8_t *indices)
{
	unsigned i;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	// Flush the context
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);

	fimgSetVertexContext(ctx, mode);
	fimgSetupAttributes(ctx, arrays);

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawElementsBufferedAutoincUByteIdx(ctx, arrays,
							count, indices);
		fimgPutHardware(ctx);
		return;
	}

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetIndexOffset(ctx, 0);

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

	fimgPutHardware(ctx);
}

static void fimgPackToVertexBufferUShortIdx(fimgContext *ctx,
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

static void fimgCopy4ToVertexBufferUShortIdx(fimgContext *ctx,
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

static void fimgCopy8ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const fglVertex8 *data = (const fglVertex8 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][1]);
	}
}

static void fimgCopy12ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const fglVertex12 *data = (const fglVertex12 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][2]);
	}
}

static void fimgCopy16ToVertexBufferUShortIdx(fimgContext *ctx,
				fimgArray *a, const uint16_t *idx, uint32_t cnt)
{
	const fglVertex16 *data = (const fglVertex16 *)a->pointer;

	while (cnt >= 4) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
		cnt -= 4;
	}

	while (cnt--) {
		fimgSendToVtxBuffer(ctx, data[*(idx)][0]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][1]);
		fimgSendToVtxBuffer(ctx, data[*(idx)][2]);
		fimgSendToVtxBuffer(ctx, data[*(idx++)][3]);
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

	fimgSetHostInterface(ctx, 1, 1);
	fimgSetIndexOffset(ctx, 1);

	for (a = arrays, i = 0; i < ctx->numAttribs; i++, a++) {
		fimgSetVtxBufferAddr(ctx, FGHI_VBADDR_ATTRIB(i, 0));
		fimgLoadVertexBufferUShortIdx(ctx, a, indices, count);
		fimgPadVertexBuffer(ctx);
	}

	fimgDrawAutoinc(ctx, 0, count);
}

void fimgDrawElementsBufferedUShortIdx(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
				unsigned int count, const uint16_t *indices)
{
	unsigned i;
	fimgArray *a;
	uint32_t buf = 0;
	unsigned int alignment;

	// Flush the context
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);

	fimgSetVertexContext(ctx, mode);
	fimgSetupAttributes(ctx, arrays);

	if (count <= 2*FGHI_VERTICES_PER_VB_ATTRIB) {
		fimgDrawElementsBufferedAutoincUShortIdx(ctx, arrays,
							count, indices);
		fimgPutHardware(ctx);
		return;
	}

	fimgSetHostInterface(ctx, 1, 0);
	fimgSetIndexOffset(ctx, 0);

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

	fimgPutHardware(ctx);
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
	ctx->numAttribs = count;
	//fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
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
#ifdef FIMG_USE_VERTEX_BUFFER
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(0, 0), FGHI_ATTRIB_VBBASE(0));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(1, 0), FGHI_ATTRIB_VBBASE(1));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(2, 0), FGHI_ATTRIB_VBBASE(2));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(3, 0), FGHI_ATTRIB_VBBASE(3));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(4, 0), FGHI_ATTRIB_VBBASE(4));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(5, 0), FGHI_ATTRIB_VBBASE(5));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(6, 0), FGHI_ATTRIB_VBBASE(6));
	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(7, 0), FGHI_ATTRIB_VBBASE(7));
//	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(8, 0), FGHI_ATTRIB_VBBASE(8));
//	fimgWrite(ctx, FGHI_VBADDR_ATTRIB(9, 0), FGHI_ATTRIB_VBBASE(9));
#endif
}
