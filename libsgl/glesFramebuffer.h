/*
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

/**
 * Sets color buffer of default GLES framebuffer.
 * @param gl Rendering context.
 * @param cbuf Backing surface.
 * @param width Suface width.
 * @param height Surface height.
 * @param format Surface color format.
 */
extern void fglSetColorBuffer(FGLContext *gl, FGLSurface *cbuf,
				unsigned int width, unsigned int height,
				unsigned int format);

/**
 * Sets depth/stencil buffer of default GLES framebuffer.
 * @param gl Rendering context.
 * @param zbuf Backing surface.
 * @param width Suface width.
 * @param height Surface height.
 * @param format Surface depth and stencil format.
 */
extern void fglSetDepthStencilBuffer(FGLContext *gl, FGLSurface *zbuf,
				unsigned int width, unsigned int height,
				unsigned int format);

#endif /* _GLESFRAMEBUFFER_H_ */
