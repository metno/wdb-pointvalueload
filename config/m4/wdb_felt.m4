# -*- autoconf -*-
#
#
# SYNOPSIS
#
#   WDB_REQUIRE_FELT(default)
#
# DESCRIPTION
#
#   The WDB_WITH_FELT macro searches for the felt library.
#
#   The WDB_REQUIRE_FELT macro is equivalent, but terminates the
#   configure script if it can not find the Felt libraries.
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
# WDB_WITH_FELT
#
# $1 = default
#
AC_DEFUN([WDB_WITH_FELT], [
    # --with-felt magic
    WDB_WITH_LIBRARY([felt], [felt], [Felt library], [$1])

    # is Felt required, or did the user request it?
    AS_IF([test x"${with_felt}" != x"no" -o x"${require_felt}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${FELT_INCLUDEDIR}" != x""], [
	    FELT_CPPFLAGS="-I${FELT_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${FELT_CPPFLAGS}"

	# library location
	AS_IF([test x"${FELT_LIBDIR}" != x""], [
	    FELT_LDFLAGS="-L${FELT_LIBDIR}"
	])
	LDFLAGS="${LDFLAGS} ${FELT_LDFLAGS}"

	# C version
        AC_REQUIRE([AC_PROG_CXXCPP])
        AC_LANG_PUSH(C++)
	AC_CHECK_HEADER([felt/FeltField.h], [], [
	    AC_MSG_ERROR([the required felt/FeltField.h header was not found])
	])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <felt/FeltConstants.h>],
                                         [using namespace felt;
                                          const double radius = EARTH_RADIUS;
                                         ])],
                          [AC_MSG_NOTICE([Felt found])],
                          [AC_MSG_ERROR([Felt not found --- stopping])])])
	AC_LANG_POP(C++)

	# export our stuff
	FELT_LIBS="${LIBS}"
	AC_SUBST([felt_CPPFLAGS], ["${FELT_CPPFLAGS}"])
	AC_SUBST([felt_LDFLAGS], ["${FELT_LDFLAGS}"])
	AC_SUBST([felt_LIBS], ["-lfelt"])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# WDB_REQUIRE_FELT
#
AC_DEFUN([WDB_REQUIRE_FELT], [
    require_felt=yes
    WDB_WITH_FELT
])
