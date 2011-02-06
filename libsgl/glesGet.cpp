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

#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <cutils/log.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include "glesCommon.h"
#include "fglobjectmanager.h"
#include "libfimg/fimg.h"
#include "s3c_g2d.h"

/**
	Client API information
*/

static char const * const gVendorString     = "GLES6410";
static char const * const gRendererString   = "S3C6410 FIMG-3DSE";
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
		2
	},
	/* [FGPF_COLOR_MODE_565] */
	{
		5, 6, 5, 0,
		GL_RGB,
		GL_UNSIGNED_SHORT_5_6_5,
		2
	},
	/* [FGPF_COLOR_MODE_4444] */
	{
		4, 4, 4, 4,
		GL_RGBA,
		GL_UNSIGNED_SHORT_4_4_4_4,
		2
	},
	/* [FGPF_COLOR_MODE_1555] */
	{
		5, 5, 5, 1,
		GL_RGBA,
		GL_UNSIGNED_SHORT_5_5_5_1,
		2
	},
	/* [FGPF_COLOR_MODE_0888] */
	{
		8, 8, 8, 0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		4
	},
	/* [FGPF_COLOR_MODE_8888] */
	{
		8, 8, 8, 8,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		4
	}
};

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

GL_API void GL_APIENTRY glGetIntegerv (GLenum pname, GLint *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_ARRAY_BUFFER_BINDING:
		params[0] = 0;
		if (ctx->arrayBuffer.isBound())
			params[0] = ctx->arrayBuffer.getName();
		break;
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
		params[0] = 0;
		if (ctx->elementArrayBuffer.isBound())
			params[0] = ctx->elementArrayBuffer.getName();
		break;
	case GL_VIEWPORT:
		params[0] = ctx->viewport.x;
		params[1] = ctx->viewport.y;
		params[2] = ctx->viewport.width;
		params[3] = ctx->viewport.height;
		break;
	case GL_DEPTH_RANGE:
		params[0] = ctx->viewport.zNear;
		params[1] = ctx->viewport.zFar;
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_POINT_SIZE);
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_LINE_WIDTH);
		break;
	case GL_CULL_FACE_MODE:
		switch (fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_MODE)) {
		case FGRA_BFCULL_FACE_FRONT:
			params[0] = GL_FRONT;
			break;
		case FGRA_BFCULL_FACE_BACK:
			params[0] = GL_BACK;
			break;
		case FGRA_BFCULL_FACE_BOTH:
			params[0] = GL_FRONT_AND_BACK;
			break;
		}
		break;
	case GL_FRONT_FACE:
		if (fimgGetRasterizerState(ctx->fimg, FIMG_FRONT_FACE))
			params[0] = GL_CW;
		else
			params[0] = GL_CCW;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR);
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS);
		break;
	case GL_TEXTURE_BINDING_2D: {
		FGLTextureObjectBinding *b =
				&ctx->texture[ctx->activeTexture].binding;
		if (b->isBound())
			params[0] = b->getName();
		else
			params[0] = 0.0f;
		break; }
	case GL_ACTIVE_TEXTURE:
		params[0] = ctx->activeTexture;
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_DEPTH_WRITEMASK:
		params[0] = ctx->perFragment.mask.depth;
		break;
	case GL_STENCIL_WRITEMASK :
		params[0] = ctx->perFragment.mask.stencil;
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = intFromClampf(ctx->clear.red);
		params[1] = intFromClampf(ctx->clear.green);
		params[2] = intFromClampf(ctx->clear.blue);
		params[3] = intFromClampf(ctx->clear.alpha);
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = intFromClampf(ctx->clear.depth);
		break;
	case GL_STENCIL_CLEAR_VALUE:
		params[0] = ctx->clear.stencil;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left;
		params[1] = ctx->perFragment.scissor.bottom;
		params[2] = ctx->perFragment.scissor.width;
		params[3] = ctx->perFragment.scissor.height;
		break;
	case GL_STENCIL_FUNC:
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_FUNC)) {
		case FGPF_STENCIL_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_STENCIL_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_STENCIL_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_STENCIL_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_STENCIL_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_STENCIL_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_STENCIL_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_STENCIL_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
	case GL_STENCIL_VALUE_MASK:
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_MASK);
		break;
	case GL_STENCIL_REF :
		params[0] = fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_REF);
		break;
	case GL_STENCIL_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_SFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_FAIL :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPFAIL)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
	case GL_STENCIL_PASS_DEPTH_PASS :
		switch (fimgGetFragmentState(ctx->fimg,
						FIMG_FRONT_STENCIL_DPPASS)) {
		case FGPF_TEST_ACTION_KEEP:
			params[0] = GL_KEEP;
			break;
		case FGPF_TEST_ACTION_ZERO:
			params[0] = GL_ZERO;
			break;
		case FGPF_TEST_ACTION_REPLACE:
			params[0] = GL_REPLACE;
			break;
		case FGPF_TEST_ACTION_INCR:
			params[0] = GL_INCR;
			break;
		case FGPF_TEST_ACTION_DECR:
			params[0] = GL_DECR;
			break;
		case FGPF_TEST_ACTION_INVERT:
			params[0] = GL_INVERT;
			break;
		}
		break;
