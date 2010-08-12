/*
 * fimg/host.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED DEFINITIONS
 *
 * Copyrights:	2010 by Tomasz Figa <tomasz.figa@gmail.com>
 */

#ifndef _HOST_H_
#define _HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*
 * Host interface
 */

/* Type definitions */
typedef union {
	unsigned int val;
	struct {
		unsigned envb		:1;
		unsigned		:5;
		unsigned idxtype	:2;
		unsigned		:7;
		unsigned autoinc	:1;
		unsigned		:11;
		unsigned envc		:1;
		unsigned numoutattrib	:4;
	} bits;
} fimgHInterface;

typedef struct {
	union {
		unsigned int val;
		struct {
			unsigned stride		:8;
			unsigned		:8;
			unsigned range		:16;
		} bits;
	} ctrl;
	union {
		unsigned int val;
		struct {
			unsigned		:16;
			unsigned addr		:16;
		} bits;
	} base;
} fimgVtxBufAttrib;

typedef union {
	struct {
		unsigned lastattr	:1;
		unsigned		:15;
		unsigned dt		:4;
		unsigned		:2;
		unsigned numcomp	:2;
		unsigned srcw		:2;
		unsigned srcz		:2;
		unsigned srcy		:2;
		unsigned srcx		:2;
	} bits;
	unsigned int val;
} fimgAttribute;

#define FGHI_NUMCOMP(i)		((i) - 1)

typedef enum {
	FGHI_ATTRIB_DT_BYTE = 0,
	FGHI_ATTRIB_DT_SHORT,
	FGHI_ATTRIB_DT_INT,
	FGHI_ATTRIB_DT_FIXED,
	FGHI_ATTRIB_DT_UBYTE,
	FGHI_ATTRIB_DT_USHORT,
	FGHI_ATTRIB_DT_UINT,
	FGHI_ATTRIB_DT_FLOAT,
	FGHI_ATTRIB_DT_NBYTE,
	FGHI_ATTRIB_DT_NSHORT,
	FGHI_ATTRIB_DT_NINT,
	FGHI_ATTRIB_DT_NFIXED,
	FGHI_ATTRIB_DT_NUBYTE,
	FGHI_ATTRIB_DT_NUSHORT,
	FGHI_ATTRIB_DT_NUINT,
	FGHI_ATTRIB_DT_HALF_FLOAT
} fimgHostDataType;

/* Functions */
unsigned int fimgGetNumEmptyFIFOSlots(void);
void fimgSendToFIFO(unsigned int bytes, void *buffer);
void fimgDrawNonIndexArrays(unsigned int numAttribs,
			fimgAttribute *pAttrib,
			unsigned int numVertices,
			void **ppvData, void **pConst,
			unsigned int *pStride);
void fimgSetHInterface(fimgHInterface HI);
void fimgSetIndexOffset(unsigned int offset);
void fimgSetVtxBufferAddr(unsigned int addr);
void fimgSendToVtxBuffer(unsigned int data);
void fimgSetAttribute(unsigned char attribIdx,
		      fimgAttribute AttribInfo);
void fimgSetVtxBufAttrib(unsigned char attribIdx,
			 fimgVtxBufAttrib AttribInfo);

#ifdef __cplusplus
}
#endif

#endif
