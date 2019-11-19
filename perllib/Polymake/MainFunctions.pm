#  Copyright (c) 1997-2019
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

package Polymake::Ext;
sub import {
   my $module=caller;
   &{"$module\::bootstrap"}();
}
BEGIN {
   bootstrap();
   $INC{"Polymake/Ext.pm"} = $INC{"Polymake/Main.pm"};
}

package Polymake;

use strict;
use vars qw($InstallTop $InstallArch $Arch $DeveloperMode @BundledExts $ConfigTime $DebugLevel);

use Polymake;
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

package Polymake::Main;

# private: only called from import()
sub _init {
   # this guarantees initialization of internal structures for signal handling
   local $SIG{INT} = 'IGNORE';
   &Main::init;
   $Scope = new Scope();
   add AtEnd "FinalCleanup", sub { undef $Scope }, after => "Object";
}

sub createNewScope {
   my $prevScope = $Scope;
   $Scope = new Scope();
   $prevScope
}

sub application_from_object {
   User::application((shift)->type->application);
}

sub set_custom {
   my $name = shift;
   if (@_==1 && substr($name, 0, 1) eq '@' && is_array($_[0])) {
      $User::application->_set_custom($Core::Prefs->custom, $name, @{$_[0]});
   } elsif (@_==1 && substr($name, 0, 1) eq '%' && is_hash($_[0])) {
      $User::application->_set_custom($Core::Prefs->custom, $name, %{$_[0]});
   } elsif (@_) {
      $User::application->_set_custom($Core::Prefs->custom, $name, @_);
   } else {
      die "set_custom: value missing\n";
   }
}

sub reset_custom {
   my $name = shift;
   $User::application->_reset_custom($Core::Prefs->custom, $name, @_);
}

sub local_custom {
   my $var = $User::application->find_custom_var(shift, $Core::Prefs->custom);
   local with($Scope->locals) {
      no strict 'refs';
      $var->prefix eq '$'
      ? (local ${$var->name} = $_[0]) :
      $var->prefix eq '@'
      ? (local ref *{$var->name} = $_[0]) :
      @_==2
      ? (local ref *{$var->name} = $_[0])
      : (local ${$var->name}{$_[0]} = $_[1]);
   }
}

sub shell_enable {
   state $init_shell = do {
      require Polymake::Core::ShellMock;
      $Shell = new Core::Shell::Mock;
      1
   };
}

sub shell_execute {
   if (!defined $User::application) {
      $@ = "current application not set";
      return;
   }
   my $gather_stdout = "";
   local open STDOUT, ">:utf8", \$gather_stdout;
   my $gather_stderr = "";
   local open STDERR, ">:utf8", \$gather_stderr;
   my $executed = $Shell->process_input(@_);
   my $exc = $@;
   $@ = "";
   ($executed, $gather_stdout, $gather_stderr, $exc);
}

sub shell_complete {
   if (!defined $User::application) {
      $@ = "current application not set";
      return;
   }
   $_[0] =~ /\S/ or return (0, "");
   $Shell->complete($_[0]);
   ($Shell->completion_offset, $Shell->term->Attribs->{completion_append_character} // "", @{$Shell->completion_proposals});
}

sub shell_context_help {
   if (!defined $User::application) {
      $@ = "current application not set";
      return;
   }
   my ($input, $pos, $full, $html) = @_;
   require Polymake::Core::HelpAsHTML if $html;

   $input =~ /\S/ or return;
   $Shell->context_help($input, $pos);
   map {
      my $writer = $html ? new Core::HelpAsHTML() : new Core::HelpAsPlainText(0);
      $_->write_text($writer, $full);
      $writer->text
   } @{$Shell->help_topics};
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
