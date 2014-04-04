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
#-----------------------------------------------------------------------------

#
#  Building and installation
#
.PHONY: all compile install install-arch install-shared \
	install-arch-prep docs release-docs clean clean-arch distclean TAGS RTAGS test

.shell := $(firstword $(wildcard /bin/bash /usr/bin/bash /usr/local/bin/bash))
ifdef .shell
  SHELL := ${.shell}
endif

ProjectTop := ${CURDIR}
include support/utils.make
ifndef BuildDir
  BuildDir := $(call _LocateBuildDir)
  ifndef BuildDir
    $(error CONFIGURATION ERROR)
  endif
endif

### load configuration
include ${BuildDir}/conf.make

### default target
all : compile

# core applications to build
Apps := $(notdir $(wildcard apps/*))

# enabled bundled extensions
BundledExts := $(notdir $(wildcard ${BuildDir}/bundled/*))

BundledExtsWithMakefile := $(patsubst %/Makefile,${ProjectTop}/%,$(wildcard $(patsubst %,bundled/%/Makefile,${BundledExts})))
FirstBundledExtWithMakefile := $(firstword ${BundledExtsWithMakefile})

define _MakeApp
	$(MAKE) -C $(firstword $(3) ${BuildDir})/apps/$(1) AppName=$(1) $(2)

endef
define _MakeAppsInExt
	$(foreach a, $(notdir $(wildcard bundled/$(1)/apps/*)), $(call _MakeApp,$a,$(2),${BuildDir}/bundled/$(1)))
endef
define _MakeApps
	$(MAKE) -C ${BuildDir}/lib/core $(1) Apps="${Apps}"
	$(foreach a, ${Apps}, $(call _MakeApp,$a,$(1)))
	$(foreach e, ${BundledExts}, $(call _MakeAppsInExt,$e,$(1)))
endef
ifdef FirstBundledExtWithMakefile
  define _MakeInBundledExtensions
	$(MAKE) --no-print-directory -C ${FirstBundledExtWithMakefile} -f ${ProjectTop}/support/bundled.make $(1) \
		ProjectTop=${ProjectTop} TopBuildDir=$(CURDIR)/${BuildDir} \
		OtherExtensions='$(filter-out ${FirstBundledExtWithMakefile},${BundledExtsWithMakefile})'
  endef
else
  _MakeInBundledExtensions :=
endif

define _InstallSubdir
${PERL} ${INSTALL_PL} -m ${DirMask} -U $(2) $(1) ${InstallTop}/$(1)

endef
define _CreateDir
${PERL} ${INSTALL_PL} -d -m ${DirMask} $(1)

endef

CallPolymake := $(if ${ARCHFLAGS},$(if $(findstring arch,${PERL}),,arch ${ARCHFLAGS})) ${PERL} perl/polymake

compile:
	@+$(call _MakeApps,compile)
	@+$(call _MakeInBundledExtensions,compile)

### installation

# scripts in support/ which should not be copied to the final installation location
InstHelpers := configure.pl convert_main_script find-provides find-requires

define _InstallSharedInApp
	$(foreach d, perllib rules scripts, $(if $(wildcard $(1)/$d), $(call _InstallSubdir,$(1)/$d)))
endef
define _InstallSharedInExt
	$(if $(wildcard bundled/$(1)/scripts), ${PERL} ${INSTALL_PL} -m ${DirMask} bundled/$(1)/scripts ${InstallTop}/scripts)
	$(foreach a, $(wildcard bundled/$(1)/apps/*), $(call _InstallSharedInApp,$a))
endef
define _InstallShared
	$(foreach a, ${Apps}, $(call _InstallSharedInApp,apps/$a))
	$(foreach e, ${BundledExts}, $(call _InstallSharedInExt,$e))
endef

install : install-shared install-arch

install-arch : all install-arch-prep
	@+$(call _MakeApps,install)
	[ -d ${InstallBin} ] || $(call _CreateDir, ${InstallBin})
	${PERL} support/convert_main_script --bindir ${InstallBin} \
		$(if ${PREFIX},--prefix ${PREFIX}) $(if ${FinkBase},--perllib ${FinkBase}/lib/perl5) \
		InstallTop=${FinalInstallTop} InstallArch=${FinalInstallArch} Arch="${Arch}"
	rm -f ${InstallArch}/conf.make
	{ sed -e '/Install.*=/ $(if ${PREFIX}, s:^\(Install.*=\)${PREFIX}:override \1$${PREFIX}:',\
					       s:^:override :') \
	      -e '/INSTALL_PL=/ s:=.*:=$${InstallTop}/support/install.pl:' \
	      -e '/DESTDIR=/ { s/^/override /; q; }' \
	      ${BuildDir}/conf.make; \
	  $(if ${PREFIX}, echo "PREFIX=${PREFIX}";) \
	} >${InstallArch}/conf.make
	chmod 444 ${InstallArch}/conf.make
	$(foreach e, ${BundledExts}, \
	  rm -f ${InstallArch}/bundled/$e/conf.make; \
	  sed -e 's|^include .*/conf\.make|include ${InstallArch}/conf.make|' ${BuildDir}/bundled/$e/conf.make >${InstallArch}/bundled/$e/conf.make; \
	  chmod 444 ${InstallArch}/bundled/$e/conf.make; )
	@+$(call _MakeInBundledExtensions,install-arch)

install-arch-prep:
	$(call _CreateDir, ${InstallArch} $(patsubst %, ${InstallArch}/bundled/%/lib, ${BundledExts}))

install-shared:
	$(call _CreateDir, ${InstallTop})
	$(call _InstallSubdir,perllib)
	$(call _InstallShared)
	@+$(call _MakeApps, install-src)
	$(call _InstallSubdir,povray)
	$(call _InstallSubdir,scripts)
	$(call _InstallSubdir,xml)
	$(call _InstallSubdir,support,$(foreach f, ${InstHelpers}, -X $f))
	@+$(call _MakeInBundledExtensions,install-arch)

ifdef DeveloperMode

### unit tests

test : compile
	$(if $(filter y% Y%,${Debug}),POLYMAKE_CLIENT_SUFFIX=-d) ${CallPolymake} \
	  --script run_testcases $(if $(filter k,$(MAKEFLAGS)),--keep-going,--coverage) --applications ${Apps}

### maintenance

tagsFLAGS = -R -e -f $@ --exclude=.svn --exclude='.\#*' --exclude='\#*' --exclude='*~'

TAGS:
	ctags ${tagsFLAGS} --language-force=c++ lib apps/*/{src,include} bundled/*/apps/*/{src,include}
RTAGS:
	ctags ${tagsFLAGS} --language-force=perl --exclude=testsuite \
			   perl perllib apps/*/{perllib,rules,scripts} scripts support/*.pl apps/*/testsuite/*/test*.pl \
			   $(wildcard $(addprefix bundled/*/apps/*/, perllib rules scripts testsuite/*/test*.pl))

include support/export.make

### separate builds in selected bundled extensions

%-bundled :
	+@$(call _MakeInBundledExtensions,$*)

endif  # DeveloperMode

### automatic part of documentation

# generate the doc pages describing the current configuration, including known extensions
docs:
	${CallPolymake} --script generate_docs ${InstallDoc} ${Apps}
	$(if ${DeveloperMode}, ${CallPolymake} --script doxygen ${InstallDoc}/PTL $(if ${Verbose},, >/dev/null 2>&1))

# constrain the documentation to the core application
release-docs:
	${CallPolymake} --ignore-config --script generate_docs ${DocOptions} ${InstallDoc} ${Apps}

### cleanup

clean-arch:
	@+$(call _MakeApps,clean)
	@+$(call _MakeInBundledExtensions,clean)

clean : clean-arch
	rm -rf doc_build

distclean:
	rm -rf build* doc_build


# Local Variables:
# mode: Makefile
# End:
