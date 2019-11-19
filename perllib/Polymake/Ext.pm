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

use strict;

package Polymake::Ext;
require DynaLoader;
require Carp;

our @ISA=qw(DynaLoader);
sub dl_load_flags { 0x01 }

bootstrap Polymake::Ext;
my $libref=$DynaLoader::dl_librefs[-1];

sub import {
   my $module=caller;
   my $bootname = "boot_$module";
   $bootname =~ s/\W/_/g;
   my $bootsym=DynaLoader::dl_find_symbol($libref, $bootname)
     or Carp::croak( "$module\::bootstrap not defined" );
   my $xs=DynaLoader::dl_install_xsub("$module\::bootstrap", $bootsym, __FILE__);
   &$xs();
}
