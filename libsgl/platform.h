/*
 * libsgl/eglPlatform.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifndef _EGLPLATFORM_H_
#define _EGLPLATFORM_H_

struct FGLContext;

#if defined(FGL_PLATFORM_ANDROID)

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/native_handle.h>

#include <utils/threads.h>

#ifdef ANDROID_FAST_TLS
#include <bionic_tls.h>

// We have a dedicated TLS slot in bionic
#define PLATFORM_HAS_FAST_TLS

static inline void setGlThreadSpecific(FGLContext* value)
{
	((uint32_t *)__get_tls())[TLS_SLOT_OPENGL] = (uint32_t)value;
}

static inline FGLContext* getGlThreadSpecific()
{
	return (FGLContext *)(((unsigned *)__get_tls())[TLS_SLOT_OPENGL]);
}
#endif /* ANDROID_FAST_TLS */

#define PLATFORM_HAS_CUSTOM_LOG

#define PLATFORM_EXTENSIONS_STRING		\
	"EGL_KHR_image_base "			\
	"EGL_ANDROID_image_native_buffer "	\
	"EGL_ANDROID_swap_rectangle "		\
	"EGL_ANDROID_get_render_buffer"		\

#elif defined(FGL_PLATFORM_FRAMEBUFFER)

#define PLATFORM_EXTENSIONS_STRING		\
	""

#else

#error No platform defined

#endif

#ifndef PLATFORM_HAS_FAST_TLS

#include <pthread.h>

extern pthread_key_t eglContextKey;

static inline FGLContext* getGlThreadSpecific()
{
	return (FGLContext *)pthread_getspecific(eglContextKey);
}

static inline void setGlThreadSpecific(FGLContext* value)
{
	pthread_setspecific(eglContextKey, value);
}
#endif /* PLATFORM_HAS_FAST_TLS */

#ifndef PLATFORM_HAS_CUSTOM_LOG

#include <cstdio>

#define LOG_ERR		"E"
#define LOG_WARN	"W"
#define LOG_INFO	"I"
#define LOG_DBG		"D"

#define pr_log(lvl, file, line, fmt, ...)	\
	fprintf(stderr, "[libsgl:%s] %s: %d: " fmt "\n", lvl, file, line, ##__VA_ARGS__)

#define LOGE(fmt, ...)	\
		pr_log(LOG_ERR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)	\
		pr_log(LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)	\
		pr_log(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)	\
		pr_log(LOG_DBG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif /* PLATFORM_HAS_CUSTOM_LOG */

#endif /* _EGLPLATFORM_H_ */
