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

static inline GLclampf clampFloat(GLclampf f)
{
	if(f < 0)
		return 0;

	if(f > 1)
		return 1;

	return f;
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
	FGLstack(unsigned int size) : max(size-1), pos(0) { data = new T[size]; };
	~FGLstack() { delete[] data; };

	inline void push(void)
	{
		if(pos < max) {
			data[pos + 1] = data[pos];
			++pos;
		}
	}
	
	inline void pop(void)
	{
		if(pos)
			--pos;
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

typedef unsigned char FGLubyte;
typedef signed char FGLbyte;
typedef unsigned short FGLushort;
typedef signed short FGLshort;
typedef unsigned int FGLuint;
typedef signed int FGLint;

#define FGL_NO_ERROR	0

enum FGLPixelFormat {
    // these constants need to match those
    // in graphics/PixelFormat.java, ui/PixelFormat.h, BlitHardware.h
    FGL_PIXEL_FORMAT_UNKNOWN    =   0,
    FGL_PIXEL_FORMAT_NONE       =   0,

    FGL_PIXEL_FORMAT_RGBA_8888   =   1,  // 4x8-bit ARGB
    FGL_PIXEL_FORMAT_RGBX_8888   =   2,  // 3x8-bit RGB stored in 32-bit chunks
    FGL_PIXEL_FORMAT_RGB_888     =   3,  // 3x8-bit RGB
    FGL_PIXEL_FORMAT_RGB_565     =   4,  // 16-bit RGB
    FGL_PIXEL_FORMAT_BGRA_8888   =   5,  // 4x8-bit BGRA
    FGL_PIXEL_FORMAT_RGBA_5551   =   6,  // 16-bit RGBA
    FGL_PIXEL_FORMAT_RGBA_4444   =   7,  // 16-bit RGBA

    FGL_PIXEL_FORMAT_A_8         =   8,  // 8-bit A
    FGL_PIXEL_FORMAT_L_8         =   9,  // 8-bit L (R=G=B = L)
    FGL_PIXEL_FORMAT_LA_88       = 0xA,  // 16-bit LA
    FGL_PIXEL_FORMAT_RGB_332     = 0xB,  // 8-bit RGB (non paletted)

    // reserved range. don't use.
    FGL_PIXEL_FORMAT_RESERVED_10 = 0x10,
    FGL_PIXEL_FORMAT_RESERVED_11 = 0x11,
    FGL_PIXEL_FORMAT_RESERVED_12 = 0x12,
    FGL_PIXEL_FORMAT_RESERVED_13 = 0x13,
    FGL_PIXEL_FORMAT_RESERVED_14 = 0x14,
    FGL_PIXEL_FORMAT_RESERVED_15 = 0x15,
    FGL_PIXEL_FORMAT_RESERVED_16 = 0x16,
    FGL_PIXEL_FORMAT_RESERVED_17 = 0x17,

    // reserved/special formats
    FGL_PIXEL_FORMAT_Z_16       =  0x18,
    FGL_PIXEL_FORMAT_S_8        =  0x19,
    FGL_PIXEL_FORMAT_SZ_24      =  0x1A,
    FGL_PIXEL_FORMAT_SZ_8       =  0x1B,

    // reserved range. don't use.
    FGL_PIXEL_FORMAT_RESERVED_20 = 0x20,
    FGL_PIXEL_FORMAT_RESERVED_21 = 0x21,
};

#endif // _LIBSGL_TYPES_H_
