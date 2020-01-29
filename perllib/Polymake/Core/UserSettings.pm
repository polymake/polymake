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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Core::UserSettings;

########################################################################
#
# Analyze the configuration mode and load the files if necessary.
#

sub init {
   my ($mode, @custom_blocks) = @_;
   if (RuleFilter::allow_config($mode)) {
      foreach (split /;/, $mode) {
         if ($_ eq "user") {
            if ($PrivateDir) {
               die "conflicting entries in configuration path: \"user\" follows $PrivateDir also designated as user's private directory.\n";
            }
            ### TODO: select MacOS-specific default location if $^O eq "darwin" ?
            $PrivateDir=$ENV{POLYMAKE_USER_DIR} || "$ENV{HOME}/.polymake";
            next;
         }
         if ($_ eq '@interactive') {
            require Polymake::Core::Help::Topic;
            next;
         }
         my $user= s/^user=//;
         my $location;
         if (/^\$/) {
            # looks like an environment var
            $location=$ENV{$'} or next;
         } else {
            $location=$_;
         }
         $location =~ s{^~/}{$ENV{HOME}/};

         if ($user) {
            ($PrivateDir &&= die "conflicting entries in configuration path: \"user=$_\" follows $PrivateDir also designated as user's private directory.\n")
              =$location;
         } elsif (-e $location) {
            if ($PrivateDir) {
               die "conflicting entries in configuration path: global location $location follows private directory $PrivateDir.\n";
            }
            if (-d _) {
               my $prefs="$location/prefer.pl";
               if (-r $prefs) {
                  $Prefs->load_global($prefs);
               } else {
                  undef $prefs;
               }
               $location .= "/customize.pl";
               unless (-f $location) {
                  warn_print( "configuration path entry ", ($location =~ $directory_re),
                              " without effect: neither customize.pl nor prefer.pl found there" ) unless $prefs;
                  next;
               }
            } elsif (!-f _) {
               die "invalid configuration path entry $location: must be a regular file or a directory\n";
            }
            if (-r _) {
               $Custom->load_global($location);
            } else {
               warn_print( "insufficient access rights to import global configuration from $location" );
            }
         } else {
            die "non-existing configuration path entry $location\n";
         }
      }

      if ($PrivateDir) {
         if (-e $PrivateDir) {
            unless (-d _) {
               die "$PrivateDir designated as a location for user's private configuration is not a directory\n";
            }
            unless (-w _ && -x _) {
               die "Insufficient access rights for private directory $PrivateDir;\n",
                   $mode =~ /(?:^|;)user(?:$|;)/ &&
                     "Please correct with `chmod' or set POLYMAKE_USER_DIR environment variable to a suitable location.\n";
            }
         } else {
            File::Path::mkpath($PrivateDir, 0, 0700);
            warn_print( "created private directory $PrivateDir" );
         }
         $Custom->load_private("$PrivateDir/customize.pl");
         $Prefs->load_private("$PrivateDir/prefer.pl");
      }
   }

   $_->() for @custom_blocks;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
