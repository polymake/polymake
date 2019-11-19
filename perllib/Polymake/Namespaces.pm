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

package namespaces;
use Polymake::Ext;
use vars qw( %special_imports );

@LEXICAL_IMPORTS=(undef);
$special_imports{"namespaces.pm"}=1;

$INC{"namespaces.pm"}=$INC{"Polymake/Namespaces.pm"};
$INC{"namespaces/AnonLvalue.pm"}=$INC{"Polymake/Namespaces.pm"};
$INC{"namespaces/Params.pm"}=$INC{"Polymake/Namespaces.pm"};
*Polymake::Namespaces::import=\&import;

sub set_autolookup {
   my $pkg=caller;
   Polymake::define_function($pkg, ".AUTOLOOKUP", $_[0]);
}

1
