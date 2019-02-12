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
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Coord>
bool included_polyhedra(perl::Object p1, perl::Object p2, perl::OptionSet options)
{
   
   std::string generator_type = p1.isa("Polytope") ? "point" : "ray";
   const bool verbose=options["verbose"];
   const Matrix<Coord> vert=p1.give("RAYS|INPUT_RAYS"), lin=p1.lookup("LINEALITY_SPACE|INPUT_LINEALITY"), ineq=p2.give("FACETS|INEQUALITIES"), eq=p2.lookup("LINEAR_SPAN|EQUATIONS");

   const int dim1 = p1.give("CONE_AMBIENT_DIM");
   const int dim2 = p2.give("CONE_AMBIENT_DIM");
   if (dim1!=dim2) {
      if (verbose) cout << "Cones/Polytopes do no live in the same ambient space."<<endl;
      return false;
   }
      
   for (auto i=entire(rows(eq)); !i.at_end(); ++i) {
      for (auto j=entire(rows(vert)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Equation " << *i << " not satisfied by " << generator_type << " " << *j <<"."<< endl;
            return false;
         }
      for (auto j=entire(rows(lin)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Equation " << *i << " not satisfied by " << "lineality space generator " << *j <<"."<< endl;
            return false;
         }
   }
   for (auto i=entire(rows(ineq)); !i.at_end(); ++i) {
      for (auto j=entire(rows(vert)); !j.at_end(); ++j)
         if ((*i)*(*j)<0) {
            if (verbose) cout << "Inequality " << *i << " not satisfied by " << generator_type << " " << *j <<"."<< endl;
            return false;
         }
      for (auto j=entire(rows(lin)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Inequality " << *i << " not satisfied by " << "lineality space generator " << *j <<"."<< endl;
            return false;
         }
   }
   return true;
}

UserFunctionTemplate4perl("# @category Comparing"
                          "# @author Sven Herrmann"
                          "# Tests if polyhedron //P1// is included in polyhedron //P2//."
                          "# @param Polytope P1 the first polytope"
                          "# @param Polytope P2 the second polytope"
                          "# @option Bool verbose Prints information on the difference between P1 and P2 if none is included in the other."
                          "# @return Bool 'true' if //P1// is included in //P2//, 'false' otherwise"
                          "# @example"
                          "# > print included_polyhedra(simplex(3),cube(3));"
                          "# | true"
                          "# To see in what way the two polytopes differ, try this:"
                          "# > print included_polyhedra(cube(2),cube(3),verbose=>1);"
                          "# | Cones/Polytopes do no live in the same ambient space."
                          "# | false",
                          "included_polyhedra<Coord>(Cone<Coord>, Cone<Coord>; { verbose => 0 })");

InsertEmbeddedRule("# @category Comparing"
                   "# @author Sven Herrmann"
                   "# Tests if the two polyhedra //P1// and //P2// are equal."
                   "# @param Polytope P1 the first polytope"
                   "# @param Polytope P2 the second polytope"
                   "# @option Bool verbose Prints information on the difference between P1 and P2 if they are not equal."
                   "# @return Bool true if the two polyhedra are equal, false otherwise"
                   "# @example [prefer cdd] > $p = new Polytope(VERTICES => [[1,-1,-1],[1,1,-1],[1,-1,1],[1,1,1]]);"
                   "# > print equal_polyhedra($p,cube(2));"
                   "# | true"
                   "# To see why two polytopes are unequal, try this:"
                   "# > print equal_polyhedra($p,cube(3),verbose => 1);"
                   "# | Cones/Polytopes do no live in the same ambient space."
                   "# | false"
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
