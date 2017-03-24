#  Copyright (c) 1997-2016
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
   if ($] < 5.016) {
      print STDERR <<".";
polymake requires perl version not lower than 5.16;
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

my (%options, %vars, %allowed_options, %allowed_with, %allowed_flags, %allowed_vars);

# expected options for the core script
@allowed_options{ qw( prefix exec-prefix bindir includedir libdir libexecdir datadir docdir build ) }=();
@allowed_with{ qw( gmp mpfr libxml2 boost permlib toolchain ) }=();
@allowed_flags{ qw( prereq callable native openmp libcxx ) }=();
@allowed_vars{ qw( CC CFLAGS CXX CXXFLAGS CXXOPT CXXDEBUG LDFLAGS LIBS PERL ) }=();

if ($^O eq "darwin") {
  @allowed_with{ qw( fink readline ) }=();
}

# check for --repeat option: must happen early enough, before any other digging in @ARGV
try_restore_config_command_line(\@ARGV);

my (@ext, @ext_disabled, %ext_with_config, %ext_requires, %ext_requires_opt, %ext_conflicts, %ext_failed, %ext_bad);

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

my @prereq_sections=qw(REQUIRE REQUIRE_OPT);

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

         my @prereq;
         foreach my $is_optional (0..1) {
            if (my $section=$sections{$prereq_sections[$is_optional]}) {
               foreach ($section =~ /(\S+)/g) {
                  if (/^bundled:(\w+)$/) {
                     push @prereq, $1;
                     $ext_requires_opt{$ext}->{$1}=$is_optional;
                  } else {
                     die "invalid reference to a prerequisite extension: $_\n"
                  }
               }
            }
         }
         $ext_requires{$ext}=\@prereq;

         my @confl;
         if ($sections{CONFLICT}) {
            @confl=map { /^bundled:(\w+)$/ ? $1 : die "invalid reference to a conflicting extension: $_\n" } $sections{CONFLICT} =~ /(\S+)/g;
         }
         $ext_conflicts{$ext}=\@confl;
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
  --without-native    build without "-march=native" flag to allow execution on different CPU types
                      (passing one of -m{cpu,arch,tune} via CFLAGS or CXXFLAGS also disables "-march=native")
.
   }
   print STDERR <<'.';

 Build dependences:

  --with-toolchain=PATH path to a full GCC or LLVM (including clang and libc++) installation
                      (sets CC, CXX, CXXFLAGS, LDFLAGS, LIBS and --with-libcxx accordingly)

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

  --with-boost=PATH   installation path of boost library, if non-standard

  --with-permlib=PATH installation path of permlib headers, if non-standard,
                      polymake uses the bundled permlib by default or if 'bundled' is given.

 Bundled extensions:
.
   foreach my $ext (sort keys %ext_with_config) {
      no strict 'refs';
      print STDERR "\n  --without-$ext  disable the bundled extension $ext completely\n";
      &{"Polymake::Bundled::$ext\::usage"}();
      print STDERR "\n";
   }

   print STDERR <<".";


 Other options:

  --with-libcxx       build against the libc++ library instead of the GNU libstdc++, useful when
                      building with LLVM/Clang. (sets -stdlib=libc++, -lc++ and -lc++abi)
                      see also --with-toolchain when using a custom LLVM installation

  --without-callable  don't build polymake callable library
                      (in the case of custom perl installations lacking libperl shared library)

  --without-prereq    don't check for availability of required libraries and perl modules
                      (to be used from within package management systems like RPM,
                       having own mechanisms for providing all prerequisites.)

  --without-openmp    disable OpenMP support for libnormaliz and to_simplex

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
   $arg = "--with-libcxx" if ($arg eq "--with-libc++");
   if ($arg eq "--help") {
      usage();
      exit(0);
   } elsif (my ($with, $out, $name, $value)= $arg =~ /^--(with(out)?-)?([-\w]+)(?:=(.*))?$/) {
      if ($with and exists $allowed_flags{$name} and !defined($value)) {
         if (defined($out)) {
            $options{$name}=".none.";
         } else {
            $options{$name}=".true.";
         }
         next;
      } elsif ($with ? exists $allowed_with{$name}
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

if ($options{toolchain}) {
   if (-x $options{toolchain}."/bin/clang++") {
      $CXX ||= $options{toolchain}."/bin/clang++";
      $CC  ||= $options{toolchain}."/bin/clang";
   } elsif (-x $options{toolchain}."/bin/g++") {
      $CXX ||= $options{toolchain}."/bin/g++";
      $CC  ||= $options{toolchain}."/bin/gcc";
   } else {
      die <<"."
Unsupported toolchain given, only GCC or LLVM/Clang are supported.
Make sure $options{toolchain}/bin/clang++ or $options{toolchain}/bin/g++ exists and is executable.
.
   }
}

my $absCXX=check_program($CXX, "g++", "c++", "icpc", "clang++")
  or die "no supported C++ compiler found; please reconfigure with CXX=name\n";

while (-l $absCXX) {
   $absCXX=readlink $absCXX;
}
$CCache= $absCXX =~ /\bccache$/;

my $build_error = build_test_program(<<".");
#include <iostream>
int main() {
#if defined(__APPLE__)
   std::cout << "apple";
#endif
#if defined(__clang__)
   std::cout << "clang " << __clang_major__ << '.' << __clang_minor__ << '.' << __clang_patchlevel__ << std::endl;
   return 0;
#elif defined(__GNUC__)
   std::cout << "gcc " << __GNUC__ << '.' << __GNUC_MINOR__ << '.' << __GNUC_PATCHLEVEL__ << std::endl;
   return 0;
#elif defined(__INTEL_COMPILER)
   std::cout << "icc " << __INTEL_COMPILER << std::endl;
   return 0;
#else
   return 1;
#endif
}
.
if ($? != 0) {
   die "C++ compiler $CXX could not compile a test program for version recognition:\n",
       $build_error,
       "\nPlease investigate and reconfigure with CXX=<appropriate C++ compiler>\n";
} else {
   local $_=run_test_program();  chomp;
   if (/^(apple)?clang /) {
      $CLANGversion=$';
      $AppleClang = "true" if $1 eq "apple";
      if (defined($AppleClang) and v_cmp($CLANGversion, "5.1") < 0) {
         die "C++ compiler $CXX is from Xcode $CLANGversion, while the minimal required Xcode version is 5.1\n";
      } elsif (!defined($AppleClang) and v_cmp($CLANGversion, "3.4") < 0) {
         die "C++ compiler $CXX says its version is $CLANGversion, while the minimal required version is 3.4\n";
      }

   } elsif (/icc (\d+)(\d\d)$/) {
      $ICCversion="$1.$2";
      if (v_cmp($ICCversion, "16.0") < 0) {
        die "C++ compiler $CXX says its version is $ICCversion, while the minimal required version is 16.0\n";
      }
      # untested but since version 16 most of C++14 should be supported
      print "warning: probably unsupported C++ compiler Intel $ICCversion.\n";
   } elsif (/(apple)?gcc /) {
      $GCCversion=$';
      # decide whether we use the gcc provided by apple, in that case we need the -arch flags, otherwise we probably don't
      $NeedsArchFlag=defined($1);
      if (v_cmp($GCCversion, "5.1") < 0) {
         die "C++ compiler $CXX says its version is $GCCversion, while the minimal required version is 5.1\n";
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
if (defined($GCCversion) or defined($CLANGversion)) {
   print "ok ($CXX is ",
            defined($GCCversion) ?
               "GCC $GCCversion" :
               defined($CLANGversion) ?
                  (defined($AppleClang) ?
                     "Apple CLANG (roughly ".clang_ver($CLANGversion).") from Xcode $CLANGversion" :
                     "CLANG $CLANGversion")
                  : "unknown", ")\n";
}

$PERL     =$vars{PERL}     || $^X;
$CXXOPT   =$vars{CXXOPT}   || "-O3";
$CXXDEBUG =$vars{CXXDEBUG} || "";
$Cflags   =$vars{CFLAGS}   || "";
$CXXflags =$vars{CXXFLAGS} || $Cflags;
$LDflags  =$vars{LDFLAGS}  || "";
$Libs     =$vars{LIBS}     || "";

$LDsharedFlags=$Config::Config{lddlflags};
$LDcallableFlags= $options{callable} eq ".none." ? "none" : "$LDsharedFlags $Config::Config{ldflags}";

print "checking C++ library ... ";

if ($options{toolchain}) {
   my $libdir;
   # CC/CXX was set earlier, the else case already died at that point
   if (-x $options{toolchain}."/bin/clang++") {
      $libdir = get_libdir($options{toolchain},"clang");
      $options{libcxx} = ".true.";
   } elsif (-x $options{toolchain}."/bin/g++") {
      $libdir = get_libdir($options{toolchain},"gcc_s");
   }
   $Cflags   .= " -I$options{toolchain}/include";
   $CXXflags .= " -I$options{toolchain}/include";
   $LDflags  .= " -L$libdir -Wl,-rpath,$libdir";
}

if ($options{libcxx} eq ".true.") {
   $CXXflags .= " -stdlib=libc++";
   $LDflags  .= " -stdlib=libc++";
   $Libs     .= " -lc++";
   $Libs     .= " -lc++abi" if ($^O eq "linux");
}

# All polymake releases after 3.0 require C++14,
# but if someone explicitly requests another standard version go along with it,
# if it is too old we will generate a warning / an error later on.
if ($CXXflags !~ /(?:^|\s)-std=/) {
   if (defined($CLANGversion) and v_cmp(clang_ver($CLANGversion), "3.5") < 0) {
      $CXXflags .= ' -std=c++1y';
   } else {
      $CXXflags .= ' -std=c++14';
   }
}

my $build_error = build_test_program(<<".");
#include <cstddef>
#include <cstdio>
#include <iostream>
int main() {
   std::cout << "cplusplus " << __cplusplus << std::endl;
#if defined(_LIBCPP_VERSION)
   std::cout << "libc++ " << _LIBCPP_VERSION << std::endl;
   return 0;
#elif defined(__GLIBCXX__)
   std::cout << "GNU libstdc++ " << __GLIBCXX__ << std::endl;
   return 0;
#elif defined(__INTEL_CXXLIB_ICC)
   std::cout << "Intel " << __INTEL_CXXLIB_ICC << std::endl;
   return 0;
#else
   return 1;
#endif
}
.
if ($? != 0) {
   die "C++ compiler $CXX could not compile a test program for C++ library recognition:\n",
       $build_error,
       "\nPlease investigate and reconfigure\n";
} else {
   local $_=run_test_program();  chomp;
   my ($cppver, $cpplib) = $_ =~ m/^cplusplus (\d+)\n(.+)$/;

   unless ($cppver >= 201402 or
           $cppver >= 201305 and
               defined($CLANGversion) and v_cmp(clang_ver($CLANGversion), "3.5") < 0
          ) {
      # C++ standard older than C++14 wont work (C++1y from clang 3.4 is also ok)
      die "C++ standard version ($cppver) too old, C++14 or later is required. Please omit any '-std=' options.\n";
   }

   # see http://sourceforge.net/p/predef/wiki/Libraries/
   if ($cpplib =~ /^GNU libstdc\+\+ /) {
      # this is some more or less useful date
      my $stdlibversion=$';
      $CPPStd = $cppver;
      print "ok ($cpplib, C++ $CPPStd)\n";
   } elsif ($cpplib =~ /^libc\+\+ /) {
      $CPPStd = $cppver;
      if ($Libs !~ /-lc\+\+/) {
         $Libs .= " -lc++";
         $Libs .= " -lc++abi" if ($^O eq "linux");
      }
      print "ok ($cpplib, C++ $CPPStd)\n";
   } elsif ($cpplib =~ /^Intel /) {
      my $intelcxxversion=$';
      $CPPStd = $cppver;
      print "warning: probably unsupported C++ library Intel $intelcxxversion.\n";
      # not tested in a long time
   } else {
      die "Unsupported C++ library, use -stdlib to specify libstdc++ or libc++.\n";
   }

   if ($LDflags !~ /-stdlib=/ and $CXXflags =~ /(-stdlib=\S+)/) {
      $LDflags .= " $1";
   }

}


my $GMP=$options{gmp};
my $boost_path = $options{boost};

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
      if ( !defined($boost_path) ) {
         print "checking fink boost installation ... ";
         ($boost_path) = <"$FinkBase/opt/boost-*/include/boost">;
         ($boost_path) = $boost_path =~ /(.*)include\/boost/;
         unless (-d $boost_path ) {
            die "Fink package boost-<some version>_nopython seems to be missing.  Further configuration is not possible.\n",
            "Please install the package boost-<some version>_nopython and repeat the configure run.\n";
         }
         print "ok\n";
      }
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
         if ( $Version =~ /^10.5/ ) { 
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
   # no arch flags set
   unless ($options{native} eq ".none." or "$Cflags $CXXflags" =~ /(?:^|\s)-m(?:arch|tune|cpu)=/) {
      $Cflags .= " -march=native";
      $CXXflags .= " -march=native";
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
   $CXXDEBUG =~ /(?:^|\s)-g\d?/ or $CXXDEBUG .= " -g";
   # TODO: remove -fno-strict-aliasing when the core library is free from reintepret_casts
   $CXXflags .= " -ftemplate-depth-200 -fno-strict-aliasing -Wno-parentheses -fwrapv";
   if ($options{openmp} ne ".none.") {
      $CXXflags .= " -fopenmp";
      $LDflags .= " -fopenmp";
   }
   # external libraries might be somehow dirtier
   $CflagsSuppressWarnings="-Wno-uninitialized -Wno-unused -Wno-parentheses -Wno-unused-but-set-variable -Wno-enum-compare -Wno-sign-compare -Wno-switch -Wno-format -Wno-write-strings";
   # gcc-specific flags
   if (v_cmp($GCCversion, "6.3.0") == 0) {
       $CXXflags .= " -Wno-maybe-uninitialized";
   }

} elsif (defined($ICCversion)) {
   $CsharedFlags="-fPIC";
   $CXXDEBUG .= " -g -Ob0";
   $CXXflags .= " -wd193,383,304,981,1419,279,810,171,1418,488,1572,561";
   $Cflags .= " -wd193,1572,561";

} elsif (defined($CLANGversion)) {
   $CsharedFlags="-fPIC";
   $CXXDEBUG =~ /(?:^|\s)-g\d?/ or $CXXDEBUG .= " -g";
   $CXXflags .= " -Wno-logical-op-parentheses -Wno-shift-op-parentheses -Wno-mismatched-tags";
   $CflagsSuppressWarnings="-Wno-uninitialized -Wno-unused -Wno-unused-variable -Wno-enum-compare -Wno-sign-compare -Wno-switch -Wno-format -Wno-write-strings -Wno-empty-body -Wno-logical-op-parentheses -Wno-shift-op-parentheses -Wno-dangling-else";
   if (v_cmp(clang_ver($CLANGversion), "3.6") >= 0) {
      $CXXflags .= " -Wno-unused-local-typedef";
   }

   # verify openmp support which is available starting with 3.7 but depends on the installation,
   # but 3.7 seems to crash when compiling libnormaliz so we skip that version
   # version 3.8 is tested to work with openmp
   if (v_cmp(clang_ver($CLANGversion), "3.8") >= 0 && $options{openmp} ne ".none.") {
      my $ompflag = "-fopenmp";
      my $build_error=build_test_program(<<'---', CXXflags => "$ompflag", LDflags => "$ompflag");
#include <stdio.h>
#include <omp.h>

int main() {
#pragma omp parallel
    printf("Hello from thread %d, nthreads %d\n", omp_get_thread_num(), omp_get_num_threads());
    return 0;
}
---
      if ($? == 0) {
         my $openmpout = run_test_program();
         my ($nthr) = $openmpout =~ /Hello from thread \d+, nthreads (\d+)/;
         if ($? == 0) {
            $CXXflags .= " $ompflag";
            $LDflags .= " $ompflag";
         }
      }
   }
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

if (defined($boost_path)) {
   $boost_path .= '/include' if (-d "$boost_path/include/boost");
   $CXXflags .= " -I$boost_path";
}

my $permlib_path = $options{permlib};
if (defined($permlib_path) and $permlib_path ne "bundled") {
   $permlib_path .= '/include' if (-d "$permlib_path/include/permlib");
   $CXXflags .= " -I$permlib_path";
} else {
   if (!-e "external/permlib/include/permlib/permlib_api.h") {
      # bundled permlib seems to be missing
      # will try to use a systemwide permlib as fallback unless explicitly requested
      die "Bundled PermLib was requested but is missing in the distribution.\n" if $permlib_path eq "bundled";
      $permlib_path = "";
   } else {
      $ExternalHeaders .= " permlib";
   }
}

if ($options{prereq} ne ".none.") {
   print "checking gmp installation ... ";
   # check GMP installation
   my $build_error=build_test_program(<<'---', Libs => "$ARCHFLAGS -lgmp");
#include <cstddef>
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
#include <cstddef>
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
      my $build_error=build_test_program(<<'---', CXXflags => (defined($CLANGversion) && "-Wno-reserved-user-defined-literal").`$PERL -MExtUtils::Embed -e ccopts`, LDflags => "$ARCHFLAGS $perlldflags" );
#include <EXTERN.h>
#include <perl.h>

static PerlInterpreter *my_perl;

int main(int argc, char **argv, char **env)
{
   const char* embedding[] = { "", "-e", "print $];", 0 };
   PERL_SYS_INIT3(&argc, &argv, &env);
   my_perl = perl_alloc();
   perl_construct(my_perl);
   PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
   perl_parse(my_perl, NULL, 3, (char**)embedding, (char **)NULL);
   perl_run(my_perl);
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

   print "checking boost installation ... ";
   # check boost installation
   $build_error=build_test_program(<<'---');
#include <boost/shared_ptr.hpp>
#include <boost/iterator/counting_iterator.hpp>
int main() {
  return 0;
}
---
   if ($?) {
      die "Could not compile a test program checking for boost library.\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "or missing at all.\n",
          "The complete error log follows:\n$build_error\n\n",
          "Please install the library and specify its location using --with-boost option, if needed.\n";
   }
   print "ok", defined($boost_path) && " ($boost_path)", "\n";

   # this includes the case $permlib_path eq "" where we try to use a systemwide installation
   if (defined($permlib_path) and $permlib_path ne "bundled") {
      print "checking permlib installation ... ";
      # check permlib installation
      $build_error=build_test_program(<<'---');
#include <permlib/permlib_api.h>
int main() {
   // from permlibs api example
	using namespace permlib;
	const unsigned long n = 10;
	// group generators
	std::list<Permutation::ptr> groupGenerators;
	Permutation::ptr gen1(new Permutation(n, std::string("1 3 5 7 9 10 2 4 6 8")));
	groupGenerators.push_back(gen1);
	Permutation::ptr gen2(new Permutation(n, std::string("1 5")));
	groupGenerators.push_back(gen2);
	boost::shared_ptr<PermutationGroup> group = construct(n, groupGenerators.begin(), groupGenerators.end());
   return 0;
}
---
      if ($?) {
         die "Could not compile a test program checking for permlib.\n",
             "The most probable reasons are that the library is installed at a non-standard location,\n",
             "or missing at all.\n",
             "The complete error log follows:\n$build_error\n\n",
             "Please install the library and specify its location using --with-permlib option, if needed.\n",
             "Or leave out the --with-permlib option to use the bundled version.\n";
      }
      print "ok", " (", $permlib_path eq "" ? "system" : ($permlib_path//"bundled"),")\n";
   }


   # check the prerequisite perl packages
   if (defined $FinkBase) { 
      lib->import("$FinkBase/lib/perl5");
   }
   my @warned;
   foreach (qw(XML::Writer XML::LibXML XML::LibXSLT Term::ReadKey Term::ReadLine)) {
      my $pkg=$_;
      print "checking perl module $pkg ... ";
      my $warn;
      local $SIG{__WARN__}=sub { $warn=shift };
      eval "require $pkg;";
      if (/^Term::ReadLine/) {
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
   foreach my $ext (sort keys %ext_unordered) {
      unless (grep { $ext_unordered{$_} } @{$ext_requires{$ext}}
                or
              grep { $ext_unordered{$_} } @{$ext_conflicts{$ext}}) {
         push @ext_ordered, $ext;
         delete $ext_unordered{$ext};
      }
   }
   last if $progress==@ext_ordered;
}

while ((my $ext, undef)=each %ext_unordered) {
   print "bundled extension $ext ... disabled because of cyclic dependencies on other extension(s): ",
         join(", ", (grep { $ext_unordered{$_} } @{$ext_requires{$ext}}),
                    (grep { $ext_unordered{$_} } @{$ext_conflicts{$ext}})), "\n";
}

# configure the bundled extensions
my %ext_survived;
foreach my $ext (@ext_ordered) {
   print "bundled extension $ext ... ";
   my $enable=1;

   my @prereq;
   foreach my $prereq (@{$ext_requires{$ext}}) {
      if ($ext_survived{$prereq}) {
         push @prereq, $prereq;
      } elsif (!$ext_requires_opt{$ext}->{$prereq}) {
         if (-d "bundled/$prereq") {
            print "disabled because of unsatisfied prerequisite: $prereq";
         } else {
            print "disabled because of unknown/missing prerequisite: $prereq";
         }
         $enable=0; last;
      }
   }
   if ($enable) {
      foreach my $conflict (@{$ext_conflicts{$ext}}) {
         if ($ext_survived{$conflict}) {
            print "disabled because of conflict with other extension: $conflict";
            $enable=0; last;
         }
      }
   }

   if ($enable) {
      eval { check_extension_source_conflicts("bundled/$ext", ".", (map { "bundled/$_" } @prereq)) };
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
      $ext_requires{$ext}=\@prereq;
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
         Running polymake without cdd is deprecated and not supported!
         Please recheck your configuration (and build.${Arch}/bundled.log).
EOF

$warning .= <<"EOF" unless (exists $ext_survived{"nauty"} or exists $ext_survived{"bliss"});

WARNING: The bundled extensions for bliss and nauty were both either disabled or failed to configure.
         Running polymake without a tool for graph-isomorphism tests is not supported!
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
   create_bundled_extension_build_dir("bundled/$ext", $BuildDir, "Polymake::Bundled::$ext", (map { "bundled/$_" } @{$ext_requires{$ext}}));
}

# delete build directories for disabled extensions, if any
if (keys %ext_survived) {
   foreach my $d (glob "$BuildDir/bundled/*") {
      $ext_survived{($d =~ $Polymake::filename_re)[0]}
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
