noinst_LTLIBRARIES = \
	libfimg.la

AM_CFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	-I$(top_builddir)/include

libfimg_la_SOURCES = \
	compat.c \
	dump.c \
	fragment.c \
	global.c \
	primitive.c \
	raster.c \
	shaders.c \
	system.c \
	texture.c

if USE_NEWHOST
libfimg_la_SOURCES += host_new.c
else
libfimg_la_SOURCES += host.c
endif

MAINTAINERCLEANFILES = \
	Makefile.in
