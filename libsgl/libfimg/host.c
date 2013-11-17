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

#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "fimg_private.h"
#include "host.h"

/**
 * Structure describing requirements of primitive mode regarding
 * geometry format.
 */
const fimgPrimitiveData primitiveData[FGPE_PRIMITIVE_MAX] = {
	[FGPE_POINT_SPRITE] = {
		.min = 1,
	},
	[FGPE_POINTS] = {
		.min = 1,
	},
	[FGPE_LINE_STRIP] = {
		.min = 2,
		.overlap = 1,
	},
	[FGPE_LINE_LOOP] = {
		/*
		 * Line loops don't go well with buffered transfers,
		 * so let's just force higher level code to emulate them
		 * using line strips.
		 */
		.min = 0,
	},
	[FGPE_LINES] = {
		.min = 2,
		.multipleOfTwo = 1,
	},
	[FGPE_TRIANGLE_STRIP] = {
		.min = 3,
		.overlap = 2,
		.extra = 1,
		.multipleOfTwo = 1,
		.repeatLast = 1,
	},
	[FGPE_TRIANGLE_FAN] = {
		.min = 3,
		.overlap = 2,
		.extra = 2,
		.shift = 1,
		.repeatFirst = 1,
	},
	[FGPE_TRIANGLES] = {
		.min = 3,
		.multipleOfThree = 1,
	},
};

/**
 * Sets output attribute count of host interface.
 * @param ctx Hardware context.
 * @param count Attribute count.
 */
void fimgSetAttribCount(fimgContext *ctx, unsigned char count)
{
	ctx->numAttribs = count;
}

/**
 * This function specifies the property of attribute
 * @param attribIdx the index of attribute, which is in [0-15]
 * @param AttribInfo fimgAttribute
 */
void fimgSetAttribute(fimgContext *ctx, unsigned int idx,
					unsigned int type, unsigned int numComp)
{
	ctx->hw.host.attrib[idx].dt = type;
	ctx->hw.host.attrib[idx].numcomp = FGHI_NUMCOMP(numComp);
}

/**
 * This function defines the property of attribute in vertex buffer
 * @param attribIdx the index of attribute
 * @param AttribInfo fimgVtxBufAttrib
 */
void setVtxBufAttrib(fimgContext *ctx, unsigned char idx,
		     unsigned short base, unsigned char stride,
		     unsigned short range)
{
	ctx->hw.host.vbbase[idx] = base;
	ctx->hw.host.vbctrl[idx].stride = stride;
	ctx->hw.host.vbctrl[idx].range = range;
	fimgQueue(ctx, ctx->hw.host.vbctrl[idx].val, FGHI_ATTRIB_VBCTRL(idx));
	fimgQueue(ctx, ctx->hw.host.vbbase[idx], FGHI_ATTRIB_VBBASE(idx));
}

/**
 * Sends a request to hardware to draw a sequence of vertices.
 * @param ctx Hardware context.
 * @param first Index of first vertex in vertex buffer.
 * @param count Vertex count.
 */
void submitDraw(fimgContext *ctx, uint32_t count)
{
	struct g3d_req_draw_buffer *req;

	fimgQueueFlush(ctx);

	req = fimgGetRequest(ctx, G3D_REQUEST_DRAW, sizeof(*req));

	req->vertices = count;
	req->handle = ctx->drawBufferHandle;
	req->offset = ctx->vertexData - ctx->vertexBuffers;
	req->length = (ctx->vertexDataSize + 31) & ~31;

	++ctx->numVertexBuffers;
	ctx->vertexData += VERTEX_BUFFER_SIZE;

	if (ctx->numVertexBuffers == FIMG_NUM_VERTEX_BUFFERS) {
		fimgFlush(ctx);
		fimgWaitForFlush(ctx);
		ctx->numVertexBuffers = 0;
		ctx->vertexData = ctx->vertexBuffers;
	}
}

/**
 * Configures hardware for attributes according to attribute array descriptors.
 * @param ctx Hardware context.
 * @param arrays Array of attribute array descriptors.
 */
static void setupAttributes(fimgContext *ctx, fimgArray *arrays)
{
	unsigned int i;

	// write attribute configuration
	for (i = 0; i < ctx->numAttribs - 1; ++i) {
		ctx->hw.host.attrib[i].lastattr = 0;
		fimgQueue(ctx, ctx->hw.host.attrib[i].val, FGHI_ATTRIB(i));
	}

	// write the last one
	ctx->hw.host.attrib[i].lastattr = 1;
	fimgQueue(ctx, ctx->hw.host.attrib[i].val, FGHI_ATTRIB(i));
}

static int arrayCompare(const void *a, const void *b, void *data)
{
	const fimgArray *attribs = data;
	const fimgArray *arrayA = &attribs[*(const uint8_t *)a];
	const fimgArray *arrayB = &attribs[*(const uint8_t *)b];

	return (intptr_t)arrayA->pointer - (intptr_t)arrayB->pointer;
}

/**
 * Checks basic drawing conditions and allocates vertex data buffer
 * if not present. Must be called before starting geometry data processing.
 * @param ctx Hardware context.
 * @param mode Primitive type.
 * @return Zero on success, negative on error.
 */
