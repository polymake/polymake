/* Copyright (c) 1997-2018
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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope  {

template <typename Scalar>
perl::Object normal_cone_impl(perl::Object p,
                              const Set<int>& F,
                              const std::string& ftv_section,
                              const std::string& facets_section,
                              perl::OptionSet options)
{
   if (p.isa("Polytope")) {
      const Set<int> far_face = p.give("FAR_FACE");
      if (incl(F, far_face) <= 0)
         throw std::runtime_error("normal_cone: face is contained in the far face");
   }
   const bool outer = options["outer"];
   const IncidenceMatrix<> ftv = p.give(ftv_section);
   const Matrix<Scalar> facet_normals = p.give(facets_section);
   Matrix<Scalar> cone_normals (facet_normals.minor(accumulate(rows(ftv.minor(F,All)), operations::mul()), ~scalar2set(0)));
   if (outer) cone_normals = -cone_normals;
   perl::Object c(perl::ObjectType::construct<Scalar>("Cone")); 
   c.take("INPUT_RAYS") << cone_normals; 
   const Matrix<Scalar> ls = p.give("LINEAR_SPAN");
   c.take("INPUT_LINEALITY") << ls.minor(All, ~scalar2set(0));
   c.take("CONE_AMBIENT_DIM") << cone_normals.cols();
   return c;
}

FunctionTemplate4perl("normal_cone_impl<Scalar>($$$$$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
