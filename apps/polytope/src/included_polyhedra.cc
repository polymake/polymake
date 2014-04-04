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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Coord>
bool included_polyhedra(perl::Object p1, perl::Object p2, perl::OptionSet options)
{
   
   std::string generator_type = p1.isa("Polytope") ? "point" : "ray";
   const bool verbose=options["verbose"];
   const Matrix<Coord> vert=p1.give("RAYS|INPUT_RAYS"), lin=p1.lookup("LINEALITY_SPACE"), ineq=p2.give("FACETS|INEQUALITIES"), eq=p2.lookup("LINEAR_SPAN|EQUATIONS");
      
   if (vert.cols()!=ineq.cols()) {
      if (verbose) cout << "Cones/Polytopes do no live in the same ambient space."<<endl;
      return false;
   }
      
   for (typename Entire< Rows < Matrix<Coord> > >::const_iterator i=entire(rows(eq)); !i.at_end(); ++i) {
      for (typename Entire< Rows < Matrix<Coord> > >::const_iterator j=entire(rows(vert)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Equation " << *i << " not satisfied by " << generator_type << " " << *j <<"."<< endl;
            return false;
         }
      for (typename Entire< Rows < Matrix<Coord> > >::const_iterator j=entire(rows(lin)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Equation " << *i << " not satisfied by " << "lineality space generator " << *j <<"."<< endl;
            return false;
         }
   }
   for (typename Entire< Rows < Matrix<Coord> > >::const_iterator i=entire(rows(ineq)); !i.at_end(); ++i) {
      for (typename Entire< Rows < Matrix<Coord> > >::const_iterator j=entire(rows(vert)); !j.at_end(); ++j)
         if ((*i)*(*j)<0) {
            if (verbose) cout << "Inequality " << *i << " not satisfied by " << generator_type << " " << *j <<"."<< endl;
            return false;
         }
      for (typename Entire< Rows < Matrix<Coord> > >::const_iterator j=entire(rows(lin)); !j.at_end(); ++j)
         if ((*i)*(*j)!=0) {
            if (verbose) cout << "Inequality " << *i << " not satisfied by " << "lineality space generator " << *j <<"."<< endl;
            return false;
         }
   }
   return true;
}

UserFunctionTemplate4perl("# @category Comparing"
                          "# Tests if polyhedron //P1// is included in polyhedron //P2//."
                          "# @param Polytope P1 the first polytope"
                          "# @param Polytope P2 the second polytope"
                          "# @return Bool 'true' if //P1// is included in //P2//, 'false' otherwise"
                          "# @author Sven Herrmann",
                          "included_polyhedra<Coord>(Cone<Coord>, Cone<Coord>; { verbose => 0 })");

InsertEmbeddedRule("# @category Comparing\n"
                   "# Tests if the two polyhedra //P1// and //P2// are equal.\n"
                   "# @param Polytope P1 the first polytope"
                   "# @param Polytope P2 the second polytope"
                   "# @return Bool true if the two polyhedra are equal, false otherwise"
                   "# @author Sven Herrmann\n"
                   "user_function equal_polyhedra<Coord>(Cone<Coord>, Cone<Coord>; { verbose => 0 } ) {"
                   "my $p1=shift;"
                   "my $p2=shift;"
                   "included_polyhedra($p1,$p2,@_) and included_polyhedra($p2,$p1,@_);  }\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
