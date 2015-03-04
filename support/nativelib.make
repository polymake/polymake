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

#  Building a native methods library (for Java JNI)

SourceDir := ${ExtensionTop}/java/native

ARCHFLAGS := ${NativeArchFlags}

Includes += ${JNIheaders} $(dir $(wildcard ${JNIheaders}/*/jni_md.h))
withInclude := $(if $(filter y,${JavaBuild}),${BuildDir}/jni,${SourceDir})/polymake_$(notdir ${ExtensionTop}).h

SharedModules := $(basename $(call _list_C_sources,${SourceDir}))
SharedObjects := $(addsuffix $O, ${SharedModules})

OwnShared := ${BuildDir}/../../lib/jni/libpolymake_$(notdir ${ExtensionTop})$(subst bundle,jnilib,$S)

${OwnShared} : ${SharedObjects}
	@mkdir -p $(dir $@)
	${CC} ${LDsharedFlags} -o $@ $^ ${LDFLAGS}

compile-native : ${OwnShared}

InstallNativeLib := ${InstallArch}/lib/jni

install-native : compile-native
	${PERL} ${INSTALL_PL} -d -m ${DirMask} ${InstallNativeLib}
	${PERL} ${INSTALL_PL} -m 555 ${OwnShared} ${InstallNativeLib}

clean-native : clean
	rm -f ${OwnShared}

ProcessDep := n
