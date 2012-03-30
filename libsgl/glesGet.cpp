/*
 * libsgl/glesGet.cpp
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

/*
	Client API information
*/

static char const * const gVendorString     = "OpenFIMG";
static char const * const gRendererString   = "FIMG-3DSE";
static char const * const gVersionString    = "OpenGL ES-CM 1.1";
static char const * const gExtensionsString =
	"GL_OES_byte_coordinates "
	"GL_OES_fixed_point "
	"GL_OES_single_precision "
	"GL_OES_read_format "
	"GL_OES_matrix_get "
	"GL_OES_draw_texture "
	"GL_OES_EGL_image "
	"GL_OES_EGL_image_external "
	"GL_OES_framebuffer_object "
	"GL_OES_packed_depth_stencil "
	"GL_OES_texture_npot "
	"GL_OES_point_size_array "
	"GL_OES_rgb8_rgba8 "
	"GL_OES_depth24 "
	"GL_OES_stencil8 "
	"GL_EXT_texture_format_BGRA8888 "
	"GL_ARB_texture_non_power_of_two"
;

static const GLint fglCompressedTextureFormats[] = {
};

const FGLPixelFormat FGLPixelFormat::table[] = {
	/*
	 * FGL_PIXFMT_NONE
	 *
	 * Dummy format used when format is unspecified
	 */
	{
		{ { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		0,
		-1,
		-1,
		0
	},

	/*
	 * Renderable formats follow
	 */

	/*
	 * FGL_PIXFMT_XRGB1555
	 * -----------------------------------------
	 * | 15 - 15 | 14 - 10 | 9  -  5 | 4  -  0 |
	 * -----------------------------------------
	 * |    X    |    R    |    G    |    B    |
	 * -----------------------------------------
	 */
	{
		{ { 10, 5 }, { 5, 5 }, { 0, 5 }, { 15, 0 } },
		GL_RGB,
		GL_UNSIGNED_SHORT_5_5_5_1,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_1555,
		FGPF_COLOR_MODE_555,
		FGL_PIX_OPAQUE
	},
	/*
	 * FGL_PIXFMT_RGB565
	 * -------------------------------
	 * | 15 - 11 | 10 -  5 | 4  -  0 |
	 * -------------------------------
	 * |    R    |    G    |    B    |
	 * -------------------------------
	 */
	{
		{ { 11, 5 }, { 5, 6 }, { 0, 5 }, { 0, 0 } },
		GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_565,
		FGPF_COLOR_MODE_565,
		0
	},
	/*
	 * FGL_PIXFMT_ARGB4444
	 * -----------------------------------------
	 * | 15 - 12 | 11 -  8 | 7  -  4 | 3  -  0 |
	 * -----------------------------------------
	 * |    A    |    R    |    G    |    B    |
	 * -----------------------------------------
	 */
	{
		{ { 8, 4 }, { 4, 4 }, { 0, 4 }, { 12, 4 } },
		GL_RGBA,
		GL_UNSIGNED_SHORT_4_4_4_4,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_4444,
		FGPF_COLOR_MODE_4444,
		0
	},
	/*
	 * FGL_PIXFMT_ARGB1555
	 * -----------------------------------------
	 * | 15 - 15 | 14 - 10 | 9  -  5 | 4  -  0 |
	 * -----------------------------------------
	 * |    A    |    R    |    G    |    B    |
	 * -----------------------------------------
	 */
	{
		{ { 10, 5 }, { 5, 5 }, { 0, 5 }, { 15, 1 } },
		GL_RGBA,
		GL_UNSIGNED_SHORT_5_5_5_1,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_1555,
		FGPF_COLOR_MODE_1555,
		0
	},
	/*
	 * FGL_PIXFMT_XRGB8888
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    X    |    R    |    G    |    B    |
	 * -----------------------------------------
	 */
	{
		{ { 16, 8 }, { 8, 8 }, { 0, 8 }, { 24, 0 } },
		GL_BGRA_EXT,
		GL_UNSIGNED_BYTE,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_8888,
		FGPF_COLOR_MODE_0888,
		FGL_PIX_OPAQUE
	},
	/*
	 * FGL_PIXFMT_ARGB8888
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    A    |    R    |    G    |    B    |
	 * -----------------------------------------
	 */
	{
		{ { 16, 8 }, { 8, 8 }, { 0, 8 }, { 24, 8 } },
		GL_BGRA_EXT,
		GL_UNSIGNED_BYTE,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_8888,
		FGPF_COLOR_MODE_8888,
		0
	},
	/*
	 * FGL_PIXFMT_XBGR8888
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    X    |    B    |    G    |    R    |
	 * -----------------------------------------
	 */
	{
		{ { 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 0 } },
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_8888,
		FGPF_COLOR_MODE_0888,
		FGL_PIX_OPAQUE | FGL_PIX_BGR
	},
	/*
	 * FGL_PIXFMT_ABGR8888
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    A    |    B    |    G    |    R    |
	 * -----------------------------------------
	 */
	{
		{ { 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 } },
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_8888,
		FGPF_COLOR_MODE_8888,
		FGL_PIX_BGR
	},

	/*
	 * Non-renderable formats follow
	 */

	/*
	 * FGL_PIXFMT_RGBA4444
	 * -----------------------------------------
	 * | 15 - 12 | 11 -  8 | 7  -  4 | 3  -  0 |
	 * -----------------------------------------
	 * |    R    |    G    |    B    |    A    |
	 * -----------------------------------------
	 */
	{
		{ { 12, 4 }, { 8, 4 }, { 4, 4 }, { 0, 4 } },
		0,
		0,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_4444,
		-1,
		FGL_PIX_ALPHA_LSB
	},
	/*
	 * FGL_PIXFMT_RGBA1555
	 * -----------------------------------------
	 * | 15 - 11 | 10 -  6 | 5  -  1 | 0  -  0 |
	 * -----------------------------------------
	 * |    R    |    G    |    B    |    A    |
	 * -----------------------------------------
	 */
	{
		{ { 11, 5 }, { 6, 5 }, { 1, 5 }, { 0, 1 } },
		0,
		0,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_1555,
		-1,
		FGL_PIX_ALPHA_LSB
	},
	/*
	 * FGL_PIXFMT_DEPTH16
	 * -----------
	 * | 15 -  0 |
	 * -----------
	 * |    D    |
	 * -----------
	 */
	{
		{ { 0, 16 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_DEPTHCOMP16,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_AL88
	 * ---------------------
	 * | 15 -  8 | 7  -  0 |
	 * ---------------------
	 * |    A    |    L    |
	 * ---------------------
	 */
	{
		{ { 0, 8 }, { 0, 0 }, { 0, 0 }, { 8, 8 } },
		0,
		0,
		2,
		FGTU_TSTA_TEXTURE_FORMAT_88,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_L8
	 * -----------
	 * | 7  -  0 |
	 * -----------
	 * |    L    |
	 * -----------
	 */
	{
		{ { 0, 8 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		1,
		FGTU_TSTA_TEXTURE_FORMAT_8,
		-1,
		0
	},

	/*
	 * Compressed formats follow
	 */

	/*
	 * FGL_PIXFMT_1BPP
	 * ---------------------------------
	 * | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	 * ---------------------------------
	 * | P | P | P | P | P | P | P | P |
	 * ---------------------------------
	 */
	{
		{ { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		0,
		FGTU_TSTA_TEXTURE_FORMAT_1BPP,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_2BPP
	 * ---------------------------------
	 * | 7 - 6 | 5 - 4 | 3 - 2 | 1 - 0 |
	 * ---------------------------------
	 * |   P   |   P   |   P   |   P   |
	 * ---------------------------------
	 */
	{
		{ { 0, 2 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		0,
		FGTU_TSTA_TEXTURE_FORMAT_2BPP,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_4BPP
	 * -----------------
	 * | 7 - 4 | 3 - 0 |
	 * -----------------
	 * |   P   |   P   |
	 * -----------------
	 */
	{
		{ { 0, 4 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		0,
		FGTU_TSTA_TEXTURE_FORMAT_4BPP,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_8BPP
	 * ---------
	 * | 7 - 0 |
	 * ---------
	 * |   P   |
	 * ---------
	 */
	{
		{ { 0, 8 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		0,
		0,
		0,
		FGTU_TSTA_TEXTURE_FORMAT_8BPP,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_S3TC
	 * -------------------------------
	 * | 63 - 32 | 31 - 16 | 15 -  0 |
	 * -------------------------------
	 * |    L    |    C1   |    C0   |
	 * -------------------------------
	 */
	{
		{ { 0, 16 }, { 16, 16 }, { 32, 32 }, { 0, 0 } },
		0,
		0,
		0,
		FGTU_TSTA_TEXTURE_FORMAT_S3TC,
		-1,
		0
	},

	/*
	 * YUV formats follow
	 */

	/*
	 * FGL_PIXFMT_Y1VY0U
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    Y1   |    V    |    Y0   |    U    |
	 * -----------------------------------------
	 */
	{
		{ { 8, 8 }, { 0, 8 }, { 24, 8 }, { 16, 8 } },
		0,
		0,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_Y1VY0U,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_VY1UY0
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    V    |    Y1   |    U    |    Y0   |
	 * -----------------------------------------
	 */
	{
		{ { 0, 8 }, { 8, 8 }, { 16, 8 }, { 24, 8 } },
		0,
		0,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_VY1UY0,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_Y1UY0V
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    Y1   |    U    |    Y0   |    V    |
	 * -----------------------------------------
	 */
	{
		{ { 8, 8 }, { 16, 8 }, { 24, 8 }, { 0, 8 } },
		0,
		0,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_Y1UY0V,
		-1,
		0
	},
	/*
	 * FGL_PIXFMT_UY1VY0
	 * -----------------------------------------
	 * | 31 - 24 | 23 - 16 | 15 -  8 | 7  -  0 |
	 * -----------------------------------------
	 * |    U    |    Y1   |    V    |    Y0   |
	 * -----------------------------------------
	 */
	{
		{ { 0, 8 }, { 24, 8 }, { 16, 8 }, { 8, 8 } },
		0,
		0,
		4,
		FGTU_TSTA_TEXTURE_FORMAT_UY1VY0,
		-1,
		0
	},
};

const FGLPixelFormat *FGLPixelFormat::get(unsigned int format)
{
	const FGLPixelFormat *desc = &table[FGL_PIXFMT_NONE];
	if (format < NELEM(table))
		desc = &table[format];
	return desc;
}

// ----------------------------------------------------------------------------

/*
	glGet*
*/

GL_API const GLubyte * GL_APIENTRY glGetString (GLenum name)
{
	switch (name) {
	case GL_VENDOR:
		return (const GLubyte *)gVendorString;
	case GL_RENDERER:
		return (const GLubyte *)gRendererString;
	case GL_VERSION:
		return (const GLubyte *)gVersionString;
	case GL_EXTENSIONS:
		return (const GLubyte *)gExtensionsString;
	default:
		setError(GL_INVALID_ENUM);
		return NULL;
	}
}

struct FGLStateGetter {
	FGLStateGetter() : _n(0) {}

	virtual void putInteger(GLint i) = 0;
	virtual void putIntegers(const GLint *i, unsigned count) = 0;
	virtual void putFloat(GLfloat f) = 0;
	virtual void putFloats(const GLfloat *f, unsigned count) = 0;
	virtual void putBoolean(GLboolean b) = 0;
	virtual void putFixed(GLfixed x) = 0;
	virtual void putEnum(GLenum e) = 0;
	virtual void putNormalized(GLfloat f) = 0;

	virtual ~FGLStateGetter() {}

protected:
	int _n;
};

struct FGLFloatGetter : FGLStateGetter {
	FGLFloatGetter(GLfloat *f) : _f(f) {}

	virtual void putInteger(GLint i) { _f[_n++] = i; }
	virtual void putFloat(GLfloat f) { _f[_n++] = f; }
	virtual void putBoolean(GLboolean b) { _f[_n++] = b; }
	virtual void putFixed(GLfixed x) { _f[_n++] = floatFromFixed(x); }
	virtual void putEnum(GLenum e) { _f[_n++] = e; }
	virtual void putNormalized(GLfloat f) { _f[_n++] = f; }

	virtual void putIntegers(const GLint *i, unsigned count)
	{
		while (count--)
			_f[_n++] = *i++;
	}

	virtual void putFloats(const GLfloat *f, unsigned count)
	{
		memcpy(_f + _n, f, count * sizeof(*f));
		_n += count;
	}

private:
	GLfloat *_f;
};

struct FGLFixedGetter : FGLStateGetter {
	FGLFixedGetter(GLfixed *x) : _x(x) {}

	virtual void putInteger(GLint i) { _x[_n++] = fixedFromInt(i); }
	virtual void putFloat(GLfloat f) { _x[_n++] = fixedFromFloat(f); }
	virtual void putBoolean(GLboolean b) { _x[_n++] = fixedFromBool(b); }
	virtual void putFixed(GLfixed x) { _x[_n++] = x; }
	virtual void putEnum(GLenum e) { _x[_n++] = e; }
	virtual void putNormalized(GLfloat f) { _x[_n++] = fixedFromFloat(f); }

	virtual void putIntegers(const GLint *i, unsigned count)
	{
		while (count--)
			_x[_n++] = fixedFromInt(*i++);
	}

	virtual void putFloats(const GLfloat *f, unsigned count)
	{
		while (count--)
			_x[_n++] = fixedFromFloat(*f++);
	}

private:
	GLfixed *_x;
};

struct FGLIntegerGetter : FGLStateGetter {
	FGLIntegerGetter(GLint *i) : _i(i) {}

	virtual void putInteger(GLint i) { _i[_n++] = i; }
	virtual void putFloat(GLfloat f) { _i[_n++] = round(f); }
	virtual void putBoolean(GLboolean b) { _i[_n++] = b; }
	virtual void putFixed(GLfixed x) { _i[_n++] = round(floatFromFixed(x)); }
	virtual void putEnum(GLenum e) { _i[_n++] = e; }
	virtual void putNormalized(GLfloat f) { _i[_n++] = intFromNormalized(f); }

	virtual void putIntegers(const GLint *i, unsigned count)
	{
		memcpy(_i + _n, i, count * sizeof(i));
		_n += count;
	}

	virtual void putFloats(const GLfloat *f, unsigned count)
	{
		while (count--)
			_i[_n++] = round(*f++);
	}

private:
	GLint *_i;
};

struct FGLBooleanGetter : FGLStateGetter {
	FGLBooleanGetter(GLboolean *b) : _b(b) {}

	virtual void putInteger(GLint i) { _b[_n++] = !!i; }
	virtual void putFloat(GLfloat f) { _b[_n++] = (f != 0.0f); }
	virtual void putBoolean(GLboolean b) { _b[_n++] = b; }
	virtual void putFixed(GLfixed x) { _b[_n++] = !!x; }
	virtual void putEnum(GLenum e) { _b[_n++] = false; }
	virtual void putNormalized(GLfloat f) { _b[_n++] = (f != 0.0f); }

	virtual void putIntegers(const GLint *i, unsigned count)
	{
		while (count--)
			_b[_n++] = (*i++ != 0);
	}

	virtual void putFloats(const GLfloat *f, unsigned count)
	{
		while (count--)
			_b[_n++] = (*f++ != 0.0f);
	}

private:
	GLboolean *_b;
};

static const GLenum matrixModeTable[FGL_MATRIX_TEXTURE(FGL_MAX_TEXTURE_UNITS)] = {
	GL_PROJECTION_MATRIX,
	GL_MODELVIEW_MATRIX,
	0,
	GL_TEXTURE_MATRIX,
	GL_TEXTURE_MATRIX
};

void fglGetState(FGLContext *ctx, GLenum pname, FGLStateGetter &state)
{
	switch (pname) {
	case GL_FRAMEBUFFER_BINDING_OES: {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		state.putInteger(fb->getName());
		break; }
	case GL_RENDERBUFFER_BINDING_OES: {
		FGLRenderbuffer *rb = ctx->renderbuffer.get();
		GLint name = (rb) ? rb->getName() : 0;
		state.putInteger(name);
		break; }
	case GL_MAX_RENDERBUFFER_SIZE_OES:
		state.putInteger(FGL_MAX_VIEWPORT_DIMS);
		break;

	case GL_SHADE_MODEL:
		state.putEnum(ctx->rasterizer.shadeModel);
		break;

	case GL_MATRIX_MODE:
		state.putEnum(matrixModeTable[ctx->matrix.activeMatrix]);
		break;

	case GL_MAX_MODELVIEW_STACK_DEPTH: {
		GLint depth = ctx->matrix.stack[FGL_MATRIX_MODELVIEW].size();
		state.putInteger(depth);
		break; }
	case GL_MAX_PROJECTION_STACK_DEPTH: {
		GLint depth = ctx->matrix.stack[FGL_MATRIX_MODELVIEW].size();
		state.putInteger(depth);
		break; }
	case GL_MAX_TEXTURE_STACK_DEPTH: {
		unsigned id = FGL_MATRIX_TEXTURE(ctx->activeTexture);
		GLint depth = ctx->matrix.stack[id].size();
		state.putInteger(depth);
		break; }

	case GL_MODELVIEW_STACK_DEPTH: {
		GLint depth = ctx->matrix.stack[FGL_MATRIX_MODELVIEW].depth();
		state.putInteger(depth);
		break; }
	case GL_PROJECTION_STACK_DEPTH: {
		GLint depth = ctx->matrix.stack[FGL_MATRIX_MODELVIEW].depth();
		state.putInteger(depth);
		break; }
	case GL_TEXTURE_STACK_DEPTH: {
		unsigned id = FGL_MATRIX_TEXTURE(ctx->activeTexture);
		GLint depth = ctx->matrix.stack[id].depth();
		state.putInteger(depth);
		break; }

	case GL_MODELVIEW_MATRIX: {
		FGLmatrix *mat = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW].top();
		state.putFloats(mat->data, 16);
		break; }
	case GL_PROJECTION_MATRIX: {
		FGLmatrix *mat = &ctx->matrix.stack[FGL_MATRIX_PROJECTION].top();
		state.putFloats(mat->data, 16);
		break; }
	case GL_TEXTURE_MATRIX: {
		unsigned id = FGL_MATRIX_TEXTURE(ctx->activeTexture);
		FGLmatrix *mat = &ctx->matrix.stack[id].top();
		state.putFloats(mat->data, 16);
		break; }

	case GL_MODELVIEW_MATRIX_FLOAT_AS_INT_BITS_OES: {
		FGLmatrix *mat = &ctx->matrix.stack[FGL_MATRIX_MODELVIEW].top();
		state.putIntegers((GLint *)mat->data, 16);
		break; }
	case GL_PROJECTION_MATRIX_FLOAT_AS_INT_BITS_OES: {
		FGLmatrix *mat = &ctx->matrix.stack[FGL_MATRIX_PROJECTION].top();
		state.putIntegers((GLint *)mat->data, 16);
		break; }
	case GL_TEXTURE_MATRIX_FLOAT_AS_INT_BITS_OES: {
		unsigned id = FGL_MATRIX_TEXTURE(ctx->activeTexture);
		FGLmatrix *mat = &ctx->matrix.stack[id].top();
		state.putIntegers((GLint *)mat->data, 16);
		break; }

	case GL_CURRENT_COLOR:
		state.putNormalized(ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_RED]);
		state.putNormalized(ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_GREEN]);
		state.putNormalized(ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_BLUE]);
		state.putNormalized(ctx->vertex[FGL_ARRAY_COLOR][FGL_COMP_ALPHA]);
		break;
	case GL_CURRENT_NORMAL:
		state.putFloat(ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NX]);
		state.putFloat(ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NY]);
		state.putFloat(ctx->vertex[FGL_ARRAY_NORMAL][FGL_COMP_NZ]);
		break;
	case GL_CURRENT_TEXTURE_COORDS: {
		unsigned id = FGL_ARRAY_TEXTURE(ctx->activeTexture);
		state.putFloat(ctx->vertex[id][FGL_COMP_S]);
		state.putFloat(ctx->vertex[id][FGL_COMP_T]);
		state.putFloat(ctx->vertex[id][FGL_COMP_R]);
		state.putFloat(ctx->vertex[id][FGL_COMP_Q]);
		break; }

	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			state.putInteger(ctx->arrayBuffer.get()->getName());
		else
			state.putInteger(0);
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			state.putInteger(ctx->elementArrayBuffer.get()->getName());
		else
			state.putInteger(0);
		break;
	case GL_VIEWPORT:
		state.putFloat(ctx->viewport.x);
		state.putFloat(ctx->viewport.y);
		state.putFloat(ctx->viewport.width);
		state.putFloat(ctx->viewport.height);
		break;
	case GL_DEPTH_RANGE:
		state.putFloat(ctx->viewport.zNear);
		state.putFloat(ctx->viewport.zFar);
		break;
	case GL_POINT_SIZE:
		state.putFloat(ctx->rasterizer.pointSize);
		break;
	case GL_LINE_WIDTH:
		state.putFloat(ctx->rasterizer.lineWidth);
		break;
	case GL_CULL_FACE_MODE:
		state.putEnum(ctx->rasterizer.cullFace);
		break;
	case GL_FRONT_FACE:
		state.putEnum(ctx->rasterizer.frontFace);
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		state.putFloat(ctx->rasterizer.polyOffFactor);
		break;
	case GL_POLYGON_OFFSET_UNITS:
		state.putFloat(ctx->rasterizer.polyOffUnits);
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			state.putInteger(b->get()->getName());
		else
			state.putInteger(0);
		break; }
	case GL_CLIENT_ACTIVE_TEXTURE:
		state.putEnum(GL_TEXTURE0 + ctx->clientActiveTexture);
		break;
	case GL_ACTIVE_TEXTURE:
		state.putEnum(GL_TEXTURE0 + ctx->activeTexture);
		break;
	case GL_COLOR_WRITEMASK:
		state.putBoolean(ctx->perFragment.mask.red);
		state.putBoolean(ctx->perFragment.mask.green);
		state.putBoolean(ctx->perFragment.mask.blue);
		state.putBoolean(ctx->perFragment.mask.alpha);
		break;
	case GL_DEPTH_WRITEMASK:
		state.putBoolean(ctx->perFragment.mask.depth);
		break;
	case GL_STENCIL_WRITEMASK :
		state.putInteger(ctx->perFragment.mask.stencil);
		break;
	case GL_COLOR_CLEAR_VALUE :
		state.putNormalized(ctx->clear.red);
		state.putNormalized(ctx->clear.green);
		state.putNormalized(ctx->clear.blue);
		state.putNormalized(ctx->clear.alpha);
		break;
	case GL_DEPTH_CLEAR_VALUE :
		state.putNormalized(ctx->clear.depth);
		break;
	case GL_STENCIL_CLEAR_VALUE:
		state.putInteger(ctx->clear.stencil);
		break;
	case GL_SCISSOR_BOX:
		state.putInteger(ctx->perFragment.scissor.left);
		state.putInteger(ctx->perFragment.scissor.bottom);
		state.putInteger(ctx->perFragment.scissor.width);
		state.putInteger(ctx->perFragment.scissor.height);
		break;
	case GL_STENCIL_FUNC:
		state.putEnum(ctx->perFragment.stencil.func);
		break;
	case GL_STENCIL_VALUE_MASK:
		state.putInteger(ctx->perFragment.stencil.mask);
		break;
	case GL_STENCIL_REF :
		state.putInteger(ctx->perFragment.stencil.ref);
		break;
	case GL_STENCIL_FAIL :
		state.putEnum(ctx->perFragment.stencil.fail);
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		state.putEnum(ctx->perFragment.stencil.passDepthFail);
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		state.putEnum(ctx->perFragment.stencil.passDepthPass);
		break;
	case GL_DEPTH_FUNC:
		state.putEnum(ctx->perFragment.depthFunc);
		break;
	case GL_BLEND_SRC:
		state.putEnum(ctx->perFragment.blendSrc);
		break;
	case GL_BLEND_DST:
		state.putEnum(ctx->perFragment.blendDst);
		break;
	case GL_LOGIC_OP_MODE:
		state.putEnum(ctx->perFragment.logicOp);
		break;
	case GL_UNPACK_ALIGNMENT:
		state.putInteger(ctx->unpackAlignment);
		break;
	case GL_PACK_ALIGNMENT:
		state.putInteger(ctx->packAlignment);
		break;
	case GL_SUBPIXEL_BITS:
		state.putInteger(FGL_MAX_SUBPIXEL_BITS);
		break;
	case GL_MAX_TEXTURE_SIZE:
		state.putInteger(FGL_MAX_TEXTURE_SIZE);
		break;
	case GL_MAX_VIEWPORT_DIMS:
		state.putInteger(FGL_MAX_VIEWPORT_DIMS);
		state.putInteger(FGL_MAX_VIEWPORT_DIMS);
		break;
	case GL_SMOOTH_POINT_SIZE_RANGE:
	case GL_ALIASED_POINT_SIZE_RANGE:
		state.putFloat(FGL_MIN_POINT_SIZE);
		state.putFloat(FGL_MAX_POINT_SIZE);
		break;
	case GL_SMOOTH_LINE_WIDTH_RANGE:
	case GL_ALIASED_LINE_WIDTH_RANGE:
		state.putFloat(FGL_MIN_LINE_WIDTH);
		state.putFloat(FGL_MAX_LINE_WIDTH);
		break;
	case GL_MAX_TEXTURE_UNITS:
		state.putInteger(FGL_MAX_TEXTURE_UNITS);
		break;
	case GL_MAX_LIGHTS:
		state.putInteger(FGL_MAX_LIGHTS);
		break;
	case GL_SAMPLE_BUFFERS :
		state.putInteger(0);
		break;
	case GL_SAMPLES :
		state.putInteger(0);
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS:
		for (int i = 0; i < (int)NELEM(fglCompressedTextureFormats); ++i)
			state.putEnum(fglCompressedTextureFormats[i]);
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		state.putInteger(NELEM(fglCompressedTextureFormats));
		break;
	case GL_RED_BITS : {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putInteger(cfg->comp[FGL_COMP_RED].size);
		break; }
	case GL_GREEN_BITS: {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putInteger(cfg->comp[FGL_COMP_GREEN].size);
		break; }
	case GL_BLUE_BITS : {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putInteger(cfg->comp[FGL_COMP_BLUE].size);
		break; }
	case GL_ALPHA_BITS : {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putInteger(cfg->comp[FGL_COMP_ALPHA].size);
		break; }
	case GL_DEPTH_BITS : {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		state.putInteger(fb->getDepthFormat() & 0xff);
		break; }
	case GL_STENCIL_BITS: {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		state.putInteger(fb->getDepthFormat() >> 8);
		break; }
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES: {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putEnum(cfg->readType);
		break; }
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES: {
		FGLAbstractFramebuffer *fb = ctx->framebuffer.get();
		fb->checkStatus();
		const FGLPixelFormat *cfg =
				FGLPixelFormat::get(fb->getColorFormat());
		state.putEnum(cfg->readFormat);
		break; }

	case GL_VERTEX_ARRAY_SIZE:
		state.putInteger(ctx->array[FGL_ARRAY_VERTEX].size);
		break;
	case GL_VERTEX_ARRAY_TYPE:
		state.putEnum(ctx->array[FGL_ARRAY_VERTEX].type);
		break;
	case GL_VERTEX_ARRAY_STRIDE:
		state.putInteger(ctx->array[FGL_ARRAY_VERTEX].stride);
		break;
	case GL_VERTEX_ARRAY_BUFFER_BINDING: {
		FGLBuffer *buf = ctx->array[FGL_ARRAY_VERTEX].buffer;
		GLint name = (buf) ? buf->getName() : 0;
		state.putInteger(name);
		break; }
	case GL_NORMAL_ARRAY_TYPE:
		state.putEnum(ctx->array[FGL_ARRAY_NORMAL].type);
		break;
	case GL_NORMAL_ARRAY_STRIDE:
		state.putInteger(ctx->array[FGL_ARRAY_NORMAL].stride);
		break;
	case GL_NORMAL_ARRAY_BUFFER_BINDING: {
		FGLBuffer *buf = ctx->array[FGL_ARRAY_NORMAL].buffer;
		GLint name = (buf) ? buf->getName() : 0;
		state.putInteger(name);
		break; }
	case GL_COLOR_ARRAY_SIZE:
		state.putInteger(ctx->array[FGL_ARRAY_COLOR].size);
		break;
	case GL_COLOR_ARRAY_TYPE:
		state.putEnum(ctx->array[FGL_ARRAY_COLOR].type);
		break;
	case GL_COLOR_ARRAY_STRIDE:
		state.putInteger(ctx->array[FGL_ARRAY_COLOR].stride);
		break;
	case GL_COLOR_ARRAY_BUFFER_BINDING: {
		FGLBuffer *buf = ctx->array[FGL_ARRAY_COLOR].buffer;
		GLint name = (buf) ? buf->getName() : 0;
		state.putInteger(name);
		break; }
	case GL_TEXTURE_COORD_ARRAY_SIZE: {
		unsigned id = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		state.putInteger(ctx->array[id].size);
		break; }
	case GL_TEXTURE_COORD_ARRAY_TYPE:{
		unsigned id = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		state.putEnum(ctx->array[id].type);
		break; }
	case GL_TEXTURE_COORD_ARRAY_STRIDE:{
		unsigned id = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		state.putInteger(ctx->array[id].stride);
		break; }
	case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING: {
		unsigned id = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		FGLBuffer *buf = ctx->array[id].buffer;
		GLint name = (buf) ? buf->getName() : 0;
		state.putInteger(name);
		break; }
	case GL_POINT_SIZE_ARRAY_TYPE_OES:
		state.putEnum(ctx->array[FGL_ARRAY_POINT_SIZE].type);
		break;
	case GL_POINT_SIZE_ARRAY_STRIDE_OES:
		state.putInteger(ctx->array[FGL_ARRAY_POINT_SIZE].stride);
		break;
	case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES: {
		FGLBuffer *buf = ctx->array[FGL_ARRAY_POINT_SIZE].buffer;
		GLint name = (buf) ? buf->getName() : 0;
		state.putInteger(name);
		break; }

	/* Single boolean values */
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
	case GL_VERTEX_ARRAY:
	case GL_NORMAL_ARRAY:
	case GL_COLOR_ARRAY:
	case GL_TEXTURE_COORD_ARRAY:
	case GL_POINT_SIZE_ARRAY_OES:
		state.putBoolean(glIsEnabled(pname));
		break;
	default:
		setError(GL_INVALID_ENUM);
		LOGD("Invalid enum: %x", pname);
	}
}

GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *params)
{
	FGLContext *ctx = getContext();
	FGLIntegerGetter state(params);

	fglGetState(ctx, pname, state);
}

GL_API void GL_APIENTRY glGetBooleanv (GLenum pname, GLboolean *params)
{
	FGLContext *ctx = getContext();
	FGLBooleanGetter state(params);

	fglGetState(ctx, pname, state);
}

GL_API void GL_APIENTRY glGetFixedv (GLenum pname, GLfixed *params)
{
	FGLContext *ctx = getContext();
	FGLFixedGetter state(params);

	fglGetState(ctx, pname, state);
}

GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *params)
{
	FGLContext *ctx = getContext();
	FGLFloatGetter state(params);

	fglGetState(ctx, pname, state);
}

GL_API void GL_APIENTRY glGetPointerv (GLenum pname, void **params)
{
	FGLContext *ctx = getContext();
	unsigned id;

	switch (pname) {
	case GL_VERTEX_ARRAY_POINTER:
		id = FGL_ARRAY_VERTEX;
		break;
	case GL_NORMAL_ARRAY_POINTER:
		id = FGL_ARRAY_NORMAL;
		break;
	case GL_COLOR_ARRAY_POINTER:
		id = FGL_ARRAY_COLOR;
		break;
	case GL_TEXTURE_COORD_ARRAY_POINTER: {
		GLint unit = ctx->clientActiveTexture;
		id = FGL_ARRAY_TEXTURE(unit);
		break; }
	case GL_POINT_SIZE_ARRAY_POINTER_OES:
		id = FGL_ARRAY_POINT_SIZE;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	const GLvoid *ptr;
	FGLBuffer *buf;

	ptr = ctx->array[id].pointer;
	buf = ctx->array[id].buffer;

	if (buf)
		ptr = buf->getOffset(ptr);

	params[0] = (void *)ptr;
}

GL_API GLboolean GL_APIENTRY glIsEnabled (GLenum cap)
{
	FGLContext *ctx = getContext();

	switch (cap) {
	case GL_CULL_FACE:
		return ctx->enable.cullFace;
	case GL_POLYGON_OFFSET_FILL:
		return ctx->enable.polyOffFill;
	case GL_SCISSOR_TEST:
		return ctx->enable.scissorTest;
	case GL_ALPHA_TEST:
		return ctx->enable.alphaTest;
	case GL_STENCIL_TEST:
		return ctx->enable.stencilTest;
	case GL_DEPTH_TEST:
		return ctx->enable.depthTest;
	case GL_BLEND:
		return ctx->enable.blend;
	case GL_DITHER:
		return ctx->enable.dither;
	case GL_COLOR_LOGIC_OP:
		return ctx->enable.colorLogicOp;
	case GL_VERTEX_ARRAY:
		return ctx->array[FGL_ARRAY_VERTEX].enabled;
	case GL_NORMAL_ARRAY:
		return ctx->array[FGL_ARRAY_NORMAL].enabled;
	case GL_COLOR_ARRAY:
		return ctx->array[FGL_ARRAY_COLOR].enabled;
	case GL_TEXTURE_COORD_ARRAY: {
		unsigned id = FGL_ARRAY_TEXTURE(ctx->clientActiveTexture);
		return ctx->array[id].enabled;
	}
	case GL_POINT_SIZE_ARRAY_OES:
		return ctx->array[FGL_ARRAY_POINT_SIZE].enabled;
	default:
		setError(GL_INVALID_ENUM);
		return GL_FALSE;
	}
}

GL_API void GL_APIENTRY glGetBufferParameteriv (GLenum target,
						GLenum pname, GLint *params)
{
	FGLBufferObjectBinding *binding;

	FGLContext *ctx = getContext();

	switch (target) {
	case GL_ARRAY_BUFFER:
		binding = &ctx->arrayBuffer;
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		binding = &ctx->elementArrayBuffer;
		break;
	default:
		setError(GL_INVALID_ENUM);
		return;
	}

	FGLBuffer *buf = binding->get();

	if (!buf) {
		setError(GL_INVALID_OPERATION);
		return;
	}

	switch (pname) {
	case GL_BUFFER_SIZE:
		*params = buf->size;
		break;
	case GL_BUFFER_USAGE:
		*params = buf->usage;
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

/*
 * Stubs
 */

GL_API void GL_APIENTRY glGetClipPlanef (GLenum pname, GLfloat eqn[4])
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetClipPlanex (GLenum pname, GLfixed eqn[4])
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetLightfv (GLenum light, GLenum pname,
							GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}

GL_API void GL_APIENTRY glGetMaterialfv (GLenum face, GLenum pname,
							GLfloat *params)
{
	FUNC_UNIMPLEMENTED;
}
