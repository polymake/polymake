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
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

template <typename Faces>
std::enable_if_t<pm::isomorphic_to_container_of<Faces, Set<Int>>::value, Set<Int>>
splits_in_subdivision(const Matrix<Rational>& verts, const Faces& subdivision, const Matrix<Rational>& splits)
{
   const Int n = verts.rows();  
   const Int n_splits = splits.rows();
   Set<Int> split_set;

   for (Int j = 0; j < n_splits; ++j) {
      const Vector<Rational> a=splits.row(j);
      Set<Int> left;
      Set<Int> right;
      for (Int k = 0; k < n; ++k) {
         const Rational val = a*verts.row(k);
         if (val > 0) {
            left.insert(k);
         } else if (val < 0) {
            right.insert(k);
         }
      }
      bool cut = false;
      for (auto k = entire(subdivision); !k.at_end();  ++k) {
         if (!((*k)*left).empty() && !((*k)*right).empty()) {
            cut = true;
            break;
         }
      }
      if (!cut) split_set.push_back(j);
   }
   return split_set;
}

Set<Int> splits_in_subdivision(const Matrix<Rational>& verts, const IncidenceMatrix<>& subdivision, const Matrix<Rational>& splits)
{
   return splits_in_subdivision(verts, rows(subdivision), splits);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Tests which of the //splits// of a polyhedron are coarsenings of the given //subdivision//."
                          "# @param Matrix vertices the vertices of the polyhedron"
                          "# @param Array<Set<Int>> subdivision a subdivision of the polyhedron"
                          "# @param Matrix splits the splits of the polyhedron"
                          "# @return Set<Int>"
                          "# @author Sven Herrmann",
                          "splits_in_subdivision(Matrix,*,Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
