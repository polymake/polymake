/* Copyright (c) 1997-2020
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
#include "polymake/Rational.h"

namespace polymake { namespace polytope {

BigObject dwarfed_product_polygons(Int d, Int s)
{
   if (d <= 2 || d%2 != 0) {
      throw std::runtime_error("dwarfed_product_polygons: d >= 4 and even required");
   }
   if (s <= 2) {
      throw std::runtime_error("dwarfed_product_polygons: s >= 3 required");
   }

   Matrix<Int> F((d/2)*s+1,d+1);
   auto f = rows(F).begin();
   for (Int i = 1; i <= d/2; ++i) {
      (*f)[2*i]=1; //y_i >= 0
      ++f;
   }
   for (Int i = 1; i <= d/2; ++i) {
      (*f)[2*i-1] = s; //sx_i
      (*f)[2*i] = -1; //-y_i
      ++f;
   }

   for (Int k = 0; k <= s-4; ++k) {
      for (Int i = 1; i <= d/2; ++i) {
         (*f)[2*i-1] = -2*k-1; //-(2k+1)x_i
         (*f)[2*i] = -1; //-y_i
         (*f)[0] = (2*k+1)*(s+k)-k*k+s*s;
         ++f;
      }
   }

   for (Int i = 1; i <= d/2; ++i) {
      (*f)[2*i-1] = -2*s+3; //-(2*s-3)x_i
      (*f)[2*i] = -1; //-y_i
      (*f)[0] = 2*s*(2*s-3);
      ++f;
   }
    
   // dwarfing halfspace
   for (Int i = 1; i <= d/2; ++i) {
      (*f)[2*i-1] = -1;
   }
   (*f)[0] = 2*s-1;

   BigObject p("Polytope<Rational>",
               "CONE_AMBIENT_DIM", d+1,
               "CONE_DIM", d+1,
               "FACETS", F,
               "BOUNDED", true,
               "POSITIVE", true);
   p.set_description() << "dwarfed product of polygons of dimension " << d << " and size " << s << endl;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional dwarfed product of polygons of size //s//."
                  "# @param Int d the dimension"
                  "# @param Int s the size"
                  "# @return Polytope"
                  "# @author Thilo Rörig",
                  &dwarfed_product_polygons, "dwarfed_product_polygons($$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
