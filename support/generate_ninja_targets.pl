#  Copyright (c) 1997-2023
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

use strict;
use feature "state";

require Config;
require File::Path;

use vars '$root', '%ConfigFlags';
my ($targets_file, $source_list_file, $cpperl_list_file, $config_file);
my (%targetlist_deps, @all_source_dirs, @all_source_files, @all_cpperl_dirs, @all_cpperl_files);

if ($ARGV[0] eq "--source-list") {
   (undef, $source_list_file, @all_source_dirs)=@ARGV;
   update_file_list($source_list_file, \@all_source_dirs, "*.{cc,cpp,C,xxs}");
   exit 0;
}
if ($ARGV[0] eq "--cpperl-list") {
   (undef, $cpperl_list_file, @all_cpperl_dirs)=@ARGV;
   update_file_list($cpperl_list_file, \@all_cpperl_dirs, "*.cpperl");
   exit 0;
}

($targets_file, $root, $config_file)=@ARGV;

unless (@ARGV == 3 && -f $config_file && -d $root) {
   die "usage: $0 BUILD_ROOT/targets.ninja PROJECT_TOP BUILD_ROOT/config.ninja\n";
}

do "$root/support/install_utils.pl";
die $@ if $@;

my $builddir = dirname($config_file);
%ConfigFlags = load_config_file($config_file, $root);

my $core_mode = !exists $ConfigFlags{extroot};
my $testscenario_pico_mode = $core_mode && $root =~ m{/testscenarios/(?:[^/]+/){2}polymake-pico};
my $srcroot = $core_mode ? $root : $ConfigFlags{extroot};
my $srcrootname = $core_mode ? '${root}' : '${extroot}';
my $DeveloperMode = -d "$srcroot/.git";

my @prereq_exts = $core_mode ? () : ($ConfigFlags{RequireExtensions} =~ /(\S+)/g);

$source_list_file = "$builddir/sources.lst";
$cpperl_list_file = "$builddir/cpperl/input.lst";
my $cpperl_gen_file = "$builddir/cpperl/gen.ninja";
my (%cpperl_def_files, %cpperl_config_dependent, %cpperl_mods);

load_cpperl_gen_rules($cpperl_gen_file);
generate_targets();
generate_filelist_rules();
record_deps($targets_file, "$targets_file.d", \%targetlist_deps);
# the tagrets file must appear younger than all its dependencies generated before
finalize_targets_file();
exit 0;

################################################################################
sub read_custom_script {
   my ($filename)=@_;
   my @result;
   if (-f $filename) {
      $targetlist_deps{$filename}=1;
      @result=do $filename;
      if ($@) {
         die "could not process configuration file $filename: $@\n";
      }
      if ($!) {
         die "could not read configuration file $filename: $!\n";
      }
   }
   @result
}

sub process_app_sources {
   my ($app, $objects, $bundled, $link_flags)=@_;
   # this will trigger regeneration after adding or removing the src or cpperl subdirectory
   $targetlist_deps{$app}=1;

   my $app_name=basename($app);
   my $app_path=($bundled ? "/bundled/$bundled" : "")."/apps/$app_name";
   my $out_dir='${buildtop}'.$app_path;
   my $cpperl_gen_dir="$app/cpperl/generated";
   process_app_cpperl_lists("$app/cpperl", $cpperl_gen_dir) if -d "$app/cpperl";

   my $obj_count_before=keys %$objects;

   my $app_build_flags="$app/src/build_flags.pl";
   my %custom_flags=read_custom_script($app_build_flags);
   my $common_cxxflags= delete($custom_flags{CXXFLAGS}) . " -DPOLYMAKE_APPNAME=$app_name";
   if ($bundled) {
      $common_cxxflags.=" -DPOLYMAKE_BUNDLED_EXT=$bundled";
      if (length($ConfigFlags{"bundled.$bundled.CXXFLAGS"})) {
         $common_cxxflags.=" \${bundled.$bundled.CXXFLAGS}";
      }
   }

   my $common_cxxincludes="  CXXincludes=" . ($bundled ? "\${bundled.$bundled.includes} " : "") . "\${app.includes} \${core.includes}\n";

   my $pre_gen="";
   if ($bundled && (my $generated=delete $custom_flags{GENERATED})) {
      my $gen_in=delete $generated->{in};
      my $gen_out=delete $generated->{out};
      my $gen_command=delete $generated->{command};
      check_for_unknown_flags($generated, "generated files for $app");

      $link_flags->{generated}->{"$bundled.$app_name"}=
        "build $gen_out : gen_sources $gen_in\n" .
        "  GenerateCommand=$gen_command\n\n";
      $pre_gen=" || $gen_out";
   }

   if (-d "$app/src") {
      push @all_source_dirs, "$app/src";

      my $ignore_sources = delete $custom_flags{IGNORE};

      foreach my $src_file (glob "$app/src/*.{cc,cpp,C}") {
         my ($src_name, $obj_name)=basename($src_file, "(?:cc|cpp|C)");
         push @all_source_files, $src_file;
         next if $ignore_sources->{$src_name};
         my $src_file_in_rules= $src_file =~ s/^\Q$srcroot\E/$srcrootname/r;
         my $obj_file="$out_dir/$obj_name.o";
         my ($cxx_extra_flags, $override_flags)=("")x2;
         my $file_specific_flags=delete $custom_flags{$src_name};
         if (ref($file_specific_flags) eq "HASH") {
            $cxx_extra_flags=delete $file_specific_flags->{CXXextraFLAGS};
            $override_flags=join("", map { "  $_=$file_specific_flags->{$_}\n" } sort keys %$file_specific_flags);
         } else {
            $cxx_extra_flags=$file_specific_flags;
         }
         $objects->{$obj_file}= <<"---" . $override_flags . $common_cxxincludes;
build $obj_file : cxxcompile $src_file_in_rules$pre_gen
  CXXextraFLAGS=$cxx_extra_flags $common_cxxflags -DPOLYMAKE_DEFINITION_SOURCE_FILE="$src_name"
---
      }
   }

   if (-d $cpperl_gen_dir) {
      my %other_custom_flags;
      if (!$core_mode) {
         # a wrapper file in one extension may refer to a source file in another extension
         # or in the core
         foreach my $top_dir (@prereq_exts, $root) {
            my $src_dir = ($top_dir =~ s|^(?=\w+$)|$root/bundled/|r) . "/apps/$app_name/src";
            my %other_flags=read_custom_script("$src_dir/build_flags.pl");
            $other_custom_flags{$src_dir}=\%other_flags;
         }
      }

      foreach my $src_file (glob "$cpperl_gen_dir/*.{cc,cpp,C}") {
         my $src_file_in_rules= $src_file =~ s/^\Q$srcroot\E/$srcrootname/r;
         if (defined (my $def_file=$cpperl_mods{$src_file_in_rules})) {
            # cpperl definition exists - check the main C++ module
            my $real_src_file=$src_file;
            my $is_wrapper= $src_file =~ s{/wrap-([^/]+)$}{/$1};
            my ($src_name, $obj_name)=basename($src_file, "(?:cc|cpp|C)");
            my $obj_file="$out_dir/$obj_name.o";
            if ($is_wrapper) {
               if (defined (my $build_rule=$objects->{$obj_file})) {
                  # the main module lives in the sibling source tree
                  $build_rule =~ s{cxxcompile \K\S+}{$src_file_in_rules};
                  $build_rule =~ s{CXXextraFLAGS=.*\K}{ -DPOLYMAKE_DEFINITION_SOURCE_DIR="../../src"}m;
                  $objects->{$obj_file}=$build_rule;
                  next;
               }
               if (!$core_mode) {
                  my $main_src_dir;
                  foreach (keys %other_custom_flags) {
                     if (-f "$_/$src_name") {
                        $main_src_dir=$_;  last;
                     }
                  }
                  if (defined $main_src_dir) {
                     my $custom_cxxflags=join(" ", grep { defined } @{$other_custom_flags{$main_src_dir}}{'CXXFLAGS', $src_name});
                     $objects->{$obj_file}= <<"---" . $common_cxxincludes;
build $obj_file : cxxcompile $src_file_in_rules
  CXXextraFLAGS=$common_cxxflags $custom_cxxflags -DPOLYMAKE_DEFINITION_SOURCE_DIR="$main_src_dir" -DPOLYMAKE_NO_EMBEDDED_RULES
---
                     next;
                  }
               }
               # main module has been deleted or renamed,
               # but the old interface definition is still lingering around there
               update_cpperl_gen_rules($def_file =~ s/^\Q$srcrootname\E/$srcroot/r);
               unlink $real_src_file;
            } else {
               # class or auto-function instantiation file
               $obj_file="$out_dir/cpperl/$obj_name.o";
               $objects->{$obj_file}= <<"---" . $common_cxxincludes;
build $obj_file : cxxcompile $src_file_in_rules$pre_gen
  CXXextraFLAGS=$common_cxxflags
---
            }
         } else {
            print STDERR "removing obsolete interface module $src_file\n";
            unlink $src_file;
         }
      }
   }

   if ($bundled) {
      if (defined(my $staticlib_flags = delete $custom_flags{staticlib})) {
         process_staticlib($app, $staticlib_flags, $pre_gen, $bundled, $link_flags);
      }
      if (length($ConfigFlags{"bundled.$bundled.LDFLAGS"})) {
         push @{$link_flags->{LDextraFLAGS}}, "\${bundled.$bundled.LDFLAGS}";
      }
      if (length($ConfigFlags{"bundled.$bundled.LIBS"})) {
         push @{$link_flags->{LIBSextra}}, "\${bundled.$bundled.LIBS}";
      }
   }
   check_for_unknown_flags(\%custom_flags, $app_build_flags);

   # check for obsolete binary artifacts
   $out_dir =~ s{^\$\{buildtop\}}{$builddir/*};
   if (keys %$objects > $obj_count_before) {
      foreach my $obj_file (glob("$out_dir/{,perl/,cpperl/}*.o")) {
         my $obj_file_in_rules = $obj_file =~ s{^\Q$builddir/\E[^/]+}{\${buildtop}}r;
         if (!exists $objects->{$obj_file_in_rules}) {
            print STDERR "removing obsolete object file $obj_file\n";
            unlink $obj_file;
         }
      }
   } else {
      # application without C++ components
      if (my @obj_dirs=glob($out_dir)) {
         print STDERR "removing obsolete objects in @obj_dirs\n";
         File::Path::remove_tree(@obj_dirs);
      }
   }
}

sub check_for_unknown_flags {
   my ($flags, $where)=@_;
   if (keys %$flags) {
      my (@unknown, @obsolete);
      foreach (keys %$flags) {
         if (/\.\w+$/) {
            push @obsolete, $_;
         } else {
            push @unknown, $_;
         }
      }
      die @unknown ? "unknown flags specified in $where: @unknown\n" : "",
          @obsolete ? "obsolete client files occur in $where: @obsolete\n" : "";
   }
}

sub process_staticlib {
   my ($app, $staticlib_flags, $pre_gen, $bundled, $link_flags) = @_;
   my $src_dir = delete $staticlib_flags->{SOURCEDIR} or die "SOURCEDIR missing in staticlib flags in $app\n";

   my $lib_name = delete $staticlib_flags->{LIBNAME} // $bundled;
   my $CFLAGS = delete $staticlib_flags->{CFLAGS};
   my $CXXFLAGS = delete $staticlib_flags->{CXXFLAGS};
   my $source_files = delete $staticlib_flags->{SOURCES} or die "SOURCES missing in staticlib flags in $app\n";

   my $out_dir = "\${buildtop}/staticlib/$lib_name";
   my $out = "";
   my @obj_files;
   foreach my $src_file (@$source_files) {
      my $obj_file = "$out_dir/";
      my $extra_flags = delete $staticlib_flags->{$src_file};
      if ($src_file =~ /\.c$/) {
         $obj_file .= $`.".o";
         $out .= <<"---";
build $obj_file : ccompile $src_dir/$src_file$pre_gen
  CextraFLAGS=$CFLAGS $extra_flags \${CflagsSuppressWarnings}
  CmodeFLAGS=\${CexternModeFLAGS}

---
      } elsif ($src_file =~ /\.(?:cc|cpp|C)$/) {
         $obj_file.=$`.".o";
         $out .= <<"---";
build $obj_file : cxxcompile $src_dir/$src_file$pre_gen
  CXXextraFLAGS=$CXXFLAGS $extra_flags \${CflagsSuppressWarnings}
  CmodeFLAGS=\${CexternModeFLAGS}

---
      } else {
         die "don't know how to compile source file $src_file for static library in $bundled\n";
      }
      push @obj_files, $obj_file;
   }

   check_for_unknown_flags($staticlib_flags, "flags for staticlib in $app");

   $lib_name="$out_dir/lib$lib_name.a";
   $out .= "build $lib_name : staticlib @obj_files\n";
   push @{$link_flags->{staticlibs}}, $lib_name;
   push @{$link_flags->{staticlibcmds}}, $out;
}

sub process_app_cpperl_lists {
   my ($cpperl_dir, $cpperl_gen_dir)=@_;
   push @all_cpperl_dirs, $cpperl_dir;
   foreach my $def_file (glob("$cpperl_dir/*.cpperl")) {
      my $def_file_in_rule= $def_file =~ s/^\Q$srcroot\E/$srcrootname/r;
      unless (exists $cpperl_def_files{$def_file_in_rule}) {
         # discovered a new cpperl definition file
         state $script_loaded = do {
            local @ARGV;
            do "$root/support/generate_cpperl_modules.pl";
            die $@ if $@;
         };
         -d $cpperl_gen_dir or File::Path::make_path($cpperl_gen_dir);
         update_cpperl_gen_rules(Polymake::GenerateCppPerl::process_def_file($def_file, $cpperl_gen_dir, {}));
      }
      push @all_cpperl_files, $def_file;
   }
}

sub generate_targets {
   # this will trigger regeneration after adding or renaming an application 
   $targetlist_deps{"$srcroot/apps"}=1;

   my @apps=glob "$srcroot/apps/*";

   # BundledExts is ordered such that dependent extensions appear after their prerequisites;
   # here the dependent static libraries, if any, should come first in the linker command
   my @bundled = $core_mode ? ($ConfigFlags{BundledExts}) =~ /(\S+)/g : ();
   my @bundled_rev = reverse(@bundled);
   foreach my $bundled (@bundled) {
      # this will trigger regeneration after adding or renaming an application in the given extension
      $targetlist_deps{"$root/bundled/$bundled/apps"}=1;
   }

   my %objects_per_app;
   my %link_flags_per_app;

   foreach my $app (@apps) {
      my $app_name=basename($app);
      my (%objects, %link_flags);
      process_app_sources($app, \%objects);
      foreach my $bundled (@bundled_rev) {
         if (-d (my $app_in_bundled="$root/bundled/$bundled/apps/$app_name")) {
            process_app_sources($app_in_bundled, \%objects, $bundled, \%link_flags);
         }
      }
      if (keys %objects) {
         $objects_per_app{$app_name}=\%objects;
         $link_flags_per_app{$app_name}=\%link_flags;
      } else {
         $objects_per_app{$app_name}=undef;
      }
   }

   open OUT, ">$targets_file.new" or die "can't create $targets_file.new: $!\n";
   select OUT;

   if ($core_mode) {
      # additional flags for building C++ modules in bundled extensions should be kept separately
      # because they are needed for temporary wrappers too
      my $bundled_flags = $targets_file =~ s{(?:^|/) \K [^/.]+ (?=\.ninja$)}{bundled_flags}xr;
      open my $B, ">", $bundled_flags
        or die "can't write to $bundled_flags: $!\n";
      foreach my $bundled (@bundled_rev) {
         my @look_in = ($bundled, $ConfigFlags{"bundled.$bundled.RequireExtensions"} =~ /(\S+)/g);
         my @includes = glob("$root/bundled/{".join(",", @look_in)."}/include/app*");
         s/^\Q$root\E/$srcrootname/ for @includes;
         print $B "bundled.$bundled.includes=", (map { " -I$_" } @includes), "\n\n";
      }
      close $B;
      print "include \${buildroot}/bundled_flags.ninja\n";
   } else {
      # an extension might depend on bundled extensions, in particular the private wrapper collection
      if (my @prereq_bundled=grep { /^\w+$/ } @prereq_exts) {
         if (-d "$root/lib/core/src") {
            # refers to the core system in a workspace
            my @add_includes = map { ("-I\${root}/bundled/$_/include/app-wrappers", "-I\${root}/bundled/$_/include/apps") } @prereq_bundled;
            print <<"---";
partial.app.includes=\${app.includes}
app.includes=\${partial.app.includes} @add_includes
---
         }
         my @add_cxxflags = map { "\${bundled.$_.CXXFLAGS}" } @prereq_bundled;
         my @add_ldflags = map { "\${bundled.$_.LDFLAGS}" } @prereq_bundled;
         my @add_libs = map { "\${bundled.$_.LIBS}" } @prereq_bundled;
         print <<"---";
partial.CXXFLAGS=\${CXXFLAGS}
CXXFLAGS=\${partial.CXXFLAGS} @add_cxxflags
partial.LDFLAGS=\${LDFLAGS}
LDFLAGS=\${partial.LDFLAGS} @add_ldflags
partial.LIBS=\${LIBS}
LIBS=\${partial.LIBS} @add_libs

---
      }
   }

   my @all_app_targets;

   foreach my $app (@apps) {
      my $app_name = basename($app);
      my $app_shared_module = "\${buildtop}/lib/$app_name.$Config::Config{dlext}";
      push @all_app_targets, $app_shared_module;

      if (defined(my $objects = $objects_per_app{$app_name})) {
         my $link_flags = $link_flags_per_app{$app_name};
         if (my $generated = delete $link_flags->{generated}) {
            foreach my $gen_target (sort keys %$generated) {
               print $generated->{$gen_target}, "\n";
            }
         }
         my @all_objects = sort keys %$objects;
         foreach my $obj_file (@all_objects) {
            print $objects->{$obj_file}, "\n";
         }
         if (my $staticlibs = delete $link_flags->{staticlibs}) {
            push @all_objects, @$staticlibs;
            foreach my $commands (@{delete $link_flags->{staticlibcmds}}) {
               print $commands, "\n";
            }
         }
         print "build $app_shared_module : sharedmod @all_objects\n";
         foreach my $flag (sort keys %$link_flags) {
            print "  $flag=", join(" ", @{$link_flags->{$flag}}), "\n";
         }
         print "\n";
      } else {
         # application without any C++ components
         if (my @obsolete = grep { (stat $_)[7] != 0 } glob("$builddir/*/lib/$app_name.$Config::Config{dlext}")) {
            print STDERR "removing obsolete shared module(s) @obsolete\n";
            unlink @obsolete;
         }
         print "build $app_shared_module : emptyfile\n\n";
      }
   }

   if (!$core_mode) {
      # wipe disappeared applications from the extension
      foreach my $app_mod (glob("$builddir/*/lib/*.$Config::Config{dlext}")) {
         my ($app_name) = $app_mod =~ m{/([^/]+)\.$Config::Config{dlext}$};
         if (!exists $objects_per_app{$app_name}) {
            print STDERR "removing obsolete shared module $app_mod\n";
            unlink $app_mod;
            File::Path::remove_tree(glob("$builddir/*/apps/$app_name"));
         }
      }
   }

   print <<"---";
