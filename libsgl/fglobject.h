/**
 * libsgl/fglobject.h
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

#ifndef _LIBSGL_FGLOBJECT_
#define _LIBSGL_FGLOBJECT_

#include <cassert>

/*
 * FGLObjectBinding
 *
 * A binding object that can be bound to a single binding point object.
 */

template<typename T>
class FGLObject;

template<typename T>
class FGLObjectBinding {
	FGLObject<T> *object;
	FGLObjectBinding<T> *next;
	FGLObjectBinding<T> *prev;

public:
	FGLObjectBinding() :
		object(0) {};

	inline bool isBound(void)
	{
		return object != NULL;
	}

	inline void unbind(void)
	{
		if (!object)
			return;

		object->unbind(this);
	}

	inline void bind(FGLObject<T> *o)
	{
		o->bind(this);
	}

	inline T* get(void)
	{
		if (!object)
			return 0;

		return &object->object;
	}

	inline unsigned int getName(void)
	{
		return object->name;
	}

	friend class FGLObject<T>;
};

/*
 * FGLObjectBindingPoint
 *
 * A binding point object to which unlimited amount of binding objects
 * can be bound.
 */
template<typename T>
class FGLObject {
	FGLObjectBinding<T> *list;
	unsigned int name;

public:
	T object;

	FGLObject(unsigned int id) :
		list(NULL), name(id), object() {};

	~FGLObject()
	{
		unbindAll();
	}

	inline void unbindAll(void)
	{
		FGLObjectBinding<T> *b = list;

		while(b) {
			b->object = NULL;
			b = b->next;
		}

		list = NULL;
	}

	inline void unbind(FGLObjectBinding<T> *b)
	{
		if (!isBound(b))
			return;

		if (b->next)
			b->next->prev = b->prev;

		if (b->prev)
			b->prev->next = b->next;
		else
			list = NULL;

		b->object = NULL;
	}

	inline void bind(FGLObjectBinding<T> *b)
	{
		if(b->isBound())
			b->unbind();

		b->next = list;
		b->prev = NULL;

		if(list)
			list->prev = b;

		list = b;
		b->object = this;
	}

	inline bool isBound(FGLObjectBinding<T> *b)
	{
		return b->object == this;
	}

	friend class FGLObjectBinding<T>;
};

#endif