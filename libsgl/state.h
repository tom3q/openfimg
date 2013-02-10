/*
 * libsgl/state.h
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

#ifndef _LIBSGL_STATE_H_
#define _LIBSGL_STATE_H_

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include "common.h"
#include "types.h"
#include "libfimg/fimg.h"

#include "fglsurface.h"
#include "fglstack.h"
#include "fglmatrix.h"
#include "fgltextureobject.h"
#include "fglbufferobject.h"
#include "fglobject.h"
#include "fglframebuffer.h"
#include "fglrenderbuffer.h"

/** Components of vertex normal. */
enum FGLNormalComponent {
	FGL_COMP_NX = 0,
	FGL_COMP_NY,
	FGL_COMP_NZ
};

/** Components of vertex texture coordinate. */
enum FGLTexCoordComponent {
	FGL_COMP_S = 0,
	FGL_COMP_T,
	FGL_COMP_R,
	FGL_COMP_Q
};

/** Vertex arrays supported by OpenFIMG. */
enum FGLArrayIndex {
	FGL_ARRAY_VERTEX = 0,
	FGL_ARRAY_NORMAL,
	FGL_ARRAY_COLOR,
	FGL_ARRAY_POINT_SIZE,
	FGL_ARRAY_TEXTURE
};
/**
 * Calculates index of vertex texture coordinate array of given texture unit.
 * @param i Index of texture unit.
 * @return Index of array.
 */
#define FGL_ARRAY_TEXTURE(i)	(FGL_ARRAY_TEXTURE + (i))

/** Structure holding state of single vertex array. */
struct FGLArrayState {
	/** Indicates if the array is enabled. */
	GLboolean enabled;
	/** Pointer to array data. */
	const GLvoid *pointer;
	/** Stride of vertex attribute. */
	GLint stride;
	/** Width of vertex attribute. */
	GLint width;
	/** Data type of vertex attribute. */
	GLint type;
	/** Number of components. */
	GLint size;
	/** Buffer object used for this array. */
	FGLBuffer *buffer;

	/**
	 * Vertex array state constructor.
	 * Initializes array state to default values.
	 */
	FGLArrayState() :
		enabled(GL_FALSE),
		pointer(NULL),
		stride(0),
		width(4),
		type(FGHI_ATTRIB_DT_FLOAT),
		size(FGHI_NUMCOMP(4)),
		buffer(0) {};
};

/** Structure holding parameters of viewport transformation. */
struct FGLViewportState {
	GLclampf zNear;
	GLclampf zFar;
	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;
};

/** Matrices supported by OpenFIMG. */
enum FGLMatrixIndex {
	FGL_MATRIX_PROJECTION = 0,
	FGL_MATRIX_MODELVIEW,
	FGL_MATRIX_MODELVIEW_INVERSE,
	FGL_MATRIX_TEXTURE
};
/**
 * Calculates index of vertex texture coordinate matrix of given texture unit.
 * @param i Index of texture unit.
 * @return Index of matrix.
 */
#define FGL_MATRIX_TEXTURE(__mtx)	(FGL_MATRIX_TEXTURE + (__mtx))

/** Structure holding state of transformation matrices. */
struct FGLMatrixState {
	/** Stacks of supported matrices. */
	FGLstack<FGLmatrix> stack[3 + FGL_MAX_TEXTURE_UNITS];
	/**
	 * Flags indicating if matrices have been modified since last
	 * rendering.
	 */
	GLboolean dirty[3 + FGL_MAX_TEXTURE_UNITS];
	/** Resulting model-view-projection matrix for libfimg. */
	FGLmatrix transformMatrix;
	/** Matrix selected for GL matrix operations. */
	GLint activeMatrix;

	/** Stack sizes of particular matrices. */
	static unsigned int stackSizes[3 + FGL_MAX_TEXTURE_UNITS];

	/** Constructor initializing matrix state to default values. */
	FGLMatrixState() : activeMatrix(0)
	{
		for(int i = 0; i < 3 + FGL_MAX_TEXTURE_UNITS; i++) {
			stack[i].create(stackSizes[i]);
			stack[i].top().identity();
			dirty[i] = GL_TRUE;
		}
	}

