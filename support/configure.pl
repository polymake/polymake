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

BEGIN {
   if ($] < 5.010) {
      print STDERR <<".";
polymake requires perl version not lower than 5.10;
your perl interpreter says it is $].

Please upgrade your perl installation;
if you already have an up-to-date perl interpreter somewhere else,
you can specify its location on the command line:

./configure PERL=/path/to/my/new/perl [other options ...]
.
      exit(1);
   }
}

use Config;
use Cwd;
use File::Path;

use strict 'vars', 'subs';
use lib "perllib";

my $TOP=Cwd::getcwd;
my ($Platform, $NeedsArchFlag);

my $Wiki="http://www.polymake.org/doku.php";

my $warning;

package FakeNoShell;
sub new { bless \(my $dummy) }
sub interactive { 0 }

# set autoflush
$| = 1;

# import some utilities and prepare the fake environment for them
package Polymake;
use vars qw( $DeveloperMode $Shell $filename_re $directory_of_cmd_re );

$DeveloperMode=-d "$TOP/testscenarios";

use Polymake::Configure;

$Shell=new FakeNoShell;
# we can't load regex.pl without compiled support for namespaces
$filename_re=qr{ ([^/]+) $ }x;
$directory_of_cmd_re=qr{ ^(.*) / [^/]+ (?: $ | \s)}x;

package Polymake::Configure;

my (%options, %vars, %allowed_options, %allowed_with, %allowed_vars);
my $check_prereq=1;

# expected options for the core script
@allowed_options{ qw( prefix exec-prefix bindir includedir libdir libexecdir datadir docdir build ) }=();
@allowed_with{ qw( gmp mpfr libxml2 callable ) }=();
@allowed_vars{ qw( CC CFLAGS CXX CXXFLAGS CXXOPT CXXDEBUG LDFLAGS LIBS PERL ) }=();

if ($^O eq "darwin") {
  @allowed_with{ qw( fink readline ) }=();
}

# check for --repeat option: must happen early enough, before any other digging in @ARGV
try_restore_config_command_line(\@ARGV);

my (@ext, @ext_disabled, %ext_with_config, %ext_requires, %ext_conflicts, %ext_failed, %ext_bad);

# sieve out disabled bundled extensions
for (my $i=$#ARGV; $i>=0; --$i) {
   if ($ARGV[$i] =~ /^--without-(.*)$/  &&  -d "bundled/$1") {
      push @ext_disabled, $1;
      $options{$1}=".none.";
      splice @ARGV, $i, 1;
   }
}

# load configuration routines for enabled bundled extensions
@ext=grep { $options{$_} ne ".none." } map { m{bundled/(.*)} } glob("bundled/*");

foreach my $ext (@ext) {
   eval {
      if (-f "bundled/$ext/polymake.ext") {
         open my $MF, "bundled/$ext/polymake.ext"
           or die "unreadable\n";
         local $/;
         local $_=<$MF>;
         s/^\s*\#.*\n//mg;
         my @sections=split /(?:\A\n*|\n{2,})([A-Z_]+)(?=\s)/m;  shift @sections;
         my %sections=@sections;
         if ($sections{URI}) {
            die "bundled extensions have implict URIs; URI section not allowed\n";
         }
         if ($sections{REQUIRE}) {
            $ext_requires{$ext}=[ map { /^bundled:(\w+)$/ ? $1 : die "invalid reference to a prerequisite extension: $_\n" } $sections{REQUIRE} =~ /(\S+)/g ];
         }
         if ($sections{CONFLICT}) {
            $ext_conflicts{$ext}=[ map { /^bundled:(\w+)$/ ? $1 : die "invalid reference to a conflicting extension: $_\n" } $sections{CONFLICT} =~ /(\S+)/g ];
         }
      } else {
         die "missing\n";
      }
   };
   if ($@) {
      $options{$ext}=".none.";
      $ext_bad{$ext}= $@ =~ /^(missing|unreadable)$/ ? "$1 description file bundled/$ext/polymake.ext" : "invalid description file bundled/$ext/polymake.ext\n$@";

   } elsif (-f "bundled/$ext/configure.pl") {
      my $err;
      eval <<"---";
      {  package Polymake::Bundled::$ext;
         do "bundled/$ext/configure.pl";
         unless (\$err=\$\@) {
            allowed_options(\\%allowed_options, \\%allowed_with);
         }
      }
---
      if ($err) {
         $options{$ext}=".none.";
         $ext_bad{$ext}="broken configuration script bundled/$ext/configure.pl:\n$err";
      } else {
         $ext_with_config{$ext}=1;
      }
   }
}

