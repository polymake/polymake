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

#  Common rules and settings for building an application, an extension,
#  or some other kind of libraries.

.PHONY: all compile install install-src compile-native install-native clean

# default target
all : compile

include ${BuildDir}/conf.make
include ${ProjectTop}/support/utils.make

ProjectTop := $(call _abs_path,${ProjectTop})
ifdef ExtensionTop
  ExtensionTop := $(call _abs_path,${ExtensionTop})
endif

Includes :=
IncludesXX :=

ifeq "${ICCversion}${GCCversion}${CLANGversion}" ""
  $(error unknown C++ compiler)
endif

ifdef ICCversion
  addinclude = -I$(patsubst %/,%,$(dir $(1))) -include $(notdir $(1))
else
  addinclude = -include $(1)
endif
ifndef make_dep
  ifdef CCache
    ifeq "$(filter CCACHE_DISABLE,$(shell echo $${!CCACHE_DISABLE*}))" "CCACHE_DISABLE"
      CCache :=
    endif
  endif
  ifdef CCache
    make_dep = -MD -MF $(@:.o=.d)
  else
    make_dep = -MD
  endif
endif

### compiler options depending on the chosen debugging features
suffix:=
COptFlags   := -DNDEBUG -DPOLYMAKE_DEBUG=0 ${CXXOPT}
CDebugFlags := -DPOLYMAKE_DEBUG=1 ${CXXDEBUG}

ifeq (${Debug},)
  doOPT:=y
else
  suffix:=-d
  ifeq (${Debug},opt)
    suffix:=${suffix}$(patsubst -%,%,${CXXOPT})
    doOPT:=y
    COptFlags += ${CXXDEBUG}
  endif
  LDflagsDebug += ${CXXDEBUG}
endif
O := ${suffix}.o
S := ${suffix}.${SO}
A := ${suffix}.a

### include conf.make of required extensions and update if necessary

ifdef ExtensionTop
  ifdef InSourceTree
    ifneq ($(filter ${ProjectTop}/bundled/%,${ExtensionTop}),)
      ${BuildDir}/conf.make : ${ExtensionTop}/polymake.ext
	@${PERL} ${ProjectTop}/perl/polymake --mscript ${ProjectTop}/support/update_extension_make_conf ${ExtensionTop}
    endif
  endif

  ifdef RequireExtensions
    ImportedIntoExtension := ${ExtensionTop}
    include $(foreach ext,${RequireExtensions},$(call _ext_conf_file,${ext}))
    ExtensionTop := ${ImportedIntoExtension}
    ImportedIntoExtension :=
  endif
endif

### include the task-specific settings

ifneq ($(findstring /staticlib/, ${CURDIR}),)
  # building the third-party static library
  include ${ProjectTop}/support/staticlib.make

else
  ifdef InSourceTree
    IncludesXX += $(addprefix ${ProjectTop}/, include/core-wrappers include/core)
  else ifneq ($(filter-out /usr/include /usr/local/include /sw/include, ${InstallInc}),)
    IncludesXX += ${InstallInc}
  endif

  ifneq ($(filter %/lib/core, ${CURDIR}),)
    # building the core library
    include ${ProjectTop}/support/corelib.make

  else ifneq ($(filter %-native, ${MAKECMDGOALS}),)
    # building the native methods library
    include ${ProjectTop}/support/nativelib.make

  else 
    # building an application
    include ${ProjectTop}/support/app.make
  endif
endif

### put all include directories together
Cflags := $(addprefix -I, ${Includes}) ${Cflags}
CXXflags := $(addprefix -I, ${IncludesXX}) ${CXXflags}

### append the user-specified values
override CFLAGS += ${Cflags}
override CXXFLAGS += ${CXXflags}
override LDFLAGS += ${LDflags} ${LDflagsDebug}
override LIBS += ${Libs}

### compilation rules
define _do_deps
	$(call _process_d,$(@:.o=.d))

endef
define _C_compile
	${CC} -c -o $@ ${CsharedFlags} ${CFLAGS} $(if ${doOPT},${COptFlags},${CDebugFlags}) $(make_dep) ${ExtraCFLAGS} \
	      $(if ${withInclude}, $(call addinclude, ${withInclude})) $<