	/** Destructor freeing memory used by matrix stacks. */
	~FGLMatrixState()
	{
		for(int i = 0; i < 3 + FGL_MAX_TEXTURE_UNITS; i++)
			stack[i].destroy();
	}
};

/** Context is current. */
#define FGL_IS_CURRENT		0x00010000
/** Context has never been current. */
#define FGL_NEVER_CURRENT	0x00020000
/** Context has lost its state. */
#define FGL_NEEDS_RESTORE	0x00100000
/** Context is marked for termination. */
#define FGL_TERMINATE		0x80000000

/** Structure holding EGL-specific state of rendering context. */
struct FGLEGLState {
	/** EGL specific flags. */
	EGLint flags;
	/** EGL display owning the context. */
	EGLDisplay dpy;
	/** EGL configuration used for the context. */
	EGLConfig config;
	/** EGL surface backing color buffer. */
	EGLSurface draw;
	/** EGL surface backing depth/stencil buffer. */
	EGLSurface depth;

	/** Constructor initializing EGL state with default values. */
	FGLEGLState() :
		flags(0),
		dpy(0),
		config(0),
		draw(0),
		depth(0) {};
};

/** Structure holding state of single texture unit. */
struct FGLTextureState {
	/** Default texture when no texture object is bound to the unit. */
	FGLTexture defTexture;
	/** Binding where texture objects are bound. */
	FGLTextureObjectBinding binding;
	/** Texture function for libfimg. */
	fimgTexFunc fglFunc;
	/** Flag indicating that texture unit is enabled. */
	bool enabled;

	/** Constructor initializing texture unit state with default values. */
	FGLTextureState() :
		defTexture(),
		binding(this),
		fglFunc(FGFP_TEXFUNC_MODULATE),
		enabled(false) {};

	/**
	 * Helper function returning texture connected to the unit.
	 * @return Texture currently connected with the unit.
	 */
	inline FGLTexture *getTexture(void)
	{
		FGLTexture *tex = binding.get();
		if (!tex)
			tex = &defTexture;
		return tex;
	}
};

/** Structure holding parameters of scissor test. */
struct FGLScissorState {
	/** Left-most coordinate of allowed region. */
	GLint left;
	/** Bottom-most coordinate of allowed region. */
	GLint bottom;
	/** Width of allowed region. */
	GLint width;
	/** Height of allowed region. */
	GLint height;

	/** Constructor initializing scissor parameters with default values. */
	FGLScissorState() :
		left(0),
		bottom(0),
		width(2048),
		height(2048) {};
};

/** Structure holding parameters of framebuffer masking. */
struct FGLMaskState {
	/** Indicates that alpha component should be written to framebuffer. */
	GLboolean alpha;
	/** Indicates that red component should be written to framebuffer. */
	GLboolean red;
	/** Indicates that green component should be written to framebuffer. */
	GLboolean green;
	/** Indicates that blue component should be written to framebuffer. */
	GLboolean blue;
	/** Indicates that depth value should be written to framebuffer. */
	GLboolean depth;
	/** Indicates that stencil value should be written to framebuffer. */
	GLint stencil;

	/** Constructor initializing masking state with default values. */
	FGLMaskState() :
		alpha(1),
		red(1),
		green(1),
		blue(1),
		depth(1),
		stencil(0xff) {};
};

/** Structure holding parameters of stencil test. */
struct FGLStencilState {
	/** Stencil test mask. */
	GLuint mask;
	/** Stencil test reference value. */
	GLint ref;
	/** Stencil test function. */
	GLenum func;
	/** Stencil test fail action. */
	GLenum fail;
	/** Stencil test pass, depth test fail action. */
	GLenum passDepthFail;
	/** Stenicl test pass, depth test pass action. */
	GLenum passDepthPass;

