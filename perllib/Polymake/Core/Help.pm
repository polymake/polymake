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
use namespaces;
use warnings qw(FATAL void syntax misc);
use feature 'state';

# A "virtual" package:
# per default offers dummy methods which do not store anything.
# Should be redefined when needed

package Polymake::Core::Help;

my $dummy = bless [ ];

sub new { $dummy }
*clone = \&new;
sub add {}
sub get { undef }
sub related { [ ] }

declare ($core, $gather);

my @activation_callbacks;

sub activate {
   $core = shift;
   $gather = true;
   {
      no strict 'refs';
      my $pkg = ref($core);
      my $pkg_new = \&{"$pkg\::new"};
      *new = sub { shift; $pkg->new(@_) };
   }
   $_->($core) for @activation_callbacks;
}

sub add_activation_callback {
   push @activation_callbacks, @_;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
