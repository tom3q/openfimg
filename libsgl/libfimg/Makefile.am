noinst_LTLIBRARIES = \
	libfimg.la

AM_CFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

AM_CFLAGS += \
	-Wall \
	-Wno-unused-parameter \
	-O2 \
	-mcpu=arm1176jzf-s \
	-mfloat-abi=softfp \
	-mfpu=vfp \
	-marm

libfimg_la_SOURCES = \
	compat.c \
	dump.c \
	fragment.c \
	global.c \
	host.c \
	primitive.c \
	raster.c \
	shaders.c \
	system.c \
	texture.c
