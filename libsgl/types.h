#ifndef _LIBSGL_TYPES_H_
#define _LIBSGL_TYPES_H_

#include <GLES/gl.h>
#include <cstring>

typedef GLfloat FGLvec4f[4];
typedef GLfloat FGLvec3f[3];
typedef GLfloat FGLvec2f[2];

static inline GLfloat floatFromUByte(GLubyte c)
{
	return (GLfloat)c / ((1 << 8) - 1);
}

static inline GLfloat floatFromByte(GLbyte c)
{
	return (GLfloat)(2*c + 1) / ((1 << 8) - 1);
}

static inline GLfloat floatFromUShort(GLushort c)
{
	return (GLfloat)c / ((1 << 16) - 1);
}

static inline GLfloat floatFromShort(GLshort c)
{
	return (GLfloat)(2*c + 1) / ((1 << 16) - 1);
}

static inline GLfloat floatFromFixed(GLfixed c)
{
	return (GLfloat)c / (1 << 16);
}

static inline GLclampf clampFloat(GLclampf f)
{
	if(f < 0)
		return 0;

	if(f > 1)
		return 1;

	return f;
}

static inline GLubyte ubyteFromClampf(GLclampf c)
{
	return clampFloat(c) * ((1 << 8) - 1);
}

static inline GLubyte ubyteFromClampx(GLclampx c)
{
	return c >> 24;
}

static inline GLint unitFromTextureEnum(GLenum texture)
{
	GLint unit;

	switch(texture) {
	case GL_TEXTURE0:
		unit = 0;
		break;
	case GL_TEXTURE1:
		unit = 1;
		break;
#if 0
	case TEXTURE2:
		unit = 2;
		break;
	case TEXTURE3:
		unit = 3;
		break;
	case TEXTURE4:
		unit = 4;
		break;
	case TEXTURE5:
		unit = 5;
		break;
	case TEXTURE6:
		unit = 6;
		break;
	case TEXTURE7:
		unit = 7;
		break;
#endif
	default:
		return -1;
	}

	return unit;
}

#define MAT4(x, y)	(4*(x) + (y))
struct FGLmatrix {
	GLfloat data[16];

	inline FGLmatrix() {};
	inline FGLmatrix(FGLmatrix const &m) { *this = m; };

	void zero(void);
	void identity(void);
	void load(const GLfloat *m);
	void load(const GLfixed *m);
	inline void load(FGLmatrix const &m) { load(m.data); };
	void multiply(const GLfloat *m);
	void multiply(const GLfixed *m);
	inline void multiply(FGLmatrix const &m) { multiply(m.data); };
	void leftMultiply(FGLmatrix const &m);
	void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void translate(GLfloat x, GLfloat y, GLfloat z);
	void inverseTranslate(GLfloat x, GLfloat y, GLfloat z);
	void scale(GLfloat x, GLfloat y, GLfloat z);
	void inverseScale(GLfloat x, GLfloat y, GLfloat z);
	void frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void inverseFrustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void inverseOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	void inverse(void);
	void transpose(void);

	inline GLfloat *operator[](unsigned int i) { return &data[MAT4(i, 0)]; };
	inline const GLfloat *operator[](unsigned int i) const { return &data[MAT4(i, 0)]; };
	inline FGLmatrix &operator=(FGLmatrix const &m) { memcpy(data, m.data, 16 * sizeof(GLfloat)); return *this; };
};

template<typename T>
class FGLstack {
	T *data;
	unsigned int pos;
	unsigned int max;

public:
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

template<typename T, int size>
class FGLPoolAllocator {
	/* Array of pointers addressed by used names */
	T		guard;
	T		**pool;
	/* Stack of unused names */
	int		*unused;
	unsigned	write;
public:
	/* Better than constructor/destructor since we can call the destructor
	   only if the constructor succeeded */
	int create(void)
	{
		if(size <= 0)
			return -1;

		pool = new T*[size];
		unused = new unsigned[size];
		write = size;

		for(unsigned i = 0; i < size; i++) {
			pool[i] = &guard;
			unused[i] = i + 1;
		}

		return 0;
	}

	void destroy(void)
	{
		delete[] unused;
		delete[] pool;
	}

	inline int get(void)
	{
		if(write == 0)
			/* Out of names */
			return -1;

		write--;
		return unused[write];
	}

	inline void put(unsigned name)
	{
		pool[name] = &guard;
		write++;
	}

	inline const T* &operator[](unsigned name) const
	{
		return pool[name - 1];
	}

	inline T* &operator[](unsigned name)
	{
		return pool[name - 1];
	}

	inline bool isValid(unsigned name)
	{
		return (name <= size) && (pool[name - 1] != &guard);
	}
};

typedef unsigned char FGLubyte;
typedef signed char FGLbyte;
typedef unsigned short FGLushort;
typedef signed short FGLshort;
typedef unsigned int FGLuint;
typedef signed int FGLint;

#define FGL_NO_ERROR	0

#endif // _LIBSGL_TYPES_H_
