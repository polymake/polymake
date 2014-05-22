#  Copyright (c) 1997-2014
#  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
#  http://www.polymake.org
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
#-------------------------------------------------------------------------------

# Performing build actions in bundled extensions in proper order

.PHONY: $(MAKECMDGOALS)

override BuildDir := ${TopBuildDir}/bundled/$(notdir $(CURDIR))

include ${BuildDir}/conf.make

ifdef RequireExtensions
  PassAhead := $(filter ${RequireExtensions},${OtherExtensions})
endif
ifdef PassAhead
  ProceedAfter := $(filter-out ${PassAhead},${OtherExtensions})
else
  ProceedAfter := ${OtherExtensions}
endif

  $(MAKECMDGOALS) :
ifdef PassAhead
	@$(MAKE) --no-print-directory -C $(firstword ${PassAhead}) -f ${ProjectTop}/support/bundled.make $@ \
		 ProjectTop=${ProjectTop} TopBuildDir=${TopBuildDir} \
		 OtherExtensions='$(filter-out $(firstword ${PassAhead}),${PassAhead})'
endif
	@$(MAKE) -f Makefile $@ ProjectTop=${ProjectTop} BuildDir=${BuildDir}
ifdef ProceedAfter
	@$(MAKE) --no-print-directory -C $(firstword ${ProceedAfter}) -f ${ProjectTop}/support/bundled.make $@ \
		 ProjectTop=${ProjectTop} TopBuildDir=${TopBuildDir} \
		 OtherExtensions='$(filter-out $(firstword ${ProceedAfter}),${ProceedAfter})'
endif
