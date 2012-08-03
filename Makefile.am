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
etcpointloaddir = /usr/share/doc/wdb-pointload
dist_etcpointload_DATA =

# Setting Targets
bin_PROGRAMS = pointLoad

# This is to be replaced by a "find wdb" macro:
AM_CXXFLAGS =			-DSYSCONFDIR=\"$(pkgsysconfdir)\" -I/usr/include/libxml2 $(wdb_CFLAGS) $(fimex_CPPFLAGS) $(grib_api_CPPFLAGS)

pointLoad_LDADD = $(wdb_LIBS) $(fimex_LDFLAGS) $(fimex_LIBS) $(grib_api_LDFLAGS) $(grib_api_LIBS)

CLEANFILES =
EXTRA_DIST =

include doc/doc.mk
include etc/etc.mk
include src/src.mk
#include test/test.mk
#include test/configuration/test.mk

include config/mk/debian.mk
