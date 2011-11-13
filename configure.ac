AC_PREREQ([2.59])
AC_INIT([openfimg], [git], [tomasz.figa@gmail.com])
AC_CONFIG_SRCDIR([libsgl/eglBase.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile libsgl/Makefile libsgl/libfimg/Makefile])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

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

AC_DEFINE(GL_GLEXT_PROTOTYPES, 1, [FIXME])

AC_OUTPUT
