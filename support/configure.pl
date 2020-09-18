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

my ($NeedsArchFlag);

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

use Polymake::ConfigureStandalone;

$Shell=new FakeNoShell;
# we can't load regex.pl without compiled support for namespaces
$filename_re=qr{ ([^/]+) $ }x;
$directory_of_cmd_re=qr{ ^(.*) / [^/]+ (?: $ | \s)}x;

package Polymake::Configure;

@ARGV=grep { !/^PERL=/ } @ARGV;

my (%options, %vars, %allowed_options, %allowed_with, %allowed_flags, %allowed_vars);

# expected options for the core script
@allowed_options{ qw( prefix exec-prefix bindir includedir libdir libexecdir datadir docdir build build-modes ) }=();
@allowed_with{ qw( gmp mpfr boost permlib toolchain ccache ccwrapper ) }=();
@allowed_flags{ qw( prereq callable native openmp libcxx ) }=();
@allowed_vars{ qw( CC CFLAGS CXX CXXFLAGS CXXOPT CXXDEBUG LDFLAGS LIBS Arch DESTDIR ) }=();

if ($^O eq "darwin") {
  @allowed_with{ qw( fink brew ) }=();
}

my (@ext, @ext_disabled, %ext_with_config, %ext_requires, %ext_requires_opt, %ext_conflicts, %ext_failed, %ext_bad,
    @ext_ordered, %ext_survived);

my $repeating_config = early_parse_command_line();
my $root = Cwd::getcwd;
$Polymake::DeveloperMode = -d "$root/testscenarios";

my $BuildDir = $options{build} ? "build.$options{build}" : "build";
my $perlxpath = "perlx/$Config::Config{version}/$Config::Config{archname}";

load_enabled_bundled_extensions();
parse_command_line(\@ARGV, $repeating_config);
construct_paths();
determine_cxx_compiler();
determine_cxx_library();
if ($^O eq "darwin") {
   check_macos_package_manager();
}
determine_architecture();
collect_compiler_specific_options();
check_build_modes();
if ($options{'alt-perl'}) {
   create_alt_perl_configuration();
   exit(0);
}

my %store_versions;
locate_gmp_and_mpfr_libraries();
locate_boost_headers();
locate_permlib();

# there is no independent package for TOSimplex and Minimball, always pick the bundled headers
$ExternalHeaders .= " TOSimplex Miniball";


if ($options{prereq} ne ".none.") {
   check_prerequisite_perl_packages();
   if ($options{callable} ne ".none.") {
      check_libperl();
   }
}

# create the main build directory
File::Path::make_path("$BuildDir/$perlxpath");

my $BundledLOG;
my $BundledLOGfile="$BuildDir/bundled.log";

print "\nConfiguring bundled extensions:\n";
foreach my $ext (@ext_disabled) {
   print "bundled extension $ext ... disabled by command-line\n";
}
while (my ($ext, $err)=each %ext_bad) {
   print "bundled extension $ext ... disabled because of a fatal error\n";
   bundled_ext_error_msg($ext, $err);
}

order_enabled_bundled_extensions();
configure_enabled_bundled_extensions();
finalize_compiler_flags();
find_ninja();

File::Path::make_path($BuildDir);
write_configuration_file("$BuildDir/config.ninja");
write_perl_specific_configuration_file("$BuildDir/$perlxpath/config.ninja");
create_build_trees($root, "$root/$BuildDir", perlxpath => $perlxpath);
delete_old_and_disabled_build_trees();

if ($Polymake::DeveloperMode) {
   print <<"---";

* Configuration successful.
* You can run  'ninja -C $BuildDir/Opt'  now to build polymake with full optimization
* or  'ninja -C $BuildDir/Debug'  to build polymake with debugging support.
* To install an immutable snapshot of polymake at $InstallTop,
* run  'ninja -C $BuildDir/Opt install'.
*
* On systems with moderate amount of memory like laptops it is advisable to
* restrict the number of parallel build processes below the default ninja limit;
*   'ninja -l <NUMBERofCPUS>'  is a good approximation to start with.
* If the system still feels overloaded, take a lower hard job limit
*   'ninja -j <N>'  instead.
---
} else {
   print <<"---";

* Configuration successful.
* You can run 'ninja -C $BuildDir/Opt install' now to build and install polymake.
---
}

print $warning if defined $warning;
exit(0);

### end of main body ###

