/**
 * libsgl/fgltextureobject.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
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

#ifndef _LIBSGL_FGLBUFFEROBJECT_
#define _LIBSGL_FGLBUFFEROBJECT_

#include "fglobject.h"

struct FGLBuffer {
	void *memory;
	int size;

	FGLBuffer() :
		memory(0), size(0) {};

	~FGLBuffer()
	{
		destroy();
	}

	int create(int s)
	{
		if (size != s) {
			if (size != 0)
				free(memory);
			memory = malloc(s);
		}

		if (likely(memory != 0)) {
			size = s;
			return 0;
		}

		return -1;
	}

	void destroy()
	{
		if (unlikely(!isValid()))
			return;

		free(memory);
		size = 0;
	}

	inline const GLvoid *getAddress(const GLvoid *offset)
	{
		if (unlikely(!isValid()))
			return 0;

		if (unlikely((int)offset >= size))
			return 0;

		return (const GLvoid *)((uint8_t *)memory + (int)offset);
	}

	inline const GLvoid *getOffset(const GLvoid *address)
	{
		if (unlikely(!isValid()))
			return 0;

		return (const GLvoid *)((uint8_t *)address - (uint8_t *)memory);
	}

	inline bool isValid(void)
	{
		return memory != 0;
	}
};

typedef FGLObject<FGLBuffer> FGLBufferObject;
typedef FGLObjectBinding<FGLBuffer> FGLBufferObjectBinding;

#endif
