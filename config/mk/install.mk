# $Id: install.mk 210 2010-04-08 13:47:48Z dages $

# This is broken.  On systems that have the BSD "install" utility,
# autoconf defines INSTALL to e.g. "/usr/bin/install -c"; on systems
# that don't, it uses something like "$(top_srcdir)/install-sh -c".
# We really want -p as well, but there is no good way to do this
# without writing our own version of AC_PROG_INSTALL, so we will
# simply assume that a working version of the BSD "install" utility is
# in PATH.
INSTALL = install -c -p