sub usage {

   print STDERR <<'---';
This is a script preparing the build and installation of polymake.
It tries to mimic well-known GNU auto-configuration scripts:

./configure [options] [VARIABLE=value ...]

Without any options, or with a sole --build option, it reuses the options specified
in the most recent run, if any; this is mostly useful for developers after an update
of the codebase.

Allowed options (and their default values) are:

 Mulitple build trees:

  --build SUFFIX      (none)  create or update one of several build directories,
                              named "build.SUFFIX".  This is primarily for developers
                              who want to test various configurations from a single code base,
                              or for packagers.
                              By default, a single "build" directory is created.

  --builddir PATH     An alternative way of specifying an existing build directory by its full path

 Installation directories:

  --prefix=PATH       ( /usr/local )                   root of the installation tree
  --exec-prefix=PATH  ( ${prefix} )                    root of the architecture-dependent tree
  --bindir=PATH       ( ${exec-prefix}/bin )           main polymake script
  --includedir=PATH   ( ${prefix}/include )            include files
  --libdir=PATH       ( ${exec-prefix}/lib )           callable library
  --libexecdir=PATH   ( ${exec-prefix}/lib/polymake )  dynamic modules loaded at the runtime
  --datadir=PATH      ( ${prefix}/share/polymake )     rules and other architecture-independent files
  --docdir=PATH       ( ${datadir}/doc )               automatically generated documentation files

 Build dependences:

  --with-toolchain=PATH      path to a full GCC or LLVM (including clang and libc++) installation;
                               overrides CC, CXX, CXXFLAGS, LDFLAGS, LIBS and --with-libcxx
  --with-ccache=PATH         use the given ccache program for compilation
  --with-ccwrapper=COMMAND   prepend the given command to every compiler, linker, and ar invocation

---
   $^O eq "darwin" and
   print STDERR <<'---';
  --with-fink=PATH         fink installation directory. You can pass "default" as argument if fink is installed into its default location ( /sw )
  --with-brew=PATH         homebrew installation directory. You can pass "default" as argument if brew is installed into its default location ( /usr/local )
.
---
   print STDERR <<"---";
  --with-gmp=PATH     ( /usr ) GNU MultiPrecision library installation directory
  --with-mpfr=PATH    ( =gmp-path ) GNU Multiple Precision Floating-point Reliable library installation directory

  --with-boost=PATH   installation path of boost library, if non-standard

  --with-permlib=PATH installation path of permlib headers, if non-standard,
                      polymake uses the bundled permlib by default or if 'bundled' is given.

 Bundled extensions:
---
   foreach my $ext (sort keys %ext_with_config) {
      no strict 'refs';
      print STDERR "\n  --without-$ext  disable the bundled extension $ext completely\n";
      &{"Polymake::Bundled::$ext\::usage"}();
      print STDERR "\n";
   }

   print STDERR <<'---';


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

---
   $^O ne "darwin" and
   print STDERR <<'---'; 
  --without-native    build without "-march=native" flag to allow execution on different CPU types
                      (passing one of -m{cpu,arch,tune} via CFLAGS or CXXFLAGS also disables "-march=native")

---
   print STDERR <<'---';
  --build-modes=MODE,MODE...
                      prepare for builds in non-standard modes; currently supported modes are:
                        Cov  generated code gathers coverage statistics
                        San  sanitized build detecting address violations and undefined behavior
                      Standard modes Opt and Debug are always supported and must not be listed here.

---
   $Polymake::DeveloperMode and
   print STDERR <<"---";
  --clone=PATH/config.ninja
                      Import the options stored in the given configuration file.
                      More options may follow; simple options may override the imported ones,
                      but no provisions are made for detecting more subtle contradictions.
                      Use at own risk.

---
   print STDERR <<"---";
Allowed variables are:

   Arch=       An abbreviation for the system architecture.
---
   $^O eq "darwin" and
   print STDERR <<'---';
               Allowed values are "i386" for 32bit or "x86_64" for 64bit.
---
   print STDERR <<'---';
               Default value is derived from the current system setup.
               Users' custom variables may have distinct values for different architectures.

   CC=cc       C compiler executable
   CFLAGS=     C compiler options
   CXX=c++     C++ compiler executable
   CXXFLAGS=   C++ compiler options
   CXXOPT=     C/C++ compiler optimization level (defaults to highest possible)
   LDFLAGS=    linker options, including linker choice (-fuse-ld)
   LIBS=       additional libraries to be linked with
   PERL=perl   perl executable

Options incompatible with any others:

   --defaults    perform a configuration assuming all default settings,
                 ignoring any existing configured builds

---
   $Polymake::DeveloperMode and
   print STDERR <<"---";
   --alt-perl=TAG    create a configuration for a non-standard perl interpreter;
                     the ninja files will be called build.TAG.ninja

---
   print STDERR <<"---";
For detailed installation instructions, please refer to the polymake Wiki site:
  $Wiki/howto/install

---
}

###################################################################################
# * sieve out disabled extensions so that their configuration scripts are not loaded
# * process --build, --builddir, --build-modes, and --defaults
# * restore options from previous runs if desired
sub early_parse_command_line {
   my (@other_args, $enforce_defaults);
   while (defined(my $arg = shift @ARGV)) {
      if ($arg eq "--defaults") {
         $enforce_defaults = 1;
      } elsif (my ($flag, $value) = $arg =~ /^--(build(?:-modes|dir)?|alt-perl)(?:=(\S+))?$/) {
         $value //= @ARGV && $ARGV[0] =~ /^\w/ ? shift @ARGV : die "option --$flag requires a value\n";
         $options{$flag} = $value;
      } elsif ($arg =~ /^--without-(\w+)$/  &&  -d "bundled/$1") {
         push @ext_disabled, $1;
         $options{$1} = ".none.";
      } else {
         push @other_args, $arg;
      }
   }

   if (my $builddir = delete $options{'builddir'}) {
      if ($options{build}) {
         die "--build and --builddir may not be specified together\n"
      }
      if (-f "$builddir/config.ninja" && $builddir =~ s{/build(?:\.([^/]+))?(?:/\w+)?$}{}) {
         chdir $builddir or die "can't change into $builddir: $!\n";
         if (defined($1)) {
            $options{build} = $1;
         }
      } else {
         die "$builddir does not look like a configured polymake build directory\n";
      }
   }

   if ($options{'alt-perl'}) {
      if ($enforce_defaults || @other_args || @ext_disabled) {
         die "option --alt-perl can't be combined with any other configuration option\n";
      }
   }

   if ($enforce_defaults) {
      if (@other_args || @ext_disabled) {
         die "option --defaults can't be combined with any other configuration option\n";
      }

   } elsif (@other_args) {
      @ARGV=splice @other_args;

   } elsif (!@ext_disabled) {
      # no configuration options means we should reuse the options from the previous run

      if ($options{build}) {
         # want to keep several architectures in parallel
         if ($^O eq "darwin") {
            $options{build} =~ s/^(?!darwin\.)/darwin./;
         }
         if (-f "build.$options{build}/config.ninja") {
            retrieve_config_command_line("build.$options{build}/config.ninja", \@ARGV);
            return 1;
         } elsif (-f "build.$options{build}/conf.make") {
            retrieve_config_command_line("build.$options{build}/conf.make", \@ARGV);
            return 1;
         }
      } elsif (-f "build/config.ninja") {
         retrieve_config_command_line("build/config.ninja", \@ARGV);
         return 1;
      } elsif (my @ninja_configs=glob("build.*/config.ninja")) {
         die "polymake has been configured for several build architectures: ", (map { m{/build\.([^/]+)/} } @ninja_configs),
              "\nPlease specify the desired one with --build option or enforce default configuration with --defaults option.\n";
      } elsif (my @old_configs=glob("build.*/conf.make")) {
         if (@old_configs==1) {
            my ($old_build)=$old_configs[0] =~ m{(build.[^/])};
            retrieve_config_command_line($old_configs[0], \@ARGV);
            rename $old_build, "build";
            return 1;
         } else {
            die "polymake has been configured for several build architectures: ", (map { m{/build\.([^/]+)/} } @old_configs),
                "\nPlease specify the one to be migrated to ninja build system with --build option.\n",
                "If you want to preserve just one of them, delete all other build.XXX directories and re-run the configure command without any options.\n";
         }
      }
   }
   0
}

