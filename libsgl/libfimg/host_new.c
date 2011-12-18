/*
 * fimg/host_new.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include "fimg_private.h"

#define FGHI_FIFO_SIZE		32

#define FGHI_DWSPACE		0x8000
#define FGHI_FIFO_ENTRY		0xc000
#define FGHI_CONTROL		0x8008
#define FGHI_IDXOFFSET		0x800c
#define FGHI_VBADDR		0x8010
#define FGHI_VB_ENTRY		0xe000

#define FGHI_ATTRIB(i)		(0x8040 + 4*(i))
#define FGHI_ATTRIB_VBCTRL(i)	(0x8080 + 4*(i))
#define FGHI_ATTRIB_VBBASE(i)	(0x80c0 + 4*(i))

typedef enum {
	FGHI_CONTROLIdxTYPE_UINT = 0,
	FGHI_CONTROLIdxTYPE_USHORT,
	FGHI_CONTROLIdxTYPE_UBYTE = 3
} fimgHostIndexType;

/*
 * Utils
 */

void fimgSetAttribCount(fimgContext *ctx, unsigned char count)
{
#ifdef FIMG_INTERPOLATION_WORKAROUND
	ctx->host.control.numoutattrib = FIMG_ATTRIB_NUM;
#else
	ctx->host.control.numoutattrib = count;
#endif
	ctx->numAttribs = count;
}

/*****************************************************************************
 * FUNCTIONS:	fimgSetAttribute
 * SYNOPSIS:	This function specifies the property of attribute
 * PARAMETERS:	[IN] attribIdx: the index of attribute, which is in [0-15]
 *		[IN] pAttribInfo: fimgAttribute
 *****************************************************************************/
void fimgSetAttribute(fimgContext *ctx, unsigned int idx,
					unsigned int type, unsigned int numComp)
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
static inline void setVtxBufAttrib(fimgContext *ctx, unsigned char idx,
		unsigned short base, unsigned char stride, unsigned short range)
{
	ctx->host.vbbase[idx] = base;
	ctx->host.vbctrl[idx].stride = stride;
	ctx->host.vbctrl[idx].range = range;
}

/*
 * BUFFERED
 */

#define VERTEX_BUFFER_SIZE	(4096)
#define VERTEX_BUFFER_CONST	(MAX_WORDS_PER_VERTEX)
#define VERTEX_BUFFER_WORDS	(VERTEX_BUFFER_SIZE / 4 - VERTEX_BUFFER_CONST)

#define MAX_ATTRIBS		(FIMG_ATTRIB_NUM)
#define MAX_WORDS_PER_ATTRIB	(4)
#define MAX_WORDS_PER_VERTEX	(MAX_ATTRIBS*MAX_WORDS_PER_ATTRIB)

#define CONST_ADDR(attrib)	(4*MAX_WORDS_PER_ATTRIB*(attrib))
#define DATA_OFFSET		(CONST_ADDR(MAX_ATTRIBS))

/*
 * Table used to calculate how many vertices will fit into vertex buffer.
 * CPUs don't like divisions, so a table of precalulated values is used,
 * as the range of input number is pretty small (1 to 32 words per vertex).
 */
