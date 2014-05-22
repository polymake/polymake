/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/PowerSet.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/polytope/simplex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include "polymake/polytope/representative_simplices.h"
#include <vector>
#include <sstream>

namespace polymake { namespace polytope {

typedef common::boost_dynamic_bitset SetType;
      //typedef Set<int> SetType;
typedef Array<SetType> SimplexArray;
typedef Array<SimplexArray> RepArray;
typedef Array<Set<int> > Orbits;



template<typename Scalar>
RepArray representative_simplices(int d, const Matrix<Scalar>& V, const Array<Array<int> >& generators)
{
   const group::PermlibGroup sym_group(generators);
   RepArray cds(d+1);
   for (int k=0; k<=d; ++k) {
      Set<SetType> reps;
      for (simplex_rep_iterator<Scalar, SetType> sit(V, k, sym_group); !sit.at_end(); ++sit) {
         reps += *sit; // the iterator may produce isomorphic simplices more than once
      }
      cds[k] = SimplexArray(reps);
   }
   return cds;
}

template <typename Scalar>
Array<SetType>
representative_max_interior_simplices(int d, const Matrix<Scalar>& V, const Array<Array<int> >& generators)
{
   const group::PermlibGroup sym_group(generators);
   Set<SetType> reps;
   for (simplex_rep_iterator<Scalar, SetType> sit(V, d, sym_group); !sit.at_end(); ++sit) 
      reps += *sit;
   return reps;
}



template <typename Scalar>
std::pair< Array<SetType>, Array<SetType> >
representative_interior_and_boundary_ridges(perl::Object p, perl::OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const int d = is_config 
      ? p.give("DIM")
      : p.give("COMBINATORIAL_DIM");

   std::string vif_property = options["vif_property"];
   if (!vif_property.size()) vif_property = is_config
                                ? std::string("CONVEX_HULL.VERTICES_IN_FACETS")
                                : std::string("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(vif_property.c_str());

   const Matrix<Scalar> V = is_config
      ? p.give("CONVEX_HULL.VERTICES")
      : p.give("RAYS");

   const Array<Array<int> > generators = p.give("GROUP.GENERATORS");
   const group::PermlibGroup sym_group(generators);

   Set<SetType> interior_ridges, boundary_ridges;
   // the iterator may produce isomorphic simplices more than once
   for (simplex_rep_iterator<Scalar, SetType> sit(V, d-1, sym_group); !sit.at_end(); ++sit) 
      if (is_in_boundary(*sit, VIF)) boundary_ridges += *sit;
      else interior_ridges += *sit;
   return std::make_pair<Array<SetType>, Array<SetType> >(interior_ridges, boundary_ridges);
}


FunctionTemplate4perl("representative_simplices<Scalar>($ Matrix<Scalar> Array<Array<Int>>)");

FunctionTemplate4perl("representative_max_interior_simplices<Scalar>($ Matrix<Scalar> Array<Array<Int>>)");

FunctionTemplate4perl("representative_interior_and_boundary_ridges<Scalar>(Polytope<Scalar> { vif_property => '' } )");

FunctionTemplate4perl("representative_max_interior_simplices<Scalar>(Polytope<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