###########################w######################################
sub parse_command_line {
   my ($arglist, $repeating_config, $ignore_unknown)=@_;
   while (defined (my $arg=shift @$arglist)) {
      # trim and allow empty arguments
      $arg =~ s/^\s+//;
      $arg =~ s/\s+$//;
      next if length($arg)==0;
      $arg =~ s/^--with-libc\K\+\+$/xx/;
      if ($arg eq "--help") {
         usage();
         exit(0);
      } elsif (my ($with, $out, $name, $value)= $arg =~ /^--(with(out)?-)?(\w[-\w]+)(?(2)|=(.*))?$/) {
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
                  defined($out) || defined($value) || @$arglist) {
            if (defined($out)) {
               $value=".none.";
            } else {
               $value //= shift(@$arglist);
               $value =~ s{^~/}{$ENV{HOME}/};
               $value =~ s{/+$}{};
            }
            if ($repeating_config && $name =~ /^build/) {
               $options{$name} //= $value;
            } elsif ($value eq "default" && $name ne "fink" && $name ne "brew") {
               delete $options{$name};
            } else {
               $options{$name}=$value;
            }
            next;
         } elsif ($Polymake::DeveloperMode and
                  $arg eq "--clone" && @$arglist  ||
                  $arg =~ s/^--clone=(?=.)//) {
            # used in testscenarios
            my @other_command_line;
            retrieve_config_command_line($arg eq "--clone" ? shift(@$arglist) : $arg, \@other_command_line);
            parse_command_line(\@other_command_line, 0, 1);
            next;
         }
      } elsif ($arg =~ /^([A-Z]\w+)=(.*)/) {
         if (exists $allowed_vars{$1}) {
            $vars{$1}=$2;
            next;
         }
      }
      if (!$ignore_unknown) {
         die "Unrecognized option $arg\nTry ./configure --help for detailed information\n";
      }
   }

   if (!$repeating_config) {
      # inherit some variables which haven't appeared on the command line from the environment
      foreach my $varname (grep { !exists $vars{$_} } keys %allowed_vars) {
         if (defined (my $value=$ENV{$varname})) {
            $vars{$varname}=$value;
         }
      }
   }

   $CXXOPT   =$vars{CXXOPT}   // "-O3";
   $CXXDEBUG =$vars{CXXDEBUG} // "-g";
   $CFLAGS   =$vars{CFLAGS}   // "";
   $CXXFLAGS =$vars{CXXFLAGS} // $CFLAGS;
   $LDFLAGS  =$vars{LDFLAGS}  // "";
   $LIBS     =$vars{LIBS}     // "";
   $DESTDIR  =$vars{DESTDIR};

   $Arch = $vars{Arch} =~ s/^darwin\.//r;
}

#################################################################
sub load_enabled_bundled_extensions {
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

      } elsif (-f (my $ext_config="bundled/$ext/support/configure.pl")) {
         my $err;
         eval <<"---";
         { package Polymake::Bundled::$ext;
           do "./$ext_config";
           unless (\$err=\$\@) {
             allowed_options(\\%allowed_options, \\%allowed_with);
           }
         }
---
         if ($err) {
            $options{$ext}=".none.";
            $ext_bad{$ext}="broken configuration script $ext_config:\n$err";
         } else {
            $ext_with_config{$ext}=1;
         }
      }
   }
}

#####################################################
sub construct_paths {
   my $prefix=$options{prefix} || "/usr/local";
   my $exec_prefix=$options{'exec-prefix'} || $prefix;

   $InstallBin=$options{bindir} || "$exec_prefix/bin";
   $InstallLib=$options{libdir} || "$exec_prefix/lib";
   $InstallTop=$options{datadir} || "$prefix/share/polymake";
   $InstallArch=$options{libexecdir} || "$exec_prefix/lib/polymake";
   $InstallInc=$options{includedir} || "$prefix/include";
}

