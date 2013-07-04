AC_PREREQ([2.68])
AC_INIT([openfimg],[git],[tomasz.figa@gmail.com])
AC_CONFIG_SRCDIR([libsgl/eglBase.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# this program makes only sense on an ARM CPU!
AC_MSG_CHECKING([host CPU])
case "${host_cpu}" in
	*arm*)
		AC_MSG_RESULT([ARM])
		;;
	*)
		AC_MSG_ERROR([Only ARM CPUs are supported])
		;;
esac

CFLAGS="${CFLAGS} -W -Wall -Wsign-compare -Wfloat-equal -Wformat-security -Wno-unused-parameter"

PKG_CHECK_MODULES([DEPS], [libdrm])

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
LT_INIT

#
# Function checks
#
AC_CHECK_FUNCS(__sync_synchronize)

# default is less verbosity while building the package
AM_SILENT_RULES([yes])

#
# Init automake
#
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2])

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
AM_CONDITIONAL([PLATFORM_ANDROID], [test "${ENABLE_ANDROID}" = "yes"])
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
AM_CONDITIONAL([PLATFORM_FRAMEBUFFER], [test "${ENABLE_FRAMEBUFFER}" = "yes"])
if test "${ENABLE_FRAMEBUFFER}" = "yes"; then
	AC_DEFINE(FGL_PLATFORM_FRAMEBUFFER, 1, [use Linux framebuffer backend])
fi

if test "${ENABLE_FRAMEBUFFER}" = "no" -a "${ENABLE_ANDROID}" = "no"; then
	AC_MSG_ERROR([At least one of the backends (Android or framebuffer) must be enabled])
fi

#
# Debugging
#
AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
		[use debug compiler flags and macros @<:@default=disabled@:>@])],
	[enable_debug="$enableval"],
	[enable_debug=no])
AC_MSG_RESULT([${enable_debug}])

AC_MSG_CHECKING([whether to enable printing GL errors])
AC_ARG_ENABLE(glerror,
	AS_HELP_STRING([--enable-glerror], [enable printing GL errors @<:@default=no@:>@]),
		[case "$enableval" in
		y | yes) ENABLE_GLERROR=yes ;;
		*) ENABLE_GLERROR=no ;;
	esac],
	[ENABLE_GLERROR=no])
AC_MSG_RESULT([${ENABLE_GLERROR}])
if test "${ENABLE_GLERROR}" = "yes"; then
	AC_DEFINE(GLES_ERR_DEBUG, 1, [print GL errors])
fi

#
# Static definitions
#
AC_DEFINE(GL_GLEXT_PROTOTYPES, 1, [Include prototypes of GL extensions])
AC_DEFINE(EGL_EGLEXT_PROTOTYPES, 1, [Include prototypes of EGL extensions])

if test "${enable_debug}" = "yes"; then
	CFLAGS="${CFLAGS} -g"
	AC_DEFINE(DEBUG, 1, [debugging])
else
	CFLAGS="${CFLAGS} -O2"
	AC_DEFINE(NDEBUG, 1, [no debugging])
fi

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
