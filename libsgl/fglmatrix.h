#ifndef _LIBSGL_FGLMATRIX_
#define _LIBSGL_FGLMATRIX_

#include <cstring>
#include "types.h"

/**
 * Calculates index into linear matrix data array of specified matrix cell.
 * @param col Matrix column.
 * @param row Matrix row.
 * @return Linear index into matrix data array.
 */
#define MAT4(col, row)	(4*(col) + (row))

/** A class representing a 4x4 single precision floating-point matrix. */
struct FGLmatrix {
	GLfloat storage[2*16];

	GLfloat *data;
	int index;

	/**
	 * Default constructor.
	 * Creates unititialized matrix.
	 */
	inline FGLmatrix() :
		data(storage),
		index(0) {};

	/**
	 * Copying constructor.
	 * @param m Matrix to copy.
	 */
	inline FGLmatrix(FGLmatrix const &m) :
		data(storage),
		index(0)
	{
		*this = m;
	}

	/** Fills the matrix with zeroes. */
	void zero(void);
	/** Sets the matrix to identity transformation. */
	void identity(void);
	/**
	 * Loads the matrix from external array.of floating point values.
	 * @param m Array of matrix values (column-major).
	 */
	void load(const GLfloat *m);
	/**
	 * Loads the matrix from external array of fixed point values.
	 * @param m Array of matrix values (column-major; 16.16 fixed point).
	 */
	void load(const GLfixed *m);
	/**
	 * Loads the matrix from another matrix.
	 * @param m Matrix to load data from.
	 */
	inline void load(FGLmatrix const &m) { load(m.data); };
	/**
	 * Multiplies the matrix with floating-point values from external array.
	 * @param m Array of matrix values of the multiplier matrix
	 * (column-major).
	 */
	void multiply(const GLfloat *m);
	/**
	 * Multiplies the matrix with fixed-point values from external array.
	 * @param m Array of matrix values of the multiplier matrix
	 * (column-major; 16.16 fixed point).
	 */
	void multiply(const GLfixed *m);
	/**
	 * Multiplies the matrix by another matrix.
	 * @param m Matrix to multiply by.
	 */
	inline void multiply(FGLmatrix const &m) { multiply(m.data); };
	/**
	 * Multiplies the matrix by another matrix using left-multiplication.
	 * @param m Matrix to multiply by.
	 */
	void leftMultiply(FGLmatrix const &m);
	/**
	 * Loads the matrix with result of multiplication of two matrices.
	 * @param a First matrix.
	 * @param b Second matrix.
	 */
	void multiply(FGLmatrix const &a, FGLmatrix const &b);
	/**
	 * Loads the matrix with rotate transformation with given parameters.
	 * Length (module) of the (x, y, z) vector must be greater than 0.
	 * @param angle Rotation angle in degrees.
	 * @param x X coordinate of the vector defining rotation axis.
	 * @param y Y coordinate of the vector defining rotation axis.
	 * @param z Z coordinate of the vector defining rotation axis.
	 */
	void rotate(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	/**
	 * Loads the matrix with translate transformation with given parameters.
	 * @param x Translation value on X axis.
	 * @param y Translation value on Y axis.
	 * @param z Translation value on Z axis.
	 */
	void translate(GLfloat x, GLfloat y, GLfloat z);
	/**
	 * Loads the matrix with inverse of translate transformation
	 * with given parameters.
	 * @param x Translation value on X axis.
	 * @param y Translation value on Y axis.
	 * @param z Translation value on Z axis.
	 */
	void inverseTranslate(GLfloat x, GLfloat y, GLfloat z);
	/**
	 * Loads the matrix with scale transformation with given parameters.
	 * @param x Scale value of X axis.
	 * @param y Scale value of Y axis.
	 * @param z Scale value of Z axis.
	 */
	void scale(GLfloat x, GLfloat y, GLfloat z);
	/**
	 * Loads the matrix with inverse of scale transformation
	 * with given parameters.
	 * @param x Scale value of X axis (must be non-zero).
	 * @param y Scale value of Y axis (must be non-zero).
	 * @param z Scale value of Z axis (must be non-zero).
	 */
	void inverseScale(GLfloat x, GLfloat y, GLfloat z);
	/**
	 * Loads the matrix with frustum transformation with given parameters.
	 * @param l Left-most coordinate of near clipping plane (must be not
	 * equal to r).
	 * @param r Right-most coordinate of near clipping plane (must be not
	 * equal to l).
	 * @param b Bottom-most coordinate of near clipping plane (must be not
	 * equal to t).
	 * @param t Top-most coordinate of near clipping plane (must be not
	 * equal to b).
	 * @param n Depth of near clipping plane (must be greater than 0
	 * and not equal to f).
	 * @param f Depth of far clipping plane (must be greater than 0
	 * and not equal to n).
	 */
	void frustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/**
	 * Loads the matrix with frustum transformation with given parameters.
	 * @param l Left-most coordinate of near clipping plane (must be not
	 * equal to r).
	 * @param r Right-most coordinate of near clipping plane (must be not
	 * equal to l).
	 * @param b Bottom-most coordinate of near clipping plane (must be not
	 * equal to t).
	 * @param t Top-most coordinate of near clipping plane (must be not
	 * equal to b).
	 * @param n Depth of near clipping plane (must be greater than 0
	 * and not equal to f).
	 * @param f Depth of far clipping plane (must be greater than 0
	 * and not equal to n).
	 */
	void inverseFrustum(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/**
	 * Loads the matrix with parallel projection transformation
	 * with given parameters.
	 * @param l Left-most coordinate of near clipping plane (must be not
	 * equal to r).
	 * @param r Right-most coordinate of near clipping plane (must be not
	 * equal to l).
	 * @param b Bottom-most coordinate of near clipping plane (must be not
	 * equal to t).
	 * @param t Top-most coordinate of near clipping plane (must be not
	 * equal to b).
	 * @param n Depth of near clipping plane (must be not equal to f).
	 * @param f Depth of far clipping plane (must be not equal to n).
	 */
	void ortho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/**
	 * Loads the matrix with inverse of parallel projection transformation
	 * with given parameters.
	 * @param l Left-most coordinate of near clipping plane (must be not
	 * equal to r).
	 * @param r Right-most coordinate of near clipping plane (must be not
	 * equal to l).
	 * @param b Bottom-most coordinate of near clipping plane (must be not
	 * equal to t).
	 * @param t Top-most coordinate of near clipping plane (must be not
	 * equal to b).
	 * @param n Depth of near clipping plane (must be not equal to f).
	 * @param f Depth of far clipping plane (must be not equal to n).
	 */
	void inverseOrtho(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
	/**
	 * Calculates matrix inverse.
	 * If the matrix is singular the result is undefined.
	 */
	void inverse(void);
	/** Transposes the matrix. */
	void transpose(void);

	/**
	 * Returns a pointer to given column of the matrix.
	 * @param i Column number.
	 * @return Pointer to specified column.
	 */
	inline GLfloat *operator[](unsigned int i) { return &data[MAT4(i, 0)]; };
	/**
	 * Returns a constant pointer to given column of the matrix.
	 * @param i Column number.
	 * @return Pointer to specified column.
	 */
	inline const GLfloat *operator[](unsigned int i) const { return &data[MAT4(i, 0)]; };

	/**
	 * Assignment operator copying matrix contents from another matrix.
	 * @param m Matrix being assigned.
	 * @return This matrix.
	 */
	inline FGLmatrix &operator=(FGLmatrix const &m) { memcpy(data, m.data, 16 * sizeof(GLfloat)); return *this; };
};

#endif