#####################################################
sub determine_cxx_compiler {
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
         die <<"---"
Unsupported toolchain given, only GCC or LLVM/Clang are supported.
Make sure $options{toolchain}/bin/clang++ or $options{toolchain}/bin/g++ exists and is executable.
---
      }
   }

   check_program($CXX, "g++", "c++", "icpc", "clang++")
     or die "no supported C++ compiler found; please reconfigure with CXX=name\n";

   my $cxx_tell_version=<<"---";
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
---
   my $build_error;

   # try to use gold linker unless another linker has been prescribed by the user
   unless ($^O eq "darwin" or $Config::Config{archname} =~ /^mips/ or "$CXXFLAGS $LDFLAGS" =~ /(?:^|\s)-fuse-ld[=\s]/) {
      $build_error = build_test_program($cxx_tell_version, LDFLAGS => "-fuse-ld=gold");
      if ($?) {
         # did not work out, maybe gold is missing?
         undef $build_error;
      } else {
         $LDFLAGS .= " -fuse-ld=gold";
      }
   }
   $build_error //= build_test_program($cxx_tell_version);

   if ($?) {
      die "C++ compiler $CXX could not compile a test program for version recognition:\n",
          $build_error,
          "\nPlease investigate and reconfigure with CXX=<appropriate C++ compiler>\n";
   }

   local $_=run_test_program();  chomp;
   if (/^(apple)?clang /) {
      if ($1 eq "apple") {
         $XcodeVersion=$';
         $CLANGversion=xcode2clang_version();
      } else {
         $CLANGversion=$';
         if (v_cmp($CLANGversion, "3.4") < 0) {
            die "C++ compiler $CXX says its version is $CLANGversion, while the minimal required version is 3.4\n";
         }
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
              and find_program($CC)) {
         $CC=$Config::Config{cc};
      }
   }
   if (defined($GCCversion) or defined($CLANGversion)) {
      print "ok ($CXX is ",
            defined($GCCversion)
            ? "GCC $GCCversion" :
            defined($XcodeVersion)
            ? "Apple CLANG (roughly $CLANGversion) from Xcode $XcodeVersion"
            : "CLANG $CLANGversion",
            ")\n";
   }
}
#####################################################
sub determine_cxx_library {
   print "checking C++ library ... ";

   if ($options{toolchain}) {
      my $libdir;
      # CC/CXX was set earlier, the else case already died at that point
      if (-x $options{toolchain}."/bin/clang++") {
         $libdir = get_libdir($options{toolchain}, "clang");
         $options{libcxx} = ".true.";
      } elsif (-x $options{toolchain}."/bin/g++") {
         $libdir = get_libdir($options{toolchain}, "gcc_s");
      }
      $CFLAGS   .= " -I$options{toolchain}/include";
      $CXXFLAGS .= " -I$options{toolchain}/include";
      $LDFLAGS  .= " -L$libdir -Wl,-rpath,$libdir";
   }

   if ($options{libcxx} eq ".true.") {
      $CXXFLAGS .= " -stdlib=libc++";
      $LDFLAGS  .= " -stdlib=libc++";
      $LIBS     .= " -lc++";
      $LIBS     .= " -lc++abi" if ($^O eq "linux");
   }

   # All polymake releases after 3.0 require C++14,
   # but if someone explicitly requests another standard version go along with it,
   # if it is too old we will generate a warning / an error later on.
   if ($CXXFLAGS !~ /(?:^|\s)-std=/) {
      if (defined($CLANGversion) and v_cmp($CLANGversion, "3.5") < 0) {
         $CXXFLAGS .= ' -std=c++1y';
      } else {
         $CXXFLAGS .= ' -std=c++14';
      }
   }

   my $build_error = build_test_program(<<"---");
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
---
   if ($?) {
      die <<"---";
C++ compiler $CXX could not compile a test program for C++ library recognition:
$build_error
Please investigate and reconfigure.
---
   }
   local $_=run_test_program();  chomp;
   my ($cppver, $cpplib) = $_ =~ m/^cplusplus (\d+)\n(.+)$/;

   unless ($cppver >= 201402 or
           $cppver >= 201305 and
               defined($CLANGversion) and v_cmp($CLANGversion, "3.5") < 0
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
      if ($LIBS !~ /-lc\+\+/) {
         $LIBS .= " -lc++";
         $LIBS .= " -lc++abi" if ($^O eq "linux");
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

   if ($LDFLAGS !~ /-stdlib=/ and $CXXFLAGS =~ /(-stdlib=\S+)/) {
      $LDFLAGS .= " $1";
   }
}

#####################################################
sub check_fink {
   $FinkBase=$options{fink};
   if ( $FinkBase eq "default" ) {
      # Fink location not specified, look for it at plausible places
      (undef, my ($fink, $error))=find_program_in_path("fink", sub {
         !($FinkBase=$_[0] =~ s|/bin/fink$||r && -f "$FinkBase/etc/fink.conf") && "!"
      });
      if ($fink) {
         if ($error) {
            die <<"---";
Found the fink program at $fink, but the corresponding configuration file is missing;
Please specify the top installation directory using option --with-fink=PATH
---
         }
      } elsif (-f "/sw/etc/fink.conf") {
         $FinkBase="/sw";
      } else {
         die <<"---";
The Fink package system cannot be found at a default location on your computer.
Please refer to $Wiki/howto/mac for installation instructions and other options to install polymake on a Mac.
If you have Fink installed at a non-standard location, please specify it using option --with-fink=PATH
---
      }  
   } elsif (-d $FinkBase) {
      unless (-f "$FinkBase/etc/fink.conf") {
         die "option --with-fink does not point to the fink installation tree: $FinkBase/etc/fink.conf not found\n";
      }
   } elsif (-x $FinkBase && $FinkBase =~ s|/bin/fink$||) {
      unless (-f "$FinkBase/etc/fink.conf") {
         die "option --with-fink points to a broken fink installation: $FinkBase/etc/fink.conf not found\n";
      }
   } else {
      die <<'---';
option --with-fink points to a wrong program: something like /path/bin/fink expected.
If you have renamed the main fink program, please specify the top directory: --with-fink=PATH
---
   }

   if (defined (my $anylib=(glob("$FinkBase/lib/*.dylib"))[0])) {
      my ($fink_arch)= `lipo -info $anylib` =~ /architecture: (\S+)/;
      if (defined $Arch) {
         if ($Arch ne $fink_arch) {
            die "Required architecture $Arch does not match installed Fink architecture $fink_arch\n";
         }
      } else {
         $Arch=$fink_arch;
      }
   } else {
      die "Fink installation seems corrupt: no shared libraries found in $FinkBase/lib\n";
   }

   print "ok ($FinkBase)\n";
}
#####################################################
sub check_brew {
   $BrewBase=$options{brew};
   if ( $BrewBase eq "default" ) {
      $BrewBase = '/usr/local';
      unless (-f "$BrewBase/bin/brew") {
         die "brew installation corrupt: $BrewBase/bin/brew not found\n";
      }
   } else {
      if (-d $BrewBase) {
         unless (-f "$BrewBase/bin/brew") {
            die "option --with-brew does not point to the brew installation tree: $BrewBase/bin/brew not found\n";
         }
      } elsif (-x $BrewBase && $BrewBase =~ s|/bin/brew$||) {
         unless (-f "$BrewBase/bin/brew") {
            die "option --with-brew points to a broken brew installation: $BrewBase/bin/brew not found\n";
         }
      } else {
         die <<'---';
option --with-brew points to a wrong program: something like --with-brew=/path/bin/brew expected.
You can also pass --with-brew=PATH,  where PATH is the base directory of your homebrew installation. 
If you installed into the default location (/usr/local/), then you can also pass -with-brew=default or omit the option completely.
---
      }

      $CFLAGS .= " -I$BrewBase/include";
      $CXXFLAGS .= " -I$BrewBase/include";
      $LDFLAGS .= " -L$BrewBase/lib";
   }
   
   if (defined (my $anylib=(glob("$BrewBase/lib/*.dylib"))[0])) {
      my ($brew_arch)= `lipo -info $anylib` =~ /architecture: (\S+)/;
      if (defined $Arch) {
         if ($Arch ne $brew_arch) {
            die "Required architecture $Arch does not match installed brew architecture $brew_arch\n";
         }
      } else {
         $Arch=$brew_arch;
      }
   } else {
      die "Homebrew installation seems corrupt: no shared libraries found in $BrewBase/lib\n";
   }

   print "ok ($BrewBase)\n";
}
#####################################################
sub check_macos_package_manager {
   print "checking for package manager ... ";
   if (defined($options{fink}) and $options{fink} ne ".none.") {
      check_fink();
   } elsif (defined($options{brew}) and $options{brew} ne ".none.") {
      check_brew();
   } else {
      print "no package manager specified\n";
   }
}
#####################################################
sub determine_architecture {
   print "determining architecture ... ";
   if ($^O eq "darwin") {
      if ( !defined($FinkBase) && !defined($BrewBase) ) {
         if (defined $Arch) {
            if ($Arch ne "i386" && $Arch ne "x86_64") {
               die "Invalid architecture $Arch for Mac OS: allowed values are i386 and x86_64.\n";
            }
         } else {
            $Arch = "x86_64";
         }
      }
      $ARCHFLAGS= $NeedsArchFlag ? "-arch $Arch" : "";
      $Arch="darwin.$Arch";
   } else {
      unless (defined $Arch) {
         if ($^O !~ /aix|rs6000|ibm/i) {
            ($Arch)= `uname -m` =~ /(\w+)/;
         }
         $Arch //= $Config::Config{archname};
      }

      # no arch flags set
      if ($options{native} ne ".none.") {
         if ("$CFLAGS $CXXFLAGS" =~ /(?:^|\s)-m(?:arch|tune|cpu)=(\w+)/) {
            $options{native} = ".none." unless $1 eq "native";
         } else {
            $CFLAGS .= " -march=native";
            $CXXFLAGS .= " -march=native";
         }
      }
   }
   print "ok ($Arch)\n";
}

#####################################################
sub collect_compiler_specific_options {
   print "determining compiler flags ... ";
   if (defined($GCCversion)) {
      # avoid using temp files, they grow indeterminately during debug builds
      $CsharedFLAGS="-fPIC -pipe";
      # TODO: remove -fno-strict-aliasing when the core library is free from reintepret_casts
      $CXXFLAGS .= " -ftemplate-depth-200 -fno-strict-aliasing";
      if ($options{openmp} ne ".none.") {
         $CXXFLAGS .= " -fopenmp";
         $LDFLAGS .= " -fopenmp";
      }
      $CXXCOV="--coverage -O1";
      $CXXSANITIZE="-fno-omit-frame-pointer -O1 " . ($CXXDEBUG =~ /(?:^|\s)-g0/ ? "-g1" : $CXXDEBUG);
      # external libraries might be somehow dirtier
      $CflagsSuppressWarnings="-Wno-unused-result";
      # gcc-specific flags
      $CXXFLAGS .= " -Wshadow -Wlogical-op -Wconversion -Wzero-as-null-pointer-constant -Wno-parentheses -Wno-error=unused-function";
      if (v_cmp($GCCversion, "6.3.0") >= 0 && v_cmp($GCCversion, "7.0.0") < 0) {
         $CXXFLAGS .= " -Wno-maybe-uninitialized";
      }
      if (v_cmp($GCCversion, "9.1.0") >= 0) {
         $CXXFLAGS .= " -Wno-stringop-overflow";
         $CflagsSuppressWarnings.=" -Wno-stringop-overflow";
      }
      if (v_cmp($GCCversion, "10.0.0") >= 0) {
         $CXXFLAGS .= " -Wno-array-bounds";
      }

   } elsif (defined($ICCversion)) {
      $CsharedFLAGS="-fPIC";
      $CXXDEBUG !~ /(?:^|\s)-O/ and $CXXDEBUG =~ s/-g(?!0)\K/ -Ob0/;
      $CXXFLAGS .= " -wd193,383,304,981,1419,279,810,171,1418,488,1572,561";
      $CFLAGS .= " -wd193,1572,561";

   } elsif (defined($CLANGversion)) {
      # avoid using temp files, they grow indeterminately during debug builds
      $CsharedFLAGS="-fPIC -pipe";
      $CXXFLAGS .= " -Wno-logical-op-parentheses -Wno-shift-op-parentheses -Wno-mismatched-tags";
      $CflagsSuppressWarnings="-Wno-unused -Wno-empty-body -Wno-format-extra-args";
      if (v_cmp($CLANGversion, "3.6") >= 0) {
         $CXXFLAGS .= " -Wno-unused-local-typedef -Wno-error=unneeded-internal-declaration";
      }
      if (v_cmp($CLANGversion, "5") >= 0) {
         $CXXFLAGS .= " -Wshadow -Wconversion -Wno-sign-conversion -Wzero-as-null-pointer-constant";
      } elsif (v_cmp($CLANGversion, "4") >= 0) {
         $CXXFLAGS .= " -Wno-unknown-pragmas -Wno-unknown-warning-option";
      } else {
         $CXXFLAGS .= " -Wno-unknown-pragmas";
      }
      # verify openmp support which is available starting with 3.7 but depends on the installation,
      # but 3.7 seems to crash when compiling libnormaliz so we skip that version
      # version 3.8 is tested to work with openmp
      if (v_cmp($CLANGversion, "3.8") >= 0 && $options{openmp} ne ".none.") {
         my $ompflag = "-fopenmp";
         my $build_error=build_test_program(<<'---', CXXFLAGS => "$ompflag", LDFLAGS => "$ompflag");
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
               $CXXFLAGS .= " $ompflag";
               $LDFLAGS .= " $ompflag";
            }
         }
      }

      if (v_cmp($CLANGversion, "3.8") >= 0) {
         # stick to the gcov-compatible coverage tool for now
         # let's experiment with -fcoverage-mapping -fprofile-instr-generate later
         $CXXCOV="--coverage -O1";
      }
      if (v_cmp($CLANGversion, "3.9") >= 0) {
         $CXXSANITIZE="-fno-omit-frame-pointer -O1 " . ($CXXDEBUG =~ /(?:^|\s)-g0/ ? "-g1" : $CXXDEBUG);
      }
   }

   $LDsharedFLAGS=$Config::Config{lddlflags};

   if ($^O eq "darwin") {
      # MacOS magic again: remove multi-architecture options for fat binaries
      my $allarch=qr/ -arch \s+ \S+ (?: \s+ -arch \s+ \S+)* /x;
      $CFLAGS =~ s/$allarch//;
      $CXXFLAGS =~ s/$allarch//;
      $LDsharedFLAGS =~ s/$allarch//;
      if ($options{callable} ne ".none.") {
         $LDcallableFLAGS = $Config::Config{ldflags} =~ s/$allarch//r;
         $LDcallableFLAGS = "$LDsharedFLAGS $LDcallableFLAGS";
         $LDcallableFLAGS =~ s/-bundle/-dynamiclib/;
         $LDsonameFLAGS = "-install_name $InstallLib/";
      } else {
         $LDcallableFLAGS="none";
      }
   } else {
      # enforce lazy binding unless it's already explicitly enabled or disabled by the user
      # (there are few strange Linux distributions where it's not enabled by default contradicting the ld manual)
      if ("$LDFLAGS $LDsharedFLAGS" !~ /-z(?:,|\s+)(?:lazy|now)\b/) {
         $LDsharedFLAGS .= " -Wl,-z,lazy";
      }

      # prevent early loading of third-party shared modules which are used only in few applications
      if ("$LDFLAGS $LDsharedFLAGS" !~ /-as-needed/) {
         $LDsharedFLAGS .= " -Wl,--as-needed";
      }

      if ($options{callable} ne ".none.") {
         $LDcallableFLAGS="$LDsharedFLAGS $Config::Config{ldflags}";
         $LDsonameFLAGS = "-Wl,-soname,";
      } else {
         $LDcallableFLAGS="none";
      }
   }

   if (defined($CLANGversion) && v_cmp($CLANGversion, "3.5") < 0) {
      # old clangs do not have this option which is actively used in newer perls
      s/-fstack-protector\K-strong//g for $LDsharedFLAGS, $LDcallableFLAGS;
   }

   print <<"---";
ok
   CFLAGS=$CFLAGS
   CXXFLAGS=$CXXFLAGS
   LDFLAGS=$LDFLAGS
---
}

