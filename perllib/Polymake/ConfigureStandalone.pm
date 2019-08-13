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

#  Utilities needed in configure scripts of core system and bundled extensions.
#  Namespace mode is not allowed here, because these configure scripts may be executed
#  before any perl extension has been built.

use strict;
require "Polymake/file_utils.pl";

package Polymake::Configure;

use vars qw(@conf_vars);

BEGIN {
   # variables used in the configuration files and ninja rules

   @conf_vars=qw( CC CXX CCACHE CCWRAPPER
                  CFLAGS CXXFLAGS ARCHFLAGS CsharedFLAGS CXXOPT CXXDEBUG CXXCOV CXXSANITIZE
                  CflagsSuppressWarnings
                  GCCversion ICCversion CLANGversion CPPStd XcodeVersion
                  LDFLAGS LDsharedFLAGS LDcallableFLAGS LDsonameFLAGS LIBS
                  LIBXML2_CFLAGS LIBXML2_LIBS ExternalHeaders
                  Arch FinkBase BrewBase BundledExts BuildModes
                  InstallTop InstallArch InstallBin InstallInc InstallLib InstallDoc DESTDIR );
}

use vars map { "\$$_" } @conf_vars;

my %supported_build_modes=( Cov => 1, San => 1 );

###############################################################################################
# this can be redefined when running the polymake main script
sub try_host_agent { }

###############################################################################################
# "progname [args]", ..., \&check_code => (first_shot, full_path) || (undef, full_path, "error text") || ()
# check_code (optional) : (full_path, args) => "error text" || "" if OK.
sub find_program_in_path {
   my $check_code = ref($_[-1]) eq "CODE" && pop;
   my @tried;
   foreach (@_) {
      my ($progname, $args)=split /\s+/, $_, 2;
      my $first_shot=1;
      foreach my $dir (split /:/, $ENV{PATH}) {
         my $full_path="$dir/$progname";
         if (-x $full_path && -f _) {
            if ($check_code && (my $error=$check_code->($full_path, $args))) {
               @tried=(undef, $full_path . ($args && " $args"), $error);
            } else {
               return ($first_shot, $full_path . ($args && " $args"));
            }
            $first_shot=0;
         }
      }
      if (not($check_code && $check_code->("env $progname"))
          && length(my $full_path=try_host_agent("find $progname"))) {
         return (0, "env $full_path" . ($args && " $args"));
      }
   }
   wantarray ? @tried : undef
}
###############################################################################################
sub verify_executable {
   my ($host_agent, $prog)= $_[0] =~ m{^(?:(env)\s+)? (\S+)}x;
   if ($host_agent) {
      if (try_host_agent("verify $prog")) {
         return 1;
      }
      if (-x $prog && -f _) {
         $_[0] =~ s/^env\s+//;
         return 1;
      }
   } else {
      if (-x $prog && -f _) {
         return 1;
      }
      if (try_host_agent("verify $prog")) {
         substr($_[0], 0, 0) .= "env ";
         return 1;
      }
   }
}
###############################################################################################
# a simplified version without interaction
sub find_program {
   my @prognames=splice @_, 1;

   if ($_[0] =~ m{^(?:env\s+)? /}x) {
      # absolute path
      if (verify_executable($_[0])) {
         return $_[0];
      }
      undef $_[0];
   }

   my ($first_shot, $full_path)=find_program_in_path(defined($_[0]) ? $_[0] : (), @prognames);

   if (defined $first_shot) {
      if ($first_shot) {
         $full_path =~ $Polymake::directory_of_cmd_re;
         $_[0]=substr($full_path, $+[1]+1);
      } else {
         $_[0]=$full_path;
      }
      $full_path;
   } else {
      undef $_[0];
   }
}
###############################################################################################
# like find_program, dies when the provided value is wrong
sub check_program {
   my $provided=$_[0];
   &find_program
     or $provided && die( $provided,
        $provided =~ m{^[/.]}
        ? (-e $provided ? " is not an executable program" : " does not exist")
        : " could not be found along your PATH, please check the spelling or provide a full path\n"
     );
}

