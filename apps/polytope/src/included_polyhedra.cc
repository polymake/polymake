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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/contains.h"

namespace polymake { namespace polytope {

template<typename Coord, typename ConstraintChecker>
void check_for_constraint_violation(const Matrix<Coord>& constraints, const Matrix<Coord>& generators, ConstraintChecker& check, std::string constraint_type, std::string generator_type)
{
   for(const auto& constraint : rows(constraints)){
      for(const auto& generator : rows(generators)){
         if(!check(constraint, generator)){
            cout << constraint_type << " " << constraint << " not satisfied by " << generator_type << " " << generator <<"."<< endl;
            return;
         }
      }
   }
}


template <typename Coord>
void find_first_violated_constraint(BigObject p1, BigObject p2)
{
   std::string generator_type = p1.isa("Polytope") ? "point" : "ray";
   const Matrix<Coord> vert=p1.give("RAYS|INPUT_RAYS"), lin=p1.lookup("LINEALITY_SPACE|INPUT_LINEALITY"), ineq=p2.give("FACETS|INEQUALITIES"), eq=p2.lookup("LINEAR_SPAN|EQUATIONS");

   const Int dim1 = p1.give("CONE_AMBIENT_DIM");
   const Int dim2 = p2.give("CONE_AMBIENT_DIM");
   if (dim1 != dim2) {
      throw std::runtime_error("Cones/Polytopes do no live in the same ambient space.");
   }

   auto check_equation = [] (const auto& c, const auto& g) { return c*g == 0; };
   check_for_constraint_violation(eq, vert, check_equation, "Equation", generator_type);
   check_for_constraint_violation(eq, lin, check_equation, "Equation", "lineality space generator");
   
   auto check_inequality = [] (const auto& c, const auto& g) { return c*g >= 0; };
   check_for_constraint_violation(ineq, vert, check_inequality, "Inequality", generator_type);
   check_for_constraint_violation(ineq, lin, check_inequality, "Inequality", "lineality space generator");
      
}

template <typename Coord>
bool included_polyhedra(BigObject p1, BigObject p2, OptionSet options)
{
   if(contains<Coord>(p1, p2)){
      return true;
   } else {
      const bool verbose=options["verbose"];
      if(verbose) find_first_violated_constraint<Coord>(p1, p2);
      return false;
   }
}
   

UserFunctionTemplate4perl("# @category Comparing"
                          "# @author Sven Herrmann"
                          "# Tests if polyhedron //P1// is included in polyhedron //P2//."
                          "# @param Polytope P1 the first polytope"
                          "# @param Polytope P2 the second polytope"
                          "# @option Bool verbose Prints information on the difference between P1 and P2 if none is included in the other."
                          "# @return Bool 'true' if //P1// is included in //P2//, 'false' otherwise"
                          "# @example [prefer ppl]"
                          "# > print included_polyhedra(simplex(3),cube(3));"
                          "# | true"
                          "# To see in what way the two polytopes differ, try this:"
                          "# > $p = new Polytope(VERTICES => [[1,-1,-1],[1,1,-1],[1,-1,1],[1,1,1]]);"
                          "# > print included_polyhedra($p,simplex(2),verbose => 1);"
                          "# | Inequality 0 1 0 not satisfied by point 1 -1 -1."
                          "# | false",
                          "included_polyhedra<Coord>(Cone<Coord>, Cone<Coord>; { verbose => 0 })");

InsertEmbeddedRule("# @category Comparing"
                   "# @author Sven Herrmann"
                   "# Tests if the two polyhedra //P1// and //P2// are equal."
                   "# @param Polytope P1 the first polytope"
                   "# @param Polytope P2 the second polytope"
                   "# @option Bool verbose Prints information on the difference between P1 and P2 if they are not equal."
                   "# @return Bool true if the two polyhedra are equal, false otherwise"
                   "# @example [prefer cdd] [require bundled:cdd]"
                   "# > $p = new Polytope(VERTICES => [[1,-1,-1],[1,1,-1],[1,-1,1],[1,1,1]]);"
                   "# > print equal_polyhedra($p,cube(2));"
                   "# | true"
                   "# To see why two polytopes are unequal, try this:"
                   "# > print equal_polyhedra($p,simplex(2),verbose => 1);"
                   "# | Inequality 1 -1 -1 not satisfied by point 1 1 1."
                   "# | false\n"
                   "user_function equal_polyhedra<Coord>(Cone<Coord>, Cone<Coord>; { verbose => 0 } ) {\n"
                   "my $p1=shift;\n"
                   "my $p2=shift;\n"
                   "included_polyhedra($p1,$p2,@_) and included_polyhedra($p2,$p1,@_);  }\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
