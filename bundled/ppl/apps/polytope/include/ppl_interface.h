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

#ifndef POLYMAKE_POLYTOPE_PPL_INTERFACE_H
#define POLYMAKE_POLYTOPE_PPL_INTERFACE_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Bitset.h"
#include "polymake/permutations.h"
#include "polymake/polytope/lpch_dispatcher.h"

namespace polymake { namespace polytope { namespace ppl_interface {

template <typename Coord>
class solver {
public:
   typedef Coord coord_type;

   solver();

   typedef std::pair< Matrix<coord_type>, Matrix<coord_type> > matrix_pair;

   /// @retval first: facets, second: affine hull
   matrix_pair
   enumerate_facets(const Matrix<coord_type>& Points, const Matrix<coord_type>& Lineality, const bool isCone = false, const bool primal = false);


   // FIXME last argument necessary as long as lrs uses old format
   matrix_pair
   enumerate_vertices(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Equations, const bool isCone = false, const bool primal = true); 


   typedef std::pair<Bitset, ListMatrix< Vector<coord_type> > > non_redundant;

   /// @retval indices of the vertices 
   Bitset
   find_vertices_among_points(const Matrix<coord_type>& Points, const Matrix<coord_type>& Lineality, const bool isCone = false);

   Bitset
   find_vertices_among_points_given_inequalities(const Matrix<coord_type>& Points, const Matrix<coord_type>& Inequalities);

   /// @retval indices of the facets 
   Bitset
   find_facets_among_inequalities(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Equations, const bool isCone = false);

   Bitset
   find_facets_among_inequalities_given_points(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Points);

   /*
   typedef std::pair<Bitset, Bitset > non_redundant_canonical;

   /// @retval first: indices of vertices, second: indices of lineality_space
   non_redundant_canonical
   canonicalize(const Matrix<coord_type>& Points, const Matrix<coord_type>& InputLineality, bool primal = false);

   Bitset
   canonicalize_lineality(const Matrix<Coord>& Points, const Matrix<Coord>& InputLineality, bool primal = false);
   */
   typedef std::pair<coord_type, Vector<coord_type> > lp_solution;

   /// @retval first: objective value, second: solution
   // LP only defined for polytopes
   lp_solution
   solve_lp(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Equations,
            const Vector<coord_type>& Objective, bool maximize);
};

} } }

#endif // POLYMAKE_POLYTOPE_PPL_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
