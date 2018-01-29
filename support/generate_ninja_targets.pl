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

use strict;
use Cwd;
use Config;
use File::Path;

use vars '$root', '%ConfigFlags';
my ($targets_file, $config_file);
($targets_file, $root, $config_file)=@ARGV;

unless (@ARGV==3 && -f $config_file && -d $root) {
   die "usage: $0 BUILD_ROOT/targets.ninja PROJECT_TOP BUILD_ROOT/config.ninja\n";
}

do "$root/support/install_utils.pl";
die $@ if $@;

my $buildroot=dirname($config_file);
%ConfigFlags=load_config_file($config_file, $root);

my $core_mode=!exists $ConfigFlags{extroot};
my $testscenario_pico_mode= $core_mode && $root =~ m{/testscenarios/(?:[^/]+/){2}polymake-pico};
my $srcroot= $core_mode ? $root : $ConfigFlags{extroot};
my $srcrootname= $core_mode ? '${root}' : '${extroot}';
my $DeveloperMode=-d "$srcroot/.git";

my @prereq_exts= $core_mode ? () : ($ConfigFlags{RequireExtensions} =~ /(\S+)/g);

my %targetlist_deps;
generate_targets();

# record the dependencies
open OUT, ">$targets_file.d"
  or die "could not create dependency file $targets_file.d: $!\n";

print OUT join(" \\\n", "$targets_file:", map { "  $_" } keys(%targetlist_deps)), "\n";
close OUT;

