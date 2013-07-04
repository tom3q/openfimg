/*
 * libsgl/common.h
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

#ifndef _LIBSGL_COMMON_H_
#define _LIBSGL_COMMON_H_

/** Allow non power of two textures */
#define FGL_NPOT_TEXTURES

/** Number of available texture units */
#define FGL_MAX_TEXTURE_UNITS		FIMG_NUM_TEXTURE_UNITS
/** Texture object namespace size */
#define FGL_MAX_TEXTURE_OBJECTS		1024
/** Buffer object namespace size */
#define FGL_MAX_BUFFER_OBJECTS		1024
/** Framebuffer object namespace size */
#define FGL_MAX_FRAMEBUFFER_OBJECTS	1024
/** Renderbuffer object namespace size */
#define FGL_MAX_RENDERBUFFER_OBJECTS	1024
/** Highest mipmap level */
#define FGL_MAX_MIPMAP_LEVEL		11
/** Number of supported light sources */
#define FGL_MAX_LIGHTS			8
/** Number of supported user clip planes */
#define FGL_MAX_CLIP_PLANES		1
/** Modelview matrix stack depth */
#define FGL_MAX_MODELVIEW_STACK_DEPTH	16
/** Projection matrix stack depth */
#define FGL_MAX_PROJECTION_STACK_DEPTH	2
/** Projection matrix stack depth */
#define FGL_MAX_TEXTURE_STACK_DEPTH	2
/** Bits of subpixel precision */
#define FGL_MAX_SUBPIXEL_BITS		4
/** Highest texture dimension */
#define FGL_MAX_TEXTURE_SIZE		2047
/** Highest viewport dimension */
#define FGL_MAX_VIEWPORT_DIMS		2047
/** Minimal point size of point rendering. */
#define FGL_MIN_POINT_SIZE		(1.0f)
/** Maximal point size of point rendering. */
#define FGL_MAX_POINT_SIZE		(2048.0f)
/** Minimal line width of line rendering. */
#define FGL_MIN_LINE_WIDTH		(1.0f)
/** Maximal line width of line rendering. */
#define FGL_MAX_LINE_WIDTH		(128.0f)

/** Compiler hint to evaluate given condition as likely to happen. */
#define likely(x)       __builtin_expect((x),1)
/** Compiler hint to evaluate given condition as unlikely to happen. */
#define unlikely(x)     __builtin_expect((x),0)

/** Issues a one-time warning after calling an unimplemented function. */
#define FUNC_UNIMPLEMENTED \
	static int flag = 0; \
	if (!flag) \
		LOGW("Application called unimplemented function: %s", __func__); \
	flag = 1

//#define TRACE_FUNCTIONS
#ifdef TRACE_FUNCTIONS
/**
 * A class that allows tracing of function calls.
 *
 * Local objects of this class created at the beginning of a function or method
 * will be destroyed when exiting from it. Constructor and destructor print
 * appropriate messages about entering and leaving of function.
 *
 * Should be used through #FUNCTION_TRACER macro at the beginning of traced
 * function.
 */
class FunctionTracer {
	const char *name;
public:
	inline FunctionTracer(const char *n)
	: name(n)
	{
		LOGD("Entering %s.", name);
	}

	inline ~FunctionTracer()
	{
		LOGD("Leaving %s.", name);
	}
};

/**
 * A macro used to enable call tracing of function in which it was inserted.
 *
 * Calling this macro at the beginning of a function will instantiate
 * a FunctionTracer object with appropriate constructor arguments. Entering
 * and leaving the function will cause object constructor and destructor
 * to print appropriate messages.
 */
#define FUNCTION_TRACER	FunctionTracer __ft(__func__)
#else
#define FUNCTION_TRACER
#endif

#undef NELEM
/** A macro returning the number of elements in a static array. */
#define NELEM(x) (sizeof(x)/sizeof(*(x)))

/** A macro returning bit mask with given number of LSBs set. */
#define BIT_MASK(x)	((1UL << (x)) - 1)
/** A macro returning bit mask with given bit set. */
#define BIT_VAL(x)	(1UL << (x))

/**
 * Returns maximum of two passed arguments.
 * @tparam T Data type used for operation.
 * @param a First value.
 * @param b Second value.
 * @return Greater value of a and b.
 */
template<typename T>
static inline T max(T a, T b)
{
	if (b > a)
		a = b;
	return a;
}

/**
 * Returns minimum of two passed arguments.
 * @tparam T Data type used for operation.
 * @param a First value.
 * @param b Second value.
 * @return Lower value of a and b.
 */
template<typename T>
static inline T min(T a, T b)
{
	if (b < a)
		a = b;
	return a;
}

/**
 * Clamps given number into given range.
 * @tparam T Data type used for operation.
 * @param v Value to clamp.
 * @param l Low bound.
 * @param h High bound.
 * @return Value v clamped into range defined by l and h.
 */
template<typename T>
static inline T clamp(T v, T l, T h)
{
	if (v > h)
		v = h;
	if (v < l)
		v = l;
	return v;
}

/**
 * Search for element with given key in given array.
 * @tparam T Type of array element.
 * @param sortedArray Array to search in.
 * @param first First index in the array.
 * @param last Last index in the array.
 * @param key Key to search for.
 * @return Index of found element or -1 if not found.
 */
template<typename T>
static int binarySearch(const T sortedArray[], int first, int last, EGLint key)
{
	while (first <= last) {
		int mid = (first + last) / 2;

		if (key > sortedArray[mid].key)
			first = mid + 1;
		else if (key < sortedArray[mid].key)
			last = mid - 1;
		else
			return mid;
	}

	return -1;
}

#endif // _LIBSGL_COMMON_H_
