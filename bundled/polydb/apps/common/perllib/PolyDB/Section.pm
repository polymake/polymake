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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Andreas Paffenholz
#   (c) 2015 - 2023
#   https://polydb.org
#   https://www.mathematik.tu-darmstadt.de/~paffenholz
#


package PolyDB::Section;

use Polymake::Struct (
   [ new => ';$$' ],
   '$client',
   '$section'
);

sub new {
   my $self=&_new;
   my ($db, $name, $options) = @_;
   $self->client = $db;

   my $client_tmp = $self->client->client;
   $self->section = $client_tmp->get_section($name);   

   $self;
}

1;