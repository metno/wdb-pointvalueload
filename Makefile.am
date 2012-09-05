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

pointloadbindir = $(bindir)

# Setting Targets
bin_PROGRAMS = pointLoad

# This is to be replaced by a "find wdb" macro:
AM_CXXFLAGS = -DSYSCONFDIR=\"$(pkgsysconfdir)\" $(wdb_CFLAGS) $(fimex_CPPFLAGS) $(grib_api_CPPFLAGS)

noinst_LIBRARIES = libpointload.a
libpointload_a_SOURCES =
libpointload_a_CPPFLAGS = $(AM_CXXFLAGS)

pointLoad_LDADD = libpointload.a -lfelt \
                  $(wdb_LIBS) $(fimex_LDFLAGS) $(fimex_LIBS) $(grib_api_LDFLAGS) $(grib_api_LIBS) \
                  $(XML_LIB) $(LOG4CPP_LIB) \
				  $(BOOST_THREAD_LIB) $(BOOST_PROGRAM_OPTIONS_LIB) $(BOOST_DATE_TIME_LIB) $(BOOST_SYSTEM_LIB) $(BOOST_REGEX_LIB) $(BOOST_FILESYSTEM_LIB)

CLEANFILES =
EXTRA_DIST =

include doc/doc.mk
include etc/etc.mk
include src/src.mk
include test/pointload.mk

include config/mk/debian.mk
