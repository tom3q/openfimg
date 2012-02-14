/*
 * libsgl/fglframebuffer.cpp
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) EGL IMPLEMENTATION
 *
 * Copyrights:	2012 by Tomasz Figa < tomasz.figa at gmail.com >
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty off
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "fglframebuffer.h"
#include "fglframebufferattachable.h"

/*
 * FGLFramebufferAttachable
 */

void FGLFramebufferAttachable::markFramebufferDirty(void)
{
	FGLFramebufferAttachableObject::iterator i = object.begin();
	for (i = object.begin(); i != object.end(); ++i) {
		FGLFramebuffer *fb = i.get();
		fb->markDirty();
	}
}

/*
 * FGLFramebuffer
 */

bool FGLFramebuffer::checkAttachment(FGLAttachmentIndex index,
						FGLFramebufferAttachable *fba)
{
	if (!fba->surface)
		return false;

	if (!(fba->mask && (1 << index)))
		return false;

	if (fba->width <= 0 || fba->height <= 0)
		return false;

	return true;
}

void FGLFramebuffer::updateStatus(void)
{
	GLint width = -1;
	GLint height = -1;
	int i;

	this->width = 0;
	this->height = 0;
	this->colorFormat = FGL_PIXFMT_NONE;
	this->depthFormat = 0;

	for (i = 0; i < FGL_ATTACHMENT_NUM; ++i) {
		if (!binding[i].isBound())
			continue;
		FGLFramebufferAttachable *fba = binding[i].get();
		if (!checkAttachment((FGLAttachmentIndex)i, fba)) {
			status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			return;
		}
		width = fba->width;
		height = fba->height;
	}

	for (; i < FGL_ATTACHMENT_NUM; ++i) {
		if (!binding[i].isBound())
			continue;
		FGLFramebufferAttachable *fba = binding[i].get();
		if (!checkAttachment((FGLAttachmentIndex)i, fba)) {
			status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;
			return;
		}
		if (width != fba->width || height != fba->height) {
			status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;
			return;
		}
	}

	if (width == -1 || height == -1) {
		status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
		return;
	}

	this->width = width;
	this->height = height;

	FGLFramebufferAttachable *color = binding[FGL_ATTACHMENT_COLOR].get();
	if (!color) {
		status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
		return;
	}

	this->colorFormat = color->pixFormat;

	FGLFramebufferAttachable *depth, *stencil;
	depth = binding[FGL_ATTACHMENT_DEPTH].get();
	stencil = binding[FGL_ATTACHMENT_STENCIL].get();

	if (depth && stencil && depth != stencil) {
		status = GL_FRAMEBUFFER_UNSUPPORTED_OES;
		return;
	}

	if (depth)
		this->depthFormat = depth->pixFormat;

	if (stencil)
		this->depthFormat = stencil->pixFormat;

	status = GL_FRAMEBUFFER_COMPLETE_OES;
}
