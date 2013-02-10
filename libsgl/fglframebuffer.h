/*
 * libsgl/fglframebuffer.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
 *
 * Copyrights:	2011 by Tomasz Figa < tomasz.figa at gmail.com >
 * 		2011 by Jordi Santiago Provencio < >
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

#ifndef _FGLFRAMEBUFFER_H_
#define _FGLFRAMEBUFFER_H_

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "fglobject.h"
#include "fglframebufferattachable.h"
#include "fglpixelformat.h"

/** Attachment points supported by OpenFIMG */
enum FGLAttachmentIndex {
	FGL_ATTACHMENT_COLOR = 0,
	FGL_ATTACHMENT_DEPTH,
	FGL_ATTACHMENT_STENCIL,

	/** Number of supported attachment points. */
	FGL_ATTACHMENT_NUM
};

/**
 * Abstract framebuffer class.
 * Defines an interface for all classes providing framebuffers.
 */
class FGLAbstractFramebuffer {
protected:
	bool dirty;

	uint32_t width;
	uint32_t height;
	uint32_t colorFormat;
	uint32_t depthFormat;

public:
	/**
	 * Default constructor.
	 * Creates and initializes the generic part of a framebuffer.
	 */
	FGLAbstractFramebuffer() :
		dirty(true),
		width(0),
		height(0),
		colorFormat(FGL_PIXFMT_NONE),
		depthFormat(0) {};

	/** Class destructor. */
	virtual ~FGLAbstractFramebuffer() {}

	/**
	 * Gets width of the framebuffer.
	 * @return Width of the framebuffer.
	 */
	inline uint32_t getWidth(void) const { return width; }
	/**
	 * Gets height of the framebuffer.
	 * @return Height of the framebuffer.
	 */
	inline uint32_t getHeight(void) const { return height; }
	/**
	 * Gets color format of the framebuffer.
	 * @return Color format of the framebuffer.
	 */
	inline uint32_t getColorFormat(void) const { return colorFormat; }
	/**
	 * Gets depth format of the framebuffer.
	 * @return Depth format of the framebuffer.
	 */
	inline uint32_t getDepthFormat(void) const { return depthFormat; }

	/**
	 * Checks if the framebuffer is valid.
	 * @return True if the framebuffer is valid, otherwise false.
	 */
	virtual bool isValid(void) = 0;
	/**
	 * Checks status of the framebuffer.
	 * @return Value representing framebuffer status according to OpenGL ES
	 * Framebuffer Object extension.
	 */
	virtual GLenum checkStatus(void) = 0;
	/**
	 * Gets attachment bound to given attachment point.
	 * @param from Attachment point.
	 * @return Attachment bound to given attachment point.
	 */
	virtual FGLFramebufferAttachable *get(enum FGLAttachmentIndex from) = 0;

	/**
	 * Checks if framebuffer configuration is dirty.
	 * @return True if framebuffer configuration has changed since last
	 * call to markClean(), otherwise false.
	 */
	inline bool isDirty(void) const
	{
		return dirty;
	}

	/** Marks framebuffer configuration as dirty */
	inline void markDirty(void)
	{
		dirty = true;
	}

	/** Marks framebuffer configuration as clean */
	inline void markClean(void)
	{
		dirty = false;
	}

	/**
	 * Gets framebuffer name.
	 * @return Name of the framebuffer.
	 */
	virtual GLint getName(void) const { return 0; }
};

struct FGLFramebufferState;

/**
 * An FGLObject that points to an FGLFramebuffer object and can be bound
 * to an FGLObjectBinding of an FGLFramebufferState object.
 */
typedef FGLObject<FGLFramebuffer, FGLFramebufferState> FGLFramebufferObject;
/**
 * An FGLObjectBinding that points to a FGLFramebufferState object to which
 * an FGLObject of an FGLFramebuffer object can be bound.
 */
typedef FGLObjectBinding<FGLFramebuffer,
			FGLFramebufferState> FGLFramebufferObjectBinding;

/**
 * A class that implements a framebuffer in terms of OpenGL ES Framebuffer
 * Object extension.
 */
class FGLFramebuffer : public FGLAbstractFramebuffer {
	FGLFramebufferAttachableBinding binding[FGL_ATTACHMENT_NUM];
	unsigned int name;
	GLenum status;

	void updateStatus(void);
	bool checkAttachment(FGLAttachmentIndex index,
						FGLFramebufferAttachable *fba);

public:
	/** FGLObject that can be bound to FGLFramebufferState */
	FGLFramebufferObject object;

	/**
	 * Constructor creating a framebuffer.
	 * @param name Framebuffer name.
	 */
	FGLFramebuffer(unsigned int name = 0) :
		FGLAbstractFramebuffer(),
		name(name),
		status(GL_NONE_OES),
		object(this)
	{
		for (int i = 0; i < FGL_ATTACHMENT_NUM; ++i)
			binding[i] = FGLFramebufferAttachableBinding(this);
	}

	/**
	 * Attaches a framebuffer attachable object to given attachment point.
	 * Any object previously bound to this binding point will be unbound.
	 * @param where Attachment point.
	 * @param what Framebuffer attachable object (or NULL to unbind).
	 */
	void attach(FGLAttachmentIndex where,
					FGLFramebufferAttachable *what)
	{
		if (what != binding[where].get()) {
			markDirty();
			status = GL_NONE_OES;
		}

		if (!what)
			binding[where].bind(0);
		else
			binding[where].bind(&what->object);
	}

	virtual FGLFramebufferAttachable *get(enum FGLAttachmentIndex from)
	{
		return binding[from].get();
	}

	virtual bool isValid(void)
	{
		if (status != GL_NONE_OES)
			return status == GL_FRAMEBUFFER_COMPLETE_OES;

		updateStatus();

		return status == GL_FRAMEBUFFER_COMPLETE_OES;
	}

	virtual GLenum checkStatus(void)
	{
		if (status != GL_NONE_OES)
			return status;

		updateStatus();

		return status;
	}

	virtual GLint getName(void) const
	{
		return name;
	}
};

/** A class that implements a legacy EGL-controlled framebuffer. */
class FGLDefaultFramebuffer : public FGLAbstractFramebuffer {
	FGLFramebufferAttachable image[FGL_ATTACHMENT_NUM];

public:
	/** Default constructor. */
	FGLDefaultFramebuffer() {}

	virtual FGLFramebufferAttachable *get(enum FGLAttachmentIndex from)
	{
		return &image[from];
	}

	virtual bool isValid(void)
	{
		return colorFormat != FGL_PIXFMT_NONE;
	}

	virtual GLenum checkStatus(void)
	{
		return 0;
	}

	/**
	 * Connects EGL surface to selected attachment point.
	 * @param where Attachment point.
	 * @param buf EGL surface.
	 * @param width Surface width.
	 * @param height Surface height.
	 * @param format Surface format.
	 */
	void setupSurface(enum FGLAttachmentIndex where, FGLSurface *buf,
		unsigned int width, unsigned int height, unsigned int format)
	{
		FGLFramebufferAttachable *fba = &image[where];

		fba->surface = buf;
		fba->width = width;
		fba->height = height;
		fba->format = format;

		this->width = width;
		this->height = height;
		if (where == FGL_ATTACHMENT_COLOR)
			colorFormat = format;
		if (where == FGL_ATTACHMENT_DEPTH)
			depthFormat = format;
		if (where == FGL_ATTACHMENT_STENCIL)
			depthFormat = format;

		markDirty();
	}
};

#endif /* _FGLFRAMEBUFFER_H_ */
