/* Copyright (c) 1997-2023
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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Integer.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {

/*
 *  Computes the Gomory-Chvátal closure of a polyhedron (if round is set to true);
 *  or computes a totally dual integral system of defining inequalities (if round is set to false).
 *  In both cases a new big object is returned.
 */
BigObject gc_and_tdi(BigObject p_in, bool round)
{
   Matrix<Rational> vertices;  // Hilbert basis and Vertices
   Set<Vector<Rational> > hilbert_ineqs;  // the new inequalities
   Vector<Rational> obj, this_vector;  // one inequality
   Int n, i, k;  // counting variables
   Rational val;  // right hand side of the inequality

   p_in.give("VERTICES") >> vertices;
   p_in.give("N_VERTICES") >> n;

   // for every vertex do
   for (i = 0; i < n; ++i) {
      if (vertices(i,0) == 0)
         continue;
      // compute the Hilbert basis of its normal cone
      BigObject C = call_function("normal_cone", p_in, i);
      Matrix<Integer> hb = C.call_method("HILBERT_BASIS");
      // for every Hilbert basis element do
      for (k=0; k < hb.rows(); ++k) {
         // compute the right hand side
         this_vector= hb.row(k);
         obj = (0 | this_vector);
         // round if the gc_closure is needed
         val = (round == 1) ? ceil(obj * (vertices[i])) : obj * (vertices[i]);
         this_vector = (-val | this_vector);

         // add inequality to the system
         hilbert_ineqs +=  this_vector;
      }
   }

   return BigObject("Polytope", mlist<Rational>(), "INEQUALITIES", hilbert_ineqs);	
}

/*
 *  Computes the Gomory-Chvátal closure of a polyhedron.
 */
BigObject gc_closure(BigObject p_in)
{
  return gc_and_tdi(p_in, 1);
}


/*
 *  Computes a totally dual integral inequality system for a polyhedron.
 */
BigObject make_totally_dual_integral(BigObject p_in)
{
  return gc_and_tdi(p_in, 0);
}

/*
 *  Checks wether a given system of inequalities is totally dual integral or not.
 */
bool totally_dual_integral(const Matrix<Rational>& inequalities)
{
   // checks if is not empty or if dimensions match
   const Int dim = inequalities.cols();
   if (dim == 0)
      throw std::runtime_error("totally_dual_integral: non-empty matrix required");

   const Matrix<Rational> ineq = inequalities / unit_vector<Rational>(dim, 0);
   BigObject p_in("Polytope<Rational>", "INEQUALITIES", ineq);
   const Matrix<Rational> vertices = p_in.give("VERTICES");
   const IncidenceMatrix<> eq_sets = p_in.give("INEQUALITIES_THRU_VERTICES");

   //FIXME constraint, since polymake cannot compute Hilbert basis of non pointed cones
   const Int polytope_dim = p_in.give("CONE_DIM");
   if (dim != polytope_dim)
      throw std::runtime_error("totally_dual_integral: the inequalities should describe a full dimensional polyhedron");

   // for every vertex
   for (Int i = 0; i < vertices.rows(); ++i) {
      if (vertices(i,0) == 0)
         continue;
      // compute the normal_cone and its Hilbert basis
      BigObject C = call_function("normal_cone", p_in, i);
      Matrix<Rational> hb = C.call_method("HILBERT_BASIS");

      // for every hilbert basis element of the normal cone
      // check whether it can be written as a non negative
      // integral linear combination of the active constraints
      for (Int j = 0; j < hb.rows(); ++j){
         // solutions is a polyhedron of all possible linear combinations
         // which give the Hilbert basis element
         Matrix<Rational> temp_matrix(-hb.row(j) | T(ineq.minor(eq_sets[i], range_from(1))));

         BigObject solutions("Polytope<Rational>",
                             "INEQUALITIES", unit_matrix<Rational>(temp_matrix.cols()),
                             "EQUATIONS", temp_matrix);

         // check if there is an integral solution. If not -> return false
         // if the solution polytope is not bounded we cannot just count them
         // but we want to just count if it is possible because latte is fast
         const bool bounded = solutions.give("BOUNDED");

         Int n_lattice;
         if (bounded) {
            solutions.give("N_LATTICE_POINTS") >> n_lattice;
         } else {
            Array< Matrix<Rational> > latticepoints(3);
            solutions.give("LATTICE_POINTS_GENERATORS") >> latticepoints;
            n_lattice = latticepoints[0].rows();
         }
         if (n_lattice == 0)
            return false;
      }
   }

   // if everything worked out -> return true
   return true;
}


UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Computes the Gomory-Chvátal closure of a full dimensional polyhedron."
                  "# See Schrijver, Theory of Linear and Integer programming (Wiley 1986), §23.1."
                  "# @param Polytope P"
                  "# @return Polytope",
                  &gc_closure, "gc_closure");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Computes a polyhedron with an totally dual integral inequality formulation of a full dimensional polyhedron."
                  "# See Schrijver, Theory of Linear and Integer programming (Wiley 1986), §22.3."
                  "# @param Polytope P"
                  "# @return Polytope",
                  &make_totally_dual_integral, "make_totally_dual_integral");

UserFunction4perl("# @category Optimization"
                  "# Checks if a given system of inequalities is totally dual integral or not."
                  "# The inequalities should describe a full dimensional polyhedron"
                  "# @param Matrix inequalities"
                  "# @return Bool"
                  "# @example [require bundled:libnormaliz]"
                  "# > print totally_dual_integral(cube(2)->FACETS);"
                  "# | true",
                  &totally_dual_integral, "totally_dual_integral");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
