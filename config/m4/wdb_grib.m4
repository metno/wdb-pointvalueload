# -*- autoconf -*-
#
#
# SYNOPSIS
#
#   WDB_REQUIRE_GRIB_API(default)
#
# DESCRIPTION
#
#   The WDB_WITH_GRIB_API macro searches for the grib_api library.
#
#   The WDB_REQUIRE_GRIB_API macro is equivalent, but terminates the
#   configure script if it can not find the grib_api libraries.
#
# TODO
#
#   The WDB_REQUIRE_C_LIBRARY macro should be improved to the point
#   where this macro can simply use it instead of duplicating
#   significant parts of it. Try m4_foreach_w(COMPONENT, [$1])
#
# AUTHORS
#
#   Aleksandar Babic aleksandarb@met.no
#

#
# WDB_WITH_GRIB_API
#
# $1 = default
#
AC_DEFUN([WDB_WITH_GRIB_API], [
    # --with-grib_api magic
    WDB_WITH_LIBRARY([grib_api], [grib_api], [grib_api library], [$1])

    # is grib_api required, or did the user request it?
    AS_IF([test x"${with_grib_api}" != x"no" -o x"${require_grib_api}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${GRIB_API_INCLUDEDIR}" != x""], [
	    GRIB_API_CPPFLAGS="-I${GRIB_API_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${GRIB_API_CPPFLAGS}"

	# library location
	AS_IF([test x"${GRIB_API_LIBDIR}" != x""], [
	    GRIB_API_LDFLAGS="-L${GRIB_API_LIBDIR}"
	])
	LDFLAGS="${LDFLAGS} ${GRIB_API_LDFLAGS}"

	# C version
        AC_REQUIRE([AC_PROG_CPP])
	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([grib_api.h], [], [
	    AC_MSG_ERROR([the required grib_api.h header was not found])
	])
	AC_CHECK_LIB([grib_api], [main], [], [
	    AC_MSG_ERROR([the required grib_api library was not found])
	])
	AC_LANG_POP(C)

	# export our stuff
	GRIB_API_LIBS="${LIBS}"
	AC_SUBST([grib_api_CPPFLAGS], ["${GRIB_API_CPPFLAGS}"])
	AC_SUBST([grib_api_LDFLAGS], ["${GRIB_API_LDFLAGS}"])
	AC_SUBST([grib_api_LIBS], ["-lgrib_api"])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# WDB_REQUIRE_GRIB_API
#
AC_DEFUN([WDB_REQUIRE_GRIB_API], [
    require_grib_api=yes
    WDB_WITH_GRIB_API
])
