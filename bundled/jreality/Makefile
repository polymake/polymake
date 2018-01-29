#  Copyright (c) 1997-2018
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

include ${ProjectTop}/support/java_in_extension.make

ifeq (${JavaBuild},y)
compile-jars ::
	@rm -f ${JarDir}/jreality
	@ln -s $(CURDIR)/external/jreality/lib ${JarDir}/jreality
endif

compile-native ::
	@$(if $(filter ${InstallArch}/%,${JoglNative}), \
	  rm -f ${BuildDir}/../../lib/jni/jreality; \
	  ln -sf $(CURDIR)/external/jreality/jni/$(notdir ${JoglNative}) ${BuildDir}/../../lib/jni/jreality)

install-arch ::
	${PERL} ${INSTALL_PL} -m 444 ${JarDir}/jReality-*.jar ${InstallArch}/jars
	${PERL} ${INSTALL_PL} -m ${DirMask} -U ${JarDir}/jreality ${InstallArch}/jars/jreality
	$(if $(filter ${InstallArch}/%,${JoglNative}), \
	  ${PERL} ${INSTALL_PL} -m ${DirMask} -U $(CURDIR)/external/jreality/jni/$(notdir ${JoglNative}) ${InstallArch}/lib/jni/jreality)

clean ::
	@$(if $(filter ${InstallArch}/%,${JoglNative}), \
	  rm -f ${BuildDir}/../../lib/jni/jreality)
