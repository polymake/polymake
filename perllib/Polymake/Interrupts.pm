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

package Polymake::Interrupts;

declare $state = 0;

use Polymake::Ext;

# standard perl system() function can't be interrupted by a signal, but pipe write can.
sub system {
   open my $dummy, join(" ", "|", @_)
   or die "can't execute @_: $!\n";
   close $dummy;
   return $?;
}

sub override_system {
   my $pkg = caller;
   namespaces::intercept_operation($pkg, "system", \&system);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
