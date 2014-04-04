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

.PHONY: all compile compile-jars compile-native install-shared install-arch clean

include ${BuildDir}/conf.make
include ${ProjectTop}/support/utils.make

ifdef RequireExtensions
  ImportedIntoExtension := ${ExtensionTop}
  include $(foreach ext,${RequireExtensions},$(call _ext_conf_file,${ext}))
  ExtensionTop := ${ImportedIntoExtension}
  ImportedIntoExtension :=
endif

ifeq (${JavaBuild},y)
  JarDir := ${BuildDir}/../../jars
else
  JarDir := $(wildcard ${ProjectTop}/jars)
endif

JarName := polymake_$(notdir ${ExtensionTop}).jar

NativeBuildDir := ${BuildDir}/jni

define _CallANT
	JAVA_HOME=${JDKHome} ${ANT} -q -f java/build.xml -Dpolymake.top=${ProjectTop} -Dbuild.dir=${BuildDir} -Djar.dir=${JarDir} -Djar.name=${JarName} ${ANTflags} $(1)

endef

ifneq ($(wildcard ${BuildDir}/lib/jni),)
  _MakeNative =	$(MAKE) -C ${BuildDir}/lib/jni $(1)-native
else
  _MakeNative := :
endif

_compile_jars := $(if $(filter y,${JavaBuild}), compile-jars)

# default target
all : compile

compile : ${_compile_jars} compile-native

compile-jars ::
	@mkdir -p ${JarDir}
	$(call _CallANT,all)

compile-native :: ${_compile_jars}
	@+$(call _MakeNative,compile)

install-shared ::
	@:

install-arch ::
	@: $(if ${JarDir},,$(error jar files were not built because of configuration option --without-java-build))
	@+$(call _MakeNative,install)
	${PERL} ${INSTALL_PL} -d -m ${DirMask} ${InstallArch}/jars
	${PERL} ${INSTALL_PL} -m 444 ${JarDir}/${JarName} ${InstallArch}/jars

clean ::
	$(call _CallANT,clean)
	@+$(call _MakeNative,clean)
