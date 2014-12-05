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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope  {

template <typename Scalar>
perl::Object normal_cone(perl::Object p, int v, bool outer)
{
   perl::Object c(perl::ObjectType::construct<Scalar>("Cone")); 
   const Set<int> far_face = p.give("FAR_FACE");
   if (far_face.contains(v))
      throw std::runtime_error("normal_cone: vertex is contained in the far face");
   const IncidenceMatrix<> vfi=p.give("FACETS_THRU_VERTICES");
   const Matrix<Scalar> m=p.give("FACETS");
   Matrix<Scalar> mm=m.minor(vfi.row(v),~scalar2set(0));
   if (outer) mm=-mm; // reverse orientation for outer normal cone!
   c.take("RAYS")<<mm; 
   const Matrix<Scalar> ls=p.give("AFFINE_HULL");
   c.take("LINEALITY_SPACE") << ls.minor(All,~scalar2set(0));
   const int dim=mm.cols();
   c.take("CONE_AMBIENT_DIM")<<dim;
   return c;
}

UserFunctionTemplate4perl("# @category Producing a cone"
                          "# Computes the normal cone of //p// at the vertex //v//."
                          "# By default this is the inner normal cone."
                          "# @param Polytope p"
                          "# @param Int v vertex number which is not contained in the far face"
                          "# @param Bool outer asks for outer normal cone.  Default value is 0 (= inner)"
                          "# @return Cone",
                          "normal_cone<Scalar>(polytope::Polytope<Scalar> $; $=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
