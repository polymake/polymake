/* Copyright (c) 1997-2019
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
#include "polymake/group/permlib.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {

namespace {

bool equivalent_modulo_nullspace(const Vector<Rational>& vec1, 
                                 const Vector<Rational>& vec2, 
                                 const Matrix<Rational>& nullspace) {
   return (nullspace.rows()==0) //full-dim
      ? (vec1 == vec2)
      : is_zero(nullspace*(vec2-vec1));
}

} // end anonymous namespace

typedef std::pair<Matrix<Rational>, Array<hash_set<int>>> MatrixOrbitPair;

// works for both facets/linear span and rays/lineality space
MatrixOrbitPair symmetrize_poly_reps(const Matrix<Rational>& facets_in, 
                                     const Matrix<Rational>& linspan, 
                                     perl::Object action) 
{
   const Matrix<Rational> facets(common::primitive(facets_in));
   const Matrix<Rational> nullspace(null_space(linspan));
   Matrix<Rational> symmetric_facets(facets.rows(),facets.cols());
   std::vector<hash_set<int>> facet_orbit_list;
   Set<int> not_checked(range(0,facets.rows()-1));
    
   for(int i=0; i<facets.rows(); ++i) {
      if (not_checked.contains(i)) {
         symmetric_facets.row(i) = facets.row(i);
         not_checked -= i;
         hash_set<int> cur_facet_orbit;
         cur_facet_orbit += i;
         const auto orbit = group::orbits_in_orbit_order_impl(action, vector2row(facets.row(i)));
         for (auto row = entire(rows(orbit.first)); !row.at_end(); ++row) {
            for (auto index = entire(not_checked); !index.at_end(); ++index) {
               if (equivalent_modulo_nullspace(*row, facets.row(*index), nullspace)) {
                  cur_facet_orbit += *index;
                  symmetric_facets.row(*index) = *row;
                  not_checked.erase(index);
                  break;
               }	      
            }
         }
         facet_orbit_list.push_back(cur_facet_orbit);
      }
   }
   return MatrixOrbitPair(symmetric_facets, Array<hash_set<int>>(facet_orbit_list));
}


Function4perl(&symmetrize_poly_reps,"symmetrize_poly_reps(Matrix, Matrix, group::PermutationAction)");

}}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
