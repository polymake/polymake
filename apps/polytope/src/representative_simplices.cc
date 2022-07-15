/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/polytope/simplex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/Bitset.h"
#include "polymake/polytope/representative_simplices.h"
#include <vector>
#include <sstream>

namespace polymake { namespace polytope {

typedef Bitset BBitset;
typedef Array<BBitset> SimplexArray;
typedef Array<SimplexArray> RepArray;

template<typename Scalar>
RepArray representative_simplices(Int d, const Matrix<Scalar>& V, const Array<Array<Int>>& generators)
{
   const group::PermlibGroup sym_group(generators);
   RepArray cds(d+1);
   for (Int k = 0; k <= d; ++k) {
      Set<BBitset> reps;
      for (simplex_rep_iterator<Scalar, BBitset> sit(V, k, sym_group); !sit.at_end(); ++sit) {
         reps += *sit; // the iterator may produce isomorphic simplices more than once
      }
      cds[k] = SimplexArray(reps);
   }
   return cds;
}

template <typename Scalar>
Array<BBitset>
representative_max_interior_simplices(Int d, const Matrix<Scalar>& V, const Array<Array<Int>>& generators)
{
   const group::PermlibGroup sym_group(generators);
   Set<BBitset> reps;
   for (simplex_rep_iterator<Scalar, BBitset> sit(V, d, sym_group); !sit.at_end(); ++sit) 
      reps += *sit;
   return Array<BBitset>(reps);
}



template <typename Scalar>
std::pair<Array<BBitset>, Array<BBitset>>
representative_interior_and_boundary_ridges(BigObject p, OptionSet options)
{
   const bool is_config = p.isa("PointConfiguration");

   const Int d = is_config 
      ? p.give("CONVEX_HULL.COMBINATORIAL_DIM")
      : p.give("COMBINATORIAL_DIM");

   AnyString VIF_property = options["VIF_property"];
   if (!VIF_property)
      VIF_property = is_config
                     ? Str("CONVEX_HULL.POINTS_IN_FACETS")
                     : Str("RAYS_IN_FACETS");
   const IncidenceMatrix<> VIF = p.give(VIF_property);

   const Matrix<Scalar> V = is_config
      ? p.give("POINTS")
      : p.give("RAYS");

   const Array<Array<Int>> generators = is_config
      ? p.give("GROUP.POINTS_ACTION.GENERATORS")
      : p.give("GROUP.RAYS_ACTION.GENERATORS");
   const group::PermlibGroup sym_group(generators);

   Set<BBitset> interior_ridges, boundary_ridges;
   // the iterator may produce isomorphic simplices more than once
   for (simplex_rep_iterator<Scalar, BBitset> sit(V, d-1, sym_group); !sit.at_end(); ++sit) 
      if (is_in_boundary(*sit, VIF)) 
         boundary_ridges += *sit;
      else 
         interior_ridges += *sit;
   return { Array<BBitset>(interior_ridges), Array<BBitset>(boundary_ridges) };
}


FunctionTemplate4perl("representative_simplices<Scalar>($ Matrix<Scalar> Array<Array<Int>>)");

FunctionTemplate4perl("representative_max_interior_simplices<Scalar>($ Matrix<Scalar> Array<Array<Int>>)");

FunctionTemplate4perl("representative_interior_and_boundary_ridges<Scalar=Rational>($ { VIF_property=>undef } )");

FunctionTemplate4perl("representative_max_interior_simplices<Scalar=Rational>($)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
