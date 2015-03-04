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

Matrix<Rational> thrackle_metric(const int n)
{
   Matrix<Rational> d(n,n);

   if (n<2) throw std::runtime_error("n >= 2 required");
  
   for (int i=1; i<=n; ++i)
      for (int j=i+1; j<=n; ++j)
         d(i-1,j-1) = d(j-1,i-1) = (j-i)*(n-(j-i));

   return d;
}

perl::Object ts_thrackle_metric(const int n)
{
   perl::Object p("TightSpan");
   p.take("METRIC") << thrackle_metric(n);
   p.take("ESSENTIALLY_GENERIC")<<true;
   return p;
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Compute a metric such that the f-vector of its tight span is maximal among all metrics with //n// points."
                  "# This metric can be interpreted as a lifting function for the thrackle triangulation (see de Loera,"
                  "# Sturmfels and Thomas: Groebner Basis and triangultaions of the second hypersimplex)"
                  "# @param Int n the number of points"
                  "# @return Matrix",
                  &thrackle_metric,"thrackle_metric");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Compute a tight span of a metric such that its f-vector is maximal among all metrics with //n// points."
                  "# This metric can be interpreted as a lifting function for the thrackle triangulation (see de Loera,"
                  "# Sturmfels and Thomas: Groebner Basis and triangultaions of the second hypersimplex)"
                  "# @param Int n the number of points"
                  "# @return TightSpan",
                  &ts_thrackle_metric,"ts_thrackle_metric");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