#if 0
	case GL_STENCIL_BACK_FUNC :
		params[0] = (float)ctx->stencil_test_data.back_face_func;
		break;
	case GL_STENCIL_BACK_VALUE_MASK :
		params[0] = (float)ctx->stencil_test_data.back_face_mask;
		break;
	case GL_STENCIL_BACK_REF :
		params[0] = (float)ctx->stencil_test_data.back_face_ref;
		break;
	case GL_STENCIL_BACK_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_fail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
		params[0] = (float)ctx->stencil_test_data.back_face_zfail;
		break;
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
		params[0] = (float)ctx->stencil_test_data.back_face_zpass;
		break;
#endif
	case GL_DEPTH_FUNC:
		switch (fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_FUNC)) {
		case FGPF_TEST_MODE_NEVER:
			params[0] = GL_NEVER;
			break;
		case FGPF_TEST_MODE_ALWAYS:
			params[0] = GL_ALWAYS;
			break;
		case FGPF_TEST_MODE_GREATER:
			params[0] = GL_GREATER;
			break;
		case FGPF_TEST_MODE_GEQUAL:
			params[0] = GL_GEQUAL;
			break;
		case FGPF_TEST_MODE_EQUAL:
			params[0] = GL_EQUAL;
			break;
		case FGPF_TEST_MODE_LESS:
			params[0] = GL_LESS;
			break;
		case FGPF_TEST_MODE_LEQUAL:
			params[0] = GL_LEQUAL;
			break;
		case FGPF_TEST_MODE_NOTEQUAL:
			params[0] = GL_NOTEQUAL;
			break;
		}
		break;
#if 0
	case GL_BLEND_SRC_RGB :
		params[0] = (float)ctx->blend_data.fn_srcRGB;
		break;
	case GL_BLEND_SRC_ALPHA:
		params[0] = (float)ctx->blend_data.fn_srcAlpha;
		break;
	case GL_BLEND_DST_RGB :
		params[0] = (float)ctx->blend_data.fn_dstRGB;
		break;
	case GL_BLEND_DST_ALPHA:
		params[0] = (float)ctx->blend_data.fn_dstAlpha;
		break;
	case GL_BLEND_EQUATION_RGB :
		params[0] =(float)ctx->blend_data.eqn_modeRGB ;
		break;
	case GL_BLEND_EQUATION_ALPHA:
		params[0] = (float)ctx->blend_data.eqn_modeAlpha;
		break;
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_UNPACK_ALIGNMENT:
		params[0] = ctx->unpackAlignment;
		break;
	case GL_PACK_ALIGNMENT:
		params[0] = ctx->packAlignment;
		break;
	case GL_SUBPIXEL_BITS:
		params[0] = FGL_MAX_SUBPIXEL_BITS;
		break;
	case GL_MAX_TEXTURE_SIZE:
		params[0] = FGL_MAX_TEXTURE_SIZE;
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = FGL_MAX_VIEWPORT_DIMS;
		params[1] = FGL_MAX_VIEWPORT_DIMS;
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
	case GL_MAX_ELEMENTS_INDICES :
		params[0] = MAX_ELEMENTS_INDICES;
		break;
	case GL_MAX_ELEMENTS_VERTICES :
		params[0] = MAX_ELEMENTS_VERTICES;
		break;
