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

#  Building core and callable libraries

.PHONY: compile-xs

_hide_significant_blanks = $(subst ',${.empty},$(subst '@,'${.blank},$(subst ${.blank},@,$(1))))

AskPerlVars := version archname archlib archlibexp ccflags ccdlflags optimize

$(foreach expr,$(call _hide_significant_blanks,$(shell ${PERL} $(patsubst %,-V:%:,${AskPerlVars}))), \
               $(eval export PERL$(subst @,${.blank},${expr})))

SourceDir := ${ProjectTop}/lib/core/src
CallableSourceDir := ${ProjectTop}/lib/callable/src

PerlExtDir := ${BuildDir}/perlx-${PERLversion}-${PERLarchname}
ObjectsAlso := ${PerlExtDir}

StandaloneModules     := standalone_init

SharedModules         := $(basename $(call _list_CXX_sources,${SourceDir}))
CallableSharedModules := $(basename $(call _list_CXX_sources,${CallableSourceDir}))
SharedObjects         := $(patsubst %,%$O, $(filter-out ${StandaloneModules}, ${SharedModules}))
StandaloneObjects     := $(patsubst %,%$O, ${StandaloneModules})
CallableSharedObjects := $(patsubst %,${PerlExtDir}/%$O, ${CallableSharedModules})

GlueModules         := $(basename $(call _list_CXX_sources,${SourceDir}/perl))
XXSModules          := $(basename $(call _list_sources,xxs,${SourceDir}/perl))
XSModules           := $(basename $(call _list_sources,xs,${SourceDir}/perl))
CallableGlueModules := $(basename $(call _list_CXX_sources,${CallableSourceDir}/perl))

vpath %.cc ${SourceDir}/perl ${CallableSourceDir} ${CallableSourceDir}/perl ${PerlExtDir}

GlueObjects         := $(patsubst %,${PerlExtDir}/%.o, ${GlueModules})
XXSObjects          := $(patsubst %,${PerlExtDir}/%.o, ${XXSModules})
XSObjects           := $(patsubst %,${PerlExtDir}/%.o, ${XSModules})
CallableGlueObjects := $(patsubst %,${PerlExtDir}/%.o, ${CallableGlueModules})

GlueCXXFlags := -I${PERLarchlibexp}/CORE ${PERLccflags} -DPerlVersion=$(subst .,${.empty},${PERLversion})

${GlueObjects} ${XXSObjects} : ExtraCXXFLAGS := ${GlueCXXFlags}
${CallableSharedObjects}     : ExtraCXXFLAGS := -I${ProjectTop}/include/callable
${CallableGlueObjects}       : ExtraCXXFLAGS := -I${ProjectTop}/include/callable -I${PerlExtDir} ${GlueCXXFlags} \
  $(foreach v, InstallTop InstallArch Arch FinkBase, $(if ${$v}, -DPOLYMAKE_CONF_$v=\"${$v}\"))

${CallableGlueObjects} : ${BuildDir}/conf.make

CoreLib := ${BuildDir}/lib/core$S
ifneq (${LDcallableFlags},none)
  ifndef LDsonameFlag
    $(error Can't create callable library: LDsonameFlag is not set.  Please rerun the configuration script)
  endif
  CallableSoVersion := $(shell sed -n -e '/^declare  *\$$Version=/ { s/^.*"\([^.]*\.[^.]*\).*".*/\1/p; q; }' ${ProjectTop}/perllib/Polymake.pm)
  CallableName := libpolymake$(subst bundle,dylib,$S)
  CallableSoName := $(subst .dylib,.${CallableSoVersion}.dylib,$(subst .so,.so.${CallableSoVersion},${CallableName}))
  FakeApplibName := $(subst polymake,polymake-apps,${CallableName})
  StubApplibName := $(subst polymake,polymake-apps,${CallableSoName})
  CallableLib := ${PerlExtDir}/${CallableSoName}
  FakeApplib := ${BuildDir}/lib/${FakeApplibName}
  StubApplib := ${BuildDir}/lib/${StubApplibName}
else
  .PHONY: no-callable-lib
  CallableLib := no-callable-lib
endif

# compile perl modules for debugging if we are using a debugging perl build, even if clients are optimized.
${GlueObjects} ${XXSObjects} ${CallableGlueObjects} : doOPT := $(if $(filter-out opt,${Debug}),,$(filter -O%,${PERLoptimize}))

${GlueObjects} ${CallableGlueObjects} : ${PerlExtDir}/%.o : %.cc
	$(_CXX_compile)
	@$(_do_deps)

${XXSObjects} : ${PerlExtDir}/%.o : ${PerlExtDir}/%.cc
	$(_CXX_compile)
	@$(_do_deps)

${CoreLib} : ${SharedObjects} ${StandaloneObjects}
	${CXX} ${LDsharedFlags} -o $@ $^ ${LDFLAGS} -lmpfr -lgmp -lpthread ${LIBS}

ifneq (${LDcallableFlags},none)
  ${CallableLib} : ${SharedObjects} ${CallableSharedObjects} ${GlueObjects} ${CallableGlueObjects} ${XXSObjects} ${XSObjects}
	${CXX} ${LDcallableFlags} ${LDsonameFlag}${CallableSoName} -o $@ $^ ${LDFLAGS} ${PERLccdlflags} \
	       -lmpfr -lgmp -lpthread ${LIBXML2_LIBS} ${LIBS} -L${PERLarchlib}/CORE -lperl
	ln -s -f ${CallableSoName} ${PerlExtDir}/${CallableName}
else
  ${CallableLib} : ${GlueObjects} ${XXSObjects}
endif
	@$(MAKE) -C ${PerlExtDir} all LD=${CXX} OTHERLDFLAGS="${LDFLAGS}" GlueObjects="$(notdir ${GlueObjects} ${XXSObjects})"
	@$(MAKE) -C ${PerlExtDir} pure_install InstallDir=.. DESTDIR=

compile-xs : ${PerlExtDir}/Makefile
	@$(MAKE) -C ${PerlExtDir} compile

${PerlExtDir}/Makefile : ${SourceDir}/perl/Makefile.PL
	@mkdir -p $(@D)
	cd $(@D); ${PERL} $< TOP=${ProjectTop} SourceDir=${SourceDir}/perl CxxOpt="${CXXOPT}" CxxDebug="${CXXDEBUG}" \
			     libxml2Cflags="${LIBXML2_CFLAGS}" libxml2Libs="${LIBXML2_LIBS}"

compile : compile-xs
	@$(MAKE) --no-print-directory ${CoreLib} ${CallableLib}

install : compile
	[ -d ${InstallLib} ] || ${PERL} ${INSTALL_PL} -d -m ${DirMask} ${InstallLib}
ifneq (${LDcallableFlags},none)
	${PERL} ${ProjectTop}/support/generate_applib_fake.pl applib_stub.c applib_fake.c $(patsubst %,../%$S,${Apps})
	${CC} ${CsharedFlags} ${LDcallableFlags} ${LDsonameFlag}${StubApplibName} -o ${FakeApplib} applib_fake.c
	${CC} ${CsharedFlags} ${LDcallableFlags} ${LDsonameFlag}${StubApplibName} -o ${StubApplib} applib_stub.c
	${PERL} ${INSTALL_PL} -m 555 ${CallableLib} ${FakeApplib} ${StubApplib} ${InstallLib}
	ln -s -f ${CallableSoName} ${InstallLib}/${CallableName}
endif
	${PERL} ${INSTALL_PL} -d -m ${DirMask} ${InstallArch}/lib
	${PERL} ${INSTALL_PL} -m 555 ${CoreLib} ${InstallArch}/lib
	@$(MAKE) -C ${PerlExtDir} pure_install InstallDir=$(if ${DESTDIR},${FinalInstallArch},${InstallArch})

install-src:
	${PERL} ${INSTALL_PL} -m ${DirMask} -U -X glue.h -X Ext.h -W ${ProjectTop}/include/core-wrappers/polymake ${ProjectTop}/lib/core/include ${InstallInc}/polymake
	${PERL} ${INSTALL_PL} -m ${DirMask} ${ProjectTop}/lib/callable/include ${InstallInc}/polymake
	${PERL} ${INSTALL_PL} -m ${DirMask} ${ProjectTop}/lib/core/skel ${InstallTop}/lib/core/skel

clean::
	rm -rf ${CoreLib} ${BuildDir}/perlx/${PERLversion}/${PERLarchname} \
	       $(if $(filter-out none,${LDcallableFlags}), ${PerlExtDir}/${CallableName} ${CallableLib} ${FakeApplib} ${StubApplib} applib_*.c)
	@[ ! -f ${PerlExtDir}/Makefile ] || \
	 if [ -f ${PerlExtDir}/Makefile.PL ]; then \
	    $(MAKE) -C ${PerlExtDir} clean; \
	 else \
	    rm -rf ${PerlExtDir}; \
	 fi
