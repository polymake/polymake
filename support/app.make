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

#  Building an application, possibly in an extension or private wrapper depot.

AppName ?= $(notdir ${CURDIR})
SourceTop := $(firstword ${ExtensionTop} ${ProjectTop})
SourceDir := ${SourceTop}/apps/${AppName}/src

-include ${SourceDir}/Makefile.inc
ifdef ExtensionTop
  -include ${ProjectTop}/apps/${AppName}/src/Makefile.inc
  ifdef RequireExtensions
    ImportedIntoExtension := ${ExtensionTop}
     -include $(addsuffix /apps/${AppName}/src/Makefile.inc, ${RequireExtensions})
    ImportedIntoExtension :=
  endif
endif

ifeq (${SharedModules},ALL)
  SharedModules :=
endif
ifeq (${SharedModules},)
  SharedModules := $(basename $(call _list_CXX_sources,${SourceDir}))
  CheckOrphans := y
else
  SharedModules := $(basename $(notdir $(wildcard $(foreach s,${CXX_suffixes}, $(patsubst %, ${SourceDir}/%.$s, ${SharedModules})))))
endif

ifneq (${IgnoreModules},)
  SharedModules := $(filter-out ${IgnoreModules},${SharedModules})
endif

ifneq ($(wildcard ${SourceDir}/perl),)
  AutoGenModules := $(filter-out perl/wrap-%, $(basename $(call _list_CXX_sources,${SourceDir},perl)))
  SharedModules += ${AutoGenModules}
  SourceSubdirs := perl
endif

SharedObjects := $(addsuffix $O, ${SharedModules})

CXXflags += -DPOLYMAKE_APPNAME=${AppName}

ifdef ExtensionTop
  IncludesXX += $(foreach ext, ${ExtensionTop} ${RequireExtensions}, $(addprefix ${ext}/, include/app-wrappers include/apps))
  IncludeSources := $(patsubst %,-I%/apps/${AppName}, ${ExtensionTop} ${ProjectTop} ${RequireExtensions})
else
  IncludeSources := -I${ProjectTop}/apps/${AppName}
endif

ifdef InSourceTree
  IncludesXX += $(addprefix ${ProjectTop}/, include/app-wrappers include/apps)
endif

Libs := -lmpfr -lgmp -lpthread ${Libs}

ifdef StaticLibs
  .PHONY: compile-staticlibs

  define _MakeStaticLib
      $(MAKE) --print-directory -C ${BuildDir}/staticlib/$(1) Debug=${Debug} $(2)

  endef
  define _MakeStaticLibs
      $(foreach x,${StaticLibs},$(call _MakeStaticLib,$x,$(1)))
  endef

  # build process of a static library might produce header files needed for our clients
  ${SharedObjects} : | compile-staticlibs

  compile-staticlibs:
	+@$(call _MakeStaticLibs,compile)

  clean::
	+@$(call _MakeStaticLibs,clean)
endif

OwnShared := ${BuildDir}/lib/${AppName}$S

