/*
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

#include <pthread.h>

/**
 * A class that manages namespaces of GL objects.
 * @tparam T Type of managed objects.
 * @tparam size Size of namespace.
 */
template<typename T, int size>
class FGLObjectManager {
	/* Array of pointers addressed by used names */
	T		**pool;
	void		**owners;
	/* Stack of unused names */
	unsigned	*unused;
	unsigned	write;
	bool		valid;
	pthread_mutex_t	mutex;
public:
	/** Constructs object manager object. */
	FGLObjectManager() :
		pool(NULL), unused(NULL), valid(false)
	{
		pthread_mutex_init(&mutex, NULL);

		if(size <= 0)
			return;

		pool = new T*[size];
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
			pool[i] = (T *)i;
			owners[i] = 0;
			unused[i] = i + 1;
		}

		valid = true;
	}

	/** Destroys object manager object. */
	~FGLObjectManager()
	{
		delete[] owners;
		delete[] unused;
		delete[] pool;

		pthread_mutex_destroy(&mutex);
	}

	/**
	 * Allocates an object name.
	 * @param owner Unique ID of name owner.
	 * @return Allocated name or -1 on failure.
	 */
	inline int get(void *owner)
	{
		int name;

		pthread_mutex_lock(&mutex);

		if(write == 0) {
			/* Out of names */
			pthread_mutex_unlock(&mutex);
			return -1;
		}

		--write;

		name = unused[write];

		pool[name - 1] = NULL;
		owners[name - 1] = owner;

		pthread_mutex_unlock(&mutex);

		return name;
	}

	/**
	 * Tries to allocate selected name.
	 * @param name Name to allocate.
	 * @param owner Unique ID of name owner.
	 * @return Allocated name or -1 on failure.
	 */
	inline int get(unsigned name, void *owner)
	{
		if (name == 0 || name > size)
			return -1;

		pthread_mutex_lock(&mutex);

		if (owners[name - 1]) {
			pthread_mutex_unlock(&mutex);
			return -1;
		}

		unsigned pos = (unsigned)pool[name - 1];

		--write;
		unused[pos] = unused[write];
		pool[unused[write] - 1] = (T *)pos;
		pool[name - 1] = NULL;
		owners[name - 1] = owner;

		pthread_mutex_unlock(&mutex);

		return name;
	}

	/**
	 * Frees an object name.
	 * @param name Name to free.
	 */
	inline void put(unsigned name)
	{
		pthread_mutex_lock(&mutex);

		owners[name - 1] = 0;
		pool[name - 1] = (T *)write;
		unused[write] = name;
		++write;

		pthread_mutex_unlock(&mutex);
	}

	/**
	 * Destroys all objects held by given owner and frees their names.
	 * @param owner Owner to destroy objects of.
	 */
	inline void clean(void *owner)
	{
		pthread_mutex_lock(&mutex);

		for(unsigned i = 0; i < size; i++) {
			if (owners[i] != owner)
				continue;
			if (pool[i])
				delete pool[i];
			owners[i] = 0;
			pool[i] = (T *)write;
			unused[write] = i + 1;
			++write;
		}

		pthread_mutex_unlock(&mutex);
	}

	/**
	 * Returns a constant reference to the pointer of object of given name.
	 * @param name Name of requested object.
	 * @return A reference to a pointer pointing to object of given name.
	 */
	inline const T* &operator[](unsigned name) const
	{
		return pool[name - 1];
	}

	/**
	 * Returns a reference to the pointer of object of given name.
	 * @param name Name of requested object.
	 * @return A reference to a pointer pointing to object of given name.
	 */
	inline T* &operator[](unsigned name)
	{
		return pool[name - 1];
	}

	/**
	 * Checks if given name is already assigned.
	 * @param name Name to check.
	 * @return True if the name is assigned, otherwise false.
	 */
	inline bool isValid(unsigned name)
	{
		if (!name || name > size)
			return false;

		pthread_mutex_lock(&mutex);

		if (!owners[name - 1]) {
			pthread_mutex_unlock(&mutex);
			return false;
		}

		pthread_mutex_unlock(&mutex);

		return true;
	}
};

#endif
