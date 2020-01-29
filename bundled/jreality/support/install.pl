#  Copyright (c) 1997-2020
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
#-----------------------------------------------------------------------------

if ($ConfigFlags{'bundled.java.ANT'} ne '.none.') {
   my $dst_dir="$InstallTop/resources/java/jars";
   foreach my $jar (glob("$buildtop/jars/jReality-*.jar")) {
      copy_file($jar, $dst_dir);
   }
   copy_dir("$buildtop/jars/jreality", "$dst_dir/jreality",
	    $ConfigFlags{'bundled.jreality.JoglJars'} ne 'bundled' ? (exclude => [ "jogl-all-.*", "gluegen-rt-.*" ]) : ());

   if ($ConfigFlags{'bundled.jreality.JoglNative'} eq 'bundled') {
      $dst_dir="$InstallTop/resources/java/jni/jreality";
      copy_dir("$root/bundled/jreality/external/jreality/jni/$ConfigFlags{'bundled.jreality.JNIarch'}",
               $dst_dir, mode => 0555);
   }
}
