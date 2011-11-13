/*
 * libfimg/platform.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL HARDWARE SUPPORT LIBRARY
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

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#if defined(FGL_PLATFORM_ANDROID)

#include <cutils/log.h>

#else

#include <stdio.h>

#define LOG_ERR		"E"
#define LOG_WARN	"W"
#define LOG_INFO	"I"
#define LOG_DBG		"D"

#define pr_log(lvl, file, line, fmt, ...)	\
	fprintf(stderr, "[libfimg:%s] %s: %d: " fmt "\n", lvl, file, line, ##__VA_ARGS__)

#define LOGE(fmt, ...)	\
		pr_log(LOG_ERR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)	\
		pr_log(LOG_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)	\
		pr_log(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)	\
		pr_log(LOG_DBG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif

#ifndef HAVE___SYNC_SYNCHRONIZE
#define __sync_synchronize()
#endif

#endif /* _PLATFORM_H_ */