#endif
	case GL_MAX_TEXTURE_UNITS:
		params[0] = FGL_MAX_TEXTURE_UNITS;
		break;
	case GL_MAX_LIGHTS:
		params[0] = FGL_MAX_LIGHTS;
		break;
	case GL_SAMPLE_BUFFERS :
		params[0] = 0;
		break;
	case GL_SAMPLES :
		params[0] = 0;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		for (int i = 0; i < NELEM(fglCompressedTextureFormats); ++i)
			params[i] = fglCompressedTextureFormats[i];
		break;
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
		params[0] = NELEM(fglCompressedTextureFormats);
		break;
#if 0
	case GL_MAX_RENDERBUFFER_SIZE :
		params[0] = FGL_MAX_RENDERBUFFER_SIZE;
		break;
#endif
	case GL_RED_BITS :
		params[0] = fglColorConfigs[ctx->surface.format].red;
		break;
	case GL_GREEN_BITS:
		params[0] = fglColorConfigs[ctx->surface.format].green;
		break;
	case GL_BLUE_BITS :
		params[0] = fglColorConfigs[ctx->surface.format].blue;
		break;
	case GL_ALPHA_BITS :
		params[0] = fglColorConfigs[ctx->surface.format].alpha;
		break;
	case GL_DEPTH_BITS :
		params[0] = ctx->surface.depthFormat & 0xff;
		break;
	case GL_STENCIL_BITS:
		params[0] = ctx->surface.depthFormat >> 8;
		break;
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
		params[0] = fglColorConfigs[ctx->surface.format].readType;
		break;
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
		params[0] = fglColorConfigs[ctx->surface.format].readFormat;
		break;
#if 0
	case GL_ALPHA_TEST_FUNC_EXP:
		params[0] = (float)ctx->alpha_test_data.func;
		break;
	case GL_ALPHA_TEST_REF_EXP:
		params[0] = ctx->alpha_test_data.refValue;
		break;
	case GL_LOGIC_OP_MODE_EXP:
		break;
#endif

	/* Single boolean values */
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
		LOGD("Invalid enum: %d", pname);
	}
}

GL_API void GL_APIENTRY glGetBooleanv (GLenum pname, GLboolean *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_VIEWPORT:
		params[0] = !!ctx->viewport.x;
		params[1] = !!ctx->viewport.y;
		params[2] = !!ctx->viewport.width;
		params[3] = !!ctx->viewport.height;
		break;
	case GL_DEPTH_RANGE:
		params[0] = !!ctx->viewport.zNear;
		params[1] = !!ctx->viewport.zFar;
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
							FIMG_POINT_SIZE) != 0;
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
							FIMG_LINE_WIDTH) != 0;
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR) != 0;
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS) != 0;
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_COLOR_CLEAR_VALUE :
		params[0] = ctx->clear.red != 0;
		params[1] = ctx->clear.green != 0;
		params[2] = ctx->clear.blue != 0;
		params[3] = ctx->clear.alpha != 0;
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = ctx->clear.depth != 0;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left != 0;
		params[1] = ctx->perFragment.scissor.bottom != 0;
		params[2] = ctx->perFragment.scissor.width != 0;
		params[3] = ctx->perFragment.scissor.height != 0;
		break;
