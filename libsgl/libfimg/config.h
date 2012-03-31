/*
 * fimg/config.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE LOW LEVEL DEFINITIONS (CONFIGURATION HEADER)
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

#ifndef _FIMG_CONFIG_H_
#define _FIMG_CONFIG_H_

/* Workaround for rasterizer bug. */
#define FIMG_INTERPOLATION_WORKAROUND

/* Use fixed pipeline emulation */
#define FIMG_FIXED_PIPELINE

/* Dump register state before sending draw request (for debugging) */
//#define FIMG_DUMP_STATE_BEFORE_DRAW

/* Use dump file to store GPU crash dumps */
#define FIMG_USE_DUMP_FILE

/* Dump file path */
#define FIMG_DUMP_FILE_PATH	"/tmp"

/* Map/unmap memory when locking/unlocking */
//#define FIMG_DEBUG_IOMEM_ACCESS

/* Check for hardware lock before register access */
//#define FIMG_DEBUG_HW_LOCK

/* Dump generated shaders */
//#define FIMG_DYNSHADER_DEBUG

/* Show shader cache hit/miss statistics in log */
//#define FIMG_SHADER_CACHE_STATS

/* Disable shader optimizer */
//#define FIMG_BYPASS_SHADER_OPTIMIZER

#endif /* _FIMG_CONFIG_H_ */
