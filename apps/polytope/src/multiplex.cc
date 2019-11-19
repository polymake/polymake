/* Copyright (c) 1997-2019
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
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {
namespace {
inline
int index (int i, int n) { return i < 0 ? 0 : i > n ? n : i; }
}

perl::Object multiplex(int d, int n)
{
   if (d < 2 || d > n)
      throw std::runtime_error("multiplex: 2 <= d <= n required");

   // FIXME: CombinatorialPolytope
   perl::Object p("Polytope<Rational>");
   IncidenceMatrix<> VIF(n+1,n+1);
   for (int j=0; j < n+1; ++j) 
      for (int k=1; k < d; ++k) 
         VIF(j,index(j+k,n)) = VIF(j,index(j-k,n)) = true;

   p.take("VERTICES_IN_FACETS") << VIF;
   p.take("COMBINATORIAL_DIM") << d;
   p.take("N_VERTICES") << n+1;
   p.take("N_FACETS") << n+1;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a combinatorial description of a multiplex with parameters //d// and //n//."
                  "# This yields a self-dual //d//-dimensional polytope with //n//+1 vertices."
                  "# "
                  "# They are introduced by"
                  "#\t T. Bisztriczky,"
                  "#\t On a class of generalized simplices, Mathematika 43:27-285, 1996,"
                  "# see also"
                  "#\t M.M. Bayer, A.M. Bruening, and J.D. Stewart,"
                  "#\t A combinatorial study of multiplexes and ordinary polytopes,"
                  "#\t Discrete Comput. Geom. 27(1):49--63, 2002."
                  "# @param Int d the dimension"
                  "# @param Int n"
                  "# @return Polytope"
                  "# @author Alexander Schwartz",
                  &multiplex, "multiplex");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