#if 0
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
#endif
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = !!FGL_MAX_VIEWPORT_DIMS;
		params[1] = !!FGL_MAX_VIEWPORT_DIMS;
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
#endif
	case GL_COMPRESSED_TEXTURE_FORMATS :
		for (int i = 0; i < NELEM(fglCompressedTextureFormats); ++i)
			params[i] = !!fglCompressedTextureFormats[i];
		break;

	/* Single integer values */
#if 0
	case GL_MAX_ELEMENTS_INDICES :
	case GL_MAX_ELEMENTS_VERTICES :
	case GL_MAX_RENDERBUFFER_SIZE :
	case GL_ALPHA_TEST_FUNC_EXP:
	case GL_ALPHA_TEST_REF_EXP:
	case GL_LOGIC_OP_MODE_EXP:
	case GL_BLEND_SRC_RGB :
	case GL_BLEND_SRC_ALPHA:
	case GL_BLEND_DST_RGB :
	case GL_BLEND_DST_ALPHA:
	case GL_BLEND_EQUATION_RGB :
	case GL_BLEND_EQUATION_ALPHA:
	case GL_STENCIL_BACK_FUNC :
	case GL_STENCIL_BACK_VALUE_MASK :
	case GL_STENCIL_BACK_REF :
	case GL_STENCIL_BACK_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
	case GL_STENCIL_BACK_WRITEMASK :
#endif
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
	case GL_ARRAY_BUFFER_BINDING:
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
	case GL_CULL_FACE_MODE:
	case GL_FRONT_FACE:
	case GL_TEXTURE_BINDING_2D:
	case GL_ACTIVE_TEXTURE:
	case GL_STENCIL_CLEAR_VALUE:
	case GL_DEPTH_WRITEMASK:
	case GL_STENCIL_WRITEMASK :
	case GL_STENCIL_FUNC:
	case GL_STENCIL_VALUE_MASK:
	case GL_STENCIL_REF :
	case GL_STENCIL_FAIL :
	case GL_STENCIL_PASS_DEPTH_FAIL :
	case GL_STENCIL_PASS_DEPTH_PASS :
	case GL_DEPTH_FUNC:
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
	case GL_UNPACK_ALIGNMENT:
	case GL_PACK_ALIGNMENT:
	case GL_SUBPIXEL_BITS:
	case GL_MAX_TEXTURE_SIZE:
	case GL_MAX_TEXTURE_UNITS:
	case GL_MAX_LIGHTS:
	case GL_SAMPLE_BUFFERS :
	case GL_SAMPLES :
	case GL_RED_BITS :
	case GL_GREEN_BITS:
	case GL_BLUE_BITS :
	case GL_ALPHA_BITS :
	case GL_DEPTH_BITS :
	case GL_STENCIL_BITS:
		GLint intval;
		glGetIntegerv(pname, &intval);
		params[0] = !!intval;
		break;

	/* Single boolean values */
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetFixedv (GLenum pname, GLfixed *params)
{
	FGLContext *ctx = getContext();
	GLint intval;

	switch (pname) {
	case GL_VIEWPORT:
		params[0] = fixedFromInt(ctx->viewport.x);
		params[1] = fixedFromInt(ctx->viewport.y);
		params[2] = fixedFromInt(ctx->viewport.width);
		params[3] = fixedFromInt(ctx->viewport.height);
		break;
	case GL_DEPTH_RANGE:
		params[0] = fixedFromFloat(ctx->viewport.zNear);
		params[1] = fixedFromFloat(ctx->viewport.zFar);
		break;
	case GL_POINT_SIZE:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
							FIMG_POINT_SIZE));
		break;
	case GL_LINE_WIDTH:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
							FIMG_LINE_WIDTH));
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR));
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fixedFromFloat(fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS));
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = fixedFromBool(ctx->perFragment.mask.red);
		params[1] = fixedFromBool(ctx->perFragment.mask.green);
		params[2] = fixedFromBool(ctx->perFragment.mask.blue);
		params[3] = fixedFromBool(ctx->perFragment.mask.alpha);
		break;
