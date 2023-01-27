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
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Core::CustomizetoJSON;

sub convert_file {
   my ($text, $init_script) = @_;
   pos($text) = 0;
   my ($pkg, $app, %result);
   my %unrecognized;

   while (pos($text) < length($text)) {

      # skip empty or comment lines, as well as the version number
      next if $text =~ m{\G \s* (?> (?: \# | \$version\s*=) .* )? \n}xmgc;

      if ($text =~ m{\G \s* package \s+ ($qual_id_re) \s*; }xomgc) {
         $pkg = $1;
         undef $app;
         next;
      }

      if ($text =~ m{\G \s* application \s+ ($id_re) \s*; }xomgc) {
         $app = $1;
         $pkg = "Polymake::$app";
         next;
      }

      if (defined($app)) {
         if ($text =~ m{\G \s* prefer \s+ (.+)(?<=\S) \s*; }xomgc) {
            my $expr = eval "$1";
            if (!$@ && defined($app)) {
               push @{$result{Polymake::Core::Preference::settings_key}{$app}}, $expr;
            }
            next;
         }
      } else {
         if ($text =~ m{\G \s* (?: ARCH\s*\(\s* $quoted_re \s*\)\s+ and \s+ )?
                        (?'var' $var_sigil_re $qual_id_re) \s*=\s*
                        (?('scalar') (?'value' .+)(?<=\S)
                            | (?=\() (?'value' $confined_multiline_re)) \s*; }xomgc) {

            my ($arch, $var, $value) = @+{qw(quoted var value)};
            my ($key, $next_level);
            if ($var =~ /::/) {
               $key = substr($var, 1);
            } elsif ($pkg eq "Polymake::User" && $var eq '%disabled_extensions') {
               $key = "_extensions::disabled";
            } elsif ($pkg eq "Polymake::Core::CPlusPlus") {
               $key = "_C++::" . substr($var, 1);
            } elsif ($var eq '%configured' && $pkg =~ /^Polymake::(\w+)$/) {
               $key = "_applications::configured";
               $next_level = $1;
            } else {
               $key = $pkg . "::" . substr($var, 1);
            }
            if (defined($arch)) {
               $key .= "#$arch";
            }
            if (substr($var, 0, 1) eq '$') {
               $value = eval $value;
            } elsif (substr($var, 0, 1) eq '@') {
               my @list = eval $value;
               $value = \@list;
            } else {
               my %hash = eval $value;
               $value = \%hash;
            }
            unless ($@) {
               if (defined($next_level)) {
                  $result{$key}{$next_level} = $value;
               } else {
                  $result{$key} = $value;
               }
            }
            next;
         }
      }

      $text =~ m{\G .* \n}xmgc;
      if (defined($init_script)) {
         my $unrecognized = $&;
         if ($unrecognized =~ /^[ \t]*[^\s\#]/m) {
            $unrecognized{$app // ""} .= $unrecognized;
         }
      }
   }

   if (keys %unrecognized) {
      open my $F, ">", $init_script or die "can't write to $init_script: $!\n";
      print $F <<".";
print STDERR <<'===== cut out up to this line =====';  return;


***** ATTENTION *****

Some start-up commands were rescued from your old preferences file.
They have been stored in $init_script, but not executed yet.
Please revise them and/or disable the script as described in the preamble.

*********************

===== cut out up to this line =====

# This script will be executed at the beginning of every polymake session.
# To run something else instead, set the custom variable:
#   set_custom \$init_script="path/to/script.pl";
#   
# To disable it altogether:
#   reset_custom \$init_script;
#

# Rescued commands follow:

.
      for $app (sort keys %unrecognized) {
         if (length($app)) {
            print $F "\nuse application '$app';\n";
         }
         print $F $unrecognized{$app};
      }
      close $F;
   }

   return \%result;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
