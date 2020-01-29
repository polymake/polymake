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

#  Generate one or more C++ files with wrapper definitions from a JSON source.
#  This is called from script generate_ninja_targets.

package Polymake::GenerateCppPerl;

use strict;
use JSON;
use Fcntl ':flock';

my $default_chunk_size = 40;
my ($temp_module, $ext_config, $allow_report_config_file_dependency);

if (@ARGV >= 2) {
   # called by ninja from generated rules
   my $gen_rules;
   while ($ARGV[0] =~ /^--/) {
      if ($ARGV[0] eq "--gen-rules") {
         $gen_rules = splice @ARGV, 0, 2;
      } elsif ($ARGV[0] eq "--temp-module") {
         $temp_module = splice @ARGV, 0, 2;
      } elsif ($ARGV[0] eq "--ext-config") {
         $ext_config = splice @ARGV, 0, 2;
      } else {
         die "$0: unknown option $ARGV[0]\n";
      }
   }
   my ($def_file, @mod_files) = @ARGV;
   my $mod_dir = $mod_files[0] =~ m{^(.*)/[^/]+$} ? $1 : (".");
   # if the interface definition file has been deleted in the meanwhile,
   # just report it for removal from the rules
   my @update = -e $def_file ? process_def_file($def_file, $mod_dir, { map { ($_ => -f) } @mod_files }) : ($def_file);
   if (@update) {
      open my $R, ">>", $gen_rules
        or die "can't append to $gen_rules: $!\n";
      flock $R, LOCK_EX
        or die "can't lock $gen_rules: $!\n";
      print $R "#\@update @update\n";
   }
} else {
   # direct call from generate_ninja_targets for a new interface definition file:
   # report it if we see any "ext" attributes in the instances
   $allow_report_config_file_dependency = 1;
}

##################################################################################
sub process_def_file {
   my ($def_file, $mod_dir, $existing_files) = @_;
   my @produced_files;
   my $data = read_def_file($def_file);

   my $app = $data->{app} // die "invalid file $def_file: must contain `app'\n";
   my $chunksize;

   my @common_include;
   my $embed = $data->{embed};
   if (!defined($embed)) {
      $chunksize = $data->{chunksize} // $default_chunk_size;
      push @common_include, "polymake/client.h";
   }

   my ($def_basename) = $def_file =~ m{(?:^|/)([^/]+)$};
   my ($stem_name) = $def_basename =~ m{^([^.]+)};
   my ($report_update, $report_config_file_dependency);

   my $instances = ref($data->{inst}) eq "ARRAY" ? $data->{inst} : [];
   if (@$instances && !defined($instances->[-1])) {
      pop @$instances;
   }
   if (@$instances) {
      my $ord = 0;
      $_->{ord} = $ord++ for @$instances;

      if (defined $ext_config) {
         $report_config_file_dependency = filter_active_extensions($instances);
      } elsif ($allow_report_config_file_dependency) {
         $report_config_file_dependency = grep { ref($_->{ext}) eq "ARRAY" } @$instances;
      }

      for (my $mod_cnt = 0; @$instances; ++$mod_cnt) {
         my @instances_in_chunk = splice @$instances, 0, $chunksize || scalar @$instances;
         my $mod_text;
         if (defined($temp_module)) {
            $mod_text= <<".";
#include <unistd.h>
namespace { void delete_temp_file() __attribute__((destructor));
            void delete_temp_file() { unlink("$temp_module"); } }
.
         } else {
            $mod_text= <<".";
// This is an automatically generated file, any manual changes done here will be overwritten!
// To change the generated C++ code, edit the script support/generate_cpperl_modules.pl
.
            if (!defined $embed) {
               $mod_text .= <<".";
// To change the number of instances per source file, specify "chunksize":N in ../$def_basename
.
            }
         }
         $mod_text .= <<".";

#define POLYMAKE_CPPERL_FILE "$stem_name"
.
         if (defined $embed) {
            $mod_text .= <<".";
#include "polymake/perl/macros.h"
#include FindDefinitionSource4perl($embed)
.
         }
         $mod_text .= include_statements(\@common_include, grep { defined } map { $_->{include} } @instances_in_chunk);
         $mod_text .= <<".";

namespace polymake { namespace $app { namespace {
.
         $mod_text .= function_callers(grep { exists $_->{func} } @instances_in_chunk);
         $mod_text .= join("", map { instance_definition($_, $def_file) } @instances_in_chunk);
         $mod_text .= <<".";
} } }
.
         my $mod_file = "$mod_dir/$stem_name" . ($mod_cnt ? "-$mod_cnt" : "");
         if ($embed =~ m{\.[^/.]+$}) {
            $mod_file .= $&;
         } else {
            $mod_file .= ".cc";
         }
         unless (delete $existing_files->{$mod_file}) {
            $report_update = 1;
         }
         push @produced_files, $mod_file;
         if (!defined($temp_module) && -f $mod_file) {
            open my $out, "+<:utf8", $mod_file
              or die "can't update $mod_file: $!\n";
            local $/;
            my $old_text = <$out>;

            # no white space normalization or other "smartness":
            # unless the old version stems from this very script without any changes, it has to be overwritten
            if ($old_text ne $mod_text) {
               seek $out, 0, 0;
               truncate $out, 0;
               print $out $mod_text;
            }
         } else {
            open my $out, ">:utf8", $mod_file
              or die "can't create $mod_file: $!\n";
            print $out $mod_text;
         }
      }
   }

   if (!defined($temp_module) and $report_update || !@produced_files || keys %$existing_files) {
      if ($report_config_file_dependency) {
         # we have seen instances involving other extensions: register dependency on the config file
         push @produced_files, "|config.file";
      }
      ($def_file, @produced_files)
   } else {
      # no updates in generating rules
      ()
   }
}
##################################################################################
sub read_def_file {
   my ($def_file) = @_;
   local $/;
   open my $in, "<:utf8", $def_file
     or die "can't read definition file $def_file: $!\n";
   JSON->new->relaxed->utf8->decode(<$in>);
}

