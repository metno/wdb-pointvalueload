#
#   wdb - weather and water data storage
#
#   Copyright (C) 2012 met.no
#   
#   Contact information:
#   Norwegian Meteorological Institute
#   Box 43 Blindern
#   0313 OSLO
#   NORWAY
#   E-mail: wdb@met.no
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
#   MA  02110-1301, USA
#
#   Checks for the presence of docbook
#

AC_DEFUN([WDB_FELT],
[
# Set up option
AC_ARG_WITH([felt],
	     	AS_HELP_STRING([--with-felt=FELT_PATH], 
			[Specify the directory in which felt is installed (by default, configure checks your PATH).]),
	    	[ac_felt_path="$withval"],
            [])

# Add path if given
PATH="$ac_docbook_path/bin:$PATH"

# Find xmlto
AC_PATH_PROG(FELT_CONFIG, felt-config, no, $PATH)

if test "$FELT_CONFIG" = "no" ; then
 
	AC_MSG_ERROR([
-------------------------------------------------------------------------
    Unable to find felt. felt is required in order to build the pointLoad.
-------------------------------------------------------------------------
])
fi

netcdf_CFLAGS=`$FELT_CONFIG --cflags`
netcdf_LIBS=`$NETCDF_CONFIG --libs`
AC_SUBST(felt_CFLAGS)
AC_SUBST(felt_LIBS)

])

AC_DEFUN([WDB_FELT],
[
AC_ARG_WITH([felt],
	     	AS_HELP_STRING([--with-felt=FELT_PATH], 
			[Specify the directory in which netcdf interface is installed (by default, configure checks your PATH).]),
	    	[LDFLAGS="-L$withval/lib $LDFLAGS"
	    	CPPFLAGS="-I$withval/include $CPPFLAGS"])

AC_CHECK_HEADER([FeltConstants.h])
##AC_CHECK_LIB([netcdf], [NC_check_name])
])
