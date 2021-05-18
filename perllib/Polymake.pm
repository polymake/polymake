#  Copyright (c) 1997-2021
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
use utf8;
use IO::Handle;

require Config;
require Cwd;
require File::Path;

########################################################################
#
# Load the foundations of polymake/perl language extension.
#
use Polymake::Namespaces;

package Polymake::RefHash;
use Polymake::Ext;

package Polymake;
use Polymake::Ext;
*UNIVERSAL::can=\&Polymake::can;

########################################################################
#
# Import fast JSON conversion functions directly
#
require JSON;
*encode_json = \&JSON::XS::encode_json;
*write_json  = \&JSON::XS::write_json;
*decode_json = \&JSON::XS::decode_json;

########################################################################
#
# Global variables
#

declare $Version = "4.4";
declare $VersionNumber = eval "v$Version";    # for string comparisons with vM.N literals

declare ($Scope,                # Scope object for the current cycle
         $PrivateDir,           # where the user's private settings, wrappers, etc. dwell
         $Settings,             # user's preferences, custom variables, auto-configuration
        );

declare $Shell = new NoShell;   # alternatively: Shell object listening to the console or some pipe

# resources for third-party programs launched by polymake
declare $Resources = $ENV{POLYMAKE_RESOURCE_DIR} // "$InstallTop/resources";

declare $mainURL = "https://polymake.org";

########################################################################
#
# Load the core modules in proper order.
#

require 'Polymake/utils.pl';
require Polymake::Interrupts;
require Polymake::Scope;
require Polymake::Pipe;
require Polymake::Tempfile;
require Polymake::TempTextFile;
require Polymake::Tempdir;
require Polymake::TempChangeDir;
require Polymake::OverwriteFile;
require Polymake::Overload;
require Polymake::Schema;
require Polymake::Core::UserSettings;
require Polymake::Core::Preference;
require Polymake::Core::Help;
require Polymake::User;
require Polymake::Core::PropertyType;
require Polymake::Core::Property;
require Polymake::Core::Permutation;
require Polymake::Core::PropertyValue;
require Polymake::Core::Rule;
require Polymake::Core::BigObjectType;
require Polymake::Core::Scheduler;
require Polymake::Core::Serializer;
require Polymake::Core::BigObject;
require Polymake::Core::Datafile;
require Polymake::Core::Application;
require Polymake::Core::Extension;
require Polymake::Core::CPlusPlus;
require Polymake::Core::StoredScript;
require Polymake::Core::RuleFilter;

package Polymake::Core;

declare $Prefs = new Preference;
declare $enable_plausibility_checks = true;

my @settings_callbacks;
sub add_settings_callback {
   push @settings_callbacks, @_;
}

########################################################################
#
# The banner resides here to be available for the callable library and
# the polymake shell.
#
package Polymake::Main;

# is set to true when a user script is executed directly from the command line, without interactive shell
declare $standalone_script = false;

sub greeting {
   my $verbose = $_[0] // 2;
   my $full_version="$Version";
   if ($DeveloperMode && -d "$InstallTop/.git") {
      my $branch = `cd '$InstallTop'; git rev-parse --abbrev-ref HEAD`;
      chomp $branch;
      $full_version .= ", branch $branch ";
      my $upstream = `cd '$InstallTop'; git rev-parse --abbrev-ref --symbolic-full-name \@{u} 2>&1`;
      chomp $upstream;
      unless ($?) {
         my ($ahead,$behind) = split '\t',`cd '$InstallTop'; git rev-list --left-right --count $branch...$upstream`;
         chomp $ahead;
         chomp $behind;
         $full_version .= "[";
         $full_version .= "$ahead ahead " if ($ahead > 0);
         $full_version .= "$behind behind " if ($behind > 0);
         $full_version .= "$upstream]";
      }
   }

   my @messages = ("polymake version $full_version", <<'.' . $mainURL . "\n", <<'.');

Copyright (c) 1997-2021
Ewgenij Gawrilow, Michael Joswig, and the polymake team
Technische Universität Berlin, Germany
.

This is free software licensed under GPL; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
.

   return join '', @messages[0 .. $verbose];
}

# initialize some modules in proper order
# "config path" =>
sub init {
   $Settings = Core::UserSettings::init(@_);
   $_->($Settings) for @settings_callbacks;
   @settings_callbacks = ();
   Core::Extension::init();
   Core::CPlusPlus::init();
}

########################################################################
#
# Dummy object to fill the $Shell variable in non-interactive scenarios
#
package Polymake::NoShell;
sub new { bless \(my $dummy) }
sub interactive { 0 }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