static const int vertexWordsToVertexCount[] = {
	4096, /* Limit for constant vertices */
	VERTEX_BUFFER_WORDS / 1,
	VERTEX_BUFFER_WORDS / 2,
	VERTEX_BUFFER_WORDS / 3,
	VERTEX_BUFFER_WORDS / 4,
	VERTEX_BUFFER_WORDS / 5,
	VERTEX_BUFFER_WORDS / 6,
	VERTEX_BUFFER_WORDS / 7,
	VERTEX_BUFFER_WORDS / 8,
	VERTEX_BUFFER_WORDS / 9,
	VERTEX_BUFFER_WORDS / 10,
	VERTEX_BUFFER_WORDS / 11,
	VERTEX_BUFFER_WORDS / 12,
	VERTEX_BUFFER_WORDS / 13,
	VERTEX_BUFFER_WORDS / 14,
	VERTEX_BUFFER_WORDS / 15,
	VERTEX_BUFFER_WORDS / 16,
	VERTEX_BUFFER_WORDS / 17,
	VERTEX_BUFFER_WORDS / 18,
	VERTEX_BUFFER_WORDS / 19,
	VERTEX_BUFFER_WORDS / 20,
	VERTEX_BUFFER_WORDS / 21,
	VERTEX_BUFFER_WORDS / 22,
	VERTEX_BUFFER_WORDS / 23,
	VERTEX_BUFFER_WORDS / 24,
	VERTEX_BUFFER_WORDS / 25,
	VERTEX_BUFFER_WORDS / 26,
	VERTEX_BUFFER_WORDS / 27,
	VERTEX_BUFFER_WORDS / 28,
	VERTEX_BUFFER_WORDS / 29,
	VERTEX_BUFFER_WORDS / 30,
	VERTEX_BUFFER_WORDS / 31,
	VERTEX_BUFFER_WORDS / 32,
	VERTEX_BUFFER_WORDS / 33,
	VERTEX_BUFFER_WORDS / 34,
	VERTEX_BUFFER_WORDS / 35,
	VERTEX_BUFFER_WORDS / 36,
	VERTEX_BUFFER_WORDS / 37,
	VERTEX_BUFFER_WORDS / 38,
	VERTEX_BUFFER_WORDS / 39,
	VERTEX_BUFFER_WORDS / 40,
	VERTEX_BUFFER_WORDS / 41,
	VERTEX_BUFFER_WORDS / 42,
	VERTEX_BUFFER_WORDS / 43,
	VERTEX_BUFFER_WORDS / 44,
	VERTEX_BUFFER_WORDS / 45,
	VERTEX_BUFFER_WORDS / 46,
	VERTEX_BUFFER_WORDS / 47,
};

/* Calculate how many vertices will fit into vertex buffer with given config */
static unsigned int calculateBatchSize(fimgArray *arrays, int count)
{
	fimgArray *a = arrays;
	unsigned int size = 0;
	int i;

	for (i = 0; i < count; ++i, ++a) {
		/* Do not count constants */
		if (!a->stride)
			continue;

		/* Round up to full words */
		size += (a->width + 3) / 4;
	}

	return vertexWordsToVertexCount[size];
}

static void fillVertexBuffer(fimgContext *ctx)
{
	volatile uint32_t *reg =
			(volatile uint32_t *)(ctx->base + FGHI_VB_ENTRY);
	uint32_t *data = (uint32_t *)ctx->vertexData;
	unsigned count = (ctx->vertexDataSize + 31) / 32;

	fimgWrite(ctx, 0, FGHI_VBADDR);

	asm volatile (
		"1:\n\t"
		"ldmia %1!, {r0-r7}\n\t"
		"stmia %0!, {r0-r7}\n\t"
		"subs %2, %2, $1\n\t"
		"bne 1b\n\t"
		:
		: "r"(reg), "r"(data), "r"(count)
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
	);
}

#define BUF_ADDR_32(buf, offs)	\
			((const uint32_t *)((const uint8_t *)(buf) + (offs)))
#define BUF_ADDR_16(buf, offs)	\
			((const uint16_t *)((const uint8_t *)(buf) + (offs)))
#define BUF_ADDR_8(buf, offs)	\
			((const uint8_t *)(buf) + (offs))

/*
 * Unindexed
 */

static uint32_t packAttribute(fimgContext *ctx, uint32_t *buf,
						fimgArray *a, int pos, int cnt)
{
	register uint32_t word;
	uint32_t size;

	/* Vertices must be word aligned */
	size = (a->width + 3) & ~3;
	size *= cnt;

	switch(a->width % 4) {
	/* words */
	case 0:
		/* Check if vertices are word aligned */
		if ((uintptr_t)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			uint32_t len, srcpad = (a->stride - a->width) / 4;
			const uint32_t *data =
					BUF_ADDR_32(a->pointer, pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len) {
					*(buf++) = *(data++);
					len -= 4;
				}

				data += srcpad;
			}

			break;
		}
	/* halfwords */
	case 2:
		/* Check if vertices are halfword aligned */
		if ((uintptr_t)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			uint32_t len, srcpad = (a->stride - a->width) / 2;
			const uint16_t *data =
					BUF_ADDR_16(a->pointer, pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					*(buf++) = word;
					len -= 4;
				}

				/* Single halfword left */
				if (len)
					*(buf++) = *(data++);

				data += srcpad;
			}

			break;
		}
	/* bytes */
	default:
		/* Fallback - no check required */
		{
			uint32_t len, srcpad = a->stride - a->width;
			const uint8_t *data =
					BUF_ADDR_8(a->pointer, pos*a->stride);

			while (cnt--) {
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					*(buf++) = word;
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					*(buf++) = word;
				}

				data += srcpad;
			}

			break;
		}
	}

	return size;
}

