SUBDIRS = libfimg

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

lib_LTLIBRARIES = \
	libGLES_fimg.la

libGLES_fimg_la_LIBADD = libfimg/libfimg.la

libGLES_fimg_la_SOURCES = \
	eglBase.cpp \
	fglmatrix.cpp \
	fglsurface.cpp \
	fglframebuffer.cpp \
	glesBase.cpp \
	glesFramebuffer.cpp \
	glesGet.cpp \
	glesMatrix.cpp \
	glesPixel.cpp \
	glesTex.cpp

MAINTAINERCLEANFILES = \
	Makefile.in

if PLATFORM_ANDROID
libGLES_fimg_la_SOURCES += eglAndroid.cpp
endif

if PLATFORM_FRAMEBUFFER
libGLES_fimg_la_SOURCES += eglFramebuffer.cpp
endif
