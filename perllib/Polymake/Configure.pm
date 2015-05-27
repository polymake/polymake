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

#  Utilities needed only in CONFIGURE clauses and configure scripts.
#  Namespace mode is not allowed here, because the main configure script is executed
#  before any perl extension has been built.

use strict;
require "Polymake/file_utils.pl";

package Polymake::Configure;

use vars qw( @make_vars @make_export_vars );

BEGIN {
   # variables used in the Makefiles

   @make_vars=qw( CC CXX Cflags CXXflags CsharedFlags CXXOPT CXXDEBUG CflagsSuppressWarnings
                  GCCversion ICCversion CLANGversion CCache
                  LDflags LDsharedFlags LDcallableFlags LDsonameFlag Libs
                  LIBXML2_CFLAGS LIBXML2_LIBS
                  Arch FinkBase BundledExts );

   @make_export_vars=qw( PERL InstallTop InstallArch InstallBin InstallInc InstallLib InstallDoc DirMask ARCHFLAGS );
}

use vars map { "\$$_" } @make_vars, @make_export_vars;

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
         if (-x (my $full_path="$dir/$progname") && !-d _) {
            if ($check_code && (my $error=$check_code->($full_path, $args))) {
               @tried=(undef, $full_path . ($args && " $args"), $error);
            } else {
               return ($first_shot, $full_path . ($args && " $args"));
            }
            $first_shot=0;
         }
      }
   }
   @tried
}
###############################################################################################
#  $variable, "program", ..., { prompt => "Prompt", check => \&code, also => \&code }
sub find_program {
   my $opts= ref($_[-1]) eq "HASH" ? pop : { };
   my @prognames=splice @_, 1;
   my $check_code=$opts->{check};
   my $prompt=($opts->{prompt} ||= @prognames>1 ? "one of the programs (".join(",",@prognames).")" : "the program `@prognames'");
   my $also=$opts->{also};

   if ($_[0] =~ m{^/}) {
      my ($executable, $args)= split /\s+/, $_[0], 2;
      if (-x $executable && -f _ && !($check_code && $check_code->($executable, $args))) {
         if ($Polymake::Shell->interactive) {
            print "Please confirm the path to $prompt or enter an alternative location:\n";
            $_[0]=$Polymake::Shell->enter_program($_[0], $opts);
         }
         return $_[0];
      }
      undef $_[0];
   }

   my ($first_shot, $full_path, $error)=find_program_in_path(defined($_[0]) ? $_[0] : (), @prognames, $check_code ? $check_code : ());
   if (!defined($first_shot) and
       defined($also) && defined($full_path=$also->())) {
      my ($executable, $args)= split /\s+/, $full_path, 2;
      if (-x $executable && -f _ && !($check_code && ($error=$check_code->($executable, $args)))) {
         $first_shot=0;
      }
   }

   if (defined $first_shot) {
      if ($Polymake::Shell->interactive) {
         print "Please confirm the path to $prompt or enter an alternative location:\n";
         $_[0]=$Polymake::Shell->enter_program($full_path, $opts);
         return $_[0] if $_[0] ne $full_path;
      }
      if ($first_shot) {
         $full_path =~ $Polymake::directory_of_cmd_re;
         $_[0]=substr($full_path, $+[1]+1);
      } else {
         $_[0]=$full_path;
      }
      $full_path;
   } elsif ($Polymake::Shell->interactive) {
      if (defined $error) {
         my ($location)= $full_path =~ $Polymake::directory_of_cmd_re;
         print "$prompt found at location $location, but did not meet the requirements: $error\n",
               "Please enter an alternative location or an empty string to abort.\n";
      } else {
         print "Could not find $prompt anywhere along your PATH.\n",
               "Please enter the full path or an empty string to abort.\n";
      }
      $_[0]=$Polymake::Shell->enter_program("", $opts);
   } else {
      undef $_[0];
   }
}

