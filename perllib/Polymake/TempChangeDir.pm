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
use feature 'state';

require Cwd;

package Polymake::TempChangeDir;

sub new {
   my ($pkg, $to_dir)=@_;
   my $cwd = Cwd::getcwd;
   chdir $to_dir or die "can't change into $to_dir: $!\n";
   bless \$cwd, $pkg;
}

sub DESTROY {
   chdir ${$_[0]} or warn "can't change back into ${$_[0]}: $!\n";
}


1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
