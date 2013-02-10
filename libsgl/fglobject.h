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

/**
 * A class that allows a single object of type T1 to be bound to an object
 * of type T2 using appropriate FGLObject object.
 * @tparam T1 Type of the object.
 * @tparam T2 Type of the binding point.
 */
template<typename T1, typename T2>
class FGLObjectBinding {
	T2 *parent;
	FGLObject<T1, T2> *object;
	FGLObjectBinding<T1, T2> *next;
	FGLObjectBinding<T1, T2> *prev;

public:
	/**
	 * Constructs a binding object.
	 * @param parent Object owning the binding.
	 */
	FGLObjectBinding(T2 *parent = 0) :
		parent(parent),
		object(0),
		next(this),
		prev(this) {};

	/**
	 * Class destructor.
	 * Unbinds bound object (if present) and destroys the binding.
	 */
	~FGLObjectBinding()
	{
		bind(0);
	}

	/**
	 * Checks if there is an object bound to the binding.
	 * @return True if there is a bound object, otherwise false.
	 */
	inline bool isBound(void)
	{
		return object != 0;
	}

	/**
	 * Binds object to the binding.
	 * @param o Pointer to FGLObject of object to bind.
	 */
	inline void bind(FGLObject<T1, T2> *o)
	{
		if (object)
			object->unbind(this);
		if (o)
			o->bind(this);
	}

	/**
	 * Gets bound object.
	 * @return Bound object or NULL if there is no bound object.
	 */
	inline T1* get(void)
	{
		T1 *ptr = 0;
		if (object)
			ptr = object->get();
		return ptr;
	}

	/**
	 * Gets pointer to object owning the binding.
	 * @return Pointer to object owning the binding.
	 */
	inline T2* getParent(void)
	{
		return parent;
	}

	friend class FGLObject<T1, T2>;
	friend class FGLObjectBindingIterator<T1, T2>;
};

/**
 * A class that allows to access all objects of type T2, to which
 * a given object of type T1 is bound using its FGLObject object.
 * @tparam T1 Type of the object.
 * @tparam T2 Type of the binding point.
 */
template<typename T1, typename T2>
class FGLObjectBindingIterator {
	FGLObject<T1, T2> *object;
	FGLObjectBinding<T1, T2> *ptr;

public:
	/**
	 * Gets object binding pointed by the iterator.
	 * @return Pointer to object binding pointed by the iterator.
	 */
	T2 *get(void)
	{
		return ptr->getParent();
	}

	/** Advances the iterator to next object binding. */
	FGLObjectBindingIterator<T1, T2> &operator++(void)
	{
		if (ptr != &object->sentinel)
			ptr = ptr->next;

		return *this;
	}

	/** Advances the iterator to next object binding. */
	FGLObjectBindingIterator<T1, T2> &operator++(int)
	{
		FGLObjectBindingIterator<T1, T2> copy = *this;

		if (ptr != &object->sentinel)
			ptr = ptr->next;

		return copy;
	}

	/**
	 * Equality operator.
	 * Compares iterator position with other iterator.
	 * @param op Iterator to compare with.
	 * @return True if both iterators point to the same position,
	 * otherwise false.
	 */
	bool operator==(const FGLObjectBindingIterator<T1, T2> &op)
	{
		return ptr == op.ptr;
	}

	/**
	 * Inequality operator.
	 * Compares iterator position with other iterator.
	 * @param op Iterator to compare with.
	 * @return False if both iterators point to the same position,
	 * otherwise true.
	 */
	bool operator!=(const FGLObjectBindingIterator<T1, T2> &op)
	{
		return ptr != op.ptr;
	}

	/**
	 * Constructs object binding iterator.
	 * @param o Object of which bindings are iterated.
	 * @param p Binding to start from.
	 */
	FGLObjectBindingIterator(FGLObject<T1, T2> *o,
						FGLObjectBinding<T1, T2> *p) :
		object(o), ptr(p) {}
};

/**
 * A class that allows a single object of type T1 to be bound to multiple
 * objects of type T2 using appropriate FGLObjectBinding objects.
 * @tparam T1 Type of the object.
 * @tparam T2 Type of the binding point.
 */
template<typename T1, typename T2>
class FGLObject {
	FGLObjectBinding<T1, T2> sentinel;
	T1 *parent;

public:
	/**
	 * Constructs a bindable object.
	 * @param parent Object owning the object being created.
	 */
	FGLObject(T1 *parent = 0) :
		parent(parent) {};

	/**
	 * Class destructor.
	 * Unbinds all bindings and destroys the object.
	 */
	~FGLObject()
	{
		unbindAll();
	}

	/**
	 * Gets pointer to object owning this object.
	 * @return Pointer to object owning this object.
	 */
	inline T1 *get(void)
	{
		return parent;
	}

	/** Unbinds the object from all bound bindings. */
	inline void unbindAll(void)
	{
		FGLObjectBinding<T1, T2> *b = sentinel.next;

		while(b != &sentinel) {
			b->object = 0;
			b = b->next;
		}

		sentinel.next = sentinel.prev = &sentinel;
	}

	/**
	 * Unbinds given binding.
	 * @param b Binding to unbind.
	 */
	inline void unbind(FGLObjectBinding<T1, T2> *b)
	{
		if (!isBound(b))
			return;

		b->next->prev = b->prev;
		b->prev->next = b->next;

		b->object = 0;
	}

	/**
	 * Binds the object to given binding.
	 * Any object currently bound to this binding will be unbound.
	 * @param b Binding to bind to.
	 */
	inline void bind(FGLObjectBinding<T1, T2> *b)
	{
		b->bind(0);

		b->next = sentinel.next;
		b->prev = &sentinel;
		sentinel.next->prev = b;
		sentinel.next = b;

		b->object = this;
	}

	/**
	 * Checks if the object is bound to any binding.
	 * @return True if the object is bound, otherwise false.
	 */
	inline bool isBound(FGLObjectBinding<T1, T2> *b)
	{
		return b->object == this;
	}

	/**
	 * Gets iterator to the first bound binding.
	 * @return Iterator pointing to the first binding bound to the object.
	 */
	inline FGLObjectBindingIterator<T1, T2> begin(void)
	{
		return FGLObjectBindingIterator<T1, T2>(this, sentinel.next);
	}

	/**
	 * Gets iterator pointing after last bound binding.
	 * @return Iterator pointing after last binding bound to the object.
	 */
	inline FGLObjectBindingIterator<T1, T2> end(void)
	{
		return FGLObjectBindingIterator<T1, T2>(this, &sentinel);
	}

	friend class FGLObjectBinding<T1, T2>;
	friend class FGLObjectBindingIterator<T1, T2>;

	typedef FGLObjectBindingIterator<T1, T2> iterator;
};

#endif
