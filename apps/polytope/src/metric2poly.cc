/* Copyright (c) 1997-2015
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
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"

namespace polymake { namespace polytope {

template <typename Scalar>
ListMatrix< Vector<Scalar> >  metric2poly(const Matrix<Scalar> &dist)
{
  const int d(dist.cols());
  
  ListMatrix< Vector<Scalar> > I = (zero_vector<Scalar>(d) | unit_matrix<Scalar>(d)); // non-negativity constraints
  for (int i=0; i<d; ++i)
    for (int j=i+1; j<d; ++j) {
      Vector<Scalar> ineq(d+1);
      ineq[0]=-dist(i,j); ineq[i+1]=ineq[j+1]=1;
      I /= ineq;
    }
  return I;
}

FunctionTemplate4perl("metric2poly(Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
