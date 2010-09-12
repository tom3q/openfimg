#ifndef _LIBSGL_COMMON_H_
#define _LIBSGL_COMMON_H_

#define FGL_MAX_TEXTURE_UNITS	2
#define FGL_MAX_TEXTURE_OBJECTS 1024

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

template<typename T>
static inline T max(T a, T b)
{
	return (a > b) ? a : b;
}

template<typename T>
static inline T min(T a, T b)
{
	return (a < b) ? a : b;
}

void fglSetClipper(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

#endif // _LIBSGL_COMMON_H_
