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

#ifndef _HOST_H_
#define _HOST_H_

#include <string.h>

#define FGHI_ATTRIB(i)		(FGHI_ATTRIB0 + (i))
#define FGHI_ATTRIB_VBCTRL(i)	(FGHI_ATTRIB_VBCTRL0 + (i))
#define FGHI_ATTRIB_VBBASE(i)	(FGHI_ATTRIB_VBBASE0 + (i))

#define VERTEX_BUFFER_CONST	(MAX_WORDS_PER_VERTEX)
#define VERTEX_BUFFER_WORDS	(VERTEX_BUFFER_SIZE / 4 - VERTEX_BUFFER_CONST)

#define MAX_ATTRIBS		(FIMG_ATTRIB_NUM)
#define MAX_WORDS_PER_ATTRIB	(4)
#define MAX_WORDS_PER_VERTEX	(MAX_ATTRIBS*MAX_WORDS_PER_ATTRIB)

#define CONST_ADDR(attrib)	(4*MAX_WORDS_PER_ATTRIB*(attrib))
#define DATA_OFFSET		(CONST_ADDR(MAX_ATTRIBS))

#define ROUND_UP(val, to)	(((val) + (to) - 1) & ~((to) - 1))

#define CBUF_ADDR_32(buf, offs)	\
			((const uint32_t *)((const uint8_t *)(buf) + (offs)))
#define CBUF_ADDR_16(buf, offs)	\
			((const uint16_t *)((const uint8_t *)(buf) + (offs)))
#define CBUF_ADDR_8(buf, offs)	\
			((const uint8_t *)(buf) + (offs))

#define BUF_ADDR_32(buf, offs)	\
			((uint32_t *)((uint8_t *)(buf) + (offs)))
#define BUF_ADDR_16(buf, offs)	\
			((uint16_t *)((uint8_t *)(buf) + (offs)))
#define BUF_ADDR_8(buf, offs)	\
			((uint8_t *)(buf) + (offs))

/**
 * Structure describing requirements of primitive mode regarding
 * geometry format.
 */
typedef struct {
	/**
	 * Minimal count of vertices in batch.
	 * Set to 0 if given primitive type is not supported.
	 */
	unsigned min;
	/** How many vertices of previous batch overlap with next batch. */
	unsigned overlap;
	/** Vertices in batch reserved for extra data. */
	unsigned extra;
	/** How many vertices to skip. */
	unsigned shift;
	/** Set to 1 if batch size must be a multiple of two. */
	unsigned multipleOfTwo:1;
	/** Set to 1 if batch size must be a multiple of three. */
	unsigned multipleOfThree:1;
	/** Set to 1 if first vertex must be repeated three times. */
	unsigned repeatFirst:1;
	/** Set to 1 if last vertex must be repeated. */
	unsigned repeatLast:1;
} fimgPrimitiveData;

typedef struct {
	fimgArray array;
	uint8_t attribs[FIMG_ATTRIB_NUM + 1];
	uint8_t offsets[FIMG_ATTRIB_NUM + 1];
	uint8_t numAttribs;
} fimgTransfer;

void setVtxBufAttrib(fimgContext *ctx, unsigned char idx,
		     unsigned short base, unsigned char stride,
		     unsigned short range);
unsigned int calculateBatchSize(const fimgTransfer *transfers);
int prepareDraw(fimgContext *ctx, const fimgArray *arrays,
		unsigned int mode, fimgTransfer *transfers);
void setupHardware(fimgContext *ctx, unsigned int mode,
			  fimgArray *arrays, unsigned int count);
void submitDraw(fimgContext *ctx, uint32_t count);

extern const fimgPrimitiveData primitiveData[];

/**
 * Checks whether given attribute is a constant.
 * @param array Attribute descriptor.
 * @return Boolean value, which is true when attribute is a constant.
 */
static inline int attributeIsConstant(const fimgArray *array)
{
	return !array->stride;
}

#endif /* _HOST_H_ */
