#ifndef _LIBSGL_FGLSTACK_
#define _LIBSGL_FGLSTACK_

template<typename T>
class FGLstack {
	T *data;
	int pos;
	int max;

public:
	int depth(void) const
	{
		return pos + 1;
	}

	int size(void) const
	{
		return max + 1;
	}

	int create(unsigned int size)
	{
		max = size - 1;
		pos = 0;

		data = new T[size];
		if(data == NULL)
			return -1;

		return 0;
	}

	void destroy(void)
	{
		delete[] data;
		max = -1;
		pos = -1;
	}

	inline int push(void)
	{
		if(pos >= max)
			return -1;

		data[pos + 1] = data[pos];
		++pos;
		return 0;
	}

	inline int pop(void)
	{
		if(!pos)
			return -1;

		--pos;
		return 0;
	}

	inline T &top(void)
	{
		return data[pos];
	}

	inline const T &top(void) const
	{
		return data[pos];
	}
};

#endif
