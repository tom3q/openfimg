/*
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

#include <cstdlib>
#include <GLES/gl.h>
#include "fglobject.h"

struct FGLBuffer;

/**
 * A wrapper class for buffer object binding.
 * The class wraps an FGLObjectBinding object into a class that can be
 * embedded as a member in another class.
 */
class FGLBufferObjectBinding {
	FGLObjectBinding<FGLBuffer, FGLBufferObjectBinding> binding;

public:
	/** Default constructor. */
	FGLBufferObjectBinding() :
		binding(this) {};

	/**
	 * Checks if there is a buffer object bound to this binding.
	 * @return True if there is a buffer object bound, otherwise false.
	 */
	inline bool isBound(void)
	{
		return binding.isBound();
	}

	/**
	 * Binds a buffer object to this binding.
	 * @param o Pointer of FGLObject of buffer object to bind.
	 */
	inline void bind(FGLObject<FGLBuffer, FGLBufferObjectBinding> *o)
	{
		binding.bind(o);
	}

	/**
	 * Returns buffer object bound to this binding.
	 * @return Pointer to bound buffer object.
	 */
	inline FGLBuffer *get(void)
	{
		return binding.get();
	}
};

/** A class representing OpenGL ES buffer object. */
struct FGLBuffer {
	void *memory;
	int size;
	GLenum usage;
	unsigned int name;
	FGLObject<FGLBuffer, FGLBufferObjectBinding> object;

	/**
	 * Class constructor. Creates buffer object of given name.
	 * @param name Name of the buffer object to create.
	 */
	FGLBuffer(unsigned int name) :
		memory(0),
		size(0),
		usage(GL_STATIC_DRAW),
		name(name),
		object(this) {};

	/**
	 * Class destructor.
	 * Destroys the buffer object and frees all memory used by it.
	 */
	~FGLBuffer()
	{
		destroy();
	}

	/**
	 * Creates backing storage for the buffer.
	 * @param s Size of the storage in bytes (0 will free existing memory).
	 * @return 0 on success, negative on error.
	 */
	int create(int s)
	{
		if (size == s)
			return 0;

		if (size)
			free(memory);

		memory = 0;
		size = 0;
		if (!s)
			return 0;

		memory = malloc(s);
		if (!memory)
			return -1;

		size = s;
		return 0;
	}

	/** Frees existing backing storage of the buffer. */
	void destroy()
	{
		if (unlikely(!isValid()))
			return;

		free(memory);
		memory = 0;
		size = 0;
	}

	/**
	 * Gets pointer to data at given offset of the buffer.
	 * @param offset Offset inside the buffer.
	 * @return Absolute pointer to given offset in the buffer.
	 */
	inline const GLvoid *getAddress(const GLvoid *offset)
	{
		if (unlikely(!isValid()))
			return 0;

		if (unlikely((int)offset >= size))
			return 0;

		return (const GLvoid *)((uint8_t *)memory + (int)offset);
	}

	/**
	 * Returns offset in the buffer of given absolute address.
	 * @param address Absolute pointer to some location inside the buffer.
	 * @return Relative offset inside the buffer.
	 */
	inline const GLvoid *getOffset(const GLvoid *address)
	{
		if (unlikely(!isValid()))
			return 0;

		return (const GLvoid *)((uint8_t *)address - (uint8_t *)memory);
	}

	/**
	 * Checks if the buffer is valid.
	 * A buffer is considered valid if it has allocated backing storage.
	 * @return True if the buffer is valid, otherwise false.
	 */
	inline bool isValid(void)
	{
		return memory != 0;
	}

	/**
	 * Gets buffer name.
	 * @return Name of the buffer.
	 */
	unsigned int getName(void) const
	{
		return name;
	}
};

#endif