#####################################################
sub check_build_modes {
   $BuildModes="Opt Debug";
   if ($options{'build-modes'}) {
      # might want to use a non-standard perl just with one build mode
      $BuildModes="" if $options{'alt-perl'};
      add_build_modes($options{'build-modes'});
   }
}

###############################################################
sub locate_gmp_and_mpfr_libraries {
   my $GMP=$options{gmp};

   unless (defined $GMP) {
      if ($^O eq "freebsd") {
         if (-f "/usr/local/include/gmp.h") {
            $GMP="/usr/local";
         }
      } elsif ($FinkBase) {
         $GMP=$FinkBase;
      }
   }
   if (defined $GMP) {
      $CFLAGS .= " -I$GMP/include";
      $CXXFLAGS .= " -I$GMP/include";
      my $libdir=get_libdir($GMP, "gmp");
      $LDFLAGS .= " -L$libdir";
      if (!$FinkBase) {
         # non-standard location
         $LDFLAGS .= " -Wl,-rpath,$libdir";
      }
   }

   my $MPFR=$options{mpfr};
   if (defined($MPFR) && $MPFR ne $GMP) {
      $CFLAGS .= " -I$MPFR/include";
      $CXXFLAGS .= " -I$MPFR/include";
      my $libdir=get_libdir($MPFR, "mpfr");
      $LDFLAGS .= " -L$libdir";
      if (!$FinkBase) {
         # non-standard location
         $LDFLAGS .= " -Wl,-rpath,$libdir";
      }
   } elsif ($FinkBase) {
      $MPFR=$FinkBase;
   }

   if ($options{prereq} ne ".none.") {
      if ($FinkBase) {
         print "checking fink gmp and mpfr packages ... ";
         -f "$FinkBase/lib/libgmp.dylib"
           or die <<"---";
Fink package gmp-shlibs seems to be missing.
Please install the packages gmp and gmp-shlibs and repeat the configure run.
---
         -f "$FinkBase/lib/libmpfr.dylib"
           or die <<"---";
Fink package mpfr-shlibs seems to be missing.
Please install the packages mpfr and mpfr-shlibs and repeat the configure run.
---
         print "ok\n";
      }

      print "checking gmp installation ... ";
      my $build_error=build_test_program(<<'---', LIBS => "$ARCHFLAGS -lgmp");
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
            if (v_cmp($is_version, "4.2.0") < 0) {
               die <<"---";
The GNU Multiprecision Library (GMP) installed at your site is of version $is_version
while 4.2.0 is the minimal required version.
---
            }
            $store_versions{GMP}=$is_version;
         } else {
            die <<"---";
Could not run a test program linked to the GNU Multiprecision Library (GMP).
Probably the shared library libgmp.$Config::Config{so} is missing or of an incompatible machine type.
---
         }
      } else {
         die <<"---", $build_error;
Could not compile a test program checking for the GNU Multiprecision Library (GMP).
The most probable reasons are that the library is installed at a non-standard location,
lacking developer's subpackage or missing at all.
Please refer to the installation instructions at $Wiki/howto/install.
The complete error log follows:
---
      }
      print "ok", defined($GMP) && " ($GMP)", "\n";

      print "checking mpfr installation ... ";
      $build_error=build_test_program(<<'---', LIBS => "$ARCHFLAGS -lmpfr -lgmp");
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
            if (v_cmp($is_version, "3.0.0") < 0) {
               die <<"---";
The Multiple Precision Floating-Point Reliable Library (MPFR) installed at your site is of version $is_version
while 3.0.0 is the minimal required version.
---
            }
            $store_versions{MPFR}=$is_version;
         } else {
            die <<"---";
Could not run a test program linked to the Multiple Precision Floating-Point Reliable Library (MPFR).
Probably the shared library libmpfr.$Config::Config{so} is missing or of an incompatible machine type.
---
         }
      } else {
         die <<"---", $build_error;
Could not compile a test program checking for the Multiple Precision Floating-Point Reliable Library (MPFR).
The most probable reasons are that the library is installed at a non-standard location,
lacking developer's subpackage or missing at all.
Please refer to the installation instructions at $Wiki/howto/install.
The complete error log follows:
---
      }
      print "ok", defined($MPFR) && " ($MPFR)", "\n";
   }
}