/* Generic vertex copy */
static uint32_t copyVertices1To1(fimgContext *ctx, fimgArray *arrays,
						uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, (const uint8_t *)a->pointer
					+ *first*a->stride, batchSize*a->width);
			offset += batchSize*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first, batchSize);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesLinestrip(fimgContext *ctx,
			fimgArray *arrays, uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, (const uint8_t *)a->pointer
					+ *first*a->stride, batchSize*a->width);
			offset += batchSize*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first, batchSize);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize - 1;
	*count -= batchSize - 1;
	return batchSize;
}

static uint32_t copyVerticesLines(fimgContext *ctx, fimgArray *arrays,
						uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, (const uint8_t*)a->pointer
					+ *first*a->stride, batchSize*a->width);
			offset += batchSize*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first, batchSize);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesTristrip(fimgContext *ctx,
			fimgArray *arrays, uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 1;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize >= *count)
		batchSize = *count;
	else if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 1);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 1);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, (const uint8_t *)a->pointer
					+ *first*a->stride, batchSize*a->width);
			offset += batchSize*a->width;
			memcpy(buf + offset, (const uint8_t *)a->pointer
					+ (*first + batchSize - 1)*a->stride, 1*a->width);
			offset += 1*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first, batchSize);
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first + batchSize - 1, 1);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 1;
}

static uint32_t copyVerticesTrifan(fimgContext *ctx,
			fimgArray *arrays, uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 2;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 2);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 2);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, a->pointer, a->width);
			offset += a->width;
			memcpy(buf + offset, a->pointer, a->width);
			offset += a->width;
			memcpy(buf + offset, a->pointer, a->width);
			offset += a->width;
			memcpy(buf + offset, (const uint8_t *)a->pointer
						+ (*first + 1)*a->stride,
						(batchSize - 1)*a->width);
			offset += (batchSize - 1)*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, 0, 1);
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, 0, 1);
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, 0, 1);
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a,
						*first + 1, batchSize - 1);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 2;
}

static uint32_t copyVerticesTris(fimgContext *ctx, fimgArray *arrays,
					uint32_t *first, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 3)
		batchSize -= batchSize % 3;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		if (a->stride == a->width && !(a->width % 4)) {
			memcpy(buf + offset, (const uint8_t *)a->pointer
					+ *first*a->stride, batchSize*a->width);
			offset += batchSize*a->width;
			continue;
		}
		offset += packAttribute(ctx,
				(uint32_t *)(buf + offset), a, *first, batchSize);
	}

	ctx->vertexDataSize = offset;

	*first += batchSize;
	*count -= batchSize;
	return batchSize;
}

/*
 * Indexed uint16_t
 */

