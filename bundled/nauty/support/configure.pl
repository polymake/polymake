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

@conf_vars=qw( NautySrc CXXFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( nauty-src nauty ) }=();
}


sub usage {
   print STDERR "                         By default polymake will use the bundled nauty source files,\n",
                "                         these options allow using a custom nauty source or installation:\n",
                "  --with-nauty-src=PATH  Source directory of nauty.\n",
                "  --with-nauty=PATH      Installation prefix of nauty.\n";
}

sub check_nauty {
   my $dir = @_ > 0 ? $_[0] : "bundled/nauty/external/nauty";
   -e "$dir/nauty-h.in"
}

sub proceed {
   my ($options)=@_;
   my $nauty_path;
   my $nauty_src;
   my $nautyver;
   $NautySrc = "bundled";
   if (defined ($nauty_path=$options->{nauty}) && $nauty_path ne "bundled") {
      my $nauty_inc="$nauty_path/include";
      my $nauty_lib=Polymake::Configure::get_libdir($nauty_path, "nauty");
      unless (-f "$nauty_inc/nauty/nauty.h"
              && ( -f "$nauty_lib/libnauty.$Config::Config{dlext}"
                   || -f "$nauty_lib/libnauty.a" ) ) {
         die "Invalid installation location of nauty library: header file nauty/nauty.h and/or library libnauty.$Config::Config{dlext} / libnauty.a not found\n";
      }
      $LDFLAGS.=" -Wl,-rpath,$nauty_lib"
         if $nauty_path !~ m|^/usr$| && -f "$nauty_lib/libnauty.$Config::Config{dlext}";
      $CXXFLAGS = "-I$nauty_inc";
      $LDFLAGS = "-L$nauty_lib";
      undef $NautySrc;
   } elsif (defined ($nauty_src=$options->{"nauty-src"}) && $nauty_src ne "bundled") {
      check_nauty($nauty_src) or
         die "Specified nauty source directory invalid, could not find 'nauty-h.in' in $nauty_src.";
      $NautySrc = "$nauty_src";
   }

   if (!$nauty_src && $nauty_path ne "bundled" && $options->{prereq} ne ".none.") {
      # compile test-program, soft-fail
      my $testcode = <<'---';
// simplified example from nautyex1.c from the nauty source

#define MAXN 1000    /* Define this before including nauty.h */
#include <nauty/nauty.h>
#include <iostream>

int
main(int argc, char *argv[])
{
    graph g[MAXN*MAXM];
    int lab[MAXN],ptn[MAXN],orbits[MAXN];
    static DEFAULTOPTIONS_GRAPH(options);
    statsblk stats;
    int n = 5;
    int m = 1;
    int v;

    options.writeautoms = FALSE;

    /* The following optional call verifies that we are linking
       to compatible versions of the nauty routines.            */

    nauty_check(WORDSIZE,m,n,NAUTYVERSIONID);
    EMPTYGRAPH(g,m,n);
    for (v = 0; v < n; ++v)  ADDONEEDGE(g,v,(v+1)%n,m);

    densenauty(g,lab,ptn,orbits,&options,&stats,m,n,NULL);
    std::cout << "VERSION " << NAUTYVERSIONID << std::endl;

    exit(0);
}
---
      my $error=Polymake::Configure::build_test_program($testcode, LIBS => "-lnauty", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");

      if ($? == 0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_nauty() and !defined($nauty_path) or
               die "Could not run a test program checking for nauty library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            ($nautyver) = $message =~ /VERSION (\d+)/;
            my $nautymin = 25000; # version*10000
            if ($nautyver < $nautymin) {
               check_nauty() and !defined($nauty_path) or
                  die "Your nauty version $nautyver is too old, at least version $nautymin is required.\n";
            } else {
               undef $NautySrc;
            }
         }
      } else {
         check_nauty() and !defined($nauty_path) or
            die "Could not compile a test program checking for nauty.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-nauty option, if needed.\n";
      }
   }

   if ($NautySrc eq "bundled" && !check_nauty()) {
      die "Bundled nauty directory seems to be missing, to use the nauty interface\n",
          "with the minimal tarball please specify a nauty source directory\n",
          "via --with-nauty-src=PATH, or a nauty installation with --with-nauty=PATH.";
   } elsif (!defined($NautySrc)) {
      $LIBS = "-lnauty";
   }

   return $NautySrc ? "source: $NautySrc" : ("version $nautyver @ ".($nauty_path//"system"));
}