##########################################################
sub locate_boost_headers {
   my $boost_path=$options{boost};

   if (defined $boost_path) {
      $boost_path .= '/include' if (-d "$boost_path/include/boost");
      $CXXFLAGS .= " -I$boost_path";
   } elsif ($FinkBase) {
      print "checking fink boost package ... ";
      ($boost_path) = glob("$FinkBase/opt/boost-*/include/boost");
      $boost_path =~ s{/include/boost$}{};
      unless (-d $boost_path) {
         die <<"---";
Fink package boost-<some version>_nopython seems to be missing.
Please install the package boost-<some version>_nopython and repeat the configure run.
---
      }
      $CXXFLAGS .= " -I$boost_path/include";
      print "ok\n";
   }

   if ($options{prereq} ne ".none.") {
      print "checking boost installation ... ";
      my $build_error=build_test_program(<<'---');
#include <boost/shared_ptr.hpp>
#include <boost/iterator/counting_iterator.hpp>
int main() {
  return 0;
}
---
      if ($?) {
         die <<"---";
Could not compile a test program checking for boost library.
The most probable reasons are that the library is installed at a non-standard location,
or missing at all.
The complete error log follows:
$build_error

Please install the library and specify its location using --with-boost option, if needed.
---
      }
      print "ok", defined($boost_path) && " ($boost_path)", "\n";
   }
}


###############################################################

sub locate_permlib {
   my $permlib_path = $options{permlib};
   if (defined($permlib_path) and $permlib_path ne "bundled") {
      $permlib_path .= '/include' if (-d "$permlib_path/include/permlib");
      $CXXFLAGS .= " -I$permlib_path";
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
      # this includes the case $permlib_path eq "" where we try to use a systemwide installation
      if (defined($permlib_path) and $permlib_path ne "bundled") {
         print "checking permlib installation ... ";
         # check permlib installation
         my $build_error=build_test_program(<<'---');
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
            die <<"---";
Could not compile a test program checking for permlib.
The most probable reasons are that the library is installed at a non-standard location,
or missing at all.
The complete error log follows:
$build_error

Please install the library and specify its location using --with-permlib option, if needed.
Alternatively, you may leave out the --with-permlib option to use the bundled version.
---
         }
         print "ok", " (", $permlib_path eq "" ? "system" : ($permlib_path // "bundled"), ")\n";
      }
   }
}

