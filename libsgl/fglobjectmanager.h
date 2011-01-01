/**
 * libsgl/fglpoolallocator.h
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

#ifndef _LIBSGL_FGLPOOLALLOCATOR_
#define _LIBSGL_FGLPOOLALLOCATOR_

template<typename T, int size>
class FGLObjectManager {
	/* Array of pointers addressed by used names */
	FGLObject<T>	guard;
	FGLObject<T>	**pool;
	void		**owners;
	/* Stack of unused names */
	unsigned	*unused;
	unsigned	write;
	bool		valid;
public:
	FGLObjectManager() :
		guard(0), pool(NULL), unused(NULL), valid(false)
	{
		if(size <= 0)
			return;

		pool = new FGLObject<T>*[size];
		if (pool == NULL)
			return;

		owners = new void*[size];
		if (owners == NULL) {
			delete[] pool;
			return;
		}

		unused = new unsigned[size];
		if (unused == NULL) {
			delete[] owners;
			delete[] pool;
			return;
		}

		write = size;

		for(unsigned i = 0; i < size; i++) {
			pool[i] = &guard;
			owners[i] = 0;
			unused[i] = i + 1;
		}

		valid = true;
	}

	~FGLObjectManager()
	{
		delete[] owners;
		delete[] unused;
		delete[] pool;
	}

	inline int get(void *owner)
	{
		if(write == 0)
			/* Out of names */
			return -1;

		--write;
		owners[write] = owner;
		return unused[write];
	}

	inline void *getOwner(unsigned name)
	{
		return owners[name - 1];
	}

	inline void put(unsigned name)
	{
		pool[name - 1] = &guard;
		owners[name - 1] = 0;
		unused[write] = name;
		++write;
	}

	inline void clean(void *owner)
	{
		for(unsigned i = 0; i < size; i++) {
			if (owners[i] != owner)
				continue;
			if (pool[i]) {
				pool[i]->unbindAll();
				delete pool[i];
			}
			put(i + 1);
		}
	}

	inline const FGLObject<T>* &operator[](unsigned name) const
	{
		return pool[name - 1];
	}

	inline FGLObject<T>* &operator[](unsigned name)
	{
		return pool[name - 1];
	}

	inline bool isValid(unsigned name)
	{
		return (name <= size) && (pool[name - 1] != &guard);
	}
};

#endif
