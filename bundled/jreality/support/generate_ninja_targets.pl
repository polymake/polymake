#  Copyright (c) 1997-2022
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
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

my @targets;

if ($ConfigFlags{"bundled.java.ANT"} ne ".none.") {

   @targets=add_jar_target("jreality");

   print <<'---';
build ${buildtop}/jars/jreality: symlink ${root}/bundled/jreality/external/jreality/lib
---
   push @targets, all => '${buildtop}/jars/jreality';

   if ($ConfigFlags{"bundled.jreality.JoglNative"} eq "bundled") {
      print <<'---';
build ${buildtop}/lib/jni/jreality: symlink ${root}/bundled/jreality/external/jreality/jni/${bundled.jreality.JNIarch}
---
      push @targets, all => '${buildtop}/lib/jni/jreality';
   }
}

@targets
