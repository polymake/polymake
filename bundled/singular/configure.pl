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

@make_vars=qw( CXXflags LDflags Libs );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( singular ) }=();
}

sub usage {
   print STDERR "  --with-singular=PATH  installation path of libsingular\n";
}

sub proceed {
   my ($options)=@_;
   my $lib_ext=$Config::Config{so};

   my $singular_config;
   my $singular_version;
   if (defined (my $singular_path=$options->{singular})) {
      $singular_config = "$singular_path/bin/libsingular-config";
   } else {
      $singular_config = Polymake::Configure::find_program_in_path("libsingular-config") or
         die "Could not find 'libsingular-config' in path";
   }

   chomp ($CXXflags=`$singular_config --cflags`);
   die "$singular_config failed: $!" if ($?);

   chomp ($LDflags=`$singular_config --libs`);

   chomp (my $singular_prefix = `$singular_config --prefix`);
   chomp $singular_prefix;
   # yes we need it twice ...

   $Libs = join(" ",$LDflags =~ m/(-l\w+)/g) . " -lfactory -lresources -lpolys -lomalloc";
   $LDflags =~ s/ -l\w+//g;
   $LDflags =~ s/-L(\S+)/-L$1 -Wl,-rpath,$1/g;
   my $libdir = $1;

   my $error=Polymake::Configure::build_test_program(<<"---", CXXflags => $CXXflags, LDflags => $LDflags, Libs => $Libs);
#include "Singular/libsingular.h"
#include <string>
#include <iostream>
int main() {
   char* cpath = omStrDup("$libdir/libSingular.$lib_ext");
   siInit(cpath);
#ifdef HAVE_NTL
   std::cout << "Version: " << VERSION << std::endl;
   return 0;
#else
   std::cout << "Your singular installation was not build with NTL support." << std::endl;
   std::cout << "Please reconfigure and rebuild singular with --with-ntl=PATH." << std::endl;
   return 1;
#endif
}
---
   if ($?==0) {
      $error=Polymake::Configure::run_test_program();
      if ($?) {
         die "Could not run a test program checking for libsingular.\n",
             "The complete error log follows:\n\n$error\n",
             "Please investigate the reasons and fix the installation.\n";
      } else {
         chomp $error;
         ($singular_version) = $error =~ m/Version: ([\d.]+)/;
         if (Polymake::Configure::v_cmp($singular_version,"4.0.1") < 0) {
            die "Your libsingular version $singular_version is too old, at least 4.0.1 is required.\n";
         }
      }
   } else {
      die "Could not compile a test program checking for libsingular.\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "is not configured to build a shared module, or missing at all.\n",
          "The complete error log follows:\n\n$error\n",
          "Please install the library and specify its location using --with-singular option, if needed.\n",
          "Please remember to enable shared modules when configuring the libsingular!\n";
   }

   return "$singular_version @ $singular_prefix";
}