sub usage {
   my $GMPdef= $^O eq "darwin" ? '${fink}' : '/usr';

   print STDERR <<'.';
This is a script preparing the build and installation of polymake.
It tries to mimic well-known GNU auto-configuration scripts:

./configure [options] [VARIABLE=value ...]

Allowed options (and their default values) are:

  Installation directories:

  --prefix=PATH       ( /usr/local )  root of the installation tree
  --exec-prefix=PATH  ( ${prefix} )   root of the architecture-dependent tree
  --bindir=PATH       ( ${exec-prefix}/bin )           main polymake script
  --includedir=PATH   ( ${prefix}/include )            include files
  --libdir=PATH       ( ${exec-prefix}/lib )           callable library
  --libexecdir=PATH   ( ${exec-prefix}/lib/polymake )  dynamic modules loaded at the runtime
  --datadir=PATH      ( ${prefix}/share/polymake )     rules and other architecture-independent files
  --docdir=PATH       ( ${datadir}/doc )               automatically generated documentation files
.
   if ($^O eq "darwin") { 
      print STDERR <<'.'; 
  --build=ARCH        abbreviation for the build architecture: "i386" for 32bit or "x86_64" for 64bit
.
   } else {
      print STDERR <<'.'; 
  --build=ARCH        abbreviation for the build architecture
.
   }
   print STDERR <<'.';

  Build dependences:

.
   if ($^O eq "darwin") {
      print STDERR <<'.';
  --with-fink=PATH         ( /sw ) fink installation directory
  --without-fink           don't use Fink packages at all
  --with-readline=PATH     where to find the gnu readline library (the version included in OSX does not suffice)
.
   }
   print STDERR <<".";
  --with-gmp=PATH     ( $GMPdef ) GNU MultiPrecision library installation directory
  --with-mpfr=PATH    ( =gmp-path ) GNU Multiple Precision Floating-point Reliable library installation directory
  --with-libxml2=PATH ( find via \$PATH ) GNU XML processing library
.

   foreach my $ext (sort keys %ext_with_config) {
      no strict 'refs';
      print STDERR "\n  --without-$ext  disable the bundled extension $ext completely\n";
      &{"Polymake::Bundled::$ext\::usage"}();
      print STDERR "\n";
   }

   print STDERR <<".";

  --without-callable  don't build polymake callable library
                      (in the case of custom perl installations lacking libperl shared library)

  --without-prereq    don't check for availability of required libraries and perl modules
                      (to be used from within package management systems like RPM,
                       having own mechanisms for providing all prerequisits.)

Allowed variables are:

   CC=cc       C compiler executable
   CFLAGS=     C compiler options
   CXX=c++     C++ compiler executable
   CXXFLAGS=   C++ compiler options
   CXXOPT=     C/C++ compiler optimization level (defaults to highest possible)
   LDFLAGS=    linker options
   LIBS=       additional libraries to be linked with
   PERL=perl   perl executable

Special option, incompatible with any others:

   --repeat ARCH  repeat the configuration with exactly the same options as
                  has been used last time for the given build architecture

For detailed installation instructions, please refer to the polymake Wiki site:
  $Wiki/howto/install

.
}

# parse the command line
while (defined (my $arg=shift @ARGV)) {
   # trim and allow empty arguments
   $arg =~ s/^\s+//;
   $arg =~ s/\s+$//;
   next if ($arg =~ /^$/);
   if ($arg eq "--help") {
      usage();
      exit(0);
   } elsif ($arg eq '--without-prereq') {
      $check_prereq=0;
      next;
   } elsif (my ($with, $out, $name, $value)= $arg =~ /^--(with(out)?-)?([-\w]+)(?:=(.*))?$/) {
      if ($with ? exists $allowed_with{$name}
                : exists $allowed_options{$name}
            and
          defined($out) || defined($value) || @ARGV) {
	 if (defined($out)) {
	    $value=".none.";
	 } else {
            $value=shift(@ARGV) unless defined($value);
            $value =~ s{^~/}{$ENV{HOME}/};
            $value =~ s{/+$}{};
         }
         $options{$name}=$value;
         next;
      }
   } elsif ($arg =~ /^([A-Z]+)=(.*)/) {
      if (exists $allowed_vars{$1}) {
         $vars{$1}=$2;
         next;
      }
   }
   print STDERR "Unrecognized option $arg;\nTry ./configure --help for detailed information\n";
   exit(1);
}

# construct paths

my $prefix=$options{prefix} || "/usr/local";
my $exec_prefix=$options{'exec-prefix'} || $prefix;

