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
#-----------------------------------------------------------------------------

if ($ConfigFlags{'bundled.java.ANT'} ne '.none.') {
   my $dst_dir="$InstallTop/resources/java/jars";
   make_dir($dst_dir, clean_dir => 1);
   foreach my $jar (glob("$buildtop/jars/polymake_*.jar")) {
      copy_file($jar, $dst_dir);
   }

   $dst_dir="$InstallTop/resources/java/jni";
   my $native_lib="libpolymake_java.$ConfigFlags{'bundled.java.NativeSO'}";
   copy_file("$buildtop/lib/jni/$native_lib", "$dst_dir/$native_lib", clean_dir => 1, mode => 0555);
}
