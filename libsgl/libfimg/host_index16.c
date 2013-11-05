/*
 * SAMSUNG S3C6410 FIMG-3DSE HOST INTERFACE RELATED FUNCTIONS
 *
 * Copyrights:	2013 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fimg_private.h"
#include "host.h"

#define INDEX_TYPE	const uint16_t*

#include "host_template.h"

/**
 * Draws a sequence of vertices described by array descriptors and a sequence
 * of uint16_t indices.
 * @param ctx Hardware context.
 * @param mode Primitive type.
 * @param arrays Array of attribute array descriptors.
 * @param count Vertex count.
 * @param indices Array of vertex indices.
 */
void fimgDrawElementsUShortIdx(fimgContext *ctx, unsigned int mode,
			      fimgArray *arrays, unsigned int count,
			      const uint16_t *indices)
{
	fimgDraw(ctx, mode, arrays, count, indices);
}
