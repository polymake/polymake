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

use vars '%all_jar_targets';

sub add_jar_target {
   my ($name, %options)=@_;
   my $prereqs;
   if (my @prereq_exts=$ConfigFlags{"bundled.${name}.RequireExtensions"} =~ /(\S+)/g) {
     $prereqs=join(" ", grep { defined } @all_jar_targets{@prereq_exts});
   }
   my $jar_name="\${buildtop}/jars/polymake_${name}.jar";
   $all_jar_targets{$name}=$jar_name;
   my $build_cmd="build $jar_name: jcompile \${root}/bundled/${name}/java/build.xml | \${root}/bundled/${name}/java/src";
   if ($prereqs) {
     $build_cmd .= " || $prereqs";
   }
   foreach my $flag (sort keys %options) {
     $build_cmd .= "\n  $flag=$options{$flag}";
   }
   $build_cmd .= "\nbuild bundled.${name}.clean: jclean \${root}/bundled/${name}/java/build.xml\n";

   print "$build_cmd\n";
   ( all => $jar_name, clean => "bundled.${name}.clean" );
}

sub add_nativelib_target {
   my ($name, %options)=@_;
   my $build_cmd="";
   my @obj_files;
   my $src_dir="$root/bundled/$name/java/native";
   foreach my $src_file (glob "$src_dir/*.c") {
      my ($src_name, $obj_name)=basename($src_file, "c");
      $src_file =~ s/^\Q$root\E/\${root}/;
      my $obj_file="\${buildtop}/bundled/$name/jni/$obj_name.o";
      $build_cmd .= <<"---";
build $obj_file: ccompile $src_file || $all_jar_targets{$name}
  CextraFLAGS=\${bundled.java.NativeCFLAGS}
  ARCHFLAGS=\${bundled.java.NativeARCHFLAGS}
---
      push @obj_files, $obj_file;
   }
   my $libname="\${buildtop}/lib/jni/libpolymake_${name}.\${bundled.java.NativeSO}";
   $build_cmd .= "build $libname: nativemod @obj_files";
   foreach my $flag (sort keys %options) {
     $build_cmd .= "\n  $flag=$options{$flag}";
   }

   print "$build_cmd\n";
   ( all => $libname );
}

if ($ConfigFlags{"bundled.java.ANT"} ne ".none.") {
   ( add_jar_target("java"), add_nativelib_target("java") )
} else {
   ( )
}