	/** Constructor initializing stencil test state with default values. */
	FGLStencilState() :
		mask(0xffffffff),
		ref(0),
		func(GL_ALWAYS),
		fail(GL_KEEP),
		passDepthFail(GL_KEEP),
		passDepthPass(GL_KEEP) {};
};

/** Structure holding state of per-fragment operations. */
struct FGLPerFragmentState {
	/** State of scissor test. */
	FGLScissorState scissor;
	/** State of stencil test. */
	FGLStencilState stencil;
	/** State of framebuffer masking. */
	FGLMaskState mask;
	/** Depth test function. */
	GLenum depthFunc;
	/** Source blend factor. */
	GLenum blendSrc;
	/** Destination blend factor. */
	GLenum blendDst;
	/** Logic operation. */
	GLenum logicOp;
	/** Indicates that framebuffer masking is enabled. */
	bool masked;
	/** Source blend factor for libfimg. */
	fimgBlendFunction fglBlendSrc;
	/** Destination blend factor for libfimg. */
	fimgBlendFunction fglBlendDst;

	/** Constructor initializing per-fragment state with default values. */
	FGLPerFragmentState() :
		blendSrc(GL_ONE),
		blendDst(GL_ZERO),
		logicOp(GL_COPY),
		masked(false),
		fglBlendSrc(FGPF_BLEND_FUNC_ONE),
		fglBlendDst(FGPF_BLEND_FUNC_ZERO) {};
};

/** Structure holding parameters of buffer clear operation. */
struct FGLClearState {
	/** Red clear value. */
	GLclampf red;
	/** Green clear value. */
	GLclampf green;
	/** Blue clear value. */
	GLclampf blue;
	/** Alpha clear value. */
	GLclampf alpha;
	/** Depth clear value. */
	GLclampf depth;
	/** Stencil clear value. */
	GLint stencil;

	/** Constructor initializing clear parameters with default values. */
	FGLClearState() :
		red(0), green(0), blue(0), alpha(0), depth(1.0), stencil(0) {};
};

/** Structure holding rasterization parameters. */
struct FGLRasterizerState {
	/** Line width for line rendering. */
	float lineWidth;
	/** Point size for point rendering. */
	float pointSize;
	/** Polygon face to cull. */
	GLenum cullFace;
	/** Which face of polygon is front. */
	GLenum frontFace;
	/** Shading model. */
	GLenum shadeModel;
	/** Polygon offset factor. */
	GLfloat polyOffFactor;
	/** Polygon offset units. */
	GLfloat polyOffUnits;

	/** Constructor initializing rasterizer state with default values. */
	FGLRasterizerState() :
		lineWidth(1.0f),
		pointSize(1.0f),
		cullFace(GL_BACK),
		frontFace(GL_CCW),
		shadeModel(GL_SMOOTH),
		polyOffFactor(0.0f),
		polyOffUnits(0.0f) {};
};

/** Structure holding information which capabilities are enabled. */
struct FGLEnableState {
	/** Indicates that face culling is enabled. */
	unsigned cullFace	:1;
	/** Indicates that polygon offset is enabled. */
	unsigned polyOffFill	:1;
	/** Indicates that scissor test is enabled. */
	unsigned scissorTest	:1;
	/** Indicates that stencil test is enabled. */
	unsigned stencilTest	:1;
	/** Indicates that depth test is enabled. */
	unsigned depthTest	:1;
	/** Indicates that blending is enabled. */
	unsigned blend		:1;
	/** Indicates that dithering is enabled. */
	unsigned dither		:1;
	/** Indicates that color logic operation is enabled. */
	unsigned colorLogicOp	:1;
	/** Indicates that alpha test is enabled. */
	unsigned alphaTest	:1;

	/** Constructor setting default capability enable state. */
	FGLEnableState() :
		cullFace(0),
		polyOffFill(0),
		scissorTest(0),
		stencilTest(0),
		depthTest(0),
		blend(0),
		dither(1),
		colorLogicOp(0) {};
};

