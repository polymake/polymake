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
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {
 
Array< Set<int> > common_refinement(const Matrix<Rational>& vertices, const Array< Set<int> >& sub1, const Array< Set<int> >& sub2,  const int dim)
{
   int n_cells = 0;

   std::list< Set<int> > l;
   for(Entire< Array<Set <int> > >::const_iterator i=entire(sub1);!i.at_end();++i)
      for(Entire< Array<Set <int> > >::const_iterator j=entire(sub2);!j.at_end();++j) {
         const Set<int> intersection = (*i)*(*j);
         if (intersection.size()>dim) {
            perl::Object p("Polytope<Rational>");
            p.take("VERTICES") << vertices.minor(intersection,All);
            const int int_dim = p.CallPolymakeMethod("DIM");
            if (int_dim == dim) {
               l.push_front(intersection);
               ++n_cells;
            }
         }
      }
  
   Array< Set<int> > refinement(n_cells);
   for (Entire< Array<Set <int> > >::iterator i=entire(refinement); !i.at_end(); ++i) {
      *i= l.front();
      l.pop_front();
   }

   return refinement;
}

perl::Object common_refinement1(perl::Object p1, perl::Object p2)
{
   int dim = p1.CallPolymakeMethod("DIM");
   const Matrix<Rational> vert=p1.give("VERTICES");
   const Array< Set<int> > sub1=p1.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
   const Array< Set<int> > sub2=p2.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
  
   perl::Object p_out("Polytope<Rational>");//FIXME: p_out should become a copy of p1 if there exists a copy method
   //  p_out.remove("WEIGHTS"); FIXME (If there is a remove method).
   if (p1.exists("POLYTOPAL_SUBDIVISION.WEIGHTS") && p2.exists("POLYTOPAL_SUBDIVISION.WEIGHTS")) {
      const Vector<Rational> w1=p1.give("POLYTOPAL_SUBDIVISION.WEIGHTS");
      const Vector<Rational> w2=p2.give("POLYTOPAL_SUBDIVISION.WEIGHTS");
      p_out.take("POLYTOPAL_SUBDIVISION.WEIGHTS")<<(w1+w2);
   }
   p_out.take("FEASIBLE")<<true;
   p_out.take("VERTICES")<<vert;//FIXME
   p_out.take("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS")<<common_refinement(vert,sub1,sub2,dim);
   return p_out;
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Computes the common refinement of two subdivisions of //points//."
                  "# It is assumed that there exists a common refinement of the two subdivisions."
                  "# @param Matrix points"
                  "# @param Array<Set> sub1 first subdivision"
                  "# @param Array<Set> sub2 second subdivision"
                  "# @param Int dim dimension of the point configuration"
                  "# @return Array<Set<Int>> the common refinement"
                  "# @author Sven Herrmann",
                  &common_refinement, "common_refinement(Matrix $ $ $)");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Computes the common refinement of two subdivisions of the same polytope //p1//, //p2//."
                  "# It is assumed that there exists a common refinement of the two subdivisions."
                  "# It is not checked if //p1// and //p2// are indeed the same!"
                  "# @param Polytope p1"
                  "# @param Polytope p2"
                  "# @return Polytope"
                  "# @author Sven Herrmann",
                  &common_refinement1, "common_refinement(Polytope Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
