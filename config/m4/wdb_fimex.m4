# -*- autoconf -*-
#
#
# SYNOPSIS
#
#   WDB_REQUIRE_FIMEX(default)
#
# DESCRIPTION
#
#   The WDB_WITH_FIMEX macro searches for the fimex library.
#
#   The WDB_REQUIRE_FIMEX macro is equivalent, but terminates the
#   configure script if it can not find the Fimex libraries.
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
# WDB_WITH_FIMEX
#
# $1 = default
#
AC_DEFUN([WDB_WITH_FIMEX], [
    # --with-fimex magic
    WDB_WITH_LIBRARY([fimex], [fimex], [Fimex library], [$1])

    # is Fimex required, or did the user request it?
    AS_IF([test x"${with_fimex}" != x"no" -o x"${require_fimex}" = x"yes"], [
	# save state
	saved_CPPFLAGS="${CPPFLAGS}"
	saved_LDFLAGS="${LDFLAGS}"
	saved_LIBS="${LIBS}"
	LIBS=""

	# header location
	AS_IF([test x"${FIMEX_INCLUDEDIR}" != x""], [
	    FIMEX_CPPFLAGS="-I${FIMEX_INCLUDEDIR}"
	])
	CPPFLAGS="${CPPFLAGS} ${FIMEX_CPPFLAGS}"

	# library location
	AS_IF([test x"${FIMEX_LIBDIR}" != x""], [
	    FIMEX_LDFLAGS="-L${FIMEX_LIBDIR}"
	])
	LDFLAGS="${LDFLAGS} ${FIMEX_LDFLAGS}"

	# C version
        AC_REQUIRE([AC_PROG_CPP])
	AC_LANG_PUSH(C)
	AC_CHECK_HEADER([fimex/c_fimex.h], [], [
	    AC_MSG_ERROR([the required fimex.h header was not found])
	])
	AC_CHECK_LIB([fimex], [mifi_new_cdminterpolator], [], [
	    AC_MSG_ERROR([the required fimex library was not found])
	])
	AC_LANG_POP(C)

	# C++ version
        AC_REQUIRE([AC_PROG_CXXCPP])
        AC_LANG_PUSH(C++)
        AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <fimex/CDMconstants.h>
                                         #include <fimex/CDM.h>
                                         #include <fimex/CDMInterpolator.h>
                                         #include <fimex/CDMFileReaderFactory.h>
                                         #include <string>],
                                         [using namespace MetNoFimex;
                                          CDMInterpolator(CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, "")).changeProjection(1, "template.nc");
                                         ])],
                          [AC_MSG_NOTICE([Fimex found])],
                          [AC_MSG_ERROR([Fimex not found --- stopping])])])

	AC_LANG_POP(C++)

	# export our stuff
	FIMEX_LIBS="${LIBS}"
	AC_SUBST([fimex_CPPFLAGS], ["${FIMEX_CPPFLAGS}"])
	AC_SUBST([fimex_LDFLAGS], ["${FIMEX_LDFLAGS}"])
	AC_SUBST([fimex_LIBS], ["-lfimex"])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# WDB_REQUIRE_FIMEX
#
AC_DEFUN([WDB_REQUIRE_FIMEX], [
    require_fimex=yes
    WDB_WITH_FIMEX
])