static uint32_t packAttributeIdx16(fimgContext *ctx, uint32_t *buf,
				fimgArray *a, const uint16_t *idx, int cnt)
{
	register uint32_t word;
	uint32_t size;
	uint32_t len;

	if (!cnt)
		return 0;

	/* Vertices must be word aligned */
	size = (a->width + 3) & ~3;
	size *= cnt;

	switch(a->width % 4) {
	/* words */
	case 0:
		/* Check if vertices are word aligned */
		if ((uintptr_t)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			const uint32_t *data, *next_data;

			next_data = BUF_ADDR_32(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_32(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len) {
					*(buf++) = *(data++);
					len -= 4;
				}
			}

			data = next_data;
			len = a->width;

			while (len) {
				*(buf++) = *(data++);
				len -= 4;
			}

			break;
		}
	/* halfwords */
	case 2:
		/* Check if vertices are halfword aligned */
		if ((uintptr_t)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			const uint16_t *data, *next_data;

			next_data = BUF_ADDR_16(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_16(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					*(buf++) = word;
					len -= 4;
				}

				/* Single halfword left */
				if (len)
					*(buf++) = *(data++);
			}

			data = next_data;
			len = a->width;

			while (len >= 4) {
				word = *(data++);
				word |= *(data++) << 16;
				*(buf++) = word;
				len -= 4;
			}

			break;
		}
	/* bytes */
	default:
		/* Fallback - no check required */
		{
			const uint8_t *data, *next_data;

			next_data = BUF_ADDR_8(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_8(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					*(buf++) = word;
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					*(buf++) = word;
				}
			}

			data = next_data;
			len = a->width;

			while (len >= 4) {
				word = *(data++);
				word |= *(data++) << 8;
				word |= *(data++) << 16;
				word |= *(data++) << 24;
				*(buf++) = word;
				len -= 4;
			}

			// Up to 3 bytes left
			if (len) {
				word = *(data++);
				if (len == 2)
					word |= *(data++) << 8;
				if (len == 3)
					word |= *(data++) << 16;

				*(buf++) = word;
			}

			break;
		}
	}

	return size;
}

static uint32_t copyVertices1To1Idx16(fimgContext *ctx, fimgArray *arrays,
			const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesLinestripIdx16(fimgContext *ctx, fimgArray *arrays,
			const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 1;
	*count -= batchSize - 1;
	return batchSize;
}


static uint32_t copyVerticesLinesIdx16(fimgContext *ctx, fimgArray *arrays,
				const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesTristripIdx16(fimgContext *ctx, fimgArray *arrays,
			const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 1;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize >= *count)
		batchSize = *count;
	else if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 1);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 1);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos + batchSize - 1, 1);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 1;
}

static uint32_t copyVerticesTrifanIdx16(fimgContext *ctx, fimgArray *arrays,
			const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 2;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 2);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 2);
		offset += packAttributeIdx16(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx16(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx16(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
					a, indices + *pos + 1, batchSize - 1);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 2;
}

static uint32_t copyVerticesTrisIdx16(fimgContext *ctx, fimgArray *arrays,
			const uint16_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 3)
		batchSize -= batchSize % 3;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx16(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

/*
 * Indexed uint8_t
 */

static uint32_t packAttributeIdx8(fimgContext *ctx, uint32_t *buf,
				fimgArray *a, const uint8_t *idx, int cnt)
{
	register uint32_t word;
	uint32_t size;
	uint32_t len;

	if (!cnt)
		return 0;

	/* Vertices must be word aligned */
	size = (a->width + 3) & ~3;
	size *= cnt;

	switch(a->width % 4) {
	/* words */
	case 0:
		/* Check if vertices are word aligned */
		if ((uintptr_t)a->pointer % 4 == 0 && a->stride % 4 == 0) {
			const uint32_t *data, *next_data;

			next_data = BUF_ADDR_32(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_32(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len) {
					*(buf++) = *(data++);
					len -= 4;
				}
			}

			data = next_data;
			len = a->width;

			while (len) {
				*(buf++) = *(data++);
				len -= 4;
			}

			break;
		}
	/* halfwords */
	case 2:
		/* Check if vertices are halfword aligned */
		if ((uintptr_t)a->pointer % 2 == 0 && a->stride % 2 == 0) {
			const uint16_t *data, *next_data;

			next_data = BUF_ADDR_16(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_16(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 16;
					*(buf++) = word;
					len -= 4;
				}

				/* Single halfword left */
				if (len)
					*(buf++) = *(data++);
			}

			data = next_data;
			len = a->width;

			while (len >= 4) {
				word = *(data++);
				word |= *(data++) << 16;
				*(buf++) = word;
				len -= 4;
			}

			break;
		}
	/* bytes */
	default:
		/* Fallback - no check required */
		{
			const uint8_t *data, *next_data;

			next_data = BUF_ADDR_8(a->pointer, *(idx++)*a->stride);
			--cnt;

			while (cnt--) {
				data = next_data;
				next_data = BUF_ADDR_8(a->pointer,
							*(idx++)*a->stride);
				len = a->width;

				while (len >= 4) {
					word = *(data++);
					word |= *(data++) << 8;
					word |= *(data++) << 16;
					word |= *(data++) << 24;
					*(buf++) = word;
					len -= 4;
				}

				// Up to 3 bytes left
				if (len) {
					word = *(data++);
					if (len == 2)
						word |= *(data++) << 8;
					if (len == 3)
						word |= *(data++) << 16;

					*(buf++) = word;
				}
			}

			data = next_data;
			len = a->width;

			while (len >= 4) {
				word = *(data++);
				word |= *(data++) << 8;
				word |= *(data++) << 16;
				word |= *(data++) << 24;
				*(buf++) = word;
				len -= 4;
			}

			// Up to 3 bytes left
			if (len) {
				word = *(data++);
				if (len == 2)
					word |= *(data++) << 8;
				if (len == 3)
					word |= *(data++) << 16;

				*(buf++) = word;
			}

			break;
		}
	}

	return size;
}

static uint32_t copyVertices1To1Idx8(fimgContext *ctx, fimgArray *arrays,
			const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesLinestripIdx8(fimgContext *ctx, fimgArray *arrays,
			const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 1;
	*count -= batchSize - 1;
	return batchSize;
}


static uint32_t copyVerticesLinesIdx8(fimgContext *ctx, fimgArray *arrays,
			const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 2)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

static uint32_t copyVerticesTristripIdx8(fimgContext *ctx, fimgArray *arrays,
			const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 1;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize >= *count)
		batchSize = *count;
	else if (batchSize % 2)
		--batchSize;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 1);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 1);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos + batchSize - 1, 1);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 1;
}

