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
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

Matrix<Rational> max_metric(const int n)
{
   if (n<2)
      throw std::runtime_error("max_metric: n >= 2 required");

   Matrix<Rational> d(n,n);

   for (int i=1; i<=n; ++i)
      for (int j=i+1; j<=n; ++j)
         d(i-1,j-1) = d(j-1,i-1) = 1 + Rational(1,n*n+i*n+j);

  return d;
}


perl::Object ts_max_metric(const int n)
{
   perl::Object p("TightSpan");
   p.take("METRIC") << max_metric(n);
   p.take("ESSENTIALLY_GENERIC")<<true;
   return p;
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Computes the tight span of a metric such that its f-vector is maximal among all metrics with //n// points."
                  "#\t S. Herrmann and M. Joswig: Bounds on the f-vectors of tight spans."
                  "#\t Contrib. Discrete Math., Vol.2, 2007 161-184"
                  "# @param Int n the number of points"
                  "# @return TightSpan",
                  &ts_max_metric, "ts_max_metric");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Compute a metric such that the f-vector of its tight span is maximal among all metrics with //n// points."
                  "#\t S. Herrmann and M. Joswig: Bounds on the f-vectors of tight spans."
                  "#\t Contrib. Discrete Math., Vol.2, 2007 161-184"
                  "# @param Int n the number of points"
                  "# @return Matrix",
                  &max_metric, "max_metric");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
