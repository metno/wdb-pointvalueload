#-----------------------------------------------------------------------------
# Project Makefile for wdb's pointLoad
#
# This file is processed by automake to generate the non-recursive
# project Makefile
#-----------------------------------------------------------------------------


# Scanned by Autoreconf; must be static (no expansion)
ACLOCAL_AMFLAGS = 		-I config/m4



# Share directory organization
wdbdatadir = 			$(datadir)/wdb
sqldir = 		        $(wdbdatadir)/sql
wdblibdir = 			$(libdir)/wdb

wdb_pointvalueloadbindir = $(bindir)

# Setting Targets
bin_PROGRAMS = pointValueLoad

# This is to be replaced by a "find wdb" macro:
AM_CXXFLAGS = -DSYSCONFDIR=\"$(pkgsysconfdir)\" $(wdb_CFLAGS) $(fimex_CPPFLAGS) $(grib_api_CPPFLAGS)

noinst_LIBRARIES = libpointvalueload.a
libpointvalueload_a_SOURCES =
libpointvalueload_a_CPPFLAGS = $(AM_CXXFLAGS) $(fimex_CPPFLAGS)  $(grib_CPPFLAGS) $(geos_CFLAGS) $(netcdf_CFLAGS) $(felt_CPPFLAGS)

pointValueLoad_LDADD = libpointvalueload.a -lfelt \
                  $(wdb_LIBS) $(fimex_LDFLAGS) $(fimex_LIBS) $(grib_api_LDFLAGS) $(grib_api_LIBS) $(geos_LDFLAGS) $(geos_LIBS) $(netcdf_LDFLAGS) $(netcdf_LIBS) $(felt_LDFLAGS) $(felt_LIBS)\
                  $(XML_LIB) $(LOG4CPP_LIB) \
				  $(BOOST_THREAD_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_SYSTEM_LIB) $(BOOST_REGEX_LIB) $(BOOST_FILESYSTEM_LIB)

man1_MANS = pointValueLoad.man

pointValueLoad.man: pointValueLoad
	help2man -N -n"Extracts point data from gridded data files to be loaded into vega wdb database " ./$< --include $(top_srcdir)/detailed.h2m > $@

CLEANFILES = $(man1_MANS)
EXTRA_DIST = detailed.h2m

include doc/doc.mk
include etc/etc.mk
include src/src.mk
include test/pointload.mk

include config/mk/debian.mk
