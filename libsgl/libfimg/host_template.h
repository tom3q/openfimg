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
 * Packs attribute data into words (uint16_t indexed variant).
 * @param buf Destination buffer.
 * @param a Attribute array descriptor.
 * @param idx Array of vertex indices.
 * @param cnt Vertex count.
 * @return Size (in bytes) of packed data.
 */
static uint32_t packAttribute(uint8_t *buf, fimgArray *a,
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
#ifndef SEQUENTIAL
		data = CBUF_ADDR_8(a->pointer, *(idx++) * a->stride);
#endif
		switch(a->width) {
		case 1:		memcpy(buf, data, 1);	break;
		case 2:		memcpy(buf, data, 2);	break;
		case 3:		memcpy(buf, data, 3);	break;
		case 4:		memcpy(buf, data, 4);	break;
		case 6:		memcpy(buf, data, 6);	break;
		case 8:		memcpy(buf, data, 8);	break;
		case 12:	memcpy(buf, data, 12);	break;
		case 16:	memcpy(buf, data, 16);	break;
		default:	assert(0);
		}

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
static uint32_t copyVertices(fimgContext *ctx, fimgArray *arrays,
			     INDEX_TYPE indices, uint32_t *pos, uint32_t *count,
			     const fimgPrimitiveData *primData)
{
	fimgArray *a = arrays;
	uint32_t batchSize;
	uint32_t i;
	uint32_t offset = DATA_OFFSET;
	uint8_t *buf = ctx->vertexData;

	batchSize = calculateBatchSize(arrays, ctx->numAttribs);
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

	for (i = 0; i < ctx->numAttribs; ++i, ++a) {
		if (attributeIsConstant(a)) {
			setupConstant(ctx, i, a, batchSize + primData->extra);
			continue;
		}

		setVtxBufAttrib(ctx, i, offset, ROUND_UP(a->width, 4),
				batchSize + primData->extra);
#ifdef SEQUENTIAL
		if (ROUND_UP(a->width, 4) == a->stride) {
			if (primData->repeatFirst) {
				memcpy(buf + offset, a->pointer, a->width);
				offset += a->width;
				memcpy(buf + offset, a->pointer, a->width);
				offset += a->width;
				memcpy(buf + offset, a->pointer, a->width);
				offset += a->width;
			}

			memcpy(buf + offset, BUF_ADDR_8(a->pointer,
				(*pos + primData->shift) * a->stride),
				(batchSize - primData->shift) * a->width);
			offset += (batchSize - primData->shift) * a->width;

			if (primData->repeatLast) {
				memcpy(buf + offset, BUF_ADDR_8(a->pointer,
					(*pos + batchSize - 1) * a->stride),
					a->width);
				offset += a->width;
			}

			continue;
		}
#endif
		if (primData->repeatFirst) {
			offset += packAttribute(BUF_ADDR_8(buf, offset), a,
						indices, 1);
			offset += packAttribute(BUF_ADDR_8(buf, offset), a,
						indices, 1);
			offset += packAttribute(BUF_ADDR_8(buf, offset), a,
						indices, 1);
		}

		offset += packAttribute(BUF_ADDR_8(buf, offset), a,
					indices + *pos + primData->shift,
					batchSize - primData->shift);

		if (primData->repeatLast)
			offset += packAttribute(BUF_ADDR_8(buf, offset), a,
					indices + *pos + batchSize - 1, 1);
	}

	ctx->vertexDataSize = offset;

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

	if (prepareDraw(ctx, mode))
		return;

	copied = copyVertices(ctx, arrays, indices, &pos,
					&count, &primitiveData[mode]);
	if (!copied)
		return;

	setupHardware(ctx, mode, arrays, count);

	do {
		submitDraw(ctx, copied);
		copied = copyVertices(ctx, arrays, indices, &pos,
						&count, &primitiveData[mode]);
	} while (copied);
}

#endif /* _HOST_TEMPLATE_H_ */