##############################################################
sub check_libperl {
   print "checking shared perl library ... ";
   my $libperl=$Config::Config{libperl};

   my $error = "";
   if (length($libperl)==0) {
      $error = <<"---";
Your perl installation seems to lack the libperl.$Config::Config{so} shared library.
On some systems it is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.

If your perl installation has been configured by hand, please make sure that
you have answered with "yes" to the question about the libperl shared library
(it is not the default choice!), or that you have passed '-Duseshrplib=true'
to the ./Configure script.
Otherwise, reconfigure and reinstall your perl.
---
   }

   # We also build a test program for libperl since e.g. on Debian based systems the
   # check for Config::Config{libperl} will not detect a missing libperl-dev package
   require ExtUtils::Embed;
   chomp(my $perlcflags = ExtUtils::Embed::ccopts());
   chomp(my $perlldflags = ExtUtils::Embed::ldopts());
   if (defined($CLANGversion)) {
      if (v_cmp($CLANGversion, "3.5") < 0) {
         s/-fstack-protector\K-strong//g for $perlcflags, $perlldflags;
      }
      if (defined($XcodeVersion) && v_cmp($XcodeVersion, "10.0") >= 0) {
         # from XCode 10.0.0 on, 32-bit builds are deprecated
         $_ =~ s/(?:^|\s+)-arch\s+\w+//g for $perlcflags, $perlldflags; 
      }
   }
   my $build_error=build_test_program(<<'---', CXXFLAGS => (defined($CLANGversion) && "-Wno-reserved-user-defined-literal ").$perlcflags, LDFLAGS => "$ARCHFLAGS $perlldflags" );
#include <EXTERN.h>
#include <perl.h>

int main(int argc, char **argv, char **env)
{
   const char* embedding[] = { "", "-e", "print $];", 0 };
   PERL_SYS_INIT3(&argc, &argv, &env);
   pTHXx = perl_alloc();
   perl_construct(aTHXx);
   PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
   perl_parse(aTHXx_ nullptr, 3, (char**)embedding, nullptr);
   perl_run(aTHXx);
   perl_destruct(aTHXx);
   perl_free(aTHXx);
   PERL_SYS_TERM();
}
---
   if ($?==0) {
      chomp(my $version = run_test_program());
      if ($? == 0) {
         if ($version ne $]) {
            $error = <<"---";
The shared perl library claims to be of a different version ($version) than
your perl interpreter ($]). Please choose a different perl installation with
PERL=/some/path/to/bin/perl or try to reinstall perl (including libperl).
---
         }
      } else {
         $error = <<"---";
Could not run a test program checking for libperl.$Config::Config{so}.
The error is as follows:
$!

On some systems the library is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.
---
      }
   } else {
      $error = <<"---";
Could not compile a test program for the libperl.$Config::Config{so} shared library.
The build error is as follows:
$build_error

On some systems the library is contained in a separate package named like
perl-devel or libperl-dev.  Please look for such a package and install it.
---
   }

   if ($error) {
      print "failed\n\n";
      die <<"---";
$error
As a last resort, you can configure polymake with the option --without-callable .
You won't be able to build the callable library any more, but at least you get
polymake compiled.
---
   }

   print "ok\n";
}

######################################################################
sub check_prerequisite_perl_packages {
   if (defined $FinkBase) { 
      lib->import("$FinkBase/lib/perl5");
   }
   my @warned;
   foreach (qw(XML::SAX Term::ReadKey Term::ReadLine JSON)) {
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
      $warning .= <<'---';
WARNING: Please install/check the following perl modules prior to starting polymake: 
---
      $warning .= "         ".join(", ",@warned)."\n";
   }
}

#########################################################################
sub bundled_ext_error_msg {
   my ($ext, $msg)=@_;
   unless (defined($BundledLOG)) {
      open $BundledLOG, ">", $BundledLOGfile;
      print $BundledLOG <<"---";
Configuration of the following bundled extensions failed, proceeding without them.
If you really need them, please carefully read the following explanations,
take the suggested actions, and repeat the configuration.
---
   }
   print $BundledLOG "\n---- $ext ----\n\n$msg\n";
}

##########################################################################
sub order_enabled_bundled_extensions {
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
}

###########################################################################
sub configure_enabled_bundled_extensions {
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
            die << "---";


ERROR:
The bundled extension $ext was explicitly requested but failed to configure.
Please recheck your argument (--with-$ext=$options{$ext}) and ${BuildDir}/bundled.log .
You can also disable it by specifying --without-$ext instead.
---
         }
         $options{$ext}=".none.";
      }
      print "\n";
   }

   # save the list of enabled bundled extensions in proper order for the make process
   $BundledExts = join(" ", grep { $ext_survived{$_} } @ext_ordered);

   if ($options{prereq} ne ".none.") {
      my $ext_warning;

      $ext_warning .= <<"---" unless exists $ext_survived{"cdd"};

WARNING: The bundled extension for cdd was either disabled or failed to configure.
         Running polymake without cdd is deprecated and not supported!
         Please recheck your configuration and $BundledLOGfile.
---

      $ext_warning .= <<"---" unless (exists $ext_survived{"nauty"} or exists $ext_survived{"bliss"});

WARNING: The bundled extensions for bliss and nauty were both either disabled or failed to configure.
         Running polymake without a tool for graph-isomorphism tests is not supported!
         Please recheck your configuration and $BundledLOGfile.
---

      $ext_warning .= <<"---" unless exists $ext_survived{"flint"};

WARNING: The bundled extension for flint was either disabled or failed to configure.
         Running polymake without flint is not supported!
         QuadraticExtension types cannot be normalized in this configuration.
         Please recheck your configuration and $BundledLOGfile.
---

      # check for non-supported configuration:
      # failed required extensions but not disabled explicitly
      if ($ext_failed{cdd} || $ext_failed{flint} ||
          $ext_failed{bliss} && $ext_failed{nauty}) {

         print $ext_warning;
         die(<<"---");

ERROR: Configuration failed:
       At least one required bundled extension failed to configure.
       This error can be suppressed by disabling them explicitly via '--without-<ext>'.
---
      } elsif ($ext_warning) {
         $warning .= <<"---";
$ext_warning
* WARNING: Unsupported configuration:
* Some required bundled extensions were disabled explicitly.
* polymake will work but with reduced functionality.
---
      }
   }

   print "* If you want to change the configuration of bundled extensions please ",
         defined($BundledLOG) && "see $BundledLOGfile and ",
         "try configure --help.\n";

   if (defined $BundledLOG) {
      close $BundledLOG;
   } else {
      # remove an outdated logfile to avoid confusions
      unlink $BundledLOGfile;
   }
}

