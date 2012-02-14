/*
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

template<typename T1, typename T2>
class FGLObject;

template<typename T1, typename T2>
class FGLObjectBindingIterator;

/*
 * FGLObjectBinding
 *
 * An object that allows a single object of type T1 to be bound to an object
 * of type T2 using appropriate FGLObject object.
 */
template<typename T1, typename T2>
class FGLObjectBinding {
	T2 *parent;
	FGLObject<T1, T2> *object;
	FGLObjectBinding<T1, T2> *next;
	FGLObjectBinding<T1, T2> *prev;

public:
	FGLObjectBinding(T2 *parent = 0) :
		parent(parent),
		object(0),
		next(this),
		prev(this) {};

	~FGLObjectBinding()
	{
		bind(0);
	}

	inline bool isBound(void)
	{
		return object != 0;
	}

	inline void bind(FGLObject<T1, T2> *o)
	{
		if (object)
			object->unbind(this);
		if (o)
			o->bind(this);
	}

	inline T1* get(void)
	{
		T1 *ptr = 0;
		if (object)
			ptr = object->get();
		return ptr;
	}

	inline T2* getParent(void)
	{
		return parent;
	}

	friend class FGLObject<T1, T2>;
	friend class FGLObjectBindingIterator<T1, T2>;
};

/*
 * FGLObjectBindingIterator
 *
 * An object that allows to access all objects of type T2, to which
 * a given object of type T1 is bound using its FGLObject object.
 */
template<typename T1, typename T2>
class FGLObjectBindingIterator {
	FGLObject<T1, T2> *object;
	FGLObjectBinding<T1, T2> *ptr;

public:
	T2 *get(void)
	{
		return ptr->getParent();
	}

	FGLObjectBindingIterator<T1, T2> &operator++(void)
	{
		if (ptr != &object->sentinel)
			ptr = ptr->next;

		return *this;
	}

	FGLObjectBindingIterator<T1, T2> &operator++(int)
	{
		FGLObjectBindingIterator<T1, T2> copy = *this;

		if (ptr != &object->sentinel)
			ptr = ptr->next;

		return copy;
	}

	bool operator==(const FGLObjectBindingIterator<T1, T2> &op)
	{
		return ptr == op.ptr;
	}

	bool operator!=(const FGLObjectBindingIterator<T1, T2> &op)
	{
		return ptr != op.ptr;
	}

	FGLObjectBindingIterator(FGLObject<T1, T2> *o,
						FGLObjectBinding<T1, T2> *p) :
		object(o), ptr(p) {}
};

/*
 * FGLObject
 *
 * An object that allows a single object of type T1 to be bound to multiple
 * objects of type T2 using appropriate FGLObjectBinding objects.
 */
template<typename T1, typename T2>
class FGLObject {
	FGLObjectBinding<T1, T2> sentinel;
	T1 *parent;

public:
	FGLObject(T1 *parent = 0) :
		parent(parent) {};

	~FGLObject()
	{
		unbindAll();
	}

	inline T1 *get(void)
	{
		return parent;
	}

	inline void unbindAll(void)
	{
		FGLObjectBinding<T1, T2> *b = sentinel.next;

		while(b != &sentinel) {
			b->object = 0;
			b = b->next;
		}

		sentinel.next = sentinel.prev = &sentinel;
	}

	inline void unbind(FGLObjectBinding<T1, T2> *b)
	{
		if (!isBound(b))
			return;

		b->next->prev = b->prev;
		b->prev->next = b->next;

		b->object = 0;
	}

	inline void bind(FGLObjectBinding<T1, T2> *b)
	{
		b->bind(0);

		b->next = sentinel.next;
		b->prev = &sentinel;
		sentinel.next->prev = b;
		sentinel.next = b;

		b->object = this;
	}

	inline bool isBound(FGLObjectBinding<T1, T2> *b)
	{
		return b->object == this;
	}

	inline FGLObjectBindingIterator<T1, T2> begin(void)
	{
		return FGLObjectBindingIterator<T1, T2>(this, sentinel.next);
	}

	inline FGLObjectBindingIterator<T1, T2> end(void)
	{
		return FGLObjectBindingIterator<T1, T2>(this, &sentinel);
	}

	friend class FGLObjectBinding<T1, T2>;
	friend class FGLObjectBindingIterator<T1, T2>;

	typedef FGLObjectBindingIterator<T1, T2> iterator;
};

#endif
