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
	host.c \
	host_contig.c \
	host_index8.c \
	host_index16.c \
	host_index32.c \
	primitive.c \
	raster.c \
	system.c \
	texture.c

MAINTAINERCLEANFILES = \
	Makefile.in

libfimg_la_LIBADD = $(DEPS_LIBS)
