SUBDIRS = libfimg

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

lib_LIBRARIES = \
	libGLES_fimg.a

libGLES_fimg_a_LIBADD = libfimg/libfimg.a

libGLES_fimg_a_SOURCES = \
	eglBase.cpp \
	fglmatrix.cpp \
	fglsurface.cpp \
	glesBase.cpp \
	glesGet.cpp \
	glesMatrix.cpp \
	glesPixel.cpp \
	glesTex.cpp

if PLATFORM_ANDROID
	libGLES_fimg_a_SOURCES += eglAndroid.cpp
endif

if PLATFORM_FRAMEBUFFER
	libGLES_fimg_a_SOURCES += eglFramebuffer.cpp
endif
