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
AC_DEFUN([WDB_WITH_FIMEX_VERSION], [
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
	    FIMEX_CPPFLAGS="-I${F_INCLUDEDIR}"
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

	# Check version of Fimex
	required_fimex_version=ifelse([$1], [], [], [$1])
	if test -n "$required_fimex_version"; then
		AC_MSG_CHECKING([if Fimex version is >= $required_fimex_version])

                # Deconstruct version string (required)
	        required_fimex_version_major=`expr $required_fimex_version : '\([[0-9]]*\)'`
	        required_fimex_version_minor=`expr $required_fimex_version : '[[0-9]]*\.\([[0-9]]*\)'`
                required_fimex_version_micro=`expr $required_fimex_version : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
                if test "x$required_fimex_version_micro" = "x"; then
                    required_fimex_version_micro="0"
                fi
        fi

	# C++ version
        AC_REQUIRE([AC_PROG_CXXCPP])
        AC_LANG_PUSH(C++)
        AC_RUN_IFELSE([AC_LANG_PROGRAM([#include <fimex/CDMconstants.h>
                                        #include <fimex/CDM.h>
                                        #include <iostream>
                                        #include <fstream>
                                        #include <string>
                                        #include <vector>
                                        #include <boost/lexical_cast.hpp>
                                        #include <boost/algorithm/string/split.hpp>
                                        #include <boost/algorithm/string/classification.hpp>],
                                         [using namespace std;
                                          using namespace MetNoFimex;
                                          string available = fimexVersion();
                                          //ofstream myfile;
                                          //myfile.open("fimex-test.out");
                                          //myfile << "available fimex version is: " <<available << "\n";

                                          vector<string> availables;
                                          boost::split(availables, available, boost::is_any_of("."));
                                          while(availables.size() < 3)
                                              availables.push_back("0");
                                          //myfile << "availables members: \n";
                                          //myfile << "[0]    " << availables.at(0) << "\n";
                                          //myfile << "[1]    " << availables.at(1) << "\n";
                                          //myfile << "[2]    " << availables.at(2) << "\n";

                                          string required  = "$required_fimex_version";
                                          vector<string> requireds;
                                          boost::split(requireds, required, boost::is_any_of("."));

                                          while(requireds.size() < 3)
                                              requireds.push_back("0");
                                          //myfile << "requireds members: \n";
                                          //myfile << "[0]    " << requireds.at(0) << "\n";
                                          //myfile << "[1]    " << requireds.at(1) << "\n";
                                          //myfile << "[2]    " << requireds.at(2) << "\n";
                                          //myfile.close();

                                          for(int i = 0; i < 3; ++i) {
                                              if(boost::lexical_cast<int>(requireds.at(i)) > boost::lexical_cast<int>(availables.at(i)))
                                                  return 1;
                                          }
                                          return 0;
                                         ])],
                          [AC_MSG_NOTICE([Fimex found])],
                          [AC_MSG_ERROR([Fimex not found --- stopping])])])

	AC_LANG_POP(C++)

	# export our stuff
	FIMEX_LIBS="${LIBS}"
	AC_SUBST([FIMEX_LIBS])
	AC_SUBST([FIMEX_LDFLAGS])
	AC_SUBST([FIMEX_CPPFLAGS])

	# restore state
	LIBS="${saved_LIBS}"
	LDFLAGS="${saved_LDFLAGS}"
	CPPFLAGS="${saved_CPPFLAGS}"
    ])
])

#
# WDB_REQUIRE_FIMEX
#
AC_DEFUN([WDB_REQUIRE_FIMEX_VERSION], [
    require_fimex=yes
    WDB_WITH_FIMEX_VERSION($1)
])