###############################################################################################
# "PackageName", "prefix", \*FILEHANDLE =>
# Write the configuration variables into a makefile.
sub write_config_vars {
   no strict 'refs';
   my ($pkg, $prefix, $conf, $augmented)=@_;

   foreach my $var (@{"$pkg\::conf_vars"}) {
      if (defined (my $value=${"$pkg\::$var"})) {
         if (defined($augmented) && $augmented->{$var}) {
            print $conf "super.$prefix$var = \${$prefix$var}\n",
                        "$prefix$var = $value \${super.$prefix$var}\n";
         } else {
            print $conf "$prefix$var = $value\n";
         }
      }
   }
}
###############################################################################################
# Write the currently active options as a single string
sub write_config_command_line {
   my ($conf, $options, $allowed_with, $vars, $ext_failed)=@_;

   if (defined $vars) {
      foreach my $item (sort keys %$vars) {
         my $value = $vars->{$item};
         $value="'$value'" if $value =~ /[\s(){}\[\]\$]/;
         print $conf " $item=$value";
      }
   }

   foreach my $item (sort keys %$options) {
      my $value = $options->{$item};
      if ($value eq ".none.") {
         unless ($ext_failed && $ext_failed->{$item}) {
            print $conf " --without-$item";
         }
      } elsif ($value eq ".true.") {
         print $conf " --with-$item";
      } else {
         $value="'$value'" if $value =~ /[\s(){}\[\]\$]/;
         print $conf exists $allowed_with->{$item}
                     ? " --with-$item=$value"
                     : " --$item=$value";
      }
   }
}
###############################################################################################
# Read the options stored in a configuration file
sub retrieve_config_command_line {
   my ($config_file, $into)=@_;
   open my $conf, $config_file
     or die "Can't read $config_file: $!\n";
   local $_;
   if ($config_file =~ /\.ninja$/) {
      while (<$conf>) {
         s{^\s*configure\.command[ \t]*=(?:[ \t]*\S+/configure(?=\s))?}{} and last;
      }
      unless ($_) {
         die <<"---";
Configuration file $config_file is corrupted: missing `configure.command' line.
Please run the configure command specifying all desired options on the command line,
or with the option --defaults to ignore the existing configuration.
---
      }
   } else {
      if (<$conf> =~ m{\#.*configured with:[ \t]*(\S.*)?$}) {
         if (defined($1)) {
            $_=$1;
         } elsif (<$conf> =~ m{\#[ \t]+\S+/configure((?=\s).*)$}) {
            $_=$1;
         }
      }
      unless (defined($_)) {
         die <<"---";
Can't restore options given to the last `configure' command because the configuration file $config_file lacks header lines.
It has probably been manually edited and/or corrupted.
Please run the configure command specifying all desired options on the command line,
or with the option --defaults to ignore the existing configuration.
---
      }
   }

   while (/\G[ \t]+ ([-\w]+ =?) (?: (['"]) (.+?) \2 | (\S+))? /gx) {
      push @$into, $1 . ($2 ? $3 : $4);
   }
}
###############################################################################################
# Try to guess the name of a directory containing the appropriate version of a (shared) library.
sub get_libdir {
   my ($prefix, $name, $suffix)=@_;
   $suffix ||= $Config::Config{so};
   if ($Config::Config{longsize}==8 && $CXXFLAGS !~ /-m32/ && -f "$prefix/lib64/lib$name.$suffix") {
      "$prefix/lib64"
   } elsif (($Config::Config{longsize}==4 || $CXXFLAGS =~ /-m32/) && -f "$prefix/lib32/lib$name.$suffix") {
      "$prefix/lib32"
   } else {
      "$prefix/lib"
   }
}

sub v_cmp($$) {
   eval "v$_[0] cmp v$_[1]";
}
###############################################################################################

my $autonomous = !exists &Polymake::Tempdir::new;
my $tempdir;

sub get_tempdir {
   $tempdir //= $autonomous ? Polymake::find_writable_dir() : Polymake::Tempdir->new("till_exit");
}

# "C++ source text", ( VarName => "value to prepend locally", ... ) => "error msg"
# Compiles and links the given program.
# @return error message from the compiler/linker.
# If the progam has been built successfully, it can be run with `run_test_program', see below.
sub build_test_program {
   if (@_ > 2) {
      my ($var, $add)=splice @_, -2;
      no strict 'refs';
      local ${$var}="$add ${$var}";
      build_test_program(@_);
   } else {
      my $dir=get_tempdir();
      open C, ">", "$dir/polymake_${$}_configure.cc" or die "$0: can't create a temporary file: $!\n";
      print C $_[0];
      close C;
      if ($_[1]) {
         # compile only
         `$CXX $CXXFLAGS -c -o $dir/polymake_${$}_configure.o $dir/polymake_${$}_configure.cc 2>&1`
      } else {
         # compile and link
         `$CXX $CXXFLAGS -o $dir/polymake_${$}_configure $dir/polymake_${$}_configure.cc $LDFLAGS $LIBS 2>&1`
      }
   }
}

# Like above, but without linking.
sub compile_test_program {
   splice @_, 1, 0, 1;
   &build_test_program;
}

# Run the program previously build with `build_test_program'.
sub run_test_program {
   my $dir=get_tempdir();
   `$dir/polymake_${$}_configure 2>&1`
}

END {
   $tempdir && $autonomous && unlink glob "$tempdir/polymake_${$}_configure*";
}
###############################################################################################
sub check_extension_source_conflicts {
   my $ext_dir=shift;
   my @conflicts;
   foreach my $app_dir (glob("$ext_dir/apps/*")) {
      foreach my $what ("/src/*.{cc,cpp,C}", "/include/*.h") {
         foreach my $file (glob("$app_dir$what")) {
            foreach my $where (@_) {
               if (-f (my $other_file=$where.substr($file, length($ext_dir)))) {
                  if ($what =~ /include/ or (`fgrep -q 'Template4perl(' $other_file`, $?==0)) {
                     push @conflicts, [ $file, $other_file ];
                  }
               }
            }
         }
      }
   }
   if (@conflicts) {
      die "Identical source file names in depending extensions and/or core applications invite all kinds of trouble!\n",
          "Please resolve conflicts by renaming one of files in the following pairs:\n\n",
          map { $_->[0]."\n".$_->[1]."\n\n" } @conflicts;
   }
}
###############################################################################################
sub add_build_modes {
   foreach my $mode (split /,/, $_[0]) {
      if ($mode eq "Cov") {
         if ($CXXCOV) {
            $BuildModes.=" $mode";
         } else {
            die "Don't know how to collect code coverage statistics with the chosen C++ compiler\n";
         }
      } elsif ($mode eq "San") {
         if ($CXXSANITIZE) {
            $BuildModes.=" $mode";
         } else {
            die "Don't know how to build sanitized code with the chosen C++ compiler\n";
         }
      } elsif ($mode eq "Opt" || $mode eq "Debug") {
         $BuildModes =~ /$mode/ or $BuildModes .= " $mode";
      } else {
         die "unknown build mode $mode\nsupported modes are: ", join(", ", keys %supported_build_modes), "\n";
      }
   }
}
##############################################################################
sub create_build_trees {
   my ($root, $buildroot, @options)=@_;

   # create a dummy target list, it will automatically be regenerated later
   unless (-f "$buildroot/targets.ninja" && (stat _)[9] >= (stat "$root/support/generate_ninja_targets.pl")[9]) {
      open my $targ, ">", "$buildroot/targets.ninja"
        or die "can't create $buildroot/targets.ninja: $!\n";
      print $targ <<'---';
build all: phony
build install: phony
---
      close $targ;
   }

   # create output directories for all supported build modes
   foreach my $mode ($BuildModes =~ /(\S+)/g) {
      create_build_mode_subtree($buildroot, $mode);
      write_build_ninja_file($buildroot, $mode, "build.ninja", @options);
   }
}
#######################################################################
sub create_build_mode_subtree {
   my ($buildroot, $mode)=@_;
   my $path="$buildroot/$mode";
   if (-d $path) {
      # remove the build timestamp in order to enforce a rebuild at the next polymake start
      unlink "$path/.apps.built";
   } else {
      File::Path::make_path($path);
   }

   # create a convenience symlink allowing to use InstallArch/config.ninja in the extension management code
   symlink "../config.ninja", "$path/config.ninja";
}
#######################################################################
sub write_build_ninja_file {
   my ($buildroot, $mode, $filename, %options)=@_;

   my @sanitize_flags;
   if ($mode eq "San") {
      foreach my $opt (qw(ccflags optimize)) {
         @sanitize_flags = $Config::Config{$opt} =~ /(-f(?:no-)?sanitize[-=]\S+)/g
           and last;
      }
      if (!@sanitize_flags) {
         die "$Config::Config{perlpath} can't be used for sanitized builds: it has not been compiled with -fsanitize flags\n";
      }
   }

   open my $conf, ">", "$buildroot/$mode/$filename"
     or die "can't create $buildroot/$mode/$filename: $!\n";

   my $include_list=$options{include};
   my $depends=$include_list ? join(" ", @$include_list) : '${config.file}';

   print $conf <<"---";
buildroot=$buildroot
buildmode=$mode
builddir=\${buildroot}/\${buildmode}
config.file=\${buildroot}/config.ninja
---
   if ($include_list) {
      print $conf <<"---";
root.config.file=${$include_list}[0]
---
      foreach my $included (@$include_list) {
         print $conf "include $included\n";
      }
   }
   print $conf <<"---";
include \${config.file}
---
   if (@sanitize_flags) {
      print $conf "PERLSANITIZE=@sanitize_flags\n";
   }
   if ($options{perlxpath}) {
      print $conf <<"---";
perlxpath=$options{perlxpath}
include \${buildroot}/\${perlxpath}/config.ninja
---
   }
   print $conf $options{addvars}, <<"---";
include \${root}/support/rules.ninja
CmodeFLAGS=\${C${mode}FLAGS}
CexternModeFLAGS=\${Cextern${mode}FLAGS}
CmodeCACHE=\${C${mode}CACHE}
LDmodeFLAGS=\${LD${mode}FLAGS}

include \${buildroot}/targets.ninja

# should rerun the target generation if any of the included files changes
build $filename: phony | $depends \${buildroot}/targets.ninja
---

   close $conf;
}

##############################################################################################
# translate apple xcode to clang versions
# some of the versions can be found here:
# https://en.wikipedia.org/wiki/Xcode#Toolchain_versions
sub xcode2clang_version {
   my $ver = eval "v$XcodeVersion";
   return "6.0" if $ver ge v10.0;
   return "5.0" if $ver ge v9.3;
   return "4.0" if $ver ge v9.0;
   return "3.9" if $ver ge v8.0;
   return "3.8" if $ver ge v7.3;
   return "3.7" if $ver ge v7.0;
   return "3.6" if $ver ge v6.3;
   return "3.5" if $ver ge v6.0;
   return "3.4" if $ver ge v5.1;
   die "Apple Xcode version is too old, minimal required version is 5.1\n";
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
