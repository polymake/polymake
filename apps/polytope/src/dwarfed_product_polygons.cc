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
#include "polymake/Rational.h"

namespace polymake { namespace polytope {

perl::Object dwarfed_product_polygons(int d, int s)
{
   if (d <= 2 || d%2 != 0) {
      throw std::runtime_error("dwarfed_product_polygons: d >= 4 and even required");
   }
   if (s <= 2) {
      throw std::runtime_error("dwarfed_product_polygons: s >= 3 required");
   }

   perl::Object p("Polytope<Rational>");
   p.set_description() << "dwarfed product of polygons of dimension " << d << " and size " << s << endl;

   Matrix<int> F((d/2)*s+1,d+1);
   Rows<Matrix <int> >::iterator f=rows(F).begin();
   int i;
   for (i = 1; i<=d/2; ++i) {
      (*f)[2*i]=1; //y_i >= 0
      ++f;
   }
   for (i = 1; i<=d/2; ++i) {
      (*f)[2*i-1] = s; //sx_i
      (*f)[2*i] = -1; //-y_i
      ++f;
   }

   for (int k = 0; k <= s-4; k++) {
      for (i = 1; i<=d/2; ++i) {
         (*f)[2*i-1] = -2*k-1; //-(2k+1)x_i
         (*f)[2*i] = -1; //-y_i
         (*f)[0] = (2*k+1)*(s+k)-k*k+s*s;
         ++f;
      }
   }

   for (i = 1; i<=d/2; ++i) {
      (*f)[2*i-1] = -2*s+3; //-(2*s-3)x_i
      (*f)[2*i] = -1; //-y_i
      (*f)[0] = 2*s*(2*s-3);
      ++f;
   }
    
   //dwarfing halfspace
   for (i=1;i<=d/2;++i) {
      (*f)[2*i-1] = -1;
   }
   (*f)[0] = 2*s-1;
    
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("FACETS") << F;
   p.take("LINEAR_SPAN") << Matrix<Rational>();
   p.take("BOUNDED") << true;
   p.take("POSITIVE") << true;

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
