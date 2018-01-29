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
#
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://solros.de
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

use MongoDB;
use MongoDB::OID;

use PolyDB::Client;
use PolyDB::DatabaseHelpers;
use PolyDB::DBInsert;
use PolyDB::PolymakeJsonConversion;
use PolyDB::DatabaseCursor;
if ($INC{"Polymake/Core/Shell.pm"}) {
   require PolyDB::Shell;
}

1;
