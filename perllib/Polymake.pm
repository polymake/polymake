#  Copyright (c) 1997-2014
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

declare $Version="2.13.1";
declare $VersionNumber=eval "v$Version";        # for string comparisons with vM.N literals

declare ($Scope,                # Scope object for the current cycle
         $DebugLevel,           # enables the debugging output and various internal checks
         $PrivateDir,           # where the user's private settings, wrappers, etc. dwell
        );

declare $Shell=new NoShell;     # alternatively: Shell object listening to the console or some pipe

declare $CoreVCS;               # version control system for core source files
if ($DeveloperMode) {
   require Polymake::SourceVersionControl;
   $CoreVCS=new SourceVersionControl($InstallTop);
}

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
require Polymake::Core::UserSettings;
require Polymake::User;
require Polymake::Core::PropertyType;
require Polymake::Core::Property;
require Polymake::Core::Permutation;
require Polymake::Core::Rule;
require Polymake::Core::PropertyValue;
require Polymake::Core::ObjectType;
require Polymake::Core::Scheduler;
require Polymake::Core::XMLfile;
require Polymake::Core::Object;
require Polymake::Core::Application;
require Polymake::Core::Extension;
require Polymake::Core::RuleFilter;
require Polymake::Core::CPlusPlus;

declare $Custom=new Core::Customize;
declare $Prefs=new Core::Preference;

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
