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

///==== this line controls the automatic file splitting: max.instances=40

#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Map.h"
#include "polymake/SparseMatrix.h"
#include "polymake/client.h"
#include "polymake/topaz/HomologyComplex.h"

namespace polymake { namespace topaz { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   Class4perl("Polymake::common::Pair_A_CycleGroup__Integer_I_Map_A_Pair_A_Int_I_Int_Z_I_Int_Z_Z", std::pair< CycleGroup< Integer >, Map< std::pair< int, int >, int > >);
   Class4perl("Polymake::common::Pair_A_Array__HomologyGroup__Integer_I_Array__CycleGroup__Integer_Z", std::pair< Array< HomologyGroup< Integer > >, Array< CycleGroup< Integer > > >);
   Class4perl("Polymake::common::Pair_A_Array__HomologyGroup__Integer_I_Array__Pair_A_SparseMatrix_A_Integer_I_NonSymmetric_Z_I_Array__Int_Z_Z", std::pair< Array< HomologyGroup< Integer > >, Array< std::pair< SparseMatrix< Integer, NonSymmetric >, Array< int > > > >);
   Class4perl("Polymake::common::Pair_A_HomologyGroup__Integer_I_SparseMatrix_A_Integer_I_NonSymmetric_Z_Z", std::pair< HomologyGroup< Integer >, SparseMatrix< Integer, NonSymmetric > >);
///==== Automatically generated contents end here.  Please do not delete this line. ====
} } }
