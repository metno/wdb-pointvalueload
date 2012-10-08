#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Pre-Conf
#-----------------------------------------------------------------------------

AC_INIT(wdb-pointLoad, 1.1.6, [wdb@met.no])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([config/m4])
AC_PROG_RANLIB

#-----------------------------------------------------------------------------
# Source & Build Directories
#-----------------------------------------------------------------------------



#-----------------------------------------------------------------------------
# Programs
#-----------------------------------------------------------------------------

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S


#-----------------------------------------------------------------------------
# Checks
#-----------------------------------------------------------------------------

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h stdlib.h sys/time.h unistd.h wordexp.h ctype.h])
AC_CHECK_HEADERS([string vector iterator iosfwd iostream sstream fstream])
AC_CHECK_HEADERS([cmath algorithm memory new stdexcept cstdlib exception])

# string exception sstream 

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MEMCMP

# Check for libraries.
AC_CXX_REQUIRE_STL

#-----------------------------------------------------------------------------
# Module Checks
#-----------------------------------------------------------------------------

# Fimex
WDB_REQUIRE_FIMEX
WDB_REQUIRE_FIMEX_VERSION(0.37.0)
AC_SUBST(fimex_LIBS)
AC_SUBST(fimex_LDFLAGS)
AC_SUBST(fimex_CPPFLAGS)

WDB_REQUIRE_GRIB_API
AC_SUBST(grib_api_LIBS)
AC_SUBST(grib_api_LDFLAGS)
AC_SUBST(grib_api_CPPFLAGS)


WDB_GEOS
AC_SUBST(geos_LIBS)
AC_SUBST(geos_LDFLAGS)
AC_SUBST(geos_CFLAGS)

WDB_NETCDF
AC_SUBST(netcdf_LIBS)
AC_SUBST(netcdf_LDFLAGS)
AC_SUBST(netcdf_CFLAGS)

WDB_REQUIRE_FELT
AC_SUBST(felt_LIBS)
AC_SUBST(felt_LDFLAGS)
AC_SUBST(felt_CPPFLAGS)

# PROJ.4
WDB_PROJ_CHECK

# WDBLOAD
WDB_LIBWDBLOAD_CHECK
WDB_LOG4CPP_CHECK
WDB_CHECK

AM_PATH_XML2
AX_BOOST_BASE(1.40.0)
AX_BOOST_PROGRAM_OPTIONS(1.40.0)
AX_BOOST_DATE_TIME(1.40.0)
AX_BOOST_THREAD(1.40.0)
AX_BOOST_REGEX(1.40.0)
AX_BOOST_UNIT_TEST_FRAMEWORK(1.40.0)

#-----------------------------------------------------------------------------
# documentation
#-----------------------------------------------------------------------------

WDB_DOCBOOK_CHECK

#-----------------------------------------------------------------------------
# Automake
#-----------------------------------------------------------------------------

AM_INIT_AUTOMAKE(-Wall -Werror subdir-objects)
AM_PROG_CC_C_O


#-----------------------------------------------------------------------------
# Configure and Exit
#-----------------------------------------------------------------------------

# Configuration Files
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Print Out Summary of Configuration
AC_MSG_RESULT([ ])
AC_MSG_RESULT([$PACKAGE_NAME $PACKAGE_VERSION])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([-------------------------------------------------------------------------])
AC_MSG_RESULT([ ])
AC_MSG_RESULT([ Database Name:	  $DATABASE_NAME])
AC_MSG_RESULT([ Source Code:      $srcdir])
AC_MSG_RESULT([ Host System:      $host])
AC_MSG_RESULT([  ])
AC_MSG_RESULT([ Prefix:           $prefix])
AC_MSG_RESULT([ Binaries:         $exec_prefix])
AC_MSG_RESULT([ Manuals:          $mandir])
AC_MSG_RESULT([ Data:             $datadir])
AC_MSG_RESULT([ System Config:    $sysconfdir])
AC_MSG_RESULT([ ])
AC_MSG_RESULT([ CPPFLAGS:         $CPPFLAGS])
AC_MSG_RESULT([ LDFLAGS:          $LDFLAGS])
AC_MSG_RESULT([ LIBS:             $LIBS])
AC_MSG_RESULT([  ])
AC_MSG_RESULT([-------------------------------------------------------------------------])
AC_MSG_RESULT([ ])
