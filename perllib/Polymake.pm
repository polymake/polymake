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
# Global variables
#

declare $Version="3.3";
declare $VersionNumber=eval "v$Version";        # for string comparisons with vM.N literals

declare ($Scope,                # Scope object for the current cycle
         $PrivateDir,           # where the user's private settings, wrappers, etc. dwell
        );

declare $Shell=new NoShell;     # alternatively: Shell object listening to the console or some pipe

# resources for third-party programs launched by polymake
declare $Resources=$ENV{POLYMAKE_RESOURCE_DIR} // "$InstallTop/resources";

########################################################################
#
# Load the core modules in proper order.
#

require 'Polymake/utils.pl';
require Polymake::Scope;
require Polymake::Pipe;
require Polymake::Tempfile;
require Polymake::OverwriteFile;
require Polymake::Overload;
require Polymake::Core::Customize;
require Polymake::Core::Preference;
require Polymake::User;
require Polymake::Core::PropertyType;
require Polymake::Core::Property;
require Polymake::Core::Permutation;
require Polymake::Core::PropertyValue;
require Polymake::Core::Rule;
require Polymake::Core::ObjectType;
require Polymake::Core::Scheduler;
require Polymake::Core::XMLfile;
require Polymake::Core::Object;
require Polymake::Core::Application;
require Polymake::Core::Extension;
require Polymake::Core::CPlusPlus;
require Polymake::Core::StoredScript;
require Polymake::Core::RuleFilter;
require Polymake::Core::UserSettings;

package Polymake::Core;

declare $Custom = new Customize;
declare $Prefs = new Preference;

my @custom_blocks;
sub add_custom_vars {
   push @custom_blocks, @_;
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

   my @messages = ("polymake version $full_version", <<'.', <<'.');

Copyright (c) 1997-2018
Ewgenij Gawrilow, Michael Joswig (TU Berlin)
http://www.polymake.org
.

This is free software licensed under GPL; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
.

   return join '',@messages[0 .. $verbose];
}

# initialize some modules in proper order
# "config path" =>
sub init {
   Core::UserSettings::init(@_, @custom_blocks);
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
