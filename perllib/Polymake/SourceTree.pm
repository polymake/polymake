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

require File::Path;
require File::Copy;

use namespaces;

package Polymake::SourceTree;

sub new {
   my ($pkg, $dir)=@_;
   bless \$dir, $pkg;
}

sub make_dir {
   my $self=shift;
   File::Path::mkpath([ map { m{^/} ? $_ : "$$self/$_" } @_ ], 0, 0755);
}

sub copy_file {
   my $self=shift;
   File::Copy::copy(map { m{^/} ? $_ : "$$self/$_" } @_);
}

1;

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