/** Structure holding framebuffer state. */
struct FGLFramebufferState {
	/** Default framebuffer when no framebuffer object is bound. */
	FGLDefaultFramebuffer defFramebuffer;
	/** Binding where framebuffer object can be bound. */
	FGLFramebufferObjectBinding binding;
	/** Currently used framebuffer. */
	FGLAbstractFramebuffer *current;
	/** Current framebuffer width. */
	uint32_t curWidth;
	/** Current framebuffer height. */
	uint32_t curHeight;
	/** Current framebuffer color format. */
	uint32_t curColorFormat;
	/** Current framebuffer Y-axis flip setting. */
	int curFlipY;

	/** Constructor initializing framebuffer state with default values. */
	FGLFramebufferState() :
		defFramebuffer(),
		binding(this),
		current(0),
		curWidth(0),
		curHeight(0),
		curColorFormat(0),
		curFlipY(-1) {};

	/**
	 * Helper function returning currently active framebuffer.
	 * @return Currently active framebuffer.
	 */
	inline FGLAbstractFramebuffer *get(void)
	{
		FGLAbstractFramebuffer *fb = binding.get();
		if (!fb)
			fb = &defFramebuffer;
		return fb;
	}
};

/** Structure storing complete state of rendering context. */
struct FGLContext {
	/** libfimg hardware context. */
	fimgContext *fimg;
	/** Vertex attribute constant values. */
	FGLvec4f vertex[4 + FGL_MAX_TEXTURE_UNITS];
	/** Vertex attribute arrays. */
	FGLArrayState array[4 + FGL_MAX_TEXTURE_UNITS];
	/** Active texture for GL texture operations. */
	GLint activeTexture;
	/** Active texture for texture coordinate array specification. */
	GLint clientActiveTexture;
	/** Transformation matrix state. */
	FGLMatrixState matrix;
	/** Texture states. */
	FGLTextureState texture[FGL_MAX_TEXTURE_UNITS];
	/** External texture states. */
	FGLTextureState textureExternal[FGL_MAX_TEXTURE_UNITS];
	/** Pixel unpack alignment (for pixel data upload). */
	GLuint unpackAlignment;
	/** Pixel pack alignment (for pixel data read). */
	GLuint packAlignment;
	/** Buffer object to use for vertex array. */
	FGLBufferObjectBinding arrayBuffer;
	/** Buffer object to use as source of vertex indices. */
	FGLBufferObjectBinding elementArrayBuffer;
	/** Viewport state. */
	FGLViewportState viewport;
	/** Rasterizer state. */
	FGLRasterizerState rasterizer;
	/** Per-fragment state. */
	FGLPerFragmentState perFragment;
	/** Framebuffer clear state. */
	FGLClearState clear;
	/** Textures that might be used by GPU at the moment. */
	FGLTexture *busyTexture[FGL_MAX_TEXTURE_UNITS];
	/** State of capability enable flags. */
	FGLEnableState enable;
	/** Framebuffer state. */
	FGLFramebufferState framebuffer;
	/** Binding where renderbuffer object can be bound. */
	FGLRenderbufferBinding renderbuffer;
	/** EGL-specific context state. */
	FGLEGLState egl;
	/** Indicates that the context does not have any pending operation. */
	bool finished;

	/** Default values for vertex attribute constants. */
	static FGLvec4f defaultVertex[4 + FGL_MAX_TEXTURE_UNITS];

	/**
	 * Constructor initializing context with default state values.
	 * @param fctx Hardware context from libfimg.
	 */
	FGLContext(fimgContext *fctx) :
		fimg(fctx),
		activeTexture(0),
		clientActiveTexture(0),
		unpackAlignment(4),
		packAlignment(4),
		finished(true)
	{
		memcpy(vertex, defaultVertex, (4 + FGL_MAX_TEXTURE_UNITS) * sizeof(FGLvec4f));
		for (int i = 0; i < FGL_MAX_TEXTURE_UNITS; ++i) {
			busyTexture[i] = 0;
			texture[i].defTexture.target = GL_TEXTURE_2D;
			textureExternal[i].defTexture.target = GL_TEXTURE_EXTERNAL_OES;
		}
	}
};

#endif // _LIBSGL_STATE_H_
