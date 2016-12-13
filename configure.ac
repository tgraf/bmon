#
# configure.in      Configure Script
#
# Copyright (c) 2001-2016 Thomas Graf <tgraf@suug.ch>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

AC_INIT(bmon, 4.0, [], [], [https://github.com/tgraf/bmon])
AC_CONFIG_HEADERS(include/bmon/defs.h)
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)], [])

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_INSTALL

AC_C_CONST
AC_C_INLINE

AC_HEADER_TIME
AC_HEADER_DIRENT

AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_TYPE_PID_T

AC_FUNC_FORK

AC_CHECK_HEADERS(getopt.h ncurses/ncurses.h ncurses.h curses.h)
AC_CHECK_HEADERS(dirent.h sys/utsname.h sys/sockio.h netinet6/in6.h)
AC_CHECK_HEADERS(fcntl.h netdb.h netinet/in.h sysctl/ioctl.h)
AC_CHECK_HEADERS(sys/param.h sys/socket.h)

AC_CHECK_TYPES(suseconds_t)

AC_CHECK_FUNCS(atexit clock_gettime memset pow socket strcasecmp)
AC_CHECK_FUNCS(strchr strdup strerror strncasecmp strstr strtol)
AC_CHECK_FUNCS(uname getdate)

AC_PATH_PROG([PKG_CONFIG], [pkg-config], [no])
AS_IF([test "x$PKG_CONFIG" = "xno"],[
   AC_MSG_ERROR([
      *** The pkg-config script could not be found. Make sure it is
      *** in your path, or set the PKG_CONFIG environment variable
      *** to the full path to pkg-config.])
   ])

AX_WITH_CURSES
if test "x$ax_cv_curses" != xyes || test "x$ax_cv_curses_color" != xyes; then
        AC_MSG_ERROR([requires an X/Open-compatible Curses library with color])
fi

PKG_CHECK_MODULES([CONFUSE], [libconfuse], [], AC_MSG_ERROR([requires libconfuse]))

case ${target_os} in
    linux*|uclinux*)
        PKG_CHECK_MODULES([LIBNL], [libnl-3.0], [], AC_MSG_ERROR([requires libnl3-dev]))
        PKG_CHECK_MODULES([LIBNL_ROUTE], [libnl-route-3.0], [], AC_MSG_ERROR([requires libnl3-route]))
    ;;
esac

AC_CHECK_LIB(m, pow, [], AC_MSG_ERROR([requires libm]))

# Don't fail if not found (for instance, OS X does not have clock_gettime)
AC_CHECK_LIB(rt, clock_gettime, [], [])

BMON_LIB=""

#####################################################################
##
## libcurses check
##
#####################################################################
CURSES="No "
AC_CHECK_LIB(ncurses, initscr, [
  AC_DEFINE_UNQUOTED(HAVE_NCURSES, "1", [have ncurses])
  LCURSES="ncurses"
  CURSES="Yes"
],[
	AC_CHECK_LIB(curses,initscr, [
		AC_DEFINE_UNQUOTED(HAVE_CURSES, "1", [have curses])
		LCURSES="curses"
		  CURSES="Yes"
	],[
		echo
		echo "*** Warning: Building bmon w/o curses support ***"
		echo
	])
])

LIBCURSES="-l$LCURSES"

AC_CHECK_LIB($LCURSES, redrawwin, [
	AC_DEFINE_UNQUOTED(HAVE_REDRAWWIN, "1", [have redrawwin])
])

AC_CHECK_LIB($LCURSES, use_default_colors, [
	  AC_DEFINE_UNQUOTED(HAVE_USE_DEFAULT_COLORS, "1", [have udc])
])

#####################################################################
##
## interface counter overflow workaround
##
#####################################################################
AC_ARG_ENABLE(cnt-workaround,
[  --disable-cnt-workaround Disables interface counter overflow workaround],[
	if test x$enableval = xno; then
		AC_DEFINE_UNQUOTED(DISABLE_OVERFLOW_WORKAROUND,"1",[no overflow workaround])
	fi
])

#####################################################################
##
## curses
##
#####################################################################
AC_ARG_ENABLE(curses,
[  --disable-curses        Disables curses output],[
	if test x$enableval = xno; then
		CURSES="No "
	fi
])

#####################################################################
##
## debug check
##
#####################################################################
DEBUG=0
AC_ARG_ENABLE(debug,
[  --enable-debug          Enable debug mode (default disabled)],[
	if test x$enableval = xyes; then
		AC_DEFINE_UNQUOTED(DEBUG, "1", [enable debugging])
		DEBUG=1;
	fi
])

#####################################################################
##
## target os eval
##
#####################################################################
case ${target_os} in
    linux*)
	AC_DEFINE_UNQUOTED(SYS_LINUX, "1", [operating system])
    ;;

    *solaris*)
	AC_DEFINE_UNQUOTED(SYS_SUNOS, "1", [operating system])
    ;;

    *bsd*)
	AC_DEFINE_UNQUOTED(SYS_BSD, "1", [operating system])
    ;;

    *darwin*)
	AC_DEFINE_UNQUOTED(SYS_BSD, "1", [operating system])
    ;;

    *)
	AC_DEFINE_UNQUOTED(SYS_OTHER, "1", [operating system])
    ;;
esac

#####################################################################
##
## export variables
##
#####################################################################
AC_SUBST(DEBUG)
AC_SUBST(STATIC)
AC_SUBST(SYS)
AC_SUBST(ARCH)

AC_SUBST(CURSES)

AC_CONFIG_FILES([
Makefile
include/Makefile
src/Makefile
man/Makefile
examples/Makefile
])

AC_OUTPUT