$InstallBin=$options{bindir} || "$exec_prefix/bin";
$InstallLib=$options{libdir} || "$exec_prefix/lib";
$InstallTop=$options{datadir} || "$prefix/share/polymake";
$InstallArch=$options{libexecdir} || "$exec_prefix/lib/polymake";
$InstallInc=$options{includedir} || "$prefix/include";
$InstallDoc=$options{docdir} || ($Polymake::DeveloperMode ? "doc_build" : "$InstallTop/doc");

$DirMask="0755";

# inherit some variables which haven't appeared on the comand line from the environment

foreach my $varname (grep { $_ ne "PERL" && !exists $vars{$_} } keys %allowed_vars) {
   if (defined (my $value=$ENV{$varname})) {
      $vars{$varname}=$value;
   }
}

# check the C++/C compiler

print "checking C++ compiler ... ";
$CXX=$vars{CXX};
$CC=$vars{CC};

my $absCXX=check_program($CXX, "g++", "c++", "icpc", "clang++")
  or die "no supported C++ compiler found; please reconfigure with CXX=name\n";

while (-l $absCXX) {
   $absCXX=readlink $absCXX;
}
$CCache= $absCXX =~ /\bccache$/;

if (my $build_error = build_test_program(<<".")) {
#include <iostream>
int main() {
#if defined(__clang__)
   std::cout << "clang " << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__ << std::endl;
   return 0;
#elif defined(__INTEL_COMPILER)
   // FIXME: investigate proper macro symbols carrying the icc version
   std::cout << "icc " << std::endl;
   return 0;
#elif defined(__GNUC__)
# if defined(__APPLE__)
   std::cout << "apple";
# endif
   std::cout << "gcc " << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__ << std::endl;
   return 0;
#else
   return 1;
#endif
}
.
   die "C++ compiler $CXX could not compile a test program for version recognition:\n",
       $build_error,
       "\nPlease investigate and reconfigure with CXX=<appropriate C++ compiler>\n";
} else {
   local $_=run_test_program();  chomp;
   if (/^clang /) {
      $CLANGversion=$';
      if (v_cmp($CLANGversion, "3.2") < 0) {
         die "C++ compiler $CXX says its version is $CLANGversion, while the minimal required version is 3.2\n";
      }

   } elsif (/icc /) {
      $ICCversion=$';
      # if (v_cmp($ICCversion, "10.0") < 0) {
      #   die "C++ compiler $CXX says its version is $ICCversion, while the minimal required version is 10.0\n";
      # }

   } elsif (/(apple)?gcc /) {
      $GCCversion=$';
      # decide whether we use the gcc provided by apple, in that case we need the -arch flags, otherwise we probably don't
      $NeedsArchFlag=defined($1);
      if (v_cmp($GCCversion, "4.2") < 0) {
         die "C++ compiler $CXX says its version is $GCCversion, while the minimal required version is 4.2\n";
      }
   } else {
      die "$CXX is an unsupported C++ compiler.  Please choose one of the supported: Intel, clang, or GCC.\n";
   }
}

if (defined $CC) {
   check_program($CC);
} else {
   $CC=$CXX;
   unless (defined($CLANGversion)
           ? $CC =~ s{clang\+\+$}{clang} :
           defined($ICCversion)
           ? $CC =~ s{\b([ei])cp?c$}{${1}cc} :
           defined($GCCversion)
           ? $CC =~ s{g\+\+(?=[^/]*)$}{gcc}
               ||
             $CC =~ s{(?:c\+\+|CC)(?=[^/]*)$}{cc}
           : 0
             and
           eval { check_program($CC) }) {
      $CC=$Config::Config{cc};
   }
}
print "ok ($CXX is ", defined($GCCversion) ? "GCC $GCCversion" : defined($CLANGversion) ? "CLANG $CLANGversion" : "ICC $ICCversion", ")\n";

$PERL     =$vars{PERL}     || $^X;
$CXXOPT   =$vars{CXXOPT}   || "-O3";
$CXXDEBUG =$vars{CXXDEBUG} || "";
$Cflags   =$vars{CFLAGS}   || "";
$CXXflags =$vars{CXXFLAGS} || $Cflags;
$LDflags  =$vars{LDFLAGS}  || "";
$Libs     =$vars{LIBS}     || "";

$LDsharedFlags=$Config::Config{lddlflags};
$LDcallableFlags= $options{callable} eq ".none." ? "none" : "$LDsharedFlags $Config::Config{ldflags}";

my $GMP=$options{gmp};

do "support/locate_build_dir";

