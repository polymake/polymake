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

use v5.16;

package Polymake::Main;

# this is called from the callable library starter module lib/callable/src/perl/Main.cc
sub import {
   (undef, my ($user_opts, $must_reset_SIGCHLD)) = @_;

   # this guarantees initialization of internal structures for signal handling
   local $SIG{INT} = 'IGNORE';

   # these redefinitions must happen before the whole slew of polymake perl code is loaded!
   if ($must_reset_SIGCHLD) {
      *CORE::GLOBAL::readpipe=sub { local $SIG{CHLD}='DEFAULT'; CORE::readpipe($_[0]) };
   }

   require DynaLoader;
   Polymake::Ext::bootstrap();
   $INC{"Polymake/Ext.pm"} = $INC{"Polymake/Main.pm"};

   require Polymake::MainFunctions;
   init($user_opts);
}

sub Polymake::Ext::import {
   no strict 'refs';
   my $module = caller;
   &{"$module\::bootstrap"}();
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
