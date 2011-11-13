AC_PREREQ([2.59])
AC_INIT([openfimg], [git], [tomasz.figa@gmail.com])
AC_CONFIG_SRCDIR([libsgl/eglBase.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

#
# libtool library versioning stuff
#
# Library code modified:            REVISION++
# Interfaces changed/added/removed: CURRENT++   REVISION=0
# Interfaces added:                 AGE++
# Interfaces removed:               AGE=0
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
LT_INIT

#
# Program checks
#
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

#
# Init automake
#
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Configuration options
AC_MSG_CHECKING([whether to enable Android EGL backend])
AC_ARG_ENABLE(android,
	AS_HELP_STRING([--enable-android], [enable Android EGL backend @<:@default=no@:>@]),
		[case "$enableval" in
		y | yes) ENABLE_ANDROID=yes ;;
		*) ENABLE_ANDROID=no ;;
	esac],
	[ENABLE_ANDROID=no])
AC_MSG_RESULT([${ENABLE_ANDROID}])
AM_CONDITIONAL([PLATFORM_ANDROID], [test x${ENABLE_ANDROID} = xyes])
if test "${ENABLE_ANDROID}" = "yes"; then
	AC_DEFINE(FGL_PLATFORM_ANDROID, 1, [use Android EGL backend])
fi

AC_MSG_CHECKING([whether to enable framebuffer EGL backend])
AC_ARG_ENABLE(framebuffer,
	AS_HELP_STRING([--enable-framebuffer], [enable framebuffer EGL backend @<:@default=no@:>@]),
		[case "$enableval" in
		y | yes) ENABLE_FRAMEBUFFER=yes ;;
		*) ENABLE_FRAMEBUFFER=no ;;
	esac],
	[ENABLE_FRAMEBUFFER=no])
AC_MSG_RESULT([${ENABLE_FRAMEBUFFER}])
AM_CONDITIONAL([PLATFORM_FRAMEBUFFER], [test x${ENABLE_FRAMEBUFFER} = xyes])
if test "${ENABLE_FRAMEBUFFER}" = "yes"; then
	AC_DEFINE(FGL_PLATFORM_FRAMEBUFFER, 1, [use Android EGL backend])
fi

#
# Static definitions
#
AC_DEFINE(GL_GLEXT_PROTOTYPES, 1, [FIXME])

#
# Go!
#
AC_CONFIG_FILES([Makefile libsgl/Makefile libsgl/libfimg/Makefile])
AC_OUTPUT

AC_MSG_NOTICE([------------------------------------------------------])
AC_MSG_NOTICE([Current settings:])
AC_MSG_NOTICE([cross compiling: $cross_compiling])
AC_MSG_NOTICE([Install prefix:  $prefix])
AC_MSG_NOTICE([sysconfdir:      $sysconfdir])
AC_MSG_NOTICE([Compiler:        $CXX])
AC_MSG_NOTICE([CFLAGS:          $CFLAGS])
AC_MSG_NOTICE([Additional libs: $LIBS])
AC_MSG_NOTICE([------------------------------------------------------])