endef
define _CXX_compile
	$(if $(filter yes,${guardedCompiler}),${PERL} ${ProjectTop}/support/guarded_compiler.pl) \
	${CXX} -c -o $@ ${CsharedFlags} ${CXXFLAGS} $(if ${doOPT},${COptFlags},${CDebugFlags}) $(make_dep) ${ExtraCXXFLAGS} \
	       $(if ${includeSource}, ${includeSource}, $(if ${withInclude}, $(call addinclude,${withInclude})) $<)

endef

%$O : %.cc
	$(_CXX_compile)
	@$(_do_deps)
%$O : %.C
	$(_CXX_compile)
	@$(_do_deps)
%$O : %.cpp
	$(_CXX_compile)
	@$(_do_deps)
%$O : %.c
	$(_C_compile)
	@$(_do_deps)

${WrappersOnly.cc} : %$O : ${SourceDir}/perl/wrap-%.cc
	$(_CXX_compile)
	@$(_do_deps)
${WrappersOnly.C} : %$O : ${SourceDir}/perl/wrap-%.C
	$(_CXX_compile)
	@$(_do_deps)
${WrappersOnly.cpp} : %$O : ${SourceDir}/perl/wrap-%.cpp
	$(_CXX_compile)
	@$(_do_deps)

ifdef SourceSubdirs
  .PHONY: mksubdirs

  ${SharedObjects} : | mksubdirs

  mksubdirs :
	@mkdir -p ${SourceSubdirs}

  ObjectsAlso += ${SourceSubdirs}
endif

vpath %.cc ${SourceDir}
vpath %.C ${SourceDir}
vpath %.cpp ${SourceDir}
vpath %.c ${SourceDir}

### update dependence list
ifneq ($(filter n%,${ProcessDep}),)
  make_dep :=
  _do_deps :=
else ifneq ($(filter clean, ${MAKECMDGOALS}),)
  make_dep :=
  _do_deps :=
else
  _replace_known_paths := -e 's:${ProjectTop}:$${ProjectTop}:g' \
			  $(if ${ExtensionTop}, -e 's:${ExtensionTop}:$${ExtensionTop}:g')

  _process_d = sed -e 's:/usr/include/[^ 	]*[ 	]*::g' \
		   -e 's:/[^ 	]*/lib/gcc/[^ 	]*/include/[^ 	]*[ 	]*::g' \
		   -e 's:/[^ 	]*/lib/gcc/[^ 	]*/include-fixed/[^ 	]*[ 	]*::g' \
		   -e 's:/[^ 	]*/lib64/gcc/[^ 	]*/include/[^ 	]*[ 	]*::g' \
		   -e 's:/[^ 	]*/lib64/gcc/[^ 	]*/include-fixed/[^ 	]*[ 	]*::g' \
		   -e 's:/[^ 	]*/gcc-lib/[^ 	]*/include/[^ 	]*[ 	]*::g' \
		   ${_replace_known_paths} \
		   -e '/^[ 	]*\\$$/ d' $(1) >$(@:.o=.dep) \
	       && rm $(1)

  _start_dep_block = $(subst /,\/,$(1))

  .dependences : $(wildcard *.dep $(if ${ObjectsAlso},${ObjectsAlso}/*.dep))
	@{ [ ! -f $@ ] || sed $(foreach f,$^, -e '/^$(call _start_dep_block,$(f:.dep=\.o)):/,/[^\\]$$/ d') $@; \
	   [ -z "$^" ] || { cat $^; rm $^; }; \
	} >$@.new
	@${MV} $@.new $@

  -include .dependences

  %.h :    ; @echo include file $@ mentioned in .dependences does not exist - ignored
  $(ProjectTop)/include/polymake/% : ; @echo include file $@ mentioned in .dependences does not exist - ignored
  %.hpp : ; @echo include file $@ mentioned in .dependences does not exist - ignored
  %.tcc : ; @echo include file $@ mentioned in .dependences does not exist - ignored
endif

### delete all non-sources
clean::
	rm -f $(foreach s, $O .d .dep .gcno .gcda , *$s $(addsuffix /*$s, $(wildcard perl))) .dependences
