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

namespace polymake { namespace polytope {

perl::Object dwarfed_cube(int d)
{
   if (d < 2) {
      throw std::runtime_error("dwarfed_cube: d >= 2 required");
   }
   perl::Object p("Polytope<Rational>");
   p.set_description() << "dwarfed cube of dimension " << d << endl;

   Matrix<Rational> F(2*d+1,d+1);
   Rows<Matrix <Rational> >::iterator f=rows(F).begin();
   for (int i=1; i<=d; ++i) {
      //         (*f)[0]=0;
      (*f)[i]=1;
      ++f;
      (*f)[0]=1;
      (*f)[i]=-1;
      ++f;
   }
      
   (*f).fill(-1);
   (*f)[0]=Rational(3,2);

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("FACETS") << F;
   p.take("LINEAR_SPAN") << Matrix<Rational>();
   p.take("BOUNDED") << true;
   p.take("POSITIVE") << true;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional dwarfed cube."
                  "# @param Int d the dimension"
                  "# @return Polytope"
                  "# @author Thilo Rörig",
                  &dwarfed_cube, "dwarfed_cube($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
