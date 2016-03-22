dnl Process this file with autoconf to produce a configure script.
AC_INIT([jamlib], [1.4.7], [raorn@altlinux.ru])
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR([src/mbase.c])
AC_CONFIG_HEADER(config.h)
AC_CANONICAL_SYSTEM
#AC_VALIDATE_CACHED_SYSTEM_TUPLE(
#	rm -f $cache_file
#	echo Please rerun configure
#	exit 1
#)

AM_INIT_AUTOMAKE

case $host_os in
	linux*)
		AC_DEFINE([__UNIX__], 1, [Define if you have POSIX-compilant OS])
		;;
	freebsd*)
		AC_DEFINE([__UNIX__], 1, [Define if you have POSIX-compilant OS])
		;;
	darwin*)
		AC_DEFINE([__UNIX__], 1, [Define if you have POSIX-compilant OS])
		;;
	nto-qnx*)
		AC_DEFINE([__UNIX__], 1, [Define if you have POSIX-compilant OS])
		;;
	mingw32*)
		if test "$host" != "$build"; then
			case "$build_os" in
				cygwin*)
					CFLAGS="$CFLAGS -mno-cygwin -mconsole"
					CPPFLAGS="$CPPFLAGS $CFLAGS"
					;;
				*)
					CC="$host_cpu-$host_os-gcc"
					AS="$CC"
					AR="$host_cpu-$host_os-ar"
					RANLIB="$host_cpu-$host_os-ranlib"
					;;
			esac
		fi
		AC_DEFINE([__WIN32__], 1, [Define if you do not have POSIX-compilant OS ;-)])
		;;
esac

AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h sys/file.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_ARG_ENABLE([lockf], AC_HELP_STRING([--enable-lockf], [use lockf(3) for locking instead of fcntl(2)]),
              AC_DEFINE([USE_LOCKF], 1, [Define if you want to use lockf(3) instead of fcntl(2) for file locking]))

AC_CONFIG_FILES([jamlib.qpg
	include/Makefile
	include/jamlib/Makefile
	src/Makefile
	doc/Makefile
	win32/Makefile
	win32/jamlib/Makefile
	Makefile])
AC_OUTPUT
