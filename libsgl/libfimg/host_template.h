/*
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2013 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifndef _HOST_TEMPLATE_H_
#define _HOST_TEMPLATE_H_

#include <assert.h>
#include <string.h>
#include "host.h"

#ifndef INDEX_TYPE
#error INDEX_TYPE must be defined before including this file.
#endif

/**
 * Copies small number of bytes from source buffer to destination buffer.
 * @param dst Destination buffer.
 * @param src Source buffer.
 * @param len Number of buffers to copy.
 */
static void small_memcpy(uint8_t *dst, const uint8_t *src, uint32_t len)
{
	while (len >= 4) {
		memcpy(dst, src, 4);
		dst += 4;
		src += 4;
		len -= 4;
	}

	while (len--)
		*(dst++) = *(src++);
}

/**
 * Packs attribute data into words (uint16_t indexed variant).
 * @param buf Destination buffer.
 * @param a Attribute array descriptor.
 * @param idx Array of vertex indices.
 * @param cnt Vertex count.
 * @return Size (in bytes) of packed data.
 */
static uint32_t packAttribute(uint8_t *buf, const fimgArray *a,
			      INDEX_TYPE idx, int cnt)
{
	const uint8_t *data;
	uint32_t size;
	uint32_t width;

	/* Vertices must be word aligned */
#ifdef SEQUENTIAL
	data = CBUF_ADDR_8(a->pointer, idx * a->stride);
#endif
	width = ROUND_UP(a->width, 4);
	size = width * cnt;

	while (cnt--) {
		uint32_t len = a->width;
#ifndef SEQUENTIAL
		data = CBUF_ADDR_8(a->pointer, *(idx++) * a->stride);
#endif
		small_memcpy(buf, data, len);
		buf += width;
#ifdef SEQUENTIAL
		data += a->stride;
#endif
	}

	return size;
}

/**
 * Prepares input vertex data for hardware processing.
 * @param ctx Hardware context.
 * @param arrays Array of attribute array descriptors.
 * @param indices Array of vertex indices.
 * @param pos Pointer to index of first vertex index.
 * @param count Pointer to count of unprocessed vertices.
 * @param primData Primitive type information.
 * @return Amount of vertices available to send to hardware.
 */
static uint32_t copyVertices(fimgContext *ctx, const fimgArray *arrays,
			     fimgTransfer *transfers, INDEX_TYPE indices,
			     uint32_t *pos, uint32_t *count,
			     const fimgPrimitiveData *primData)
{
	uint16_t offsets[FIMG_ATTRIB_NUM + 1];
	const fimgArray *a;
	fimgTransfer *t;
	uint32_t batchSize;
	uint32_t offset;
	uint8_t *buf;
	uint32_t i;

	batchSize = calculateBatchSize(transfers);
	batchSize -= primData->extra;

	if (batchSize > *count)
		batchSize = *count;

	if (batchSize < primData->min)
		return 0;

	if (batchSize > primData->min) {
		if (primData->multipleOfTwo && (batchSize % 2))
			--batchSize;
		if (primData->multipleOfThree && (batchSize % 3))
			batchSize -= batchSize % 3;
	}

	offset = DATA_OFFSET;
	for (i = 0, t = transfers; t->array.width; ++t, ++i) {
		unsigned int width = ROUND_UP(t->array.width, 4);
		uint32_t j;

		for (j = 0; j < t->numAttribs; ++j) {
			a = &arrays[t->attribs[j]];
			setVtxBufAttrib(ctx, t->attribs[j],
					offset + t->offsets[j], width,
					batchSize + primData->extra);
		}

		offsets[i] = offset;
		offset += width * (batchSize + primData->extra);
	}
	ctx->vertexDataSize = offset;

	for (i = 0, t = transfers; t->array.width; ++t, ++i) {
		buf = BUF_ADDR_8(ctx->vertexData, offsets[i]);
		a = &t->array;
#ifdef SEQUENTIAL
		if (ROUND_UP(a->width, 4) == a->stride) {
			if (primData->repeatFirst) {
				memcpy(buf, a->pointer, a->width);
				buf += a->stride;
				memcpy(buf, a->pointer, a->width);
				buf += a->stride;
				memcpy(buf, a->pointer, a->width);
				buf += a->stride;
			}

			memcpy(buf, BUF_ADDR_8(a->pointer,
				(*pos + primData->shift) * a->stride),
				(batchSize - primData->shift) * a->stride);
			buf += (batchSize - primData->shift) * a->stride;

			if (primData->repeatLast) {
				memcpy(buf, BUF_ADDR_8(a->pointer,
					(*pos + batchSize - 1) * a->stride),
					a->width);
				buf += a->stride;
			}

			continue;
		}
#endif
		if (primData->repeatFirst) {
			buf += packAttribute(buf, a, indices, 1);
			buf += packAttribute(buf, a, indices, 1);
			buf += packAttribute(buf, a, indices, 1);
		}

		buf += packAttribute(buf, a, indices + *pos + primData->shift,
					batchSize - primData->shift);

		if (primData->repeatLast)
			buf += packAttribute(buf, a,
					indices + *pos + batchSize - 1, 1);
	}

	*pos += batchSize - primData->overlap;
	*count -= batchSize - primData->overlap;
	return batchSize + primData->extra;
}

/**
 * Draws a sequence of vertices described by array descriptors.
 * @param ctx Hardware context.
 * @param mode Primitive type.
 * @param arrays Array of attribute array descriptors.
 * @param count Vertex count.
 * @param indices First index.
 */
static void fimgDraw(fimgContext *ctx, unsigned int mode, fimgArray *arrays,
		     unsigned int count, INDEX_TYPE indices)
{
	unsigned int copied;
	unsigned int pos = 0;
	fimgTransfer transfers[FIMG_ATTRIB_NUM + 1];

	if (prepareDraw(ctx, arrays, mode, transfers))
		return;

	copied = copyVertices(ctx, arrays, transfers, indices, &pos,
					&count, &primitiveData[mode]);
	if (!copied)
		return;

	setupHardware(ctx, mode, arrays, count);

	do {
		submitDraw(ctx, copied);
		copied = copyVertices(ctx, arrays, transfers, indices, &pos,
						&count, &primitiveData[mode]);
	} while (copied);
}

#endif /* _HOST_TEMPLATE_H_ */