#if 0
	case GL_STENCIL_BACK_WRITEMASK :
		params[0] = (float)ctx->back_stencil_writemask;
		break;
#endif
	case GL_COLOR_CLEAR_VALUE :
		params[0] = fixedFromFloat(ctx->clear.red);
		params[1] = fixedFromFloat(ctx->clear.green);
		params[2] = fixedFromFloat(ctx->clear.blue);
		params[3] = fixedFromFloat(ctx->clear.alpha);
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = fixedFromFloat(ctx->clear.depth);
		break;
	case GL_SCISSOR_BOX:
		params[0] = fixedFromInt(ctx->perFragment.scissor.left);
		params[1] = fixedFromInt(ctx->perFragment.scissor.bottom);
		params[2] = fixedFromInt(ctx->perFragment.scissor.width);
		params[3] = fixedFromInt(ctx->perFragment.scissor.height);
		break;
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = fixedFromInt(FGL_MAX_VIEWPORT_DIMS);
		params[1] = fixedFromInt(FGL_MAX_VIEWPORT_DIMS);
		break;
#if 0
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
#endif
	case GL_COMPRESSED_TEXTURE_FORMATS :
		for (int i = 0; i < NELEM(fglCompressedTextureFormats); ++i)
			params[i] = fglCompressedTextureFormats[i];
		break;

	/* Single integer values */
#if 0
	case GL_MAX_ELEMENTS_INDICES :
	case GL_MAX_ELEMENTS_VERTICES :
	case GL_MAX_RENDERBUFFER_SIZE :
	case GL_ALPHA_TEST_REF_EXP:
	case GL_LOGIC_OP_MODE_EXP:
	case GL_STENCIL_BACK_VALUE_MASK :
	case GL_STENCIL_BACK_REF :
	case GL_STENCIL_BACK_WRITEMASK :
#endif
	case GL_ARRAY_BUFFER_BINDING:
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
	case GL_TEXTURE_BINDING_2D:
	case GL_ACTIVE_TEXTURE:
	case GL_STENCIL_CLEAR_VALUE:
	case GL_DEPTH_WRITEMASK:
	case GL_STENCIL_WRITEMASK :
	case GL_STENCIL_VALUE_MASK:
	case GL_STENCIL_REF :
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
	case GL_UNPACK_ALIGNMENT:
	case GL_PACK_ALIGNMENT:
	case GL_SUBPIXEL_BITS:
	case GL_MAX_TEXTURE_SIZE:
	case GL_MAX_TEXTURE_UNITS:
	case GL_MAX_LIGHTS:
	case GL_SAMPLE_BUFFERS :
	case GL_SAMPLES :
	case GL_RED_BITS :
	case GL_GREEN_BITS:
	case GL_BLUE_BITS :
	case GL_ALPHA_BITS :
	case GL_DEPTH_BITS :
	case GL_STENCIL_BITS :
		glGetIntegerv(pname, &intval);
		params[0] = fixedFromInt(intval);
		break;

	/* Single enumerated values */
#if 0
	case GL_ALPHA_TEST_FUNC_EXP:
	case GL_BLEND_SRC_RGB :
	case GL_BLEND_SRC_ALPHA:
	case GL_BLEND_DST_RGB :
	case GL_BLEND_DST_ALPHA:
	case GL_BLEND_EQUATION_RGB :
	case GL_BLEND_EQUATION_ALPHA:
	case GL_STENCIL_BACK_FUNC :
	case GL_STENCIL_BACK_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