# flag file may be deleted after wrapper generation and extension reconfiguring
build \${buildtop}/.apps.built : emptyfile | @all_app_targets

---
   my @all_targets = ('${buildtop}/.apps.built', @all_app_targets);
   my @all_clean_targets;

   if ($core_mode) {
      if ($ConfigFlags{LDcallableFLAGS} ne "none") {
         print <<"---";
build \${buildroot}/applib/fake.c : gen_applib_stubs @all_app_targets
build \${buildtop}/lib/callable/fake.o : ccompile \${buildroot}/applib/fake.c
  CextraFLAGS=-DPOLYMAKE_FAKE_FUNCTIONS
build \${buildtop}/lib/callable/stub.o : ccompile \${buildroot}/applib/fake.c

---
      }

      my @all_bundled_targets;
      foreach my $bundled (@bundled) {
         if (-f "$root/bundled/$bundled/support/rules.ninja") {
            print "include $srcrootname/bundled/$bundled/support/rules.ninja\n";
            $targetlist_deps{"$root/bundled/$bundled/support/rules.ninja"}=1;
         }
         add_custom_targets("$root/bundled/$bundled/support/generate_ninja_targets.pl",
                            all => \@all_bundled_targets, clean => \@all_clean_targets);
      }
      if (@all_bundled_targets) {
         print <<"---";
build all.bundled : phony @all_bundled_targets
---
         push @all_targets, 'all.bundled';
      }
      my ($install_deps, $install_libs) = generate_corelib_targets();
      print <<"---";
build install : install_core || all $install_deps
  install_libs=$install_libs
---
      if (!$testscenario_pico_mode) {
         push @all_targets, "all.libs";
      }

      # enforce reconfiguration when one of the scripts has changed
      my @bundled_config_scripts = map { s/^\Q$root\E/$srcrootname/r } grep { -f }
                                   map { "$root/bundled/$_/support/configure.pl" } @bundled;
      print <<"---";
build \${config.file}: reconfigure | \${root}/support/configure.pl @bundled_config_scripts
---
   } else {
      if (-f "$srcroot/support/rules.ninja") {
         print "include $srcrootname/support/rules.ninja\n";
         $targetlist_deps{"$srcroot/support/rules.ninja"}=0;
      }
      add_custom_targets("$srcroot/support/generate_ninja_targets.pl",
                         all => \@all_targets, clean => \@all_clean_targets);
      print <<"---";
build install : install_ext || all
---
   }

   print <<"---";
