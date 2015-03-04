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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object transportation(const Vector<Scalar>& r, const Vector<Scalar>& c)
{
   const int m = r.size();
   const int n = c.size();
  
   if (m*n == 0) 
      throw std::runtime_error("transportation polytope: r and c must have nonzero length");
     
   if (ones_vector<Scalar>(m)*r != ones_vector<Scalar>(n)*c)
      throw std::runtime_error("transportation polytope: sum of entries of r and c must be equal");

   for (typename Entire<Vector<Scalar> >::const_iterator x = entire(r); !x.at_end(); ++x)
      if (*x < 0)
         throw std::runtime_error("transportation polytope: r and c must have nonnegative entries");
   
   for (typename Entire<Vector<Scalar> >::const_iterator x = entire(c); !x.at_end(); ++x)
      if (*x < 0)
         throw std::runtime_error("transportation polytope: r and c must have nonnegative entries");

   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   p.set_description() << "transportation polytope for r=(" << r << ") and c=(" << c << ")" << endl;

   p.take("CONE_AMBIENT_DIM") << (m*n+1);

   Matrix<Scalar> ineq(m*n,m*n+1);
   ineq.minor(range(0,m*n-1),range(1,m*n)).diagonal().fill(1);
   Matrix<Scalar> eq(m+n,m*n+1);
   for(int i = 0; i < m; i++) {
      eq.minor(range(i,i),range(i*n+1,(i+1)*n)).fill(1);
      eq.minor(range(m,m+n-1),range(i*n+1,(i+1)*n)).diagonal().fill(1);
   }
   eq.col(0) = (-1) * (r | c);

   p.take("INEQUALITIES") << ineq;
   p.take("EQUATIONS") << eq;
   p.take("BOUNDED") << true;
   p.take("POSITIVE") << true;
   p.take("FEASIBLE") << true;

   return p;
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce the transportation polytope from two vectors //r// of length m and //c// of length n,"
                          "# i.e. all positive m&times;n Matrizes with row sums equal to //r// and column sums equal to //c//."
                          "# @param Vector r"
                          "# @param Vector c"
                          "# @return Polytope",
                          "transportation<Scalar>(Vector<Scalar>, Vector<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
