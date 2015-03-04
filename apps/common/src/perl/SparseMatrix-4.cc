/* Copyright (c) 1997-2015
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

///==== this line controls the automatic file splitting: max.instances=20

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace common { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====

   Class4perl("Polymake::common::SparseMatrix_A_Float_I_NonSymmetric_Z", SparseMatrix< double, NonSymmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_Rational_I_NonSymmetric_Z", SparseMatrix< Rational, NonSymmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_Rational_I_Symmetric_Z", SparseMatrix< Rational, Symmetric >);
   Class4perl("Polymake::common::SparseMatrix_A_Int_I_Symmetric_Z", SparseMatrix< int, Symmetric >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
