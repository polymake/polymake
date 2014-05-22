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
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

template <typename Coord>
perl::Object normal_fan(perl::Object p)
{
   perl::ObjectType t=perl::ObjectType::construct<Coord>("PolyhedralFan");
   perl::Object f(t); 
   Matrix<Coord> m=p.give("FACETS");
   IncidenceMatrix<> vfi=p.give("FACETS_THRU_VERTICES");
   const Matrix<Coord> ls=p.give("AFFINE_HULL");
   const bool com=p.give("BOUNDED");
   const int dim=p.CallPolymakeMethod("AMBIENT_DIM");

   int ff = -1;
   if ( !com ) {   // we have to check whether the far face is a facet
      const Vector<Coord> far_facet(unit_vector<Coord>(dim+1,0));
      for (int i = 0; i < m.rows(); ++i) 
         if ( m.row(i) == far_facet ) {
            ff = i;
            break;
         }
   }

   if ( ff != -1 ) {      
      m=m.minor(~scalar2set(ff),~scalar2set(0));
      Set<int> bounded_verts = p.CallPolymakeMethod("BOUNDED_VERTICES");
      vfi = vfi.minor(bounded_verts,All);
      vfi.squeeze();
   } else {
      m=m.minor(All,~scalar2set(0));
   }

   f.take("RAYS") << m;
   f.take("MAXIMAL_CONES") << vfi;
   f.take("REGULAR") << true;

   f.take("LINEALITY_SPACE") << ls.minor(All, ~scalar2set(0));

   f.take("COMPLETE") << com;
   f.take("FAN_DIM") << dim;

   return f;
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes the normal fan of //p//."
                          "# @param Polytope p"
                          "# @tparam Coord"
                          "# @return PolyhedralFan",
                          "normal_fan<Coord>(polytope::Polytope<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