ifneq ($(wildcard ${SourceDir}/perl/*.cc ${SourceDir}/perl/*.C ${SourceDir}/perl/*.cpp),)
  groom_error := $(shell ${PERL} ${ProjectTop}/support/groom_wrappers.pl ${SourceDir}/perl 2>&1)
  ifneq (${groom_error},)
    $(error ${groom_error})
  endif
endif

WithWrappers.cc  := $(patsubst wrap-%,%$O,$(basename $(notdir $(wildcard ${SourceDir}/perl/wrap-*.cc))))
WithWrappers.C   := $(patsubst wrap-%,%$O,$(basename $(notdir $(wildcard ${SourceDir}/perl/wrap-*.C))))
WithWrappers.cpp := $(patsubst wrap-%,%$O,$(basename $(notdir $(wildcard ${SourceDir}/perl/wrap-*.cpp))))
WithWrappers     := ${WithWrappers.cc} ${WithWrappers.C} ${WithWrappers.cpp}
WrappersOnly     := $(filter-out ${SharedObjects}, ${WithWrappers})
WithWrappers     := $(filter-out ${WrappersOnly}, ${WithWrappers})
WrappersOnly.cc  := $(filter ${WrappersOnly}, ${WithWrappers.cc})
WrappersOnly.C   := $(filter ${WrappersOnly}, ${WithWrappers.C})
WrappersOnly.cpp := $(filter ${WrappersOnly}, ${WithWrappers.cpp})
WithWrappers.cc  := $(filter-out ${WrappersOnly.cc},  ${WithWrappers.cc})
WithWrappers.C   := $(filter-out ${WrappersOnly.C},   ${WithWrappers.C})
WithWrappers.cpp := $(filter-out ${WrappersOnly.cpp}, ${WithWrappers.cpp})

ifdef WrappersOnly
  SharedObjects := ${SharedObjects} ${WrappersOnly}
endif

override _remove-orphans :=

ifeq (${CheckOrphans},y)
  Orphans := $(wildcard *$O) $(filter-out perl/wrap-%, $(wildcard perl/*$O))
  ifdef SharedObjects
    Orphans := $(filter-out ${SharedObjects}, ${Orphans})
    ifeq (${suffix},)
      Orphans := $(filter-out $(patsubst %$O,%-d%$O,${SharedObjects}), ${Orphans})
    endif
  endif
  ifneq ($(strip ${Orphans}),)
    .PHONY: remove-orphans
    override _remove_orphans := remove-orphans

    remove-orphans:
	@echo removing obsolete object files without corresponding sources:
	rm -f ${Orphans}

  endif
endif

${WithWrappers.cc}  : %$O : ${SourceDir}/perl/wrap-%.cc
${WithWrappers.C}   : %$O : ${SourceDir}/perl/wrap-%.C
${WithWrappers.cpp} : %$O : ${SourceDir}/perl/wrap-%.cpp

${WithWrappers} : includeSource = ${IncludeSources} $(call addinclude,src/$(notdir $<)) ${SourceDir}/perl/wrap-$(notdir $<)
${WrappersOnly} : includeSource = ${IncludeSources} $(call addinclude,src/$(patsubst wrap-%,%,$(notdir $<))) $<
${WrappersOnly} : ExtraCXXFLAGS += -DPOLYMAKE_NO_EMBEDDED_RULES

${WithWrappers} ${WrappersOnly} : guardedCompiler = yes
$(addsuffix $O, ${AutoGenModules}) : guardedCompiler = yes

ifdef SharedObjects
  ifdef TempWrapperFor
    ${SharedObjects} : includeSource = ${IncludeSources} $(call addinclude,src/${TempWrapperFor}) $<
  endif

  ${OwnShared} : $(if ${OnlyModules}, $(filter $(addsuffix $O,${OnlyModules}), ${SharedObjects}), ${SharedObjects}) ${_remove_orphans}
	${CXX} ${LDsharedFlags} -o $@ ${SharedObjects} ${LDFLAGS} ${LIBS}

else
  # no C++ clients in this application/extension: create an empty file

  ${OwnShared} : ${_remove_orphans}
	@[ ! -s $@ ] || { echo removing obsolete shared library $@ - no source files found;  rm -f $@; }
	@touch $@

endif

compile : ${OwnShared}

ifdef ExtensionTop
  InstallSuffix := /bundled/$(notdir ${ExtensionTop})
endif

install-src:
	${PERL} ${INSTALL_PL} -d -m ${DirMask} ${InstallTop}${InstallSuffix}
	${PERL} ${INSTALL_PL} -m ${DirMask} $(if ${InstallSuffix},,-U) -W ${SourceTop}/include/app-wrappers/polymake/${AppName} \
		${SourceTop}/apps/${AppName}/include ${InstallInc}/polymake/${AppName}
	${PERL} ${INSTALL_PL} -m ${DirMask} -U -X perl \
		${SourceTop}/apps/${AppName}/src ${InstallTop}${InstallSuffix}/apps/${AppName}/src
	$(if ${ExtensionTop},${PERL} ${INSTALL_PL} -m 444 ${SourceTop}/polymake.ext ${InstallTop}${InstallSuffix})

install : compile
	${PERL} ${INSTALL_PL} -m 555 ${OwnShared} ${InstallArch}${InstallSuffix}/lib

clean::
	rm -f ${OwnShared}

# ignore deleted wrappers in dependences
${SourceDir}/perl/wrap-%.cc ${SourceDir}/perl/wrap-%.C ${SourceDir}/perl/wrap-%.cpp :
	@echo wrapper file $@ mentioned in .dependences does not exist - ignored