exit 0;

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
   -d "$app/src" or return;
   $targetlist_deps{"$app/src"}=1;

   my $app_name=basename($app);
   my $out_dir='${builddir}'.($bundled ? "/bundled/$bundled" : "")."/apps/$app_name";
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
        "build $gen_out: gen_sources $gen_in\n" .
        "  GenerateCommand=$gen_command\n\n";
      $pre_gen=" || $gen_out";
   }

   my $ignore_sources = delete $custom_flags{IGNORE};

   foreach my $src_file (glob "$app/src/*.{cc,cpp,C}") {
      my ($src_name, $obj_name)=basename($src_file, "(?:cc|cpp|C)");
      $src_file =~ s/^\Q$srcroot\E/$srcrootname/;
      next if $ignore_sources->{$src_name};
      $obj_name="$out_dir/$obj_name.o";
      my ($in, $includeSource);
      if (-f ($in="$app/src/perl/wrap-${src_name}")) {
         $in =~ s/^\Q$srcroot\E/$srcrootname/;
         $includeSource=$src_file;
      } else {
         $in=$src_file;
      }
      my ($cxx_extra_flags, $override_flags)=("")x2;
      my $file_specific_flags=delete $custom_flags{$src_name};
      if (ref($file_specific_flags) eq "HASH") {
         $cxx_extra_flags=delete $file_specific_flags->{CXXextraFLAGS};
         $override_flags=join("", map { "  $_=$file_specific_flags->{$_}\n" } sort keys %$file_specific_flags);
      } else {
         $cxx_extra_flags=$file_specific_flags;
      }
      $objects->{$obj_name}=
        "build $obj_name : cxxcompile $in$pre_gen\n" .
        ($includeSource && "  includeSource= -include $includeSource\n") .
        "  CXXextraFLAGS=$cxx_extra_flags $common_cxxflags\n" .
        $override_flags . $common_cxxincludes;
   }

   if (-d "$app/src/perl") {
      $targetlist_deps{"$app/src/perl"}=1;
      my (@to_groom, %other_custom_flags);
      foreach my $src_file (glob "$app/src/perl/*.{cc,cpp,C}") {
         push @to_groom, $src_file;
         my ($src_name, $obj_name)=basename($src_file, "(?:cc|cpp|C)");
         my $real_src_file=$src_file;
         $src_file =~ s/^\Q$srcroot\E/$srcrootname/;
         if (not $obj_name =~ s/^wrap-//) {
            $obj_name="$out_dir/perl/$obj_name.o";
            $objects->{$obj_name}=
              "build $obj_name : cxxcompile $src_file$pre_gen\n" .
              "  CXXextraFLAGS=$common_cxxflags\n" .
              $common_cxxincludes;
         } else {
            $obj_name="$out_dir/$obj_name.o";
            if (!exists $objects->{$obj_name}) {
               if ($core_mode) {
                  if ($DeveloperMode) {
                     print STDERR "WARNING: deleting stray wrapper file $real_src_file\n";
                     $src_file =~ s|^\$\{\w+\}/?||;
                     system("cd $root; git rm $src_file");
                  } else {
                     print STDERR "WARNING: ignoring a stray wrapper file $real_src_file\n";
                  }
               } else {
                  # wrapper in one extension refers to the source file in another extension or in the core system
                  $src_name =~ s/^wrap-//;
                  my ($src_dir, $includeSource);
                  foreach my $top_dir (@prereq_exts, $root) {
                     ($src_dir=$top_dir) =~ s|^(?=\w+$)|$root/bundled/|;
                     if (-f "$src_dir/apps/$app_name/src/$src_name") {
                        $src_dir .= "/apps/$app_name/src";
                        $includeSource="$src_dir/$src_name";
                        $includeSource=~ s|^\Q$root\E|\${root}|;
                        last;
                     }
                  }
                  if (defined $includeSource) {
                     $other_custom_flags{$src_dir} //= do {
                        my %other_flags=read_custom_script("$src_dir/build_flags.pl");
                        \%other_flags;
                     };
                     my $custom_cxxflags=join(" ", grep { defined } @{$other_custom_flags{$src_dir}}{'CXXFLAGS', $src_name});
                     $objects->{$obj_name}=
                       "build $obj_name : cxxcompile $src_file\n" .
                       "  includeSource= -include $includeSource\n" .
                       "  CXXextraFLAGS=$common_cxxflags $custom_cxxflags -DPOLYMAKE_NO_EMBEDDED_RULES\n" .
                       $common_cxxincludes;
                  } elsif (-w "$app/src/perl") {
                     print STDERR "deleting obsolete wrapper file $real_src_file\n";
                     unlink $real_src_file;
                  } else {
                     print STDERR "WARNING: ignoring obsolete wrapper file $real_src_file\n";
                  }
               }
            }
         }
      }

      if (@to_groom && -w "$app/src/perl") {
         require "$root/support/groom_wrappers.pl";
         $targetlist_deps{"$root/support/groom_wrappers.pl"}=1;
         groom_wrappers("$app/src/perl", @to_groom);
      }
   }

   if ($bundled) {
      if (defined (my $staticlib_flags=delete $custom_flags{staticlib})) {
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
   $out_dir =~ s{^\$\{builddir\}}{$buildroot/*};
   if (keys %$objects > $obj_count_before) {
      foreach my $obj_file (glob("$out_dir/*.o")) {
         (my $obj_name=$obj_file) =~ s{^\Q$buildroot/\E[^/]+}{\${builddir}};
         if (!exists $objects->{$obj_name}) {
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
   my ($app, $staticlib_flags, $pre_gen, $bundled, $link_flags)=@_;
   my $src_dir=delete $staticlib_flags->{SOURCEDIR} or die "SOURCEDIR missing in staticlib flags in $app\n";

   my $lib_name=delete $staticlib_flags->{LIBNAME} // $bundled;
   my $CFLAGS=delete $staticlib_flags->{CFLAGS};
   my $CXXFLAGS=delete $staticlib_flags->{CXXFLAGS};
   my $source_files=delete $staticlib_flags->{SOURCES} or die "SOURCES missing in staticlib flags in $app\n";

   my $out_dir="\${builddir}/staticlib/$lib_name";
   my $out="";
   my @obj_files;
   foreach my $src_file (@$source_files) {
      my $obj_file="$out_dir/";
      my $extra_flags=delete $staticlib_flags->{$src_file};
      if ($src_file =~ /\.c$/) {
         $obj_file.=$`.".o";
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
   $out .= "build $lib_name : staticlib " . join(" ", @obj_files) . "\n";
   push @{$link_flags->{staticlibs}}, $lib_name;
   push @{$link_flags->{staticlibcmds}}, $out;
}

sub generate_targets {
   $targetlist_deps{"$srcroot/apps"}=1;
   if ($core_mode && (!$testscenario_pico_mode || -d "$root/bundled")) {
      $targetlist_deps{"$root/bundled"}=1;
   }

   my @apps=glob "$srcroot/apps/*";

   # BundledExts is ordered such that dependent extensions appear after their prerequisites;
   # here the dependent static libraries, if any, should come first in the linker command
   my @bundled=$core_mode ? ($ConfigFlags{BundledExts}) =~ /(\S+)/g : ();
   my @bundled_rev=reverse(@bundled);
   foreach my $bundled (@bundled) {
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
      foreach my $bundled (@bundled_rev) {
         my @look_in=($bundled, $ConfigFlags{"bundled.$bundled.RequireExtensions"} =~ /(\S+)/g);
         my @includes=glob("$root/bundled/{".join(",", @look_in)."}/include/app*");
         s|^\Q$root\E|$srcrootname| for @includes;
         print "bundled.$bundled.includes=", (map { " -I$_" } @includes), "\n\n";
      }
   } else {
      # an extension might depend on bundled extensions, in particular the private wrapper collection
      if (my @prereq_bundled=grep { /^\w+$/ } @prereq_exts) {
         if (-d "$root/lib/core/src") {
            # refers to the core system in a workspace
            my @add_includes=map { ("-I\${root}/bundled/$_/include/app-wrappers", "-I\${root}/bundled/$_/include/apps") } @prereq_bundled;
            print <<"---";
partial.app.includes=\${app.includes}
app.includes=\${partial.app.includes} @add_includes
---
         }
         my @add_cxxflags=map { "\${bundled.$_.CXXFLAGS}" } @prereq_bundled;
         my @add_ldflags=map { "\${bundled.$_.LDFLAGS}" } @prereq_bundled;
         my @add_libs=map { "\${bundled.$_.LIBS}" } @prereq_bundled;
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
      my $app_name=basename($app);
      my $app_shared_module="\${builddir}/lib/$app_name.$Config::Config{dlext}";
      push @all_app_targets, $app_shared_module;

      if (defined (my $objects=$objects_per_app{$app_name})) {
         my $link_flags=$link_flags_per_app{$app_name};
         if (my $generated=delete $link_flags->{generated}) {
            foreach my $gen_target (sort keys %$generated) {
               print $generated->{$gen_target}, "\n";
            }
         }
         my @all_objects=sort keys %$objects;
         foreach my $obj_name (@all_objects) {
            print $objects->{$obj_name}, "\n";
         }
         if (my $staticlibs=delete $link_flags->{staticlibs}) {
            push @all_objects, @$staticlibs;
            foreach my $commands (@{delete $link_flags->{staticlibcmds}}) {
               print $commands, "\n";
            }
         }
         print "build $app_shared_module: sharedmod @all_objects\n";
         foreach my $flag (sort keys %$link_flags) {
            print "  $flag=", join(" ", @{$link_flags->{$flag}}), "\n";
         }
         print "\n";
      } else {
         # application without any C++ components
         if (my @obsolete=grep { (stat $_)[7] != 0 } glob("$buildroot/*/lib/$app_name.$Config::Config{dlext}")) {
            print STDERR "removing obsolete shared module(s) @obsolete\n";
            unlink @obsolete;
         }
         print "build $app_shared_module: emptyfile\n\n";
      }
   }

   if (!$core_mode) {
      # wipe disappeared applications from the extension
      foreach my $app_mod (glob("$buildroot/*/lib/*.$Config::Config{dlext}")) {
         my ($app_name)= $app_mod =~ m{/([^/]+)\.$Config::Config{dlext}$};
         if (!exists $objects_per_app{$app_name}) {
            print STDERR "removing obsolete shared module $app_mod\n";
            unlink $app_mod;
            File::Path::remove_tree(glob("$buildroot/*/apps/$app_name"));
         }
      }
   }

   print <<"---";
