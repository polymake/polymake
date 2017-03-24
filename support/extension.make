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


###########################################################
#
#  Building and installing a polymake extension
#

ExtensionName := $(notdir ${CURDIR})

.PHONY: all compile install install-arch install-shared clean TAGS RTAGS test

include ${ProjectTop}/support/utils.make
ifndef BuildDir
  BuildDir := $(call _LocateBuildDir)
  ifndef BuildDir
    $(error CONFIGURATION ERROR)
  endif
endif
include ${BuildDir}/conf.make

### default target
all : compile

InstallSuffix := /ext/${ExtensionName}

Apps := $(notdir $(wildcard ${BuildDir}/apps/*))
InstallApps := $(notdir $(wildcard apps/*))

define _MakeApp
	$(MAKE) -C $(notdir ${BuildDir})/apps/$(1) AppName=$(1) $(2)

endef
define _MakeApps
	$(foreach a,${Apps},$(call _MakeApp,$a,$(1)))
endef
define _InstallSubdir
${PERL} ${INSTALL_PL} -m ${DirMask} -U $(2) $(1) ${InstallTop}${InstallSuffix}/$(1)

endef
define _InstallShared
	$(foreach d, perllib rules scripts, $(if $(wildcard $(1)/$d), $(call _InstallSubdir,$(1)/$d)))
endef
define _CreateDir
${PERL} ${INSTALL_PL} -d -m ${DirMask} $(1)

endef

compile clean:
	@+$(call _MakeApps, $@)

install : install-shared install-arch

install-arch : compile
	$(call _CreateDir,${InstallArch}${InstallSuffix}/lib)
	@+$(call _MakeApps, install InstallSuffix=${InstallSuffix})
	rm -f ${InstallArch}${InstallSuffix}/conf.make
	sed -e 's|^include .*/conf\.make|include ${InstallArch}/conf.make|' ${BuildDir}/conf.make >${InstallArch}${InstallSuffix}/conf.make
	chmod 444 ${InstallArch}${InstallSuffix}/conf.make

install-shared:
	$(call _CreateDir,${InstallTop}${InstallSuffix})
	${PERL} ${INSTALL_PL} -m 444 polymake.ext ${InstallTop}${InstallSuffix}
	$(foreach a, ${InstallApps}, $(call _InstallShared,apps/$a))
	@+$(call _MakeApps, install-src InstallSuffix=${InstallSuffix})
	$(foreach dir,resources scripts xml,$(if $(wildcard ${dir}), $(call _InstallSubdir,${dir})))

distclean:
	rm -rf ${BuildDir}

AppsWithTests := $(foreach a,${Apps},$(if $(wildcard apps/$a/testsuite), $a))

ifdef AppsWithTests
test : all
	${PERL} $(firstword $(wildcard ${ProjectTop}/perl/polymake ${InstallBin}/polymake)) --script run_testcases --applications ${AppsWithTests} --extensions ${CURDIR}
else
test :
	@echo No testcases found!
	@false
endif
