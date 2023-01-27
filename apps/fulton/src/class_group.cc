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
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/integer_linalg.h"

namespace polymake { namespace fulton {

// rational_divisor_class_group provides a pair of matrices: 
// a projection map to the class group, and a lifting map to a deivisor on the fan
// 
// let A be the matrix with rows the primitive generators of the fan
// the projection equals Gale transform of A, 
// i.e. a matrix G such that the cols g of G span the kernel of A in the form g^tA=0
// this implies that if divisors x,y 
// (i.e. right hand sides for the polytope defined by the normal fan spanned by A)
// are mapped to the same element:
// if x^tG=y^tG, then there is p such that x-y=Ap, i.e. x and y differ by a linear map
// (or, seen as right hand side of a polytope, they differ by a translation)
// 
// for the converse direction we write G as D=LGR for unimodular matrices L,R (Smith normal form)
// by construction, D is diagonal (as far as possible), and there are only ones on the diagonal
// we can then lift with R*L.minor(range(0,rk(D)-1),All)

std::pair<Matrix<Integer>, Matrix<Integer>> rational_divisor_class_group(BigObject fan)
{
  Matrix<Rational> rational_rays = fan.give("RAYS");
  Matrix<Integer> projection;
  Matrix<Integer> t_int_rays = T(common::primitive(rational_rays));

  // here we call the perl function "integer_kernel" defined in the 
  // lll-extension (http://polymake.org/polytopes/paffenholz/data/polymake/extensions/lll). 
  // this should be replaced by a c++-call 
  // once we can detect the ntl extension (see ticket #504), or ntl is a core extension
  //
  // Rem: pm now has integer_linalg.
  // CallPolymakeFunction("integer_kernel",t_int_rays,false) >> projection;
  projection = null_space_integer(t_int_rays);

  SmithNormalForm<Integer> SNF = smith_normal_form(T(projection));

  // the matrix D has only +1,-1,0 on the diagonal, but we need to know the positions of the -1's
  // hence, we can't do the following, but really need to multiply (or find them otherwise)
  //Matrix<Integer> lifting(R*L.minor(range(0,r-1),All));
  projection = T(T(projection) * inv(SNF.right_companion));
  Matrix<Integer> lifting(T(SNF.form) * inv(SNF.left_companion)); 

  std::pair<Matrix<Integer>, Matrix<Integer> > class_group_data;
  class_group_data.first = T(projection);
  class_group_data.second = lifting;

  return class_group_data;
}

Function4perl(&rational_divisor_class_group, "rational_divisor_class_group($)");

} }