# flag file may be deleted after wrapper generation and extension reconfiguring
build \${builddir}/.apps.built: emptyfile | @all_app_targets

---
   my @all_targets=('${builddir}/.apps.built', @all_app_targets);
   my @all_clean_targets;

   if ($core_mode) {
      if ($ConfigFlags{LDcallableFLAGS} ne "none") {
         print <<"---";
build \${buildroot}/applib/fake.c: gen_applib_stubs @all_app_targets
build \${builddir}/lib/callable/fake.o: ccompile \${buildroot}/applib/fake.c
  CextraFLAGS=-DPOLYMAKE_FAKE_FUNCTIONS
build \${builddir}/lib/callable/stub.o: ccompile \${buildroot}/applib/fake.c

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
build all.bundled: phony @all_bundled_targets
---
         push @all_targets, 'all.bundled';
      }
      my $install_input=generate_corelib_targets();
      print <<"---";
build install: install_core $install_input || all
---
      if (!$testscenario_pico_mode) {
         push @all_targets, "all.libs";
      }
   } else {
      if (-f "$srcroot/support/rules.ninja") {
         print "include $srcrootname/support/rules.ninja\n";
         $targetlist_deps{"$srcroot/support/rules.ninja"}=1;
      }
      add_custom_targets("$srcroot/support/generate_ninja_targets.pl",
                         all => \@all_targets, clean => \@all_clean_targets);
      print <<"---";
build install: install_ext || all
---
   }

   print <<"---";
