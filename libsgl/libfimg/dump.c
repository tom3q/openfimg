/*
 * fimg/dump.c
 *
 * SAMSUNG S3C6410 FIMG-3DSE STATE DUMPER
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

#include <stdio.h>
#include <string.h>
#include <cutils/log.h>
#include "fimg_private.h"

struct block {
	unsigned int	start;
	unsigned int	length;
};

static const struct block blocks[] = {
	/* Global block */
	{ 0x00000000, 0x60 },
	/* Host interface */
	{ 0x00008000, 0x100 },
	/* Primitive engine */
	{ 0x00030000, 0x20 },
	/* Rasterizer */
	{ 0x00038000, 0x30 },
	/* Rasterizer #2 */
	{ 0x0003c000, 0x10 },
	/* Texturing engine */
	{ 0x00060000, 0x300 },
	/* Per fragment unit */
	{ 0x00070000, 0x40 },
	/* End */
	{ 0, 0 }
};

#define VALUES_PER_LINE	(16)
#define LINE_SIZE	(8 + 1)

void fimgDumpState(fimgContext *ctx, unsigned mode, unsigned count, const char *func)
{
	const struct block *b;
	char buf[VALUES_PER_LINE * LINE_SIZE + 1];
	char *s;

	LOG(LOG_DEBUG, "fimgdump", "DUMP %d %d (%s %d %d)", getpid(), ctx->fd, func, mode, count);

	for (b = blocks; b->length; ++b) {
		unsigned addr	= b->start;
		unsigned len	= b->length;
		do {
			unsigned i;
			s = buf;
			for (i = 0; i < VALUES_PER_LINE && len; ++i) {
				sprintf(s, "%08x ", fimgRead(ctx, addr));
				addr += 4;
				len -= 4;
				s += LINE_SIZE;
			}
			*(--s) = '\0';
			LOG(LOG_DEBUG, "fimgdump", "%s", buf);
		} while (len);
	}
}