static uint32_t copyVerticesTrifanIdx8(fimgContext *ctx, fimgArray *arrays,
			const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs) - 2;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize + 2);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize + 2);
		offset += packAttributeIdx8(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx8(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx8(ctx,
				(uint32_t *)(buf + offset), a, indices, 1);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
					a, indices + *pos + 1, batchSize - 1);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize - 2;
	*count -= batchSize - 2;
	return batchSize + 2;
}

static uint32_t copyVerticesTrisIdx8(fimgContext *ctx, fimgArray *arrays,
				const uint8_t *indices, uint32_t *pos, uint32_t *count)
{
	fimgArray *a = arrays;
	uint32_t batchSize = calculateBatchSize(arrays, ctx->numAttribs);
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	if (*count < 3)
		return 0;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize % 3)
		batchSize -= batchSize % 3;

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (!a->stride) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, batchSize);
			memcpy(buf + CONST_ADDR(i), a->pointer, a->width);
			continue;
		}
		setVtxBufAttrib(ctx, i, offset, (a->width + 3) & ~3, batchSize);
		offset += packAttributeIdx8(ctx, (uint32_t *)(buf + offset),
						a, indices + *pos, batchSize);
	}

	ctx->vertexDataSize = offset;

	*pos += batchSize;
	*count -= batchSize;
	return batchSize;
}

/*
 * Primitive engine has problems with triangle strips and triangle fans,
 * so in those cases geometry must be converted to separate triangles
 */
struct primitiveHandler {
	uint32_t (*direct)(fimgContext *, fimgArray *, uint32_t *, uint32_t *);
	uint32_t (*indexed_8)(fimgContext *, fimgArray *,
				const uint8_t *, uint32_t *, uint32_t *);
	uint32_t (*indexed_16)(fimgContext *, fimgArray *,
				const uint16_t *, uint32_t *, uint32_t *);
};