build all: phony @all_targets
default all
build clean.all: phony @all_clean_targets
---
   close OUT;

   unlink $targets_file;
   rename "$targets_file.new", $targets_file
     or die "could not rename $targets_file.new to $targets_file: $!\n";
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

   my $Ext_module="\${builddir}/\${perlxpath}/auto/Polymake/Ext/Ext.$Config::Config{dlext}";

   my ($callable_lib_name, $stublib_name);
   if ($ConfigFlags{LDcallableFLAGS} ne "none") {
      my $callable_version=extract_polymake_version($root);
      $callable_lib_name=compose_sharedlib_versioned_name("polymake", $callable_version);
      $stublib_name=compose_sharedlib_versioned_name("polymake-apps", $callable_version);
      print <<"---";
callable_lib=\${builddir}/\${perlxpath}/$callable_lib_name
callable_link=\${builddir}/\${perlxpath}/libpolymake.$Config::Config{so}
callable_fakeapplib=\${builddir}/lib/callable/libpolymake-apps.$Config::Config{so}
callable_stubapplib=\${builddir}/lib/callable/$stublib_name
---
   }

   if (!$testscenario_pico_mode) {
      my (@corelib_objects, @callable_objects);

      # perl-independent modules
      my $out_dir='${builddir}/lib/core';
      foreach my $src_file (glob "$root/lib/core/src/*.cc") {
         my ($src_name, $obj_file)=basename($src_file, "cc");
         $src_file =~ s/^\Q$root\E/$srcrootname/;
         $obj_file="$out_dir/$obj_file.o";
         push @corelib_objects, $obj_file;
         print "build $obj_file : cxxcompile $src_file\n" .
               "  CXXincludes=\${core.includes}\n";
      }

      # perl-dependent modules
      $out_dir='${builddir}/lib/${perlxpath}';
      my $mode_flags=$Config::Config{optimize} =~ /-O[1-9]/ ? "" : "  CmodeFLAGS=\${CDebugFLAGS}\n";

      my @xxs=glob("$root/lib/core/src/perl/*.xxs");
      my @cc_from_xxs=map { "\${buildroot}/\${perlxpath}/".basename($_,"xxs").".cc" } @xxs;

      foreach my $src_file (@cc_from_xxs) {
         my $xxs_file=shift(@xxs);
         $xxs_file =~ s/^\Q$root\E/$srcrootname/;
         print "build $src_file: xxs_to_cc $xxs_file\n";
      }
      print "\n";

      my %glue_custom_flags=read_custom_script("$root/lib/core/src/perl/build_flags.pl");

      foreach my $src_file (@cc_from_xxs, glob("$root/lib/core/src/perl/*.cc")) {
         my ($src_name, $obj_file)=basename($src_file, "cc");
         $src_file =~ s/^\Q$root\E/$srcrootname/;
         $obj_file="$out_dir/$obj_file.o";
         push @corelib_objects, $obj_file;
         print "build $obj_file : cxxcompile $src_file\n",
               "  CXXextraFLAGS = $glue_custom_flags{CXXFLAGS} $glue_custom_flags{$src_name}\n",
               $mode_flags,
               "  CXXincludes=\${core.includes}\n\n";
      }

      print <<"---";
build $Ext_module: sharedmod @corelib_objects
  LIBSextra=$glue_custom_flags{LIBS}

---

      $targetlist_deps{"$root/lib/core/src"}=1;
      $targetlist_deps{"$root/lib/core/src/perl"}=1;

      if ($ConfigFlags{LDcallableFLAGS} ne "none") {
         my %callable_custom_flags=read_custom_script("$root/lib/callable/src/perl/build_flags.pl");
         print <<"---";
bootstrapXS.h=\${buildroot}/\${perlxpath}/polymakeBootstrapXS.h
build \${bootstrapXS.h}: gen_xs_bootstrap @cc_from_xxs

---
         foreach my $src_file (glob("$root/lib/callable/src/perl/*.cc")) {
            my ($src_name, $obj_file)=basename($src_file, "cc");
            $src_file =~ s/^\Q$root\E/$srcrootname/;
            $obj_file="$out_dir/$obj_file.o";
            push @callable_objects, $obj_file;
            print "build $obj_file : cxxcompile $src_file || \${bootstrapXS.h}\n",
                  "  CXXextraFLAGS = $callable_custom_flags{CXXFLAGS} $callable_custom_flags{$src_name} $glue_custom_flags{CXXFLAGS}\n",
                  $mode_flags,
                  "  CXXincludes= -I\${buildroot}/\${perlxpath} \${core.includes} -I\${root}/include/callable\n\n";
         }

         print <<"---";
build \${callable_lib}: sharedmod @corelib_objects @callable_objects
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$callable_lib_name \${LIBperlFLAGS}
  LIBSextra=$glue_custom_flags{LIBS}
build \${callable_link}: symlink_samedir \${callable_lib}

build \${callable_fakeapplib}: sharedmod \${builddir}/lib/callable/fake.o
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$stublib_name
build \${callable_stubapplib}: sharedmod \${builddir}/lib/callable/stub.o
  LDsharedFLAGS=\${LDcallableFLAGS}
  LDextraFLAGS=\${LDsonameFLAGS}$stublib_name

---
         $targetlist_deps{"$root/lib/callable/src/perl"}=1;
      }
   }

   if ($ConfigFlags{LDcallableFLAGS} ne "none") {
      print <<"---";
build all.libs: phony $Ext_module \${callable_link}
---
      return $Ext_module.' ${callable_fakeapplib} ${callable_stubapplib} ${callable_lib} ${callable_link}';
   } else {
      print <<"---";
build all.libs: phony $Ext_module
---
      return $Ext_module;
   }
}

sub compose_sharedlib_versioned_name {
   my ($name, $version)=@_;
   $Config::Config{so} eq "so" ? "lib$name.$Config::Config{so}.$version" : "lib$name.$version.$Config::Config{so}";
}


# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
