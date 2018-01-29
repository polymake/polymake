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
 *  computes the either gomory-chvatal closure of a polyhedron
 *  or a totally dual integral system
 */
perl::Object gc_and_tdi(perl::Object p_in, bool round)
{
  perl::Object p_out(perl::ObjectType::construct<Rational>("Polytope")), C;
  Matrix<Rational> vertices;  // Hilbert Basis and Vertices
  Set<Vector<Rational> > hilbert_ineqs;  // the new inequalities
  Vector<Rational> obj, this_vector;  // one inequality
  int n, i, k;  // counting variables
  Rational val;  // right hand side of the inequality

  p_in.give("VERTICES") >> vertices;
  p_in.give("N_VERTICES") >> n;

  // For every vertex do
  for (i=0; i<n; ++i) {
    if (vertices(i,0) == 0)
      continue;
    // Compute the Hilbertbasis of its normal cone
    C = call_function("normal_cone", p_in, i);
    Matrix<Integer> hb=C.call_method("HILBERT_BASIS");
    // For every Hilbertbasis element do
    for (k=0; k < hb.rows(); ++k) {
      // compute the right hand side
      this_vector= hb.row(k);
      obj = (0 | this_vector);
      // round if the gc_closure is needed
      val = (round == 1) ? (ceil(obj * (vertices[i]))) : val =  (obj * (vertices[i]));
      this_vector = (-val | this_vector);

      // add inequality to the system
      hilbert_ineqs +=  this_vector;
    }
  }

  p_out.take("INEQUALITIES") << hilbert_ineqs;

  return p_out;	
}


/*
 *  computes the gomory-chvatal closure of a polyhedron
 */
perl::Object gc_closure(perl::Object p_in)
{
  return gc_and_tdi(p_in, 1);
}


/*
 *  computes a totally dual integral inequality system for a polyhedron
 */
perl::Object make_totally_dual_integral(perl::Object p_in)
{
  return gc_and_tdi(p_in, 0);
}

/*
 *  Checks wether a given system of inequalities is totally dual integral or not
 */
bool totally_dual_integral(const Matrix<Rational>& inequalities)
{
  // checks if is not empty or if dimensions match
  const int dim = inequalities.cols();
  if (dim == 0)
    throw std::runtime_error("totally_dual_integral: non-empty matrix required");

  // variables decleration and initialization
  perl::Object p_in(perl::ObjectType::construct<Rational>("Polytope"));
  int n_lattice;
  perl::Object C;

  Matrix<Rational> ineq (inequalities / (unit_vector<Rational>(dim,0)));

  p_in.take("INEQUALITIES") << ineq;
  const Matrix<Rational> vertices = p_in.give("VERTICES");
  const IncidenceMatrix<> eq_sets = p_in.give("INEQUALITIES_THRU_VERTICES");

  //FIXME constraint, since polymake cannot compute hilbertbasis of non pointed cones
  const int polytope_dim = p_in.give("CONE_DIM");
  if (dim != polytope_dim)
    throw std::runtime_error("totally_dual_integral: the inequalities should descibe a full dimensional polyhedron");

  // for every vertex
  for (int i=0; i<vertices.rows(); ++i) {
    if (vertices(i,0) == 0)
      continue;
    // compute the normal_cone and its hilbert basis
    C = call_function("normal_cone", p_in, i);
    Matrix<Rational> hb = C.call_method("HILBERT_BASIS");

    // for every hilbert basis element of the normal cone
    // check weather it can be written as a non negative
    // integral linear combination of the active constraints
    for (int j=0; j < hb.rows(); ++j){
      // solutions is a polyhedron of all possible linear combinations
      // which give the hilbert basis element
      perl::Object solutions("Polytope<Rational>");

      // building the equations and ineq
      Matrix<Rational> temp_matrix (-hb.row(j) | T(ineq.minor(eq_sets[i], ~scalar2set(0))));
      Matrix<Rational> unit(unit_matrix<Rational>(temp_matrix.cols()));

      solutions.take("INEQUALITIES") << unit;
      solutions.take("EQUATIONS") << temp_matrix;

      //FIXME: workaround for the scheduling problem. Ticket #195
      solutions.give("VERTICES") >> temp_matrix;

      // check if there is an integral solution. If not -> return false
      // if the solution polytope is not bounded we cannot just count them
      // but we want to just count if it is possible because latte is fast
      bool bounded = 1;
      solutions.give("BOUNDED") >> bounded;

      if (bounded){
         solutions.give("N_LATTICE_POINTS") >> n_lattice;
      }else{
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
                  "# Produces the gomory-chvatal closure of a full dimensional polyhedron"
                  "# @param Polytope P"
                  "# @return Polytope",
                  &gc_closure, "gc_closure");

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Produces a polyhedron with an totally dual integral inequality formulation of a full dimensional polyhedron"
                  "# @param Polytope P"
                  "# @return Polytope",
                  &make_totally_dual_integral, "make_totally_dual_integral");

UserFunction4perl("# @category Optimization"
                  "# Checks weather a given system of inequalities is totally dual integral or not."
                  "# The inequalities should describe a full dimensional polyhedron"
                  "# @param Matrix inequalities"
                  "# @return Bool"
                  "# @example"
                  "# > print totally_dual_integral(cube(2)->FACETS);"
                  "# | 1",
                  &totally_dual_integral, "totally_dual_integral");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