static const struct primitiveHandler primitiveHandler[FGPE_PRIMITIVE_MAX] = {
	[FGPE_POINT_SPRITE] = {
		.direct		= copyVertices1To1,
		.indexed_8	= copyVertices1To1Idx8,
		.indexed_16	= copyVertices1To1Idx16
	},
	[FGPE_POINTS] = {
		.direct		= copyVertices1To1,
		.indexed_8	= copyVertices1To1Idx8,
		.indexed_16	= copyVertices1To1Idx16
	},
	[FGPE_LINE_STRIP] = {
		.direct		= copyVerticesLinestrip,
		.indexed_8	= copyVerticesLinestripIdx8,
		.indexed_16	= copyVerticesLinestripIdx16
	},
	[FGPE_LINE_LOOP] = {
		/*
		 * Line loops don't go well with buffered transfers,
		 * so let's just force higher level code to emulate them
		 * using line strips.
		 */
		.direct		= 0,
		.indexed_8	= 0,
		.indexed_16	= 0
	},
	[FGPE_LINES] = {
		.direct		= copyVerticesLines,
		.indexed_8	= copyVerticesLinesIdx8,
		.indexed_16	= copyVerticesLinesIdx16
	},
	[FGPE_TRIANGLE_STRIP] = {
		.direct		= copyVerticesTristrip,
		.indexed_8	= copyVerticesTristripIdx8,
		.indexed_16	= copyVerticesTristripIdx16
	},
	[FGPE_TRIANGLE_FAN] = {
		.direct		= copyVerticesTrifan,
		.indexed_8	= copyVerticesTrifanIdx8,
		.indexed_16	= copyVerticesTrifanIdx16
	},
	[FGPE_TRIANGLES] = {
		.direct		= copyVerticesTris,
		.indexed_8	= copyVerticesTrisIdx8,
		.indexed_16	= copyVerticesTrisIdx16
	},
};

static inline void drawAutoinc(fimgContext *ctx,
				uint32_t first, uint32_t count)
{
	fimgWrite(ctx, count, FGHI_FIFO_ENTRY);
	fimgWrite(ctx, first, FGHI_FIFO_ENTRY);
}

static void setupAttributes(fimgContext *ctx, fimgArray *arrays)
{
	fimgAttribute last;
	unsigned int i;

	// write attribute configuration
	for (i = 0; i < ctx->numAttribs - 1; ++i)
		fimgWrite(ctx, ctx->host.attrib[i].val, FGHI_ATTRIB(i));

	// write the last one
	last = ctx->host.attrib[i];
	last.lastattr = 1;
	fimgWrite(ctx, last.val, FGHI_ATTRIB(i));
}

static void setupVertexBuffer(fimgContext *ctx)
{
	unsigned int i;

	for (i = 0; i < ctx->numAttribs; i++) {
		fimgWrite(ctx, ctx->host.vbctrl[i].val, FGHI_ATTRIB_VBCTRL(i));
		fimgWrite(ctx, ctx->host.vbbase[i], FGHI_ATTRIB_VBBASE(i));
	}
}

void fimgDrawArrays(fimgContext *ctx, unsigned int mode,
					fimgArray *arrays, unsigned int count)
{
	unsigned int copied;
	unsigned int first = 0;

	if (mode >= FGPE_PRIMITIVE_MAX)
		return;

	if (!primitiveHandler[mode].direct) {
		LOGE("%s: Unsupported mode %d", __func__, mode);
		return;
	}

	if (!ctx->vertexData) {
		ctx->vertexData = memalign(32, VERTEX_BUFFER_SIZE);
		if (!ctx->vertexData) {
			LOGE("Failed to allocate vertex data buffer. Terminating.");
			exit(ENOMEM);
		}
	}

	/* Prepare first batch without waiting for hardware */
	copied = primitiveHandler[mode].direct(ctx, arrays, &first, &count);
	if (!copied)
		return;

	/* Get hardware */
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	setupAttributes(ctx, arrays);
#ifdef FIMG_DUMP_STATE_BEFORE_DRAW
	fimgDumpState(ctx, mode, count, __func__);
#endif

	do {
#if 0
		fimgSelectiveFlush(ctx, FGHI_PIPELINE_FIFO
				| FGHI_PIPELINE_HOSTIF | FGHI_PIPELINE_HVF);
#else
		fimgFlush(ctx);
#endif
		fillVertexBuffer(ctx);
		setupVertexBuffer(ctx);
		drawAutoinc(ctx, 0, copied);
		copied = primitiveHandler[mode].direct(ctx,
							arrays, &first, &count);
	} while (copied);

	/* Release hardware */
	fimgPutHardware(ctx);
}

