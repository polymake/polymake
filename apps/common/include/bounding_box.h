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

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"

namespace polymake { namespace common {

// for sparse matrices
template<typename Coord, typename MatrixTop>
std::enable_if_t<MatrixTop::is_sparse, Matrix<Coord>>
bounding_box(const GenericMatrix<MatrixTop, Coord>& V)
{
   const Int d = V.cols();
   Matrix<Coord> BB(2, d);
   if (d) {
      for (auto c = entire(cols(V)); !c.at_end(); ++c) {
         if (c->size() == V.rows()) { //non-sparse col
            BB(0, c.index()) = (*c)[0];
            BB(1, c.index()) = (*c)[0];
         }
         for (auto it = entire(*c); !it.at_end(); ++it) {
            assign_min_max(BB(0,c.index()), BB(1,c.index()), *it);
         }
      }
   }
   return BB;
}

// for non-sparse matrices
template <typename Coord, typename MatrixTop>
std::enable_if_t<!MatrixTop::is_sparse, Matrix<Coord>>
bounding_box(const GenericMatrix<MatrixTop, Coord>& V)
{
   const Int d = V.cols();
   Matrix<Coord> BB(2,d);
   if (V.rows()) {
      auto r = entire(rows(V));
      BB[0] = *r;
      BB[1] = *r;
      while (!(++r).at_end()) {
         auto c = r->begin();
         for (Int j = 0; j < d; ++j, ++c)
            assign_min_max(BB(0,j), BB(1,j), *c);
      }
   }
   return BB;
}

template <typename Coord>
void extend_bounding_box(Matrix<Coord>& BB, const Matrix<Coord>& BB2)
{
   if (BB.rows()) {
      const Int d = BB.cols();
      auto src = concat_rows(BB2).begin();
      auto dst = concat_rows(BB).begin();
      for (Int j = 0; j < d; ++j, ++dst, ++src) assign_min(*dst, *src);
      for (Int j = 0; j < d; ++j, ++dst, ++src) assign_max(*dst, *src);
   } else {
      BB = BB2;
   }
}


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