#endif
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
	case GL_CULL_FACE_MODE:
	case GL_FRONT_FACE:
	case GL_STENCIL_FUNC:
	case GL_STENCIL_FAIL :
	case GL_STENCIL_PASS_DEPTH_FAIL :
	case GL_STENCIL_PASS_DEPTH_PASS :
	case GL_DEPTH_FUNC:
		glGetIntegerv(pname, &intval);
		params[0] = intval;
		break;

	/* Single boolean value */
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
		params[0] = fixedFromBool(glIsEnabled(pname));
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
}

GL_API void GL_APIENTRY glGetFloatv (GLenum pname, GLfloat *params)
{
	FGLContext *ctx = getContext();

	switch (pname) {
	case GL_VIEWPORT:
		params[0] = ctx->viewport.x;
		params[1] = ctx->viewport.y;
		params[2] = ctx->viewport.width;
		params[3] = ctx->viewport.height;
		break;
	case GL_DEPTH_RANGE:
		params[0] = ctx->viewport.zNear;
		params[1] = ctx->viewport.zFar;
		break;
	case GL_POINT_SIZE:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_POINT_SIZE);
		break;
	case GL_LINE_WIDTH:
		params[0] = fimgGetRasterizerStateF(ctx->fimg, FIMG_LINE_WIDTH);
		break;
	case GL_POLYGON_OFFSET_FACTOR:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_FACTOR);
		break;
	case GL_POLYGON_OFFSET_UNITS:
		params[0] = fimgGetRasterizerStateF(ctx->fimg,
						FIMG_DEPTH_OFFSET_UNITS);
		break;
	case GL_COLOR_WRITEMASK:
		params[0] = ctx->perFragment.mask.red;
		params[1] = ctx->perFragment.mask.green;
		params[2] = ctx->perFragment.mask.blue;
		params[3] = ctx->perFragment.mask.alpha;
		break;
	case GL_COLOR_CLEAR_VALUE :
		params[0] = ctx->clear.red;
		params[1] = ctx->clear.green;
		params[2] = ctx->clear.blue;
		params[3] = ctx->clear.alpha;
		break;
	case GL_DEPTH_CLEAR_VALUE :
		params[0] = ctx->clear.depth;
		break;
	case GL_SCISSOR_BOX:
		params[0] = ctx->perFragment.scissor.left;
		params[1] = ctx->perFragment.scissor.bottom;
		params[2] = ctx->perFragment.scissor.width;
		params[3] = ctx->perFragment.scissor.height;
		break;
#if 0
	case GL_BLEND_COLOR :
		params[0] = ctx->blend_data.blnd_clr_red  ;
		params[1] = ctx->blend_data.blnd_clr_green;
		params[2] = ctx->blend_data.blnd_clr_blue;
		params[3] = ctx->blend_data.blnd_clr_alpha;
		break;
	case GL_ALIASED_POINT_SIZE_RANGE:
		//TODO
		break;
	case GL_ALIASED_LINE_WIDTH_RANGE:
		//TODO
		break;
#endif
	case GL_MAX_VIEWPORT_DIMS:
		params[0] = FGL_MAX_VIEWPORT_DIMS;
		params[1] = FGL_MAX_VIEWPORT_DIMS;
		break;
	case GL_COMPRESSED_TEXTURE_FORMATS :
		for (int i = 0; i < NELEM(fglCompressedTextureFormats); ++i)
			params[i] = fglCompressedTextureFormats[i];
		break;

	/* Single integer values */
#if 0
	case GL_MAX_ELEMENTS_INDICES :
	case GL_MAX_ELEMENTS_VERTICES :
	case GL_MAX_RENDERBUFFER_SIZE :
	case GL_ALPHA_TEST_FUNC_EXP:
	case GL_ALPHA_TEST_REF_EXP:
	case GL_LOGIC_OP_MODE_EXP:
	case GL_BLEND_SRC_RGB :
	case GL_BLEND_SRC_ALPHA:
	case GL_BLEND_DST_RGB :
	case GL_BLEND_DST_ALPHA:
	case GL_BLEND_EQUATION_RGB :
	case GL_BLEND_EQUATION_ALPHA:
	case GL_STENCIL_BACK_FUNC :
	case GL_STENCIL_BACK_VALUE_MASK :
	case GL_STENCIL_BACK_REF :
	case GL_STENCIL_BACK_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_FAIL :
	case GL_STENCIL_BACK_PASS_DEPTH_PASS :
	case GL_STENCIL_BACK_WRITEMASK :
