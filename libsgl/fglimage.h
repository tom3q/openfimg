/*
 * libsgl/fglimage.h
 *
 * SAMSUNG S3C6410 FIMG-3DSE (PROPER) OPENGL ES IMPLEMENTATION
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

#ifndef _FGLIMAGE_H_
#define _FGLIMAGE_H_

struct FGLSurface;

/** A class representing an image as defined by EGL_KHR_image_base extension. */
struct FGLImage {
/** A magic number used to mark image objects created by OpenFIMG. */
#define FGL_IMAGE_MAGIC 0x00474d49
	/** Magic number used for object validation. */
	uint32_t magic;
	/** Pixel format of the image. */
	uint32_t pixelFormat;
	/** Width of the image. */
	uint32_t width;
	/** Height of the image. */
	uint32_t height;
	void *buffer;
	/** Surface backing the image. */
	FGLSurface *surface;
	/** A flag indicating that the image is marked for deletion. */
	bool terminated;
	/** Reference counter. */
	uint32_t refCount;

	/** Constructor creating an image object. */
	FGLImage() :
		magic(FGL_IMAGE_MAGIC),
		buffer(0),
		surface(0),
		terminated(false),
		refCount(0) {}

	/** Class destructor. */
	virtual ~FGLImage() {}

	/**
	 * Checks whether the image is valid.
	 * The image is assumed to be valid if magic number matches and there
	 * is a backing storage attached.
	 * @return True if the image is valid, otherwise false.
	 */
	bool isValid()
	{
		return magic == FGL_IMAGE_MAGIC && surface != NULL;
	}

	/** Increases reference count of the image. */
	void connect()
	{
		++refCount;
	}

	/**
	 * Decreases reference count of the image.
	 * If reference counter goes down to 0 and the image is marked for
	 * termination, it will be deleted.
	 */
	void disconnect()
	{
		if(--refCount == 0 && terminated)
			delete this;
	}

	/**
	 * Marks the image for termination.
	 * The image will be terminated as soon as reference counts goes down
	 * to 0.
	 */
	void terminate()
	{
		terminated = true;
		if (!refCount)
			delete this;
	}
};

#endif /* _FGLIMAGE_H_ */
