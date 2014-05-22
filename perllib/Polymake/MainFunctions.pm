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

package Polymake::Ext;
sub import {
   my $module=caller;
   &{"$module\::bootstrap"}();
}
BEGIN {
   bootstrap();
   $INC{"Polymake/Ext.pm"}=$INC{"Polymake/Main.pm"};
}

package Polymake;

use strict;
use vars qw($InstallTop $InstallArch $Arch $DeveloperMode);

use Polymake;
use namespaces;

package Polymake::Main;

# private: only called from import()
sub _init {
   Core::UserSettings::init(shift);
   init Core::CPlusPlus(0);
   init Core::Extension;
   $Scope=new Scope();
   add AtEnd "FinalCleanup", sub { undef $Scope }, after => "Object";
}

sub createNewScope {
   my $prevScope=$Scope;
   $Scope=new Scope();
   $prevScope
}

sub application_from_object {
   User::application((shift)->type->application);
}

sub set_custom {
   my $name=shift;
   if (@_==1 && substr($name,0,1) eq '@' && is_ARRAY($_[0])) {
      $User::application->_set_custom($Prefs->custom, $name, @{$_[0]});
   } elsif (@_==1 && substr($name,0,1) eq '%' && is_hash($_[0])) {
      $User::application->_set_custom($Prefs->custom, $name, %{$_[0]});
   } elsif (@_) {
      $User::application->_set_custom($Prefs->custom, $name, @_);
   } else {
      die "set_custom: value missing\n";
   }
}

sub reset_custom {
   my $name=shift;
   $User::application->_reset_custom($Prefs->custom, $name, @_);
}

sub local_custom {
   my $var=$User::application->find_custom_var(shift, $Prefs->custom);
   $Scope->begin_locals;
   no strict 'refs';
   $var->prefix eq '$'
   ? local_scalar(*{$var->name}, $_[0]) :
   $var->prefix eq '@'
   ? local_array(*{$var->name}, $_[0]) :
   @_==2
   ? local_hash(*{$var->name}, $_[0])
   : (local ${$var->name}{$_[0]}=$_[1]);
   $Scope->end_locals;
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