if ($^O eq "darwin") {
   if ($options{fink} ne ".none.") {
      print "checking fink installation ... ";
      if (defined( $FinkBase=$options{fink} )) { 
         if (-d $FinkBase) {
            unless (-f "$FinkBase/etc/fink.conf") {
               die "option --with-fink does not point to the fink installation tree: $FinkBase/etc/fink.conf not found\n";
            }
         } elsif (-x $FinkBase && $FinkBase =~ s|/bin/fink$||) {
            unless (-f "$FinkBase/etc/fink.conf") {
               die "option -with-fink points to a broken fink installation: $FinkBase/etc/fink.conf not found\n";
            }
         } else {
            die "option -with-fink points to a wrong program: something like /path/bin/fink expected.\n",
            "If you have renamed the main fink program, please specify the top directory: --with-fink=/path\n";
         }
      } else {
         # Fink location not specified, look for it at plausible places
         (undef, my ($fink, $error))=find_program_in_path("fink", sub {
             !(($FinkBase=$_[0]) =~ s|/bin/fink$|| && -f "$FinkBase/etc/fink.conf") && "!"
                                                          });
         if ($fink) {
            if ($error) {
               die "found the fink program at $fink, but the corresponding configuration file is missing;\n",
               "Please specify the top installation directory using option --with-fink\n";
            }
         } elsif (-f "/sw/etc/fink.conf") {
            $FinkBase="/sw";
         } else {
            die "The Fink package system is a mandatory prerequisite to build and use polymake under MacOS.\n",
                "Please refer to $Wiki/mac for details and installation instructions.\n",
                "If you already have Fink installed at a non-standard location, please specify it using option --with-fink\n";
         }
      }
      print "ok ($FinkBase)\n";

      print "checking fink gmp installation ... ";
      if (-f "$FinkBase/lib/libgmp.dylib") {
         ($Platform)= `lipo -info $FinkBase/lib/libgmp.dylib` =~ /architecture: (\S+)/;
      } else {
         die "Fink package gmp-shlibs seems to be missing.  Further configuration is not possible.\n",
         "Please install the packages gmp and gmp-shlibs and repeat the configure run.\n";
      }
      print "ok\n";
      print "checking fink mpfr installation ... ";
      unless (-f "$FinkBase/lib/libmpfr.dylib") {
         die "Fink package mpfr-shlibs seems to be missing.  Further configuration is not possible.\n",
         "Please install the packages mpfr and mpfr-shlibs and repeat the configure run.\n";            
      }
      print "ok\n";
      if ( $NeedsArchFlag ) {
         $ARCHFLAGS="-arch $Platform";
      } else {
         $ARCHFLAGS="";
      }
      $Arch="darwin.$Platform";
      $GMP ||= $FinkBase;
   } else { # without fink
      print "determining architecture ... ";
      if ( defined( $Platform=$options{build}) ) {
         if ( $Platform ne "i386" && $Platform ne "x86_64" ) {
            die "architecture for Mac OS must be one of i386 or x86_64.\n";
         }
      } else { # choose i386 for 10.5 and x86_64 for all others
         `sw_vers | grep ProductVersion` =~ /(10.*)/;
         my $Version=$1;
         if ( $Version =~ /10.5/ ) { 
            $Platform="i386";
         } else {
            $Platform="x86_64";
         }
      }
      if ( $NeedsArchFlag ) {
         $ARCHFLAGS="-arch $Platform";
      } else {
         $ARCHFLAGS="";
      }
      $Arch="darwin.$Platform";
      print "ok ($Arch)\n";
   }
} else {
   # not MacOS
   print "determining architecture ... ";
   $Platform=platform_name();
   unless ($CXXflags) {
      $CXXflags="-march=native";
      $Cflags=$CXXflags;
   }
   $Arch=$options{build} || $Platform;

   if ($^O eq "freebsd" && !$GMP) {
      if (-f "/usr/local/include/gmp.h") {
         $GMP="/usr/local";
      }
   }
   print "ok ($Arch)\n";
}

