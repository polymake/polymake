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

#  Building static libraries out of bundled third-party software

SourceDir := $(firstword ${ExtensionTop} ${ProjectTop})/staticlib/$(notdir ${CURDIR})

include ${SourceDir}/Makefile.inc

Cflags += ${CflagsSuppressWarnings}
CXXflags += ${CflagsSuppressWarnings}

ifeq ($(words ${OwnLibraries}),1)
  Archive := $(addsuffix $A, ${OwnLibraries})
  LibObjects := $(addsuffix $O, ${LibModules})

  .PHONY: ${OwnLibraries}
  .PRECIOUS: ${LibObjects}
  .INTERMEDIATE: ${LibObjects}

  ${OwnLibraries} : % : %$A(${LibObjects})
	@NewObjects=`for o in $?; do [ -f $$o ] && echo $$o; done`; \
	[ -z "$$NewObjects" ] || { \
	  set -x; \
	  ${AR} -rc $@$A $$NewObjects; \
	  rm $$NewObjects; \
	  ${RANLIB} $@$A; \
	}

  (%.o) : %.o ;

  _archive_member = ${Archive}($(1))

  ${OwnLibraries} : lib_dep_target = $(call _archive_member,$@)

  compile : ${OwnLibraries}

else
  compile :
	+@$(foreach l,${OwnLibraries},$(MAKE) --no-print-directory compile OwnLibraries=$l Debug=${Debug};)
endif

clean::
	rm -f *$A ${ExtraCLEAN}
