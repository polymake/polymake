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

#ifndef POLYMAKE_POLYTOPE_LRS_INTERFACE_H
#define POLYMAKE_POLYTOPE_LRS_INTERFACE_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Bitset.h"
#include "polymake/polytope/linsolver.h"

namespace polymake { namespace polytope { namespace lrs_interface {

class solver {
public:
   typedef Rational coord_type;

   typedef std::pair< Matrix<Rational>, Matrix<Rational> > matrix_pair;

   /// @retval first: facets, second: affine hull
   matrix_pair
   enumerate_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool isCone = false, const bool primal = false); 

   // only used for lrs
   long
   count_facets(const Matrix<Rational>& Points,const Matrix<Rational>& Lineality, const bool isCone = false);

   // FIXME argument 3 unused, for consitency with cdd_interface
   // as cdd does not return the origin for cones represented as polyhedra
   matrix_pair
   enumerate_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, bool isCone = false, const bool primal = true);

   // verts: pair of integers: first:  number of vertices
   //                          second: number of bounded vertices
   //        the first entry is 0 if only N_BOUNDED_VERTICES is requested
   //        (doing this for a cone leads to an error
   // lin: dimension (== num of generators) for lineality space
   // only used for lrs
   struct vertex_count { std::pair<long,long> verts; long lin; };

   // only used for lrs
   vertex_count
   count_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
                  bool only_bounded=false);

   typedef std::pair<Bitset, Matrix<Rational> > non_redundant;

   /// @retval first: indices of vertices, second: affine hull (primal)
   /// @retval first: indices of facets, second: lineality space (dual)
   // function only works for pointed objects: lrs does not remove generators in the lineality space from input
   non_redundant find_irredundant_representation(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool dual);

   Vector<Rational>
   find_a_vertex(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations);

   bool check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations);
   bool check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, Vector<Rational>& ValidPoint);

   typedef std::pair<Rational, Vector<Rational> > lp_solution;

   /// @retval first: objective value, second: solution
   lp_solution
   solve_lp(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
            const Vector<Rational>& Objective, bool maximize, int* linearity_dim_p = 0);

private:
   struct dictionary;
};

} } }

#endif // POLYMAKE_POLYTOPE_LRS_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
