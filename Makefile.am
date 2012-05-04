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
AM_CXXFLAGS =			-DSYSCONFDIR=\"$(pkgsysconfdir)\" $(wdb_CFLAGS)

pointLoad_LDADD = $(wdb_LIBS) \
                      -lboost_system -lfimex -lgrib_api -lwdbLoad -lwdbConfig -lwdbProj -lwdbExcept -lwdbLog -lwdbMath

CLEANFILES =
EXTRA_DIST =

include doc/doc.mk
include etc/etc.mk
include src/src.mk
#include test/test.mk
#include test/configuration/test.mk

include config/mk/debian.mk