###############################################################################################
# like find_program without options and without interactive confirmation
sub check_program {
   if ($_[0] =~ m{^[/.]}) {
      if (-x $_[0] && -f _) {
         Cwd::abs_path($_[0])
      } else {
         die "$_[0] ", -e _ ? "is not an executable program" : "does not exist", "\n";
      }
   } elsif (defined $_[0]) {
      my $provided=$_[0];
      find_program($_[0], $provided)
         or
      die "$provided could not be found along your PATH, please check the spelling or provide a full path\n";
   } else {
      find_program(@_)
   }
}

###############################################################################################
# $variable, "filename", "dir", ..., { prompt => "Prompt", check => \&code, abort => "text" }
sub find_file_and_confirm {
   my $opts= ref($_[-1]) eq "HASH" ? pop : { };
   my $prompt=($opts->{prompt} ||= "file $_[1]");
   my $abort=$opts->{abort} || "abort";
   my $check_code=$opts->{check};
   $opts->{check}=sub { -f $_[0] ? $check_code && &$check_code : "file $_[0] does not exist" };

   if (defined $_[0]) {
      if (-f $_[0] && !($check_code && $check_code->($_[0]))) {
         if ($Polymake::Shell->interactive) {
            print "Please confirm the location of $prompt or enter an alternative location:\n";
            $_[0]=$Polymake::Shell->enter_filename($_[0], $opts);
         }
         return $_[0];
      }
      undef $_[0];
   }

   my $filename=$_[1];
   my $full_path;
   foreach my $dir (@_[2..$#_]) {
      if (-d $dir && -f ($full_path="$dir/$filename") && !($check_code && $check_code->($full_path))) {
         if ($Polymake::Shell->interactive) {
            print "Please confirm the location of $prompt or enter an alternative location:\n";
            $_[0]=$Polymake::Shell->enter_filename($full_path, $opts);
         } else {
            $_[0]=$full_path;
         }
         return $_[0];
      }
   }
   if ($Polymake::Shell->interactive) {
      print "Could not find $prompt at any usual place.\n",
            "Please enter the location or an empty string to $abort.\n";
      $_[0]=$Polymake::Shell->enter_filename("", $opts);
   } else {
      undef $_[0];
   }
}

###############################################################################################
# $variable, "path", ..., { prompt => "Prompt", check => \&code, abort => "text" }
sub find_location {
   my $opts= ref($_[-1]) eq "HASH" ? pop : { };
   my $prompt=($opts->{prompt} ||= ($_[1] =~ $Polymake::filename_re)[0] . " directory");
   my $check_code=$opts->{check};

   if (defined $_[0]) {
      if (-d $_[0] && !($check_code && $check_code->($_[0]))) {
         if ($Polymake::Shell->interactive) {
            print "Please confirm the location of $prompt or enter an alternative location:\n";
            $_[0]=$Polymake::Shell->enter_filename($_[0], $opts);
         }
         return $_[0];
      }
      undef $_[0];
   }

   foreach my $dir (@_[1..$#_]) {
      if (-d $dir && !($check_code && $check_code->($dir))) {
         if ($Polymake::Shell->interactive) {
            print "Please confirm the location of $prompt or enter an alternative location:\n";
            $_[0]=$Polymake::Shell->enter_filename($dir, $opts);
         } else {
            $_[0]=$dir;
         }
         return $_[0];
      }
   }
   if ($Polymake::Shell->interactive) {
      print "Could not find $prompt at any usual place.\n",
            "Please enter the location or an empty string to abort.\n";
      $_[0]=$Polymake::Shell->enter_filename("", $opts);
   } else {
      undef $_[0];
   }
}

###############################################################################################
# Read all variables configured for the current platform
# and make them available as scalars in the package Polymake::Configure.
sub load_make_vars {
   return if defined $Arch;
   return unless -f (my $conf_file="$Polymake::InstallArch/conf.make");
   open my $CF, $conf_file
     or die "can't read $conf_file: $!\n";
   local $_;
   while (<$CF>) {
      if (/^\s* (\w+) \s*:=\s* (.*?) \s*$/x) {
         no strict 'refs';
         ${$1}=$2;
      } elsif (! /^\s* (?: \# | $ )/x) {
         last;
      }
   }
}
###############################################################################################
# Read the makefile variables configured for the current extension and its prerequisites,
# return them by reference in a hash map.
sub load_extension_config_vars {
   return unless defined $Polymake::Core::Application::extension;

   my %result;
   foreach my $ext ($Polymake::Core::Application::extension, @{$Polymake::Core::Application::extension->requires}) {
      my $ext_dir=$ext->dir;
      my $conf_file= $ext_dir =~ s{^${Polymake::InstallTop}/(ext|bundled)/}{${Polymake::InstallArch}/$1/}
                     ? "$ext_dir/conf.make"
                     : "$ext_dir/build/${Polymake::Arch}/conf.make";
      if (open my $CF, $conf_file) {
         local $_;
         while (<$CF>) {
            if (my ($name, $value)=/^\s* (\w+) \s*:=\s* (.*?) \s*$/x) {
               # don't read incrementally growing variables like CFlags
               if ($name ne "RequireExtensions" && $value !~ /\$[{(]$name[})]/) {
                  $result{$name}=$value;
               }
            }
         }
      }
   }
   \%result;
}
###############################################################################################
# "PackageName", \*FILEHANDLE =>
# Write the configuration variables into a makefile.
sub write_conf_vars {
   no strict 'refs';
   my ($pkg, $conf)=@_;

   foreach my $var (@{"$pkg\::make_vars"}, @{"$pkg\::make_export_vars"}) {
      if (defined (my $value=${"$pkg\::$var"})) {
         if ($pkg ne __PACKAGE__ && $var =~ /(?:flags|libs)/i) {
            print $conf "$var := $value \${$var}\n";
         } else {
            print $conf "$var := $value\n";
         }
      }
   }

   if (my @export=grep { defined(${"$pkg\::$_"}) } @{"$pkg\::make_export_vars"}) {
      print $conf "export ", join(" ", @export), "\n";
   }
}
###############################################################################################
# Write the currently active options in the comment line of the config file as to be picked later
sub write_config_command_line {
   my ($conf, $options, $allowed_with, $vars, $ext_failed)=@_;

   if (defined $vars) {
      while (my ($item, $value)=each %$vars) {
         $value="'$value'" if $value =~ /[\s(){}\[\]\$]/;
         print $conf " $item=$value";
      }
   }

   while (my ($item, $value)=each %$options) {
      if ($value eq ".none.") {
         unless ($ext_failed && $ext_failed->{$item}) {
            print $conf " --without-$item";
         }
      } else {
         $value="'$value'" if $value =~ /[\s(){}\[\]\$]/;
         print $conf exists $allowed_with->{$item}
                     ? " --with-$item=$value"
                     : " --$item=$value";
      }
   }
}
###############################################################################################
# If requested, read the leading comments of a config file and restore the command line options
# \@ARGV, "top_directory" =>
sub try_restore_config_command_line {
   my $argv=shift;

   # look for --repeat option; it may be combined with PERL=XXX only.
   return if !@$argv || @$argv > 3;

   my @argv=@$argv;
   my $perl;
   if (@argv > 1) {
      if ($argv[0] =~ /^PERL=.*/) {
         $perl=shift @argv;
      } elsif ($argv[-1] =~ /^PERL=.*/) {
         $perl=pop @argv;
      }
   }

   my $repeat;
   if (@argv==1 && $argv[0] =~ /^--repeat=(.*)/) {
      $repeat=$1;
   } elsif (@argv==2 && $argv[0] eq "--repeat") {
      $repeat=$argv[1];
   }

   if (defined $repeat) {
      my $builddir=(shift || ".");
      $builddir.= $^O eq "darwin" && $repeat !~ /^darwin\./ ? "/build.darwin.$repeat" : "/build.$repeat";
      -f "$builddir/conf.make"
        or die "Wrong value of --repeat option: configuration file $builddir/conf.make does not exist\n";
      open my $conf, "$builddir/conf.make"
        or die "Can't read $builddir/conf.make: $!\n";

      @$argv=();
      local $_;
      if (<$conf> =~ m{\#.*configured with:\s*(\S.*)?$}) {
         if (defined($1)) {
            $_=$1;
         } elsif (<$conf> =~ m{\#\s+\S+/configure((?=\s).*)$}) {
            $_=$1;
         } else {
            $_="";
         }
      }
      if (defined($_)) {
         while (/\G\s+ ([-\w]+ =?) (?: (['"]) ([^'])+ \2 | (\S+))? /gx) {
            push @$argv, $1 . ($2 ? $3 : $4);
         }
         push @$argv, $perl if defined($perl);
      } else {
         die <<".";
Can't restore options given to the last `configure' command because the configuration file $builddir/conf.make lacks header lines.
It has probably been manually edited and/or corrupted.
Please run the configure command specifying all desired options on the command line.
.
      }
   }
}
###############################################################################################
# Try to guess the name of a directory containing the appropriate version of a (shared) library.
sub get_libdir {
   my ($prefix, $name, $suffix)=@_;
   $suffix ||= $Config::Config{so};
   if ($Config::Config{longsize}==8 && $CXXflags !~ /-m32/ && $Arch =~ /i?86/ && -f "$prefix/lib64/lib$name.$suffix") {
      "$prefix/lib64"
   } elsif (($Config::Config{longsize}==4 || $CXXflags =~ /-m32/) && -f "$prefix/lib32/lib$name.$suffix") {
      "$prefix/lib32"
   } else {
      "$prefix/lib"
   }
}

sub v_cmp($$) {
   eval "v$_[0] cmp v$_[1]";
}
###############################################################################################

my $autonomous=!exists &Polymake::Tempfile::new;
my $tempdir=$autonomous ? Polymake::find_writable_dir() : Polymake::Tempfile->new_dir();

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
      open C, ">$tempdir/polymake_${$}_configure.cc" or die "$0: can't create a temporary file: $!\n";
      print C $_[0];
      close C;
      if ($_[1]) {
         # compile only
         `$CXX $CXXflags -c -o $tempdir/polymake_${$}_configure.o $tempdir/polymake_${$}_configure.cc 2>&1`
      } else {
         # compile and link
         `$CXX $CXXflags -o $tempdir/polymake_${$}_configure $tempdir/polymake_${$}_configure.cc $LDflags $Libs 2>&1`
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
   `$tempdir/polymake_${$}_configure`
}

END {
   $autonomous && unlink glob "$tempdir/polymake_${$}_configure*";
}
###############################################################################################
sub create_build_dir {
   my ($build_dir, $project_top, $ext_top)=@_;
   unless (-f "$build_dir/Makefile") {
      $project_top ||= $ext_top ? '${BuildDir}/../../..' : '${BuildDir}/..';   # bundled extensions lie two storeys deeper
      File::Path::mkpath($build_dir);
      open my $MF, ">$build_dir/Makefile" or die "can't create $build_dir/Makefile: $!\n";
      print $MF <<".";
BuildDir := ../..
ProjectTop := $project_top
ExtensionTop := $ext_top
include \${ProjectTop}/support/build.make
.
      close $MF;
   }
}
###############################################################################################
sub populate_bundled_extension_build_dir {
   my ($ext_dir, $build_dir)=@_;
   my $rel_ext_dir;
   if ($ext_dir =~ m{^/}) {
      die "wrong usage" if index($ext_dir, $Polymake::InstallTop);
      # $ext_dir is an absolute path starting with $Polymake::InstallTop = source tree,
      # because creating build directories for bundled extensions is only possible in developer mode
      $rel_ext_dir=substr($ext_dir, length($Polymake::InstallTop)+1);
   } else {
      $rel_ext_dir=$ext_dir;
   }
   $build_dir.="/$rel_ext_dir";

   File::Path::mkpath("$build_dir/lib");

   foreach (glob("$ext_dir/{apps,staticlib}/*")) {
      substr($_, 0, length($ext_dir))=$build_dir;
      create_build_dir($_, undef, "\${ProjectTop}/$rel_ext_dir");
   }
   if (-d "$ext_dir/java/native") {
      create_build_dir("$build_dir/lib/jni", undef, "\${ProjectTop}/$rel_ext_dir");
   }
   $build_dir
}
###############################################################################################
sub populate_extension_build_dir {
   my ($ext_dir)=@_;
   my $build_dir="$ext_dir/build.${Polymake::Arch}";

   File::Path::mkpath("$build_dir/lib");

   foreach (glob("$ext_dir/{apps,staticlib}/*")) {
      substr($_, 0, length($ext_dir))=$build_dir;
      create_build_dir($_, $Polymake::InstallTop, $ext_dir);
   }

   $build_dir
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
my $eval_pkg="Polymake::StandaloneExt";

# Extension => error message (if any occurred)
sub configure_extension {
   require Symbol;
   eval { &do_configure_extension };
   my $err=$@;
   Symbol::delete_package($eval_pkg);
   $err
}

sub do_configure_extension {
   no strict 'refs';

   my $ext=shift;
   my $ext_dir=$ext->dir;
   try_restore_config_command_line(\@_, $ext_dir);

   my (%options, %allowed_options, %allowed_with);

   check_extension_source_conflicts($ext_dir, $Polymake::InstallTop, map { $_->dir } @{$ext->requires});

   if (-f "$ext_dir/configure.pl") {
      load_make_vars();
      my $load_error=eval "package $eval_pkg; do '$ext_dir/configure.pl'; \$\@ ";
      if ($load_error) {
         die "corrupt configuration script:\n$load_error\n";
      }
      &{"$eval_pkg\::allowed_options"}(\%allowed_options, \%allowed_with);
   }

   while (defined (my $arg=shift)) {
      # trim and allow empty arguments
      $arg =~ s/^\s+//;
      $arg =~ s/\s+$//;
      next if ($arg =~ /^$/);
      if ($arg eq "--help") {
         if (exists &{"$eval_pkg\::usage"}) {
            print "Extension ", $ext->URI, " can be configured using following options:\n\n";
            &{"$eval_pkg\::usage"}();
         } else {
            print "Extension ", $ext->URI, " does not have any configuration options.\n";
         }
         die "silent\n";

      } elsif (my ($with, $out, $name, $value)= $arg =~ /^--(with(out)?-)?([-\w]+)(?:=(.*))?$/) {
         if ($with ? exists $allowed_with{$name}
                   : exists $allowed_options{$name}
               and
             defined($out) || defined($value) || @_) {
            if (defined($out)) {
               $value=".none.";
            } else {
               $value=shift unless defined($value);
               $value =~ s{^~/}{$ENV{HOME}/};
               $value =~ s{/+$}{};
            }
            $options{$name}=$value;
            next;
         }
      }
      if (exists &{"$eval_pkg\::usage"}) {
         print "Unrecognized option $arg;\nFollowing options are allowed:\n\n";
         &{"$eval_pkg\::usage"}();
      } else {
         print "Unrecognized option $arg;\nExtension ", $ext->URI, " does not have any configuration options.\n";
      }
      die "silent\n";
   }

   if (exists &{"$eval_pkg\::proceed"}) {
      &{"$eval_pkg\::proceed"}(\%options);
   }

   my $ext_build_dir=populate_extension_build_dir($ext_dir);
   my $conf_file="$ext_build_dir/conf.make";
   open my $CF, ">", $conf_file
      or die "can't create $conf_file: $!\n";
   print $CF "# last configured with:";
   write_config_command_line($CF, \%options, \%allowed_with);
   print $CF "\n\n",
             "ifndef ImportedIntoExtension\n",
             "# not included on behalf of another extension\n",
             "include $Polymake::InstallArch/conf.make\n",
             required_extensions($ext),
             "endif\n";

   if (@{"$eval_pkg\::make_vars"}) {
      print $CF "ExtensionTop := $ext_dir\n";
      write_conf_vars($eval_pkg, $CF);
   }
   close $CF;
   $ext->configured_at=time;

   open my $MF, ">$ext_dir/Makefile"
     or die "can't create $ext_dir/Makefile: $!\n";
   print $MF <<".";
ProjectTop := $Polymake::InstallTop
include \${ProjectTop}/support/extension.make
.
   close $MF;
}

sub update_extension_build_dir {
   my ($ext)=@_;
   my $ext_build_dir= $ext->is_bundled ? populate_bundled_extension_build_dir($ext->dir, $Polymake::InstallArch) : populate_extension_build_dir($ext->dir);
   my $conf_file="$ext_build_dir/conf.make";
   open my $CF, $conf_file
     or die "can't open $conf_file: $!\n";
   my @conf=<$CF>;
   close $CF;
   open $CF, ">", $conf_file
      or die "can't create $conf_file: $!\n";
   foreach (@conf) {
      if (/^\s*RequireExtensions\s*:?=/) {
         print $CF required_extensions($ext);
      } else {
         print $CF $_;
      }
   }
   close $CF;
}

sub create_bundled_extension_build_dir {
   my ($ext_dir, $top_build_dir, $conf_pkg, @prereqs)=@_;
   my $ext_build_dir=populate_bundled_extension_build_dir($ext_dir, $top_build_dir);
   my $has_old_conf=rename "$ext_build_dir/conf.make", "$ext_build_dir/conf.make.old";
   open my $CF, ">$ext_build_dir/conf.make"
     or die "can't create $ext_build_dir/conf.make: $!\n";
   print $CF "ifndef ImportedIntoExtension\n",
             "# not included on behalf of another extension\n",
             "include \${BuildDir}/../../conf.make\n",
             "RequireExtensions := ", join(" ", map { "\${ProjectTop}/$_" } @prereqs), "\n",
             "endif\n";
   no strict 'refs';
   if (defined($conf_pkg) && defined *{"$conf_pkg\::make_vars"}{ARRAY}) {
      print $CF "ExtensionTop := \${ProjectTop}/$ext_dir\n";
      write_conf_vars($conf_pkg, $CF);
   }
   close $CF;
   if ($has_old_conf) {
      require File::Compare;
      if (File::Compare::compare("$ext_build_dir/conf.make.old", "$ext_build_dir/conf.make")) {
         # configuration changed: delete all binary artifacts
         require File::Find;
         File::Find::find({ wanted => \&delete_binaries, no_chdir => 1 }, $ext_build_dir);
      }
      unlink "$ext_build_dir/conf.make.old";
   }
}

# passed as a `wanted' callback to File::Find::find
sub delete_binaries {
   /\.(?:o|a|so|bundle|dep)$/ and unlink $File::Find::name
}

sub required_extensions {
   my $ext=shift;
   if (@{$ext->requires} && ref($ext->requires->[0])) {
      ("RequireExtensions := ", join(" ", map { $_->is_bundled ? do { (my $dir=$_->URI) =~ tr|:|/|; "\${ProjectTop}/$dir" } : $_->dir } @{$ext->requires}), "\n");
   } else {
      ("RequireExtensions := ", join(" ", map { (my $dir=$_) =~ tr|:|/|; "\${ProjectTop}/$dir" } @{$ext->requires}), "\n");
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
