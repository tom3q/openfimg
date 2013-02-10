/*
 * libsgl/fglstack.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011-2012 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifndef _LIBSGL_FGLSTACK_
#define _LIBSGL_FGLSTACK_

/**
 * A class implementing a basic fixed depth stack.
 * @tparam T Type of stack elements.
 */
template<typename T>
class FGLstack {
	T *data;
	int pos;
	int max;

public:
	/**
	 * Gets current stack depth.
	 * @return Stack depth.
	 */
	int depth(void) const
	{
		return pos + 1;
	}

	/**
	 * Gets stack capacity.
	 * @return Stack capacity.
	 */
	int size(void) const
	{
		return max + 1;
	}

	/**
	 * Allocates stack of given capacity.
	 * @param size Stack capacity.
	 * @return Zero on success, non-zero on failure.
	 */
	int create(unsigned int size)
	{
		max = size - 1;
		pos = 0;

		data = new T[size];
		if(data == NULL)
			return -1;

		return 0;
	}

	/** Frees stack memory. */
	void destroy(void)
	{
		delete[] data;
		max = -1;
		pos = -1;
	}

	/**
	 * Pushes new element on the stack.
	 * The element is a copy of current stack top.
	 * @return 0 on success, -1 on overflow.
	 */
	inline int push(void)
	{
		if(pos >= max)
			return -1;

		data[pos + 1] = data[pos];
		++pos;
		return 0;
	}

	/**
	 * Removes element from top of the stack.
	 * @return 0 on success, -1 on underflow.
	 */
	inline int pop(void)
	{
		if(!pos)
			return -1;

		--pos;
		return 0;
	}

	/**
	 * Gets element from top of the stack.
	 * @return Element from top of the stack.
	 */
	inline T &top(void)
	{
		return data[pos];
	}

	/**
	 * Gets element from top of the stack (const version).
	 * @return Element from top of the stack.
	 */
	inline const T &top(void) const
	{
		return data[pos];
	}
};

#endif