# add compiler-specific options
print "determining compiler flags ... ";
if (defined($GCCversion)) {
   $CsharedFlags="-fPIC";
   $CXXDEBUG .= " -g";
   $CXXflags .= " -ftemplate-depth-200 -Wall -Wno-strict-aliasing -Wno-parentheses";
   $Cflags .= " -Wall";
   # external libraries might be somehow dirtier
   $CflagsSuppressWarnings="-Wno-uninitialized -Wno-unused -Wno-parentheses";
   if (v_cmp($GCCversion, "4.6")<0) {
      $CXXflags .= " -fno-inline-functions-called-once";
   } else {
      $CXXflags .= " -fwrapv -fopenmp";
      $LDflags .= " -fopenmp";
      $CflagsSuppressWarnings .= " -Wno-unused-but-set-variable -Wno-enum-compare -Wno-sign-compare -Wno-switch -Wno-format -Wno-write-strings";
   }
   if (v_cmp($GCCversion, "4.3")<0) {
      $CXXflags .= " -Wno-uninitialized";
   }
   if ($^O eq "solaris") {
      $CXXflags .= " -D__C99FEATURES__ -D_GLIBCXX_USE_C99_MATH";
   }

} elsif (defined($ICCversion)) {
   $CsharedFlags="-fPIC";
   $CXXDEBUG .= " -g -Ob0";
   $CXXflags .= " -Wall -wd193,383,304,981,1419,279,810,171,1418,488,1572,561";
   $Cflags .= " -Wall -wd193,1572,561";

} elsif (defined($CLANGversion)) {
   $CsharedFlags="-fPIC";
   $CXXDEBUG .= " -g";
   $CXXflags .= " -Wall -Wno-logical-op-parentheses -Wno-shift-op-parentheses -Wno-duplicate-decl-specifier";
   $CflagsSuppressWarnings="-Wno-uninitialized -Wno-unused -Wno-unused-variable -Wno-enum-compare -Wno-sign-compare -Wno-switch -Wno-format -Wno-write-strings -Wno-empty-body -Wno-logical-op-parentheses -Wno-shift-op-parentheses -Wno-dangling-else";
}

if ($^O eq "darwin") {
   # MacOS magic again
   my $allarch=qr/ -arch \s+ \S+ (?: \s+ -arch \s+ \S+)* /x;
   $Cflags =~ s/$allarch//o;
   $Cflags .= ' ${ARCHFLAGS}';
   $CXXflags =~ s/$allarch//o;
   $CXXflags .= ' ${ARCHFLAGS}';
   $LDsharedFlags =~ s/$allarch/\${ARCHFLAGS}/o;
   if ($options{callable} ne ".none.") {
      ($LDcallableFlags = $Config::Config{ldflags}) =~ s/$allarch//o;
      $LDcallableFlags = "$LDsharedFlags $LDcallableFlags";
      $LDcallableFlags =~ s{-bundle}{-dynamiclib};
      $LDsonameFlag = "-install_name $InstallLib/";
   }
} else {
   if ($options{callable} ne ".none.") {
      $LDsonameFlag = "-Wl,-soname,";
   }
}

print <<"EOF";
ok
   CFLAGS=$Cflags
   CXXFLAGS=$CXXflags
EOF

# detect whether socket functions reside in a separate library

if ($Config::Config{libs} =~ "-lsocket") {
   $Libs="-lsocket $Libs";
}

# locate GMP and MPFR libraries

if (defined $GMP) {
   $Cflags .= " -I$GMP/include";
   $CXXflags .= " -I$GMP/include";
   my $libdir=get_libdir($GMP, "gmp");
   $LDflags .= " -L$libdir";
   if (($^O ne "darwin" || $options{fink} eq ".none.") && exists $options{gmp}) {
      # non-standard location
      $LDflags .= " -Wl,-rpath,$libdir";
   }
}

my $MPFR=$options{mpfr};
if (defined($MPFR) && $MPFR ne $GMP) {
   $Cflags .= " -I$MPFR/include";
   $CXXflags .= " -I$MPFR/include";
   my $libdir=get_libdir($MPFR, "mpfr");
   $LDflags .= " -L$libdir";
   if ($^O ne "darwin" || $options{fink} eq ".none.") {
      # non-standard location
      $LDflags .= " -Wl,-rpath,$libdir";
   }
}

