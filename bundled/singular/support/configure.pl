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
#-------------------------------------------------------------------------------

@conf_vars=qw( CXXFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( singular ) }=();
}

sub usage {
   print STDERR "  --with-singular=PATH  installation path of libsingular\n";
}

sub singular_config_approach {
   my($singular_config) = @_;
   my $singular_prefix;
   my $lib_ext=$Config::Config{so};
   chomp ($CXXFLAGS=`$singular_config --cflags`);
   die "$singular_config failed: $!" if ($?);

   # some of these additional libraries might be unnecessary
   # but we keep them for backwards compatibility for now
   chomp ($LDFLAGS=`$singular_config --libs`);
   while ($LDFLAGS =~ s/(?: ^ | \s) -l\w+//x) {
      $LIBS .= $&;
   }
   $LIBS .= " -lfactory -lsingular_resources -lpolys -lomalloc";

   # yes we need it twice ...
   chomp ($singular_prefix = `$singular_config --prefix`);
   chomp $singular_prefix;
   $LDFLAGS =~ s/-L(\S+)/-L$1 -Wl,-rpath,$1/g;
   my $libdir = $1;

   # newer versions of singular need -lsingular_resources while older ones needed -lresources
   if (!-e "$libdir/libsingular_resources.${lib_ext}" and -e "$libdir/libresources.${lib_ext}") {
      $LIBS =~ s/-lsingular_resources/-lresources/;
   }
   return $singular_prefix;
}

sub pkg_config_approach {
   my ($pkg_config, $options) = @_;
   my $pc_singular_prefix;
   if (defined (my $singular_path=$options->{singular})) {
      $pc_singular_prefix = Cwd::abs_path(`$pkg_config --variable=prefix Singular`);
      chomp($pc_singular_prefix);
      if ($pc_singular_prefix ne Cwd::abs_path($singular_path)) {
         die "libsingular-config not found, using pkg-config with PKG_CONFIG_PATH.\nThe path provided for Singular is: $singular_path\nIt does not agree with the path from pkg-config: $pc_singular_prefix";
      }
   }
   chomp ($CXXFLAGS=`$pkg_config --cflags Singular`);
   die "$pkg_config failed: $!" if ($?);
   chomp ($LDFLAGS=`$pkg_config --libs-only-other --libs-only-L Singular`);
   if (defined ($options->{singular})){
      $LDFLAGS =~ s/-L(\S+)/-L$1 -Wl,-rpath,$1/g;
   }
   chomp ($LIBS=`$pkg_config --libs-only-l Singular`);
   return $pc_singular_prefix;
}

sub fail_gracefully {
   my ($options)=@_;
   my $errorString = "Tried to locate libsingular, but failed due to one of the following reasons:\n";
   if(defined (my $singular_path=$options->{singular})){
      $errorString .= " Could not find libsingular on path: $singular_path\n";
   } else {
      $errorString .= " No path for libsingular provided. Did you forget to provide it with --with-singular=PATH?\n";
   }
   if($pkg_config = Polymake::Configure::find_program_in_path("pkg-config")){
      $errorString .= " pkg-config failed to locate package Singular.\n";
   } else {
      $errorString .= " pkg-config not installed.\n";
   }
   die $errorString."libsingular not found: neither via 'libsingular-config' nor 'pkg-config Singular'.";
}

sub build_singular_test {
   return Polymake::Configure::build_test_program(<<"---", CXXFLAGS => $CXXFLAGS, LDFLAGS => $LDFLAGS, LIBS => $LIBS);
#include <dlfcn.h>
#include "Singular/libsingular.h"
#include <string>
#include <iostream>
int main() {
   Dl_info dli;
   if (!dladdr((void*)&siInit,&dli)) {
      throw std::runtime_error("*** could not find symbol from libsingular ***");
   }

   char* cpath = omStrDup(dli.dli_fname);
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
}

sub proceed {
   my ($options)=@_;

   my $pkg_config;
   $ENV{PKG_CONFIG_PATH} .= ":$options->{singular}/lib/pkgconfig:$options->{singular}/lib64/pkgconfig" if $options->{singular};
   my $singular_prefix;
   my $singular_config;
   my $singular_version;
   if ((defined (my $singular_path=$options->{singular})) 
         and (-x "$options->{singular}/bin/libsingular-config")) {
      # The user provided a path and libsingular-config is present.
      $singular_config = "$singular_path/bin/libsingular-config";
      $singular_prefix = singular_config_approach($singular_config);
   } elsif ($pkg_config = Polymake::Configure::find_program_in_path("pkg-config") 
         and (`$pkg_config --exists Singular`, !$?)){
      # We found pkg-config and pkg-config found Singular. If the user provided
      # a path, it is verified inside the next method, whether it leads to the
      # same Singular installation.
      $singular_prefix = pkg_config_approach($pkg_config, $options);
   } elsif ($singular_config = Polymake::Configure::find_program_in_path("libsingular-config")) {
      # We found a Singular installation on the system that provides
      # libsingular-config.
      $singular_prefix = singular_config_approach($singular_config);
   } else {
      fail_gracefully($options);
   }


   if (defined $Polymake::Configure::GCCversion) {
      $CXXFLAGS .= " -Wno-unused-value";
   }
   if (defined $Polymake::Configure::CLANGversion) {
      $CXXFLAGS .= " -Wno-deprecated-register";
   }

   # remove old -std= versions from CXXFLAGS which would override our c++14
   $CXXFLAGS =~ s/-std=(?:c|gnu)\+\+(?:11|0x|03)//g;
   
   $LIBS .= " -ldl" unless $LIBS =~ /-ldl/ or $^O ne "linux";

   my $error= build_singular_test();
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

   return "$singular_version @ ".($singular_prefix//"system");
}
