#
# Makefile gnucobol/cobc
#
# Copyright (C) 2003-2012, 2014-2016 Free Software Foundation, Inc.
# Written by Keisuke Nishida, Roger While, Simon Sobisch
#
# This file is part of GnuCOBOL.
#
# The GnuCOBOL compiler is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# GnuCOBOL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GnuCOBOL.  If not, see <http://www.gnu.org/licenses/>.

bin_PROGRAMS = cobc
cobc_SOURCES = cobc.c cobc.h config.c tree.c tree.h reserved.c error.c \
	parser.c parser.h scanner.c field.c typeck.c codegen.c ppparse.c \
	ppparse.h pplex.c config.def flag.def warning.def codeoptim.def \
	ppparse.def codeoptim.c

cobc_CFLAGS = -I$(top_srcdir)
cobc_LDADD = $(COBC_LIBS) $(top_builddir)/libcob/libcob.la $(top_builddir)/lib/libsupport.a

dist_man_MANS = cobc.1

cobc.1: cobc.c config.def flag.def warning.def $(top_srcdir)/configure.ac
	$(MAKE) $(AM_MAKEFLAGS) cobc$(EXEEXT)
	help2man --output=cobc.1  --info-page=gnucobol ./cobc$(EXEEXT)

BUILT_SOURCES = parser.c parser.h scanner.c ppparse.c ppparse.h pplex.c
EXTRA_DIST = ppparse.y pplex.l parser.y scanner.l
CLEANFILES = parser.output ppparse.output

# bugfix for non-GNU compilers that define __STDC_VERSION__ >= 199901L but don't work correct
# "sed -i" isn't supported on all systems --> use sed && mv
.l.c:
	LC_CTYPE=C flex -s -o$@ $< && \
	sed -e 's/199901L/199901L \&\&(!defined(_MSC_VER) || _MSC_VER >= 1800)/g' \
	  $@ > $@.tmp && mv -f $@.tmp $@
  
.y.c:
	bison -o $@ $<