sub distinct {
   my $i = -1;
   grep { ++$i == 0 || $_[$i-1] ne $_ } @_;
}

sub sort_distinct {
   distinct( sort { $a cmp $b } @_ );
}

sub include_statements {
   join("", map { qq{#include "$_"\n} } sort_distinct(map { @$_ } @_))
}
##################################################################################
sub read_ext_config {
   my %exts;
   open my $cf, "<", $ext_config
     or die "can't read $ext_config: $!\n";
   while (<$cf>) {
      if (s/^\s*RequireExtensions\s*=\s*//) {
         @exts{ m/(\S+)/g } = ();
         last;
      }
   }
   \%exts;
}
##################################################################################
sub filter_active_extensions {
   my ($instances) = @_;
   my $active_extensions;
   for (my $i = 0; $i <= $#$instances; ++$i) {
      if (ref(my $ext_list = $instances->[$i]->{ext}) eq "ARRAY") {
         $active_extensions //= read_ext_config();
         if (grep { !exists $active_extensions->{$_} } @$ext_list) {
            splice @$instances, $i--, 1;
         }
      }
   }
   $active_extensions && @$instances
}
##################################################################################
sub function_callers {
   return "" unless @_;

   my %caller_tags;
   foreach my $inst (@_) {
      my $tags = \%caller_tags;
      for my $part (split /::/, $inst->{func}) {
         $tags = ($tags->{$part} //= { });
      }
   }
   my $text = "FunctionCallerStart4perl {" . caller_tags(\%caller_tags) . "};\n";

   my %callers;
   foreach my $inst (@_) {
      my $func = $inst->{func};
      my $kind = $inst->{kind} // "free";
      $kind .= "_t" if $inst->{tp};
      $callers{"$func, $kind"} //= 1;
   }

   $text . join("", map { "FunctionCaller4perl($_);\n" } sort_distinct(keys %callers));
}
##################################################################################
sub caller_tags {
   my ($tags) = @_;
   join("", map {
      my $next_level = $tags->{$_};
      keys %$next_level
        ? "struct $_ {" . caller_tags($next_level) . "};"
        : "\nenum class $_;\n"
   } sort keys %$tags);
}
##################################################################################
sub instance_definition {
   my ($inst, $def_file) = @_;
   if (defined (my $pkg = $inst->{pkg})) {
      my $guard = defined($inst->{guard_name}) && <<".";
#if !defined(POLYMAKE_$inst->{guard_name}_H)
#  error automatically generated wrapper file $inst->{wrapper_file} is missing
#endif
.
      if (defined(my $class = $inst->{class})) {
         $guard . "Class4perl($inst->{ord}, $pkg, $class);\n"
      } elsif (defined(my $builtin = $inst->{builtin})) {
         $guard . "Builtin4perl($inst->{ord}, $pkg, $builtin);\n"
      } else {
         $guard . "ClassTemplate4perl($inst->{ord}, $pkg);\n"
      }

   } else {
      my $ret = $inst->{ret} // "normal";
      my $args = $inst->{args};
      if (ref($args) ne "ARRAY") {
         die "invalid `args' element = $args in $def_file\n";
      }
      $args = "(" . join(", ", @$args) . ")";
      if (defined(my $cross_apps = $inst->{apps})) {
         $args = join(", ", $args, map { qq{perl::CrossApp("$_")} } @$cross_apps);
      }
      if (defined(my $func = $inst->{func})) {
         my $kind = $inst->{kind} // "free";
         my $tparams = $inst->{tp} // 0;
         $kind .= "_t" if $tparams;
         "FunctionTemplateInstance4perl($inst->{ord}, $func, $kind, $inst->{sig}, perl::Returns::$ret, $tparams, $args);\n"

      } elsif (defined(my $op = $inst->{op})) {
         "OperatorTemplateInstance4perl($inst->{ord}, $op, $inst->{sig}, perl::Returns::$ret, $args);\n"

      } else {
         die "invalid `inst' element in $def_file: must have one of `pkg', `func', or `op' elements\n";
      }
   }
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
