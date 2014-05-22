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
#include "polymake/Array.h"

namespace polymake { namespace polytope {

perl::Object hypertruncated_cube(const int d, const Rational k, const Rational lambda)
{
   if (d < 2)
      throw std::runtime_error("hypertruncated_cube: dimension d >= 2 required");
   if (k <= 1 || k >=d)
      throw std::runtime_error("hypertruncated_cube: 1 < k < d required");
   if (lambda*d <= k)
      throw std::runtime_error("hypertruncated_cube: lambda > k/d required");

   perl::Object p("Polytope<Rational>");
   p.set_description() << "hypertruncated_cube(" << d << "," << k << "," << lambda << ")" << endl;

   const int n_ineqs=4*d;
   Matrix<Rational> Inequalities(n_ineqs,d+1);
   int i=0;
  
   // facets through origin (= non-negativity constraints)
   for (int j=1; j<=d; ++j, ++i)
      Inequalities(i,j)=1;
  
   // opposite cube facets
   for (int j=1; j<=d; ++j, ++i) {
      Inequalities(i,0)=1; Inequalities(i,j)=-1;
   }
  
   // deletion facets through lambda(1,1,...,1)
   for (int j=1; j<=d; ++j, ++i) {
      Inequalities(i,0)=k;
      for (int jj=1; jj<j; ++jj)
         Inequalities(i,jj)=-1;
      Inequalities(i,j)=d-1-k/lambda;
      for (int jj=j+1; jj<=d; ++jj)
         Inequalities(i,jj)=-1;
   }
  
   // contraction facets through lambda(1,1,...,1)
   for (int j=1; j<=d; ++j, ++i) {
      Inequalities(i,0)=lambda*(d-k);
      for (int jj=1; jj<j; ++jj)
         Inequalities(i,jj)=lambda-1;
      Inequalities(i,j)=k-1-lambda*(d-1);
      for (int jj=j+1; jj<=d; ++jj)
         Inequalities(i,jj)=lambda-1;
   }
  
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("INEQUALITIES") << Inequalities;
   p.take("N_INEQUALITIES") << n_ineqs+1; //+1 for 1 0 0 0  ...
   p.take("BOUNDED") << true;

   // symmetric linear objective function
   perl::Object LP("LinearProgram<Rational>");
   LP.take("LINEAR_OBJECTIVE") << Vector<Rational>(0|ones_vector<Rational>(d));
   LP.attach("INTEGER_VARIABLES") << Array<bool>(d,true);
   p.take("LP") << LP;
  
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional hypertruncated cube."
                  "# With symmetric linear objective function (0,1,1,...,1)."
                  "# "
                  "# @param Int d the dimension"
                  "# @param Rational k cutoff parameter"
                  "# @param Rational lambda scaling of extra vertex"
                  "# @return Polytope",
                  &hypertruncated_cube, "hypertruncated_cube");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
