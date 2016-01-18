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
#include "polymake/list"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

Matrix<Rational> min_metric(const int n)
{
   if (n<2)
      throw std::runtime_error("min_metric: n >= 2 required");

   Matrix<Rational> d(n,n);

   for (int i=1; i<=n; ++i)
      for (int j=i+1; j<=n; ++j)
         switch (n%3) {
         case 0:
         case 1:
            d(i-1,j-1) = d(j-1,i-1) = (i-1)/3==(j-1)/3 ? Rational(2) : 1 + Rational(1,n*n+i*n+j);
            break;
         case 2:
            d(i-1,j-1) = d(j-1,i-1) = ((i-1)/3==(j-1)/3) && (i<n) && (j<n) ? Rational(2) : 1 + Rational(1,n*n+i*n+j);
         }

   return d;
}

perl::Object ts_min_metric(const int n)
{
   perl::Object p("TightSpan");
   p.take("METRIC") << min_metric(n);
   p.take("ESSENTIALLY_GENERIC")<<true;
   return p;
}

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute a metric such that the f-vector of its tight span is minimal among all metrics with //n// points."
                  "#\t S. Herrmann and M. Joswig: Bounds on the f-vectors of tight spans."
                  "#\t Contrib. Discrete Math., Vol.2, 2007 161-184"
                  "# @param Int n the number of points"
                  "# @return Matrix"
                  "# @example To compute the min-metric of four points and display the f-vector of its tight span, do this:"
                  "# > $M = min_metric(5);"
                  "# > $w = new Vector(1,1,1,2,3);"
                  "# > print tight_span($M,$w)->F_VECTOR;"
                  "# | 6 15 20 15 6",
                  &min_metric, "min_metric");

UserFunction4perl("# @category Finite metric spaces"
                  "# Compute the tight span of a metric such its f-vector is minimal among all metrics with //n// points."
                  "#\t S. Herrmann and M. Joswig: Bounds on the f-vectors of tight spans."
                  "#\t Contrib. Discrete Math., Vol.2, 2007 161-184"
                  "# @param Int n the number of points"
                  "# @return TightSpan",
                  &ts_min_metric, "ts_min_metric");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
