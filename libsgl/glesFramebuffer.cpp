/**
 * libsgl/glesFramebuffer.cpp
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"

/*
 * Buffers (render surfaces)
 */

void fglSetColorBuffer(FGLContext *gl, FGLSurface *cbuf, unsigned int width,
                unsigned int height, unsigned int format)
{
        if (!cbuf) {
                gl->surface.draw = 0;
                return;
        }

        fimgSetFrameBufSize(gl->fimg, width, height);
        fimgSetFrameBufParams(gl->fimg, 1, 0, 255, (fimgColorMode)format);
        fimgSetColorBufBaseAddr(gl->fimg, cbuf->paddr);
        gl->surface.draw = cbuf;
        gl->surface.width = width;
        gl->surface.height = height;
        gl->surface.format = format;
}

void fglSetDepthBuffer(FGLContext *gl, FGLSurface *zbuf, unsigned int format)
{
        if (!zbuf || !format) {
                gl->surface.depth = 0;
                gl->surface.depthFormat = 0;
                return;
        }

        fimgSetZBufBaseAddr(gl->fimg, zbuf->paddr);
        gl->surface.depth = zbuf;
        gl->surface.depthFormat = format;
}
