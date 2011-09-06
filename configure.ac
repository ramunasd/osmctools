AC_INIT([osmctools], [1.0], [markus.weber@gmx.com])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC

AC_CHECK_LIB(z, gzopen, [enable_osmconvert=yes], [enable_osmconvert=no])
if test "x$enable_osmconvert" = "xno"; then
	AC_MSG_WARN([zlib not found, osmconvert will not be compiled])
fi
AM_CONDITIONAL(ENABLE_OSMCONVERT, test "x$enable_osmconvert" = xyes)

AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