#endif
	case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
	case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
	case GL_ARRAY_BUFFER_BINDING:
	case GL_ELEMENT_ARRAY_BUFFER_BINDING:
	case GL_CULL_FACE_MODE:
	case GL_FRONT_FACE:
	case GL_TEXTURE_BINDING_2D:
	case GL_ACTIVE_TEXTURE:
	case GL_STENCIL_CLEAR_VALUE:
	case GL_DEPTH_WRITEMASK:
	case GL_STENCIL_WRITEMASK :
	case GL_STENCIL_FUNC:
	case GL_STENCIL_VALUE_MASK:
	case GL_STENCIL_REF :
	case GL_STENCIL_FAIL :
	case GL_STENCIL_PASS_DEPTH_FAIL :
	case GL_STENCIL_PASS_DEPTH_PASS :
	case GL_DEPTH_FUNC:
	case GL_NUM_COMPRESSED_TEXTURE_FORMATS :
	case GL_UNPACK_ALIGNMENT:
	case GL_PACK_ALIGNMENT:
	case GL_SUBPIXEL_BITS:
	case GL_MAX_TEXTURE_SIZE:
	case GL_MAX_TEXTURE_UNITS:
	case GL_MAX_LIGHTS:
	case GL_SAMPLE_BUFFERS :
	case GL_SAMPLES :
	case GL_RED_BITS :
	case GL_GREEN_BITS:
	case GL_BLUE_BITS :
	case GL_ALPHA_BITS :
	case GL_DEPTH_BITS :
	case GL_STENCIL_BITS:
		GLint intval;
		glGetIntegerv(pname, &intval);
		params[0] = intval;
		break;

	/* Single boolean values */
#if 0
	case GL_ALPHA_TEST_EXP:
#endif
	case GL_CULL_FACE:
	case GL_POLYGON_OFFSET_FILL:
	case GL_SCISSOR_TEST:
	case GL_STENCIL_TEST:
	case GL_DEPTH_TEST:
	case GL_BLEND:
	case GL_DITHER:
	case GL_COLOR_LOGIC_OP:
		params[0] = glIsEnabled(pname);
		break;
	default:
		setError(GL_INVALID_ENUM);
	}
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
		return fimgGetRasterizerState(ctx->fimg, FIMG_CULL_FACE_EN);
		break;
	case GL_POLYGON_OFFSET_FILL:
		return fimgGetRasterizerState(ctx->fimg, FIMG_DEPTH_OFFSET_EN);
		break;
	case GL_SCISSOR_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_SCISSOR_TEST);
		break;
	case GL_STENCIL_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_STENCIL_TEST);
		break;
	case GL_DEPTH_TEST:
		return fimgGetFragmentState(ctx->fimg, FIMG_DEPTH_TEST);
		break;
	case GL_BLEND:
		return fimgGetFragmentState(ctx->fimg, FIMG_BLEND);
		break;
	case GL_DITHER:
		return fimgGetFragmentState(ctx->fimg, FIMG_DITHER);
		break;
#if 0
	case GL_ALPHA_TEST_EXP:
		break;
#endif
	case GL_COLOR_LOGIC_OP:
		return fimgGetFragmentState(ctx->fimg, FIMG_COLOR_LOGIC_OP);
		break;
#if 0
	case GL_POINT_SPRITE_OES_EXP:
		break;
#endif
	default:
		setError(GL_INVALID_ENUM);
		return GL_FALSE;
	}
}