##############################################################################
sub finalize_compiler_flags {
   # add flags and libraries that should not appear in the test builds tried before

   if ($options{callable} ne ".none."
       and $LIBS !~ /-ldl/
       and $^O eq "linux") {
      $LIBS .= " -ldl";
   }

   $LIBS="$LIBS -lmpfr -lgmp -lpthread";

   # be rigorous about own code
   if ($Polymake::DeveloperMode) {
      $CXXFLAGS="-Wall -Werror $CXXFLAGS";
   }

   if (defined($options{ccache}) && $options{ccache} ne ".none.") {
      $CCACHE=$options{ccache};
      check_program($CCACHE);
   }
   if (defined($options{ccwrapper}) && $options{ccwrapper} ne ".none.") {
      $CCWRAPPER=$options{ccwrapper};
   }
}

##############################################################################
sub find_ninja {
   find_program_in_path("ninja")
     or die <<'---';
ERROR: The ninja program is required for building polymake.

It can usually be installed as a ready-to-use package, e.g.
on Ubuntu:
   apt-get install ninja-build
on MacOS with Homebrew:
   brew install ninja

As a fallback, you can obtain its source code from GitHub, build it yourself,
and install at a location of your choice:

INSTALL_DIR=/usr/local/bin  # or anything else reachable through your PATH

git clone https://github.com/ninja-build/ninja.git
cd ninja
./configure.py --bootstrap
sudo install -s -m 555 ninja $INSTALL_DIR

---
}

##############################################################################
sub write_configuration_file {
   my ($filename) = @_;
   open my $conf, ">", $filename
     or die "can't create $filename: $!\n";

   # preserve a copy of the command line for later reference
   print $conf "# last configured with:\n", "configure.command=$root/configure";
   write_config_command_line($conf, \%options, \%allowed_with, \%vars, \%ext_failed);

   my @external_includes=map { "-I\${root}/include/external/$_" } $ExternalHeaders =~ /(\S+)/g;
   print $conf <<"---";

root=$root
core.includes=-I\${root}/include/core-wrappers -I\${root}/include/core
app.includes=-I\${root}/include/app-wrappers -I\${root}/include/apps @external_includes

---
   write_config_vars(__PACKAGE__, "", $conf);

   print $conf <<"---";
AR = $Config::Config{ar}
---

   foreach my $pkg (sort keys %store_versions) {
      print $conf <<"---";
${pkg}.version = $store_versions{$pkg}
---
   }

   # write configuration variables for successfully configured bundled extensions
   foreach my $ext (sort keys %ext_survived) {
      write_config_vars("Polymake::Bundled::$ext", "bundled.$ext.", $conf);
      print $conf "bundled.$ext.RequireExtensions=@{$ext_requires{$ext}}\n";
   }

   close $conf;
}

#######################################################################
sub write_perl_specific_configuration_file {
   my ($filename)=@_;
   open my $conf, ">", $filename
     or die "can't create $filename: $!\n";

   my $PerlVersion=$Config::Config{version};
   $PerlVersion =~ s/\.//g;

   my $no_warn="-Wno-nonnull";
   # gcc 5 seems to not understand a #pragma ignoring -Wliteral-suffix
   if ($] < 5.020 && defined($GCCversion)) {
      $no_warn .= " -Wno-literal-suffix";
   }
   if (defined($CLANGversion) && v_cmp($CLANGversion, "10") >= 0) {
      $no_warn .= " -Wno-xor-used-as-pow";
   }

   # Xcode 10 deeply hides perl headers at unpredictable locations, it requires a special clang option to find them
   my $includePerlCore = defined($XcodeVersion) && v_cmp($XcodeVersion, "10.0") >= 0 && $Config::Config{perlpath} eq "/usr/bin/perl" ? "-iwithsysroot " : "-I";

   # fedora has ExtUtils::ParseXS in vendorlib
   my ($xsubpp) = grep { -e $_ } map { "$Config::Config{$_}/ExtUtils/xsubpp" } qw(privlib vendorlib);
   my ($typemap) = grep { -e $_ } map { "$Config::Config{$_}/ExtUtils/typemap" } qw(privlib vendorlib);

   print $conf <<"---";
PERL=$Config::Config{perlpath}
CXXglueFLAGS=$includePerlCore$Config::Config{archlibexp}/CORE $Config::Config{ccflags} -DPerlVersion=$PerlVersion $no_warn
LIBperlFLAGS=-L$Config::Config{archlib}/CORE -lperl $Config::Config{ccdlflags}
ExtUtils_xsubpp=$xsubpp
ExtUtils_typemap=$typemap
---

   close $conf;
}

#######################################################################
sub delete_old_and_disabled_build_trees {
   # delete artifacts for disabled extensions, if any, and create .disabled markers
   foreach my $bundled (@ext_disabled, grep { !$ext_survived{$_} } @ext) {
      File::Path::remove_tree(grep { -d $_ } glob("$BuildDir/{Opt,Debug,Cov,San}/bundled/$bundled"));
   }

   # delete old (pre-ninja) build artifacts
   if ($Polymake::DeveloperMode && -f "$BuildDir/conf.make") {
      File::Path::remove_tree((grep { -d $_ } "$BuildDir/bundled", "$BuildDir/lib", "$BuildDir/apps", "$BuildDir/jars", "$BuildDir/java"),
                              glob("$BuildDir/perlx-*"), glob("$BuildDir/perlx/*/*/auto"));
      unlink "$BuildDir/conf.make";
   }
}

#######################################################################
sub create_alt_perl_configuration {
   -f "$BuildDir/config.ninja"
     or die "option --alt-perl can only be used on the top of an existing full configuration\n";

   File::Path::make_path("$BuildDir/$perlxpath");
   write_perl_specific_configuration_file("$BuildDir/$perlxpath/config.ninja");
   my $builddir = "$root/$BuildDir";
   foreach my $mode ($BuildModes =~ /(\S+)/g) {
      create_build_mode_subtree($builddir, $mode);
      write_build_ninja_file($builddir, $mode, "build.$options{'alt-perl'}.ninja", perlxpath => $perlxpath);
   }
}

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
