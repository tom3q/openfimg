#ifndef _LIBSGL_FGLMATRIX_
#define _LIBSGL_FGLMATRIX_

#include <cstring>
#include "types.h"

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
	void multiply(FGLmatrix const &a, FGLmatrix const &b);
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

#endif
