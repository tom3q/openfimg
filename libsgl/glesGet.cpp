/**
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

/**
	Client API information
*/

static char const * const gVendorString     = "OpenFIMG";
static char const * const gRendererString   = "FIMG-3DSE";
static char const * const gVersionString    = "OpenGL ES-CM 1.1";
static char const * const gExtensionsString =
#if 0
	"GL_OES_point_sprite "                  // TODO
	"GL_ARB_texture_env_combine "		// TODO
	"GL_ARB_texture_env_crossbar "		// TODO
	"GL_ARB_texture_env_dot3 "		// TODO
	"GL_ARB_texture_mirrored_repeat "	// TODO
	"GL_ARB_vertex_buffer_object "		// TODO
	"GL_ATI_texture_compression_atitc "	// TODO
	"GL_EXT_blend_equation_separate "	// TODO
	"GL_EXT_blend_func_separate "		// TODO
	"GL_EXT_blend_minmax "			// TODO
	"GL_EXT_blend_subtract "		// TODO
	"GL_EXT_stencil_wrap "			// TODO
	"GL_OES_matrix_palette "		// TODO
	"GL_OES_vertex_buffer_object "		// TODO
	"GL_QUALCOMM_vertex_buffer_object "	// TODO
	"GL_QUALCOMM_direct_texture "		// TODO
#endif
	"GL_OES_byte_coordinates "
	"GL_OES_fixed_point "
	"GL_OES_single_precision "
	"GL_OES_read_format "
	//"GL_OES_compressed_paletted_texture "   // TODO
	"GL_OES_draw_texture "
	//"GL_OES_matrix_get "                    // TODO
	//"GL_OES_query_matrix "                  // TODO
	"GL_OES_EGL_image "
	"GL_OES_EGL_image_external "
	"GL_EXT_texture_format_BGRA8888 "
	//"GL_OES_compressed_ETC1_RGB8_texture "  // TODO
	//"GL_ARB_texture_compression "           // TODO IMPORTANT
#ifdef FGL_NPOT_TEXTURES
	"GL_ARB_texture_non_power_of_two "
#endif
	//"GL_ANDROID_user_clip_plane "           // TODO
	//"GL_ANDROID_vertex_buffer_object "      // TODO
	//"GL_ANDROID_generate_mipmap "           // TODO
	"GL_OES_point_size_array"
;

static const GLint fglCompressedTextureFormats[] = {
#if 0
	GL_PALETTE4_RGB8_OES,
	GL_PALETTE4_RGBA8_OES,
	GL_PALETTE4_R5_G6_B5_OES,
	GL_PALETTE4_RGBA4_OES,
	GL_PALETTE4_RGB5_A1_OES,
	GL_PALETTE8_RGB8_OES,
	GL_PALETTE8_RGBA8_OES,
	GL_PALETTE8_R5_G6_B5_OES,
	GL_PALETTE8_RGBA4_OES,
	GL_PALETTE8_RGB5_A1_OES,
	GL_RGB_S3TC_OES,
	GL_RGBA_S3TC_OES
#endif
};

const FGLColorConfigDesc fglColorConfigs[] = {
	/* [FGPF_COLOR_MODE_555] */
	{
		5, 5, 5, 0,
		GL_RGB,
		GL_UNSIGNED_SHORT_5_5_5_1,
		2,
		10, 5, 0, 0,
		GL_TRUE, /* Force opaque */
		FGTU_TSTA_TEXTURE_FORMAT_1555,
	},
	/* [FGPF_COLOR_MODE_565] */
	{
		5, 6, 5, 0,
		GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5,
		2,
		11, 5, 0, 0,
		GL_FALSE,
		FGTU_TSTA_TEXTURE_FORMAT_565,
	},
	/* [FGPF_COLOR_MODE_4444] */
	{
		4, 4, 4, 4,
		GL_RGBA,
		GL_UNSIGNED_SHORT_4_4_4_4,
		2,
		12, 8, 4, 0,
		GL_FALSE,
		FGTU_TSTA_TEXTURE_FORMAT_4444,
	},
	/* [FGPF_COLOR_MODE_1555] */
	{
		5, 5, 5, 1,
		GL_RGBA,
		GL_UNSIGNED_SHORT_5_5_5_1,
		2,
		10, 5, 0, 15,
		GL_FALSE,
		FGTU_TSTA_TEXTURE_FORMAT_1555,
	},
	/* [FGPF_COLOR_MODE_0888] */
	{
		8, 8, 8, 8,
		GL_BGRA_EXT,
		GL_UNSIGNED_BYTE,
		4,
		16, 8, 0, 24,
		GL_TRUE, /* Force opaque */
		FGTU_TSTA_TEXTURE_FORMAT_8888,
	},
	/* [FGPF_COLOR_MODE_8888] */
	{
		8, 8, 8, 8,
		GL_BGRA_EXT,
		GL_UNSIGNED_BYTE,
		4,
		16, 8, 0, 24,
		GL_FALSE,
		FGTU_TSTA_TEXTURE_FORMAT_8888,
	},
};

const FGLColorConfigDesc *fglGetColorConfigDesc(unsigned int format)
{
	assert(format < NELEM(fglColorConfigs));

	return &fglColorConfigs[format];
}

// ----------------------------------------------------------------------------

/**
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
	virtual void putFloat(GLfloat f) = 0;
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

private:
	GLboolean *_b;
};

void fglGetState(FGLContext *ctx, GLenum pname, FGLStateGetter &state)
{
	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		if (ctx->arrayBuffer.isBound())
			state.putInteger(ctx->arrayBuffer.getName());
		else
			state.putInteger(0);
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		if (ctx->elementArrayBuffer.isBound())
			state.putInteger(ctx->elementArrayBuffer.getName());
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
			state.putInteger(b->getName());
		else
			state.putInteger(0);
		break; }
	case GL_ACTIVE_TEXTURE:
		state.putInteger(ctx->activeTexture);
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
	case GL_ALIASED_POINT_SIZE_RANGE:
		state.putFloat(FGL_MIN_POINT_SIZE);
		state.putFloat(FGL_MAX_POINT_SIZE);
		break;
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
	case GL_RED_BITS :
		state.putInteger(fglColorConfigs[ctx->surface.format].red);
		break;
	case GL_GREEN_BITS:
		state.putInteger(fglColorConfigs[ctx->surface.format].green);
		break;
	case GL_BLUE_BITS :
		state.putInteger(fglColorConfigs[ctx->surface.format].blue);
		break;
	case GL_ALPHA_BITS :
		state.putInteger(fglColorConfigs[ctx->surface.format].alpha);
		break;
	case GL_DEPTH_BITS :
		state.putInteger(ctx->surface.depthFormat & 0xff);
		break;
	case GL_STENCIL_BITS:
		state.putInteger(ctx->surface.depthFormat >> 8);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
		state.putEnum(fglColorConfigs[ctx->surface.format].readType);
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
		state.putEnum(fglColorConfigs[ctx->surface.format].readFormat);
		break;

	/* Single boolean values */
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
		state.putBoolean(glIsEnabled(pname));
		break;
	default:
		setError(GL_INVALID_ENUM);
		LOGD("Invalid enum: %d", pname);
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
	default:
		setError(GL_INVALID_ENUM);
		return GL_FALSE;
	}
}