build all : phony @all_targets
build clean.all : phony @all_clean_targets
---
}

sub add_custom_targets {
   my ($gen_script, %target_lists)=@_;
   my @targets=read_custom_script($gen_script);
   while (my ($group, $target)=splice @targets, 0, 2) {
      if (defined (my $list=$target_lists{$group})) {
         push @$list, $target;
      } else {
         die "unknown top-level target $group produced by script $gen_script\n";
      }
   }
}

sub generate_corelib_targets {

   my $Ext_module="\${buildtop}/\${perlxpath}/auto/Polymake/Ext/Ext.$Config::Config{dlext}";

   my ($callable_libname, $callable_link, $fakeapps_libname, $fakeapps_link, $stubapps_libname, $stubapps_link, $corelib_archive);
   if ($ConfigFlags{LDcallableFLAGS} ne "none") {
      my $callable_version = extract_polymake_version($root);
      ($callable_libname, $callable_link) = compose_sharedlib_names("polymake", $callable_version);
      ($fakeapps_libname, $fakeapps_link) = compose_sharedlib_names("polymake-apps", $callable_version);
      ($stubapps_libname, $stubapps_link) = compose_sharedlib_names("polymake-apps-rt", $callable_version);
      $corelib_archive = "libpolymake-core.a";
      print <<"---";
callable_lib =\${buildtop}/\${perlxpath}/$callable_libname
callable_link=\${buildtop}/\${perlxpath}/$callable_link
fakeapps_lib =\${buildtop}/lib/callable/$fakeapps_libname
fakeapps_link=\${buildtop}/lib/callable/$fakeapps_link
stubapps_lib =\${buildtop}/lib/callable/$stubapps_libname
stubapps_link=\${buildtop}/lib/callable/$stubapps_link
corelib_archive=\${buildtop}/lib/$corelib_archive
---
   }

   if (!$testscenario_pico_mode) {
      my (@corelib_objects, @corelib_xs_objects, @callable_objects);

      # perl-independent modules
      my $out_dir = '${buildtop}/lib/core';
      foreach my $src_file (glob "$root/lib/core/src/*.cc") {
         my ($src_name, $obj_file) = basename($src_file, "cc");
         push @all_source_files, $src_file;
         $src_file =~ s/^\Q$root\E/$srcrootname/;
         $obj_file = "$out_dir/$obj_file.o";
         push @corelib_objects, $obj_file;
         print "build $obj_file : cxxcompile $src_file\n" .
               "  CXXincludes=\${core.includes}\n";
      }

      # perl-dependent modules
      $out_dir = '${buildtop}/lib/${perlxpath}';
      my $mode_flags = $Config::Config{optimize} =~ /-O[1-9]/ ? "" : "  CmodeFLAGS=\${CDebugFLAGS}\n";

      my @perl_cc = glob("$root/lib/core/src/perl/*.cc");
      my @perl_xxs = glob("$root/lib/core/src/perl/*.xxs");
      push @all_source_files, @perl_cc, @perl_xxs;
      my @cc_from_xxs = map { "\${buildroot}/\${perlxpath}/".basename($_, "xxs").".cc" } @perl_xxs;

      foreach my $src_file (@cc_from_xxs) {
         my $xxs_file = shift(@perl_xxs);
         my $xxs_file_in_rules = $xxs_file =~ s/^\Q$root\E/$srcrootname/r;
         print "build $src_file : xxs_to_cc $xxs_file_in_rules\n";
         if (-f (my $typemap = $xxs_file =~ s/\.xxs$/.typemap/r)) {
            $typemap =~ s/^\Q$root\E/$srcrootname/;
            print "  XSextraTYPEMAPS = -typemap $typemap\n";
         }
      }
      print "\n";

      my %glue_custom_flags=read_custom_script("$root/lib/core/src/perl/build_flags.pl");

      foreach my $src_file (@perl_cc, @cc_from_xxs) {
         my ($src_name, $obj_file) = basename($src_file, "cc");
         $src_file =~ s/^\Q$root\E/$srcrootname/;
         $obj_file="$out_dir/$obj_file.o";
         push @corelib_xs_objects, $obj_file;
         print "build $obj_file : cxxcompile $src_file\n",
               "  CXXextraFLAGS = $glue_custom_flags{CXXFLAGS} $glue_custom_flags{$src_name}\n",
               $mode_flags,
               "  CXXincludes=\${core.includes}\n\n";
      }

      print <<"---";
build $Ext_module : sharedmod @corelib_objects @corelib_xs_objects
  LIBSextra=$glue_custom_flags{LIBS}

---

      push @all_source_dirs, "$root/lib/core/src", "$root/lib/core/src/perl";

      if ($ConfigFlags{LDcallableFLAGS} ne "none") {
         my %callable_custom_flags=read_custom_script("$root/lib/callable/src/perl/build_flags.pl");
         print <<"---";
bootstrapXS.h=\${buildroot}/\${perlxpath}/polymakeBootstrapXS.h
build \${bootstrapXS.h} : gen_xs_bootstrap @cc_from_xxs

---
         foreach my $src_file (glob("$root/lib/callable/src/perl/*.cc")) {
            my ($src_name, $obj_file)=basename($src_file, "cc");
            push @all_source_files, $src_file;
            $src_file =~ s/^\Q$root\E/$srcrootname/;
            $obj_file="$out_dir/$obj_file.o";
            push @callable_objects, $obj_file;
            print "build $obj_file : cxxcompile $src_file || \${bootstrapXS.h}\n",
                  "  CXXextraFLAGS = $callable_custom_flags{CXXFLAGS} $callable_custom_flags{$src_name} $glue_custom_flags{CXXFLAGS}\n",
                  $mode_flags,
                  "  CXXincludes= -I\${buildroot}/\${perlxpath} \${core.includes} -I\${root}/include/callable\n\n";
         }

         print <<"---";
build \${callable_lib} : sharedmod @corelib_objects @corelib_xs_objects @callable_objects
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$callable_libname \${LIBperlFLAGS}
  LIBSextra=$glue_custom_flags{LIBS}
build \${callable_link} : symlink_samedir \${callable_lib}

build \${fakeapps_lib} : sharedmod \${buildtop}/lib/callable/fake.o
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$stubapps_libname
build \${fakeapps_link} : symlink_samedir \${fakeapps_lib}

build \${stubapps_lib} : sharedmod \${buildtop}/lib/callable/stub.o
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$stubapps_libname
build \${stubapps_link} : symlink_samedir \${stubapps_lib}

build \${corelib_archive} : staticlib @corelib_objects

---
         push @all_source_dirs, "$root/lib/callable/src/perl";
      }
   }

   if ($ConfigFlags{LDcallableFLAGS} ne "none") {
      print <<"---";
build all.libs : phony $Ext_module \${callable_link}
build callable-apps-libs : phony \${fakeapps_link} \${stubapps_link}
build all.corelib : phony \${corelib_archive}
---
      return ('callable-apps-libs',
              '--xs '.$Ext_module.' --callable ${callable_lib} ${callable_link} --fakelibs ${stubapps_lib} ${stubapps_link} ${fakeapps_lib} ${fakeapps_link}');
   } else {
      print <<"---";
build all.libs : phony $Ext_module
---
      return ('', '--xs '.$Ext_module);
   }
}


