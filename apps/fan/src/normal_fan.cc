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
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

template <typename Coord>
perl::Object normal_fan(perl::Object p)
{
   perl::ObjectType t=perl::ObjectType::construct<Coord>("PolyhedralFan");
   perl::Object f(t);

   const int dim=p.call_method("AMBIENT_DIM");
   const int p_dim=p.call_method("DIM");
   const int ldim=p.give("LINEALITY_DIM");

   if (!p.give("FEASIBLE")) {
      f.take("FAN_AMBIENT_DIM") << dim;
      f.take("FAN_DIM") << -1;
      f.take("RAYS") << Matrix<Coord>(0,dim);
      f.take("MAXIMAL_CONES") << IncidenceMatrix<>(0,0);
      f.take("REGULAR") << true;

      f.take("LINEALITY_SPACE") << Matrix<Coord>(0,dim);
      f.take("COMPLETE") << false;
      return f;
   }
   Matrix<Coord> m=p.give("FACETS");
   IncidenceMatrix<> vfi=p.give("FACETS_THRU_VERTICES");
   const Matrix<Coord> ls=p.give("AFFINE_HULL");
   const bool com=p.give("BOUNDED");

   struct id_collector {
      mutable Set<int> oldids;
      void operator() (const int& i, const int& j) const {
         oldids += i;
      }
   };

   // we remove the rows that correspond to far vertices
   // then we squeeze the cols to remove the far face
   // this also removes the facet inequality of polytopes with combinatorial dim zero
   if (!com || p_dim == 0) {
      Set<int> bounded_verts = p.call_method("BOUNDED_VERTICES");
      vfi = vfi.minor(bounded_verts,All);
      // we only squeeze the cols since sometimes we want to keep an empty set
      // as this might be needed for the {0} cone
      id_collector coll;
      vfi.squeeze_cols(coll);
      m=m.minor(coll.oldids, ~scalar2set(0));
   } else {
      m=m.minor(All,~scalar2set(0));
   }

   f.take("RAYS") << m;
   f.take("MAXIMAL_CONES") << vfi;
   f.take("REGULAR") << true;

   f.take("LINEALITY_SPACE") << ls.minor(All, ~scalar2set(0));

   f.take("COMPLETE") << com;
   f.take("FAN_DIM") << dim - ldim;
   f.take("FAN_AMBIENT_DIM") << dim;

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
