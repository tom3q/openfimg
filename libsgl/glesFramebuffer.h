/**
 * libsgl/glesFramebuffer.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:  2011 by Tomasz Figa < tomasz.figa at gmail.com >
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

#ifndef _GLESFRAMEBUFFER_H_
#define _GLESFRAMEBUFFER_H_

extern void fglSetColorBuffer(FGLContext *, FGLSurface *, unsigned int,
				unsigned int, unsigned int);

extern void fglSetDepthBuffer(FGLContext *, FGLSurface *, unsigned int);

#endif /* _GLESFRAMEBUFFER_H_ */
