/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/client.h"
#include "polymake/topaz/ChainComplex.h"
#include "polymake/topaz/Filtration.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   Class4perl("Polymake::common::Serialized__Filtration__SparseMatrix_A_Rational_I_NonSymmetric_Z", pm::Serialized< Filtration< SparseMatrix< Rational, NonSymmetric > > >);
   Class4perl("Polymake::common::Serialized__Cell", pm::Serialized< Cell >);
   Class4perl("Polymake::common::Serialized__Filtration__SparseMatrix_A_Integer_I_NonSymmetric_Z", pm::Serialized< Filtration< SparseMatrix< Integer, NonSymmetric > > >);
   Class4perl("Polymake::common::Serialized__ChainComplex__SparseMatrix_A_Integer_I_NonSymmetric_Z", pm::Serialized< ChainComplex< SparseMatrix< Integer, NonSymmetric > > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
