#  Copyright (c) 1997-2015
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

# convert a space-separated list into a colon-separated path
.empty:=
.blank:=${.empty} ${.empty}
_list2path = $(subst ${.blank},:,$(strip $(1)))

PERL ?= perl
_LocateBuildDir = $(shell $(if $(filter command, $(origin Arch)), Arch=${Arch}) ${PERL} -e '\
    do '"'${ProjectTop}/support/locate_build_dir'"'; \
    if ($$_=eval { locate_build_dir($(if $(1),'"'$(1)'"')) }) { print } else { print STDERR $$@ }')

MV := mv -f

# recursively expand a variable which may contain names of other variables
_expand_var = $(foreach word,$(1),$(if $(${word}),$(call _expand_var,$(${word})), ${word}))

# suffixes of C++ source files (not headers!)
CXX_suffixes := cc cpp C

# list all sources in a directory
_list_sources = $(patsubst $(2)/%,%,$(wildcard $(addprefix $(2)$(if $(3),/$(3))/*., $(1))))
_list_C_sources = $(call _list_sources,c ${CXX_suffixes},$(1),$(2))
_list_CXX_sources = $(call _list_sources,${CXX_suffixes},$(1),$(2))

# list all subdirectories of the current directory
_list_dirs = $(shell $(if $(1),cd $(1) || exit 1;) for d in *; do [ -d $$d ] && echo $$d; done)

# convert a relative path to an absolute one
_abs_path = $(if $(filter /%,$(1)),$(1),$(shell ${PERL} -MCwd=abs_path -e 'print abs_path(q{$(1)})'))

ifdef ExtensionTop
  # locate the configuration results for an extension
  ifdef InSourceTree
    _ext_conf_file = $(firstword $(wildcard $(addsuffix /conf.make, $(1)/build.${Arch} $(patsubst ${ProjectTop}/%,${ProjectTop}/build.${Arch}/%,$(1)) $(patsubst ../../../../../%,${ProjectTop}/build.${Arch}/%,$(1)))))
  else
    _ext_conf_file = $(firstword $(wildcard $(addsuffix /conf.make, $(1)/build.${Arch} $(patsubst ${InstallTop}/%,${InstallArch}/%,$(1)))))
  endif
endif

override DeveloperMode := $(if $(wildcard ${ProjectTop}/support/export.make), yes)

# Local Variables:
# mode: Makefile
# End:
