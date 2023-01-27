#  Copyright (c) 1997-2023
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; either version 2, or (at your option) any
#  later version: http://www.gnu.org/licenses/gpl.txt.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#-----------------------------------------------------------------------------

#
#  Mostly forwarding to ninja, for backward compatibility reasons
#
.PHONY: all install clean distclean test

NINJA := ninja

override DeveloperMode := $(wildcard support/export.make)
override BuildRoot := $(firstword $(wildcard build.${Arch}) build)
_BuildDir = ${BuildRoot}/$(if $(filter Debug=y% Debug=Y%, ${MAKEFLAGS}),Debug,Opt)

# DESTDIR is often specified as make parameter but ninja doesnt support this
# and we forward it via an environment variable
ifdef $(DESTDIR)
	export DESTDIR
endif

all install :
	${NINJA} -C $(_BuildDir) $@

test : all
	perl/polymake --script run_testcases

clean :
	${NINJA} -C $(_BuildDir) clean.all
	${NINJA} -C $(_BuildDir) -t clean all
	rm -rf ${BuildRoot}/doc

distclean:
	rm -rf build build.*

ifdef DeveloperMode
  include support/export.make
endif