void fimgDrawElementsUByteIdx(fimgContext *ctx, unsigned int mode,
		fimgArray *arrays, unsigned int count, const uint8_t *indices)
{
	unsigned int copied;
	unsigned int pos = 0;

	if (mode >= FGPE_PRIMITIVE_MAX)
		return;

	if (!primitiveHandler[mode].indexed_8) {
		LOGE("%s: Unsupported mode %d", __func__, mode);
		return;
	}

	if (!ctx->vertexData) {
		ctx->vertexData = memalign(32, VERTEX_BUFFER_SIZE);
		if (!ctx->vertexData) {
			LOGE("Failed to allocate vertex data buffer. Terminating.");
			exit(ENOMEM);
		}
	}

	/* Prepare first batch without waiting for hardware */
	copied = primitiveHandler[mode].indexed_8(ctx,
						arrays, indices, &pos, &count);
	if (!copied)
		return;

	/* Get hardware */
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	setupAttributes(ctx, arrays);
#ifdef FIMG_DUMP_STATE_BEFORE_DRAW
	fimgDumpState(ctx, mode, count, __func__);
#endif

	do {
#if 0
		fimgSelectiveFlush(ctx, FGHI_PIPELINE_FIFO
				| FGHI_PIPELINE_HOSTIF | FGHI_PIPELINE_HVF);
#else
		fimgFlush(ctx);
#endif
		fillVertexBuffer(ctx);
		setupVertexBuffer(ctx);
		drawAutoinc(ctx, 0, copied);
		copied = primitiveHandler[mode].indexed_8(ctx,
						arrays, indices, &pos, &count);
	} while (copied);

	/* Release hardware */
	fimgPutHardware(ctx);
}

void fimgDrawElementsUShortIdx(fimgContext *ctx, unsigned int mode,
		fimgArray *arrays, unsigned int count, const uint16_t *indices)
{
	unsigned int copied;
	unsigned int pos = 0;

	if (mode >= FGPE_PRIMITIVE_MAX)
		return;

	if (!primitiveHandler[mode].indexed_16) {
		LOGE("%s: Unsupported mode %d", __func__, mode);
		return;
	}

	if (!ctx->vertexData) {
		ctx->vertexData = memalign(32, VERTEX_BUFFER_SIZE);
		if (!ctx->vertexData) {
			LOGE("Failed to allocate vertex data buffer. Terminating.");
			exit(ENOMEM);
		}
	}

	/* Prepare first batch without waiting for hardware */
	copied = primitiveHandler[mode].indexed_16(ctx,
						arrays, indices, &pos, &count);
	if (!copied)
		return;

	/* Get hardware */
	fimgGetHardware(ctx);
	fimgFlush(ctx);
	fimgFlushContext(ctx);
	fimgSetVertexContext(ctx, mode);

	setupAttributes(ctx, arrays);
#ifdef FIMG_DUMP_STATE_BEFORE_DRAW
	fimgDumpState(ctx, mode, count, __func__);
#endif

	do {
#if 0
		fimgSelectiveFlush(ctx, FGHI_PIPELINE_FIFO
				| FGHI_PIPELINE_HOSTIF | FGHI_PIPELINE_HVF);
#else
		fimgFlush(ctx);
#endif
		fillVertexBuffer(ctx);
		setupVertexBuffer(ctx);
		drawAutoinc(ctx, 0, copied);
		copied = primitiveHandler[mode].indexed_16(ctx,
						arrays, indices, &pos, &count);
	} while (copied);

	/* Release hardware */
	fimgPutHardware(ctx);
}

/*
 * Context management
 */

void fimgCreateHostContext(fimgContext *ctx)
{
	int i;
	fimgAttribute template;

	ctx->host.control.autoinc = 1;
	ctx->host.control.envb = 1;
	ctx->host.control.numoutattrib = FIMG_ATTRIB_NUM;

	template.val = 0;
	template.srcx = 0;
	template.srcy = 1;
	template.srcz = 2;
	template.srcw = 3;

	for(i = 0; i < FIMG_ATTRIB_NUM; i++)
		ctx->host.attrib[i].val = template.val;
}

void fimgRestoreHostState(fimgContext *ctx)
{
	fimgWrite(ctx, 1, FGHI_IDXOFFSET);
	fimgWrite(ctx, ctx->host.control.val, FGHI_CONTROL);
}