if ($check_prereq) {
   print "checking gmp installation ... ";
   # check GMP installation
   my $build_error=build_test_program(<<'---', Libs => "$ARCHFLAGS -lgmp");
#include <gmp.h>
#include <iostream>
int main() {
   mpz_t a;
   mpz_init(a);
   std::cout << __GNU_MP_VERSION << '.' << __GNU_MP_VERSION_MINOR << '.' << __GNU_MP_VERSION_PATCHLEVEL << std::flush;
   return 0;
}
---
   if ($?==0) {
      my $is_version=run_test_program();
      if ($?==0) {
         if (v_cmp($is_version,"4.2.0")<0) {
            die "The GNU Multiprecision Library (GMP) installed at your site is of version $is_version\n",
                "while 4.2.0 is the minimal required version.\n";
         }
      } else {
         die "Could not run a test program linked to the GNU Multiprecision Library (GMP).\n",
             "Probably the shared library libgmp.$Config::Config{dlext} is missing or of an incompatible machine type.\n";
      }
   } else {
      die "Could not compile a test program checking for the GNU Multiprecision Library (GMP).\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "lacking developer's subpackage or missing at all.\n",
          "Please refer to the installation instructions at $Wiki/howto/install.\n",
          "The complete error log follows:\n", $build_error;
   }
   print "ok", defined($GMP) && " ($GMP)", "\n";

   # check MPFR installation
   print "checking mpfr installation ... ";
   my $build_error=build_test_program(<<'---', Libs => "$ARCHFLAGS -lmpfr -lgmp");
#include <mpfr.h>
#include <iostream>
int main() {
   mpfr_t a;
   mpfr_init(a);
   std::cout << MPFR_VERSION_MAJOR << '.' << MPFR_VERSION_MINOR << '.' << MPFR_VERSION_PATCHLEVEL << std::flush;
   return 0;
}
---
   if ($?==0) {
      my $is_version=run_test_program();
      if ($?==0) {
         if (v_cmp($is_version,"3.0.0")<0) {
            die "The Multiple Precision Floating-Point Reliable Library (MPFR) installed at your site is of version $is_version\n",
                "while 3.0.0 is the minimal required version.\n";
         }
      } else {
         die "Could not run a test program linked to the Multiple Precision Floating-Point Reliable Library (MPFR).\n",
             "Probably the shared library libmpfr.$Config::Config{dlext} is missing or of an incompatible machine type.\n";
      }
   } else {
      die <<".", $build_error;
Could not compile a test program checking for the Multiple Precision Floating-Point Reliable Library (MPFR).
The most probable reasons are that the library is installed at a non-standard location,
lacking developer's subpackage or missing at all.
Please refer to the installation instructions at $Wiki/howto/install.
The complete error log follows:
.
   }
   print "ok", defined $MPFR ? " ($MPFR)" : "","\n";

   if ($options{callable} ne ".none.") {
      # check if libperl is there
      print "checking shared perl library ... ";
      my $libperl=$Config::Config{libperl};

      my $error = "";
      if (length($libperl)==0) {
         $error = <<".";
Your perl installation seems to lack the libperl.$Config::Config{dlext} shared library.
On some systems it is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.

If your perl installation has been configured by hand, please make sure that
you have answered with "yes" to the question about the libperl shared library
(it is not the default choice!), or that you have passed '-Duseshrplib=true'
to the ./Configure script.
Otherwise, reconfigure and reinstall your perl.
.
      }

      # We also build a test program for libperl since e.g. on Debian based systems the
      # check for Config::Config{libperl} will not detect a missing libperl-dev package
      chomp(my $perlldflags = `$PERL -MExtUtils::Embed -e ldopts`);
      my $build_error=build_test_program(<<'---', CXXflags => `$PERL -MExtUtils::Embed -e ccopts`, LDflags => "$ARCHFLAGS $perlldflags" );
#include <EXTERN.h>
#include <perl.h>

static PerlInterpreter *my_perl;

int main(int argc, char **argv, char **env)
{
   char *embedding[] = { "", "-e", "0" };
   PERL_SYS_INIT3(&argc,&argv,&env);
   my_perl = perl_alloc();
   perl_construct(my_perl);
   PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
   perl_parse(my_perl, NULL, 3, embedding, (char **)NULL);
   perl_run(my_perl);
   eval_pv("print $]",TRUE);
   perl_destruct(my_perl);
   perl_free(my_perl);
   PERL_SYS_TERM();
}
---
      if ($?==0) {
         chomp(my $version = run_test_program());
         if ($? == 0) {
            if ($version ne $]) {
               $error = <<".";
The shared perl library claims to be of a different version ($version) than
your perl interpreter ($]). Please choose a different perl installation with
PERL=/some/path/to/bin/perl or try to reinstall perl (including libperl).
.
            }
         } else {
            $error = <<".";
Could not run a test program checking for libperl.$Config::Config{dlext}.
The error is as follows:
$!

On some systems the library is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.
.
         }
      } else {
         $error = <<".";
Could not compile a test program for the libperl.$Config::Config{dlext} shared library.
The build error is as follows:
$build_error

On some systems the library is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.
.
      }

      if ($error) {
         print "failed\n\n";
         die <<".";
$error
As a last resort, you can configure polymake with the option --without-callable .
You won't be able to build the callable library any more, but at least you get
polymake compiled.
.
      }

      print "ok\n";
   }

   # check the prerequisite perl packages
   if (defined $FinkBase) { 
      lib->import("$FinkBase/lib/perl5");
   }
   my @warned;
   foreach (qw(XML::Writer XML::LibXML XML::LibXSLT Term::ReadLine)) {
      my $pkg=$_;
      print "checking perl module $pkg ... ";
      my $warn;
      local $SIG{__WARN__}=sub { $warn=shift };
      eval "require $pkg;";
      if (/^Term/) {
         $pkg.="::Gnu";
         unless ($@) {
            eval "new $pkg('polymake');";
         }
      }
      if ($@) {
         print "failed\n";
         if ($@ =~ /\ACan't locate /) {
            print STDERR <<".";
WARNING: perl module $pkg required for polymake not found on your machine.
         Please be sure to install it prior to starting to use polymake. 
         If you have installed them in a non-standard location
         please add the path to the environment variable PERL5LIB.
.
         } else {
            $@ =~ / at .*? line \d+/m;
            print STDERR <<".";
WARNING: perl module $pkg required for polymake seems to be unusable.
         An attempt to load it has failed because of the following:
         $`
         Please be sure to rectify the problem prior to starting to use polymake.
.
         }
         push @warned,$pkg;
      } elsif ($warn =~ /^(?!Use of inherited AUTOLOAD for non-method Term::ReadLine::Gnu::)./s) {
         print "\n";
         print STDERR <<".";
WARNING: perl module $pkg required for polymake might cause problems.
         An attempt to load it was successful, but was accompanied
         by the following message:
         $warn
         Please be sure to investigate it prior to starting to use polymake.
.
         push @warned,$pkg;
      } else {
         print "ok\n";
      }
   }
   if (@warned) {
      $warning .= <<'EOF';
  WARNING: Please install/check the following perl modules prior to starting polymake: 
EOF
      $warning .= "           ".join(", ",@warned)."\n";
   }
}


print "checking libxml2 installation ... ";
# gather build options for libxml2
my $xml2=$options{libxml2};
if (defined($xml2)) {
   if (-x "$xml2/bin/xml2-config") {
      $xml2="$xml2/bin/xml2-config";
   } elsif (-x "$xml2/xml2-config") {
      $xml2="$xml2/xml2-config";
   } elsif ($xml2 !~ /-config$/ || !-x $xml2) {
      die "Could not derive the location of the configuration program xml2-config from the option --with-libxml2\n";
   }
} else {
   $xml2="xml2-config";
}
$LIBXML2_CFLAGS=`xml2-config --cflags`;
if ($?) {
   die <<".";
Could not find configuration program xml2-config for library libxml2.
Probably you need to install development package for it;
usually it is called libxml2-devel or similarly.
If the library is installed at a non-standard location,
please specify it in option --with-libxml2
.
}
chomp $LIBXML2_CFLAGS;
$LIBXML2_LIBS=`xml2-config --libs`;
chomp $LIBXML2_LIBS;
print "ok", defined $options{libxml2} ? " ($options{libxml2})" : "","\n";

# create the main build directory and Makefile
my $BuildDir="build.$Arch";
File::Path::mkpath($BuildDir);

my $LOG;
my $LOGfile="$BuildDir/bundled.log";

sub bundled_ext_error_msg {
   my ($ext, $msg)=@_;
   unless (defined($LOG)) {
      open $LOG, ">", $LOGfile;
      print $LOG <<".";
Configuration of the following bundled extensions failed, proceeding without them.
If you really need them, please carefully read the following explanations,
take the suggested actions, and repeat the configuration.
.
   }
   print $LOG "\n---- $ext ----\n\n$msg\n";
}

print "\nConfiguring bundled extensions:\n";
foreach my $ext (@ext_disabled) {
   print "bundled extension $ext ... disabled by command-line\n";
}
while (my ($ext, $err)=each %ext_bad) {
   print "bundled extension $ext ... disabled because of a fatal error\n";
   bundled_ext_error_msg($ext, $err);
}

# order the enabled bundled extensions
my @ext_ordered;
my %ext_unordered=map { ($_ => 1) } grep { $options{$_} ne ".none." } @ext;
while (keys %ext_unordered) {
   my $progress=@ext_ordered;
   while ((my $ext, undef)=each %ext_unordered) {
      unless ($ext_requires{$ext} && grep { $ext_unordered{$_} } @{$ext_requires{$ext}}
                or
              $ext_conflicts{$ext} && grep { $ext_unordered{$_} } @{$ext_conflicts{$ext}}) {
         push @ext_ordered, $ext;
         delete $ext_unordered{$ext};
      }
   }
   last if $progress==@ext_ordered;
}

while ((my $ext, undef)=each %ext_unordered) {
   print "bundled extension $ext ... disabled because of cyclic dependencies on other extension(s): ",
         join(", ", $ext_requires{$ext} ? (grep { $ext_unordered{$_} } @{$ext_requires{$ext}}) : (),
                    $ext_conflicts{$ext} ? (grep { $ext_unordered{$_} } @{$ext_conflicts{$ext}}) : ()), "\n";
}

# configure the bundled extensions
my %ext_survived;
foreach my $ext (@ext_ordered) {
   print "bundled extension $ext ... ";
   my $enable=1;


   if ($ext_requires{$ext}) {
      foreach my $prereq (@{$ext_requires{$ext}}) {
         unless ($ext_survived{$prereq}) {
            if (-d "bundled/$prereq") {
               print "disabled because of unsatisfied prerequisite: $prereq";
            } else {
               print "disabled because of unknown/missing prerequisite: $prereq";
            }
            $enable=0; last;
         }
      }
   }
   if ($enable && $ext_conflicts{$ext}) {
      foreach my $conflict (@{$ext_conflicts{$ext}}) {
         if ($ext_survived{$conflict}) {
            print "disabled because of conflict with other extension: $conflict";
            $enable=0; last;
         }
      }
   }

   if ($enable) {
      eval { check_extension_source_conflicts("bundled/$ext", ".", $ext_requires{$ext} ? (map { "bundled/$_" } @{$ext_requires{$ext}}) : ()) };
      if ($@) {
         print "disabled because of the following conflicts:\n\n$@";
         $enable=0;

      } elsif ($ext_with_config{$ext}) {
         my $result=eval { no strict 'refs'; &{"Polymake::Bundled::$ext\::proceed"}(\%options) };
         if ($@) {
            print "failed";
            bundled_ext_error_msg($ext, $@);
            $enable=0;
         } else {
            print "ok", $result ? " ($result)":"";
         }
      } else {
         print "ok";
      }
   }

   if ($enable) {
      $ext_survived{$ext}=1;
   } else {
      $ext_failed{$ext}=1;
      if (exists($options{$ext}) and $options{$ext} ne ".none.") {
         die << "EOF";


ERROR:
The bundled extension $ext was explicitly requested but failed to configure.
Please recheck your argument (--with-$ext=$options{$ext}) and build.${Arch}/bundled.log.
You can also disable it by specifying --without-$ext instead.
EOF
      }
      $options{$ext}=".none.";
   }
   print "\n";
}

# save the list of enabled bundled extensions in proper order for the make process
$BundledExts = join(" ",grep { $ext_survived{$_} } @ext_ordered);

$warning .= <<"EOF" unless exists $ext_survived{"cdd"};

WARNING: The bundled extension for cdd was either disabled or failed to configure.
         Running polymake without cdd is discouraged and unsupported!
         Please recheck your configuration (and build.${Arch}/bundled.log).
EOF

print "* If you want to change the configuration of bundled extensions please ",
      defined($LOG) && "see $BuildDir/bundled.log and ",
      "try configure --help.\n";

if (defined $LOG) {
   close $LOG;
} else {
   # remove an outdated logfile to avoid confusions
   unlink $LOGfile;
}

# create the core build directories
create_build_dir("$BuildDir/$_") for "lib/core", glob("{apps,staticlib}/*");

# write the core configuration file
open CONF, ">$BuildDir/conf.make";

# preserve a copy of the command line for later reference
print CONF "# last configured with:\n# $TOP/configure";
if (defined (my $with_perl=delete $options{perl})) {
   print CONF " PERL=$with_perl";
}
write_config_command_line(\*CONF, \%options, \%allowed_with, \%vars, \%ext_failed);
print CONF "\n\n";

write_conf_vars(__PACKAGE__, \*CONF);

print CONF <<".";
RANLIB=$Config::Config{ranlib}
SO=$Config::Config{dlext}
export INSTALL_PL=$TOP/support/install.pl
DESTDIR=
export FinalInstallTop := \${InstallTop}
export FinalInstallArch := \${InstallArch}
InstallTop := \${DESTDIR}\${InstallTop}
InstallArch := \${DESTDIR}\${InstallArch}
InstallBin := \${DESTDIR}\${InstallBin}
InstallInc := \${DESTDIR}\${InstallInc}
InstallLib := \${DESTDIR}\${InstallLib}
InstallDoc := \${DESTDIR}\${InstallDoc}
InSourceTree := y
.
close CONF;

# create build directories for successfully configured bundled extensions
foreach my $ext (keys %ext_survived) {
   create_bundled_extension_build_dir("bundled/$ext", $BuildDir, "Polymake::Bundled::$ext", $ext_requires{$ext} ? (map { "bundled/$_" } @{$ext_requires{$ext}}) : ());
}

# delete build directories for disabled extensions, if any
if (keys %ext_survived) {
   foreach my $d (glob "$BuildDir/bundled/*") {
      exists $ext_survived{($d =~ $Polymake::filename_re)[0]}
        or File::Path::rmtree($d);
   }
} else {
   rmdir "$BuildDir/bundled";
}

print <<"EOF";

* Configuration successful. You should run 'make' now to build polymake. 
* If you have a multicore CPU use e.g. 'make -j2' to speed things up by using two parallel compile processes.
EOF
print $warning if defined $warning;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