# stem, version => libname, versioned libname
sub compose_sharedlib_names {
   my ($stem, $version)=@_;
   ( $Config::Config{so} eq "so" ? "lib$stem.$Config::Config{so}.$version" : "lib$stem.$version.$Config::Config{so}",
     "lib$stem.$Config::Config{so}"
   )
}

sub record_deps {
   my ($target, $deps_file, $deps)=@_;
   open my $D, ">", $deps_file
     or die "could not create dependency file $deps_file: $!\n";

   print $D join(" \\\n", "$target:", map { "  $_" } keys(%$deps)), "\n";
   close $D;
}

sub finalize_targets_file {
   print "\ndefault all\n";
   close OUT;
   unlink $targets_file;
   rename "$targets_file.new", $targets_file
     or die "could not rename $targets_file.new to $targets_file: $!\n";
}
########################################################################################
sub create_file_list {
   my ($filename, $list)=@_;
   my $text = join(" ", sort @$list);
   open my $L, ">", $filename or die "can't create $filename: $!\n";
   print $L $text;
}

sub update_file_list {
   my ($filename, $dirlist, $pattern)=@_;
   my $text = join(" ", sort map { glob("$_/$pattern") } @$dirlist);
   if (-f $filename) {
      open my $L, "+<", $filename or die "can't modify $filename: $!\n";
      local $/;
      # only overwrite it if comething has changed, otherwise the target list generation can be skipped
      if (<$L> ne $text) {
         seek $L, 0, 0;
         truncate $L, 0;
         print $L $text;
      }
   } else {
      open my $L, ">", $filename or die "can't create $filename: $!\n";
      print $L $text;
   }
}
########################################################################################
sub load_cpperl_gen_rules {
   my ($filename)=@_;
   if (-f $filename) {
      open my $F, "<", $filename
        or die "can't read $filename: $!\n";
      while (<$F>) {
         if (my ($out, $in, $config_depend)= /^ *build +(.*?) *: *gen_cpperl_mod +(\S+)\s*(?:\| *\S+( +\$\{config\.file\})?|$)/) {
            my @out= $out =~ /(\S+)/g;
            my $in_file= $in =~ s/\$\{\w+\}/$srcroot/r;
            if (-f $in_file) {
               $cpperl_def_files{$in}=\@out;
               $cpperl_mods{$_}=$in for @out;
               $cpperl_config_dependent{$in}= $config_depend ne "";
            }
         } elsif (my ($files)= /^ *# *\@update +(.*)/) {
            # an update remark appended by prior rule
            my ($in_file, @out_files)= $files =~ /(\S+)/g;
            if (-f $in_file) {
               update_cpperl_gen_rules($in_file, @out_files);
            }
         } elsif (/^ *(?!build +(.*?) *: *(?:inspect|phony)\b)[^#\s]/) {
            die "invalid line in generated rules $filename: $_";
         }
      }
   }
}

# input_file, out_files ... =>
sub update_cpperl_gen_rules {
   my $in_file=shift;
   my $config_depend;
   if (@_ && $_[-1] eq "|config.file") {
      pop @_;
      $config_depend=1;
   }
   my $in=$in_file =~ s/^\Q$srcroot\E/$srcrootname/r;
   my @out= map { s/^\Q$srcroot\E/$srcrootname/r } @_;
   my $old_out_list=delete $cpperl_def_files{$in};
   if ($old_out_list) {
      delete @cpperl_mods{@$old_out_list};
   }
   if (@out) {
      $cpperl_def_files{$in}=\@out;
      $cpperl_config_dependent{$in}=$config_depend;
      $cpperl_mods{$_}=$in for @out;
   } else {
      # completely disappeared
      if ($core_mode && $DeveloperMode) {
         print STDERR "deleting obsolete interface definition file $in_file\n";
         $in_file =~ s{^\Q$root\E/}{};
         system("cd $root; git rm -f $in_file || rm -f $in_file");
      } elsif (-w $in_file) {
         print STDERR "deleting obsolete interface definition file $in_file\n";
         unlink $in_file;
      } else {
         print STDERR "WARNING: encountered an empty or obsolete interface definition file in_file\n";
      }
   }
}

sub write_cpp_gen_rules {
   my ($filename, $filename_in_rules)=@_;
   open my $R, ">", $filename
     or die "can't write to $filename: $!\n";
   my (@all_inputs, @all_generated);
   foreach my $in (sort keys %cpperl_def_files) {
      my @out=sort @{$cpperl_def_files{$in}};
      my $out=join(" ", @out);
      push @all_inputs, $in;
      push @all_generated, @out;
      my $depends='${root}/support/generate_cpperl_modules.pl';
      if ($cpperl_config_dependent{$in}) {
         $depends .= ' ${config.file}';
      }
      print $R <<"---";
build $out : gen_cpperl_mod $in | $depends
build $in : phony
---
   }
   print $R <<"---";
build $filename_in_rules : inspect @all_inputs || @all_generated
---
}
########################################################################################
sub generate_filelist_rules {
   my %file_in_rules=map { ($_ => s/\Q$builddir\E/\${buildroot}/r) }
                     $source_list_file, $cpperl_list_file, $cpperl_gen_file;

   my $after_cpperl_gen="";

   if (@all_cpperl_files) {
      # regeneration of the c++/perl interface list
      $targetlist_deps{$cpperl_list_file}=0;
      @all_cpperl_dirs = map { s/^\Q$srcroot\E/$srcrootname/r } sort @all_cpperl_dirs;
      print <<"---";

build $file_in_rules{$cpperl_list_file} : gen_file_list @all_cpperl_dirs | \${root}/support/generate_ninja_targets.pl
  what=cpperl-list
---
      print <<"---" for @all_cpperl_dirs;
build $_ : phony
---
      -d "$builddir/cpperl" or File::Path::make_path("$builddir/cpperl");
      create_file_list($cpperl_list_file, \@all_cpperl_files);
      write_cpp_gen_rules($cpperl_gen_file, $file_in_rules{$cpperl_gen_file});
      $targetlist_deps{$cpperl_gen_file}=0;
      print "include $file_in_rules{$cpperl_gen_file}\n";

      $after_cpperl_gen=" || $file_in_rules{$cpperl_gen_file}";
   } else {
      # there are currently no interface definitions, but some directories where new ones might pop up later
      # so we should watch for changes there
      $targetlist_deps{$_}=1 for @all_cpperl_dirs;
      unlink $cpperl_list_file, $cpperl_gen_file;
   }

   # regeneration of the source list
   $targetlist_deps{$source_list_file}=0;
   my @all_source_dirs = map { s/^\Q$srcroot\E/$srcrootname/r } sort @all_source_dirs;
   print <<"---";

build $file_in_rules{$source_list_file} : gen_file_list @all_source_dirs | \${root}/support/generate_ninja_targets.pl $after_cpperl_gen
  what=source-list
---
   print <<"---" for @all_source_dirs, (map { s/^\Q$srcroot\E/$srcrootname/r } grep { $targetlist_deps{$_} } sort keys %targetlist_deps);
build $_ : phony
---
   create_file_list($source_list_file, \@all_source_files);
}

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
