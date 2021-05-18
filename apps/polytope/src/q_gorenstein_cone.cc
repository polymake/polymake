/* Copyright (c) 1997-2021
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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {

// first entry: cone is gorenstein_cone
// second entry: Gorenstein index if cone is Gorenstein
std::pair<bool, Int> q_gorenstein_cone(Matrix<Rational> rational_rays, Int dim)
{
  std::pair<bool, Int> gorenstein{ false, -1 };
    
  const Matrix<Integer> rays = common::primitive(rational_rays);    
  if (rank(rays - repeat_row(rays.row(0),rays.rows())) == dim)
    return gorenstein;
    
  SmithNormalForm<Integer> SNF = smith_normal_form(rays);
    
  for (Int i = 0; i < SNF.rank; ++i) { // adjust orientation of the transformation
      if (SNF.form(i,i) < 0) {           // all entries of M >= 0
        SNF.form(i,i).negate();
        SNF.left_companion.col(i).negate();
      }
    }
    
    const Matrix<Integer> NV = (SNF.left_companion * SNF.form).minor(All, range(0, SNF.rank-1));    
    const Matrix<Integer> LNV = NV - repeat_row(NV.row(0),NV.rows());
    const Matrix<Rational> ns = null_space(Matrix<Rational>(LNV));
    const Vector<Integer> normal = common::primitive(ns.row(0));

    gorenstein.first = true;
    gorenstein.second = Int(abs(NV.row(0)*normal));
    return gorenstein;
  }

  Function4perl( &q_gorenstein_cone, "q_gorenstein_cone(Matrix, $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
