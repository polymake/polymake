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

make_dir("$InstallArch/jars", clean_dir => 1);
if (-d "$builddir/jars") {
  copy_dir("$builddir/jars", "$InstallArch/jars");
}
if (!$ConfigFlags{'bundled.java.JavaBuild'} && -d "$root/jars") {
  copy_dir("$root/jars", "$InstallArch/jars");
}

my $native_lib="lib/jni/libpolymake_java.$ConfigFlags{'bundled.java.NativeSO'}";
copy_file("$builddir/$native_lib", "$InstallArch/$native_lib", clean_dir => 1);
