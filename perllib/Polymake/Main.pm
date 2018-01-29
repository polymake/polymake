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

use v5.16;

package Polymake::Main;

# this is called from the callable library starter module lib/callable/src/perl/Main.cc
sub import {
   (undef, my ($user_opts, $must_reset_SIGCHLD))=@_;

   # these redefinitions must happen before the whole slew of polymake perl code is loaded!
   if ($must_reset_SIGCHLD) {
      *CORE::GLOBAL::readpipe=sub { local $SIG{CHLD}='DEFAULT'; CORE::readpipe(@_) };
   }

   require Polymake::MainFunctions;
   _init($user_opts);
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