int prepareDraw(fimgContext *ctx, const fimgArray *attribs,
		unsigned int mode, fimgTransfer *transfers)
{
	uint8_t arrays[FIMG_ATTRIB_NUM + 1];
	unsigned int numArrays;
	unsigned int i;

	if (mode >= FGPE_PRIMITIVE_MAX || !primitiveData[mode].min) {
		LOGE("%s: Unsupported mode %d", __func__, mode);
		return -EINVAL;
	}

	if (!ctx->vertexBuffers) {
		int ret;

		ret = fimgCreateGEM(ctx, DRAW_BUFFER_SIZE,
					FIMG_GEM_DRAW_BUFFER,
					&ctx->drawBufferHandle);
		if (ret < 0) {
			LOGE("Failed to allocate draw buffer");
			exit(ENOMEM);
		}

		ctx->vertexBuffers = fimgMapGEM(ctx, ctx->drawBufferHandle,
						DRAW_BUFFER_SIZE);
		if (!ctx->vertexBuffers) {
			LOGE("Failed to map draw buffer");
			exit(EFAULT);
		}

		ctx->vertexData = ctx->vertexBuffers;
	}

	/* Filter out constants */
	numArrays = 0;
	for (i = 0; i < ctx->numAttribs; ++i) {
		const fimgArray *a = &attribs[i];

		if (attributeIsConstant(a)) {
			setVtxBufAttrib(ctx, i, CONST_ADDR(i), 0, 0xffff);

			memcpy(BUF_ADDR_8(ctx->vertexData, CONST_ADDR(i)),
				a->pointer, a->width);

			continue;
		}

		arrays[numArrays++] = i;
	}

	/* Check if there are any variable arrays */
	if (!numArrays) {
		/* Mark last transfer entry as sentinel */
		transfers->array.width = 0;
		return 0;
	}

	/* Try to detect interleaved arrays */
	qsort_r(arrays, numArrays, sizeof(*arrays), arrayCompare,
		(void *)attribs);

	for (i = 0; i < numArrays;) {
		uint8_t attrib = arrays[i];
		fimgArray *array = &transfers->array;
		unsigned int j = i + 1;

		transfers->array = attribs[attrib];
		transfers->attribs[0] = attrib;
		transfers->offsets[0] = 0;
		transfers->numAttribs = 1;

		/* Filter out crazy strides */
		if (array->stride > 4 * MAX_WORDS_PER_VERTEX)
			goto skipInterleaved;

		for (; j < numArrays; ++j) {
			uint8_t attrib2 = arrays[j];
			const fimgArray *array2 = &attribs[attrib2];
			uint8_t offset;

			/* Interleaved arrays must be contiguous */
			if ((intptr_t)array2->pointer >
			    ROUND_UP((intptr_t)array->pointer
			    + array->width, 4))
				break;

			/* All attributes must have the same stride */
			if (array2->stride != array->stride)
				break;

			offset = array2->pointer - array->pointer;

			/* Attributes must be word aligned */
			if (offset % 4)
				break;

			if (offset + array2->width > array->width)
				array->width = offset + array2->width;

			transfers->offsets[transfers->numAttribs] = offset;
			transfers->attribs[transfers->numAttribs++] = attrib2;
		}

skipInterleaved:
		assert(array->width <= array->stride);
		transfers++;
		i = j;
	}

	/* Mark last transfer entry as sentinel */
	transfers->array.width = 0;

	return 0;
}

/**
 * Configures hardware for drawing operation - flushes fixed pipeline
 * emulation context, sets vertex context and configures attributes.
 * @param ctx Hardware context.
 * @param mode Primitive type.
 * @param arrays Array describing layout of attributes.
 * @param count Count of vertices to draw.
 */
void setupHardware(fimgContext *ctx, unsigned int mode,
			  fimgArray *arrays, unsigned int count)
{
#ifdef FIMG_FIXED_PIPELINE
	fimgCompatFlush(ctx);
#endif
	fimgSetVertexContext(ctx, mode);
	setupAttributes(ctx, arrays);
#ifdef FIMG_DUMP_STATE_BEFORE_DRAW
	fimgDumpState(ctx, mode, count, __func__);
#endif
}

/**
 * Table used to calculate how many vertices will fit into vertex buffer.
 * CPUs don't like divisions, so a table of precalulated values is used,
 * as the range of input number is pretty small (1 to 32 words per vertex).
 *
 * Each entry is vertex buffer capacity divided by number of words per vertex
 * equal to index into the array.
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

/**
 * Calculates how many vertices will fit into vertex buffer with given config.
 * @param arrays Pointer to array of attribute array descriptors.
 * @param numArrays Count of attribute array descriptors.
 * @param count Count of available vertices in input data.
 * @param primData Primitive type information.
 * @return Count of vertices that will fit into vertex buffer.
 */
unsigned int calculateBatchSize(const fimgTransfer *transfers)
{
	unsigned int size = 0;

	while (transfers->array.width) {
		/* Round up to full words */
		size += (transfers->array.width + 3) / 4;
		++transfers;
	}

	return vertexWordsToVertexCount[size];
}

/*
 * Context management
 */

/**
 * Creates hardware context for host interface block.
 * @param ctx Hardware context.
 */
void fimgCreateHostContext(fimgContext *ctx)
{
	int i;
	fimgAttribute template;

	template.val = 0;
	template.srcx = 0;
	template.srcy = 1;
	template.srcz = 2;
	template.srcw = 3;
	template.lastattr = 1;

	for(i = 0; i < FIMG_ATTRIB_NUM + 1; i++)
		ctx->hw.host.attrib[i].val = template.val;
}

void fimgDestroyHostContext(fimgContext *ctx)
{
	if (ctx->vertexBuffers)
		fimgUnmapAndDestroyGEMHandle(ctx, ctx->drawBufferHandle,
				ctx->vertexBuffers, DRAW_BUFFER_SIZE);
}
