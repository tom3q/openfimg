SUBDIRS = libfimg

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

AM_CPPFLAGS += \
	-O2 \
	-mcpu=arm1176jzf-s \
	-mfpu=vfp -Wall \
	-Wno-unused-parameter \
	-marm

lib_LTLIBRARIES = \
	libGLES_fimg.la

libGLES_fimg_la_LIBADD = libfimg/libfimg.la

libGLES_fimg_la_SOURCES = \
	eglBase.cpp \
	fglmatrix.cpp \
	fglsurface.cpp \
	glesBase.cpp \
	glesGet.cpp \
	glesMatrix.cpp \
	glesPixel.cpp \
	glesTex.cpp

if PLATFORM_ANDROID
	libGLES_fimg_la_SOURCES += eglAndroid.cpp
endif

if PLATFORM_FRAMEBUFFER
	libGLES_fimg_la_SOURCES += eglFramebuffer.cpp
endif
