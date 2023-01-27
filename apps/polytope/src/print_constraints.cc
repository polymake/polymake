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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include <fstream>
#include "polymake/common/print_constraints.h"

namespace polymake { namespace polytope {

template <typename Scalar>
void print_constraints(BigObject P, OptionSet options)
{
   const bool ispoly = P.isa("Polytope");
   Matrix<Scalar> Ineqs = P.give("FACETS|INEQUALITIES");
   Array<std::string> coord_labels;
   P.lookup("COORDINATE_LABELS") >> coord_labels;
   if (Ineqs.rows() > 0){
      cout << (P.exists("FACETS") ? "Facets:" : "Inequalities:") << endl;
      common::print_constraints_sub(Ineqs, coord_labels, options["ineq_labels"], 0, !ispoly);
   }
   if (P.exists("LINEAR_SPAN") || P.exists("EQUATIONS")) {
     // do not force the computation of LINEAR_SPAN if only INEQUALITIES are given
     Matrix<Scalar> Eqs = P.give("LINEAR_SPAN|EQUATIONS");
     if (Eqs.rows() > 0) {
       cout << (P.exists("LINEAR_SPAN") ? (ispoly ? "Affine hull:" : "Linear span:") : "Equations:") << endl;
       common::print_constraints_sub(Eqs, coord_labels, options["eq_labels"], 1, !ispoly);
     }
   }
}

UserFunctionTemplate4perl("# @category Optimization"
           "# Write the [[FACETS]] / [[INEQUALITIES]] and the [[LINEAR_SPAN]] / [[EQUATIONS]] (if present)"
           "# of a polytope //P// or cone //C// in a readable way."
           "# [[COORDINATE_LABELS]] are adopted if present."
           "# @param Cone<Scalar> C the given polytope or cone"
           "# @option Array<String> ineq_labels changes the labels of the inequality rows"
           "# @option Array<String> eq_labels changes the labels of the equation rows"
           "# @example The following prints the facet inequalities of the square, changing the labels."
           "# > print_constraints(cube(2),ineq_labels=>['zero','one','two','three']);"
           "# | Facets:"
           "# | zero: x1 >= -1"
           "# | one: -x1 >= -1"
           "# | two: x2 >= -1"
           "# | three: -x2 >= -1",
           "print_constraints<Scalar>(Cone<Scalar>; { ineq_labels => undef, eq_labels => undef })");
} }
