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

#ifndef POLYMAKE_POLYTOPE_TRANSFORM_H
#define POLYMAKE_POLYTOPE_TRANSFORM_H

#include "polymake/client.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

/** Transform a coordinate section by applying a given transformation matrix.
    @param p_out destination file
    @param p_in source file
    @param section_req read request for the section to be transformed
    @param tau transformation matrix
*/
template <typename TransMatrix> inline
void transform_section(perl::Object& p_out, perl::Object& p_in,
                       const AnyString& section_req,
                       const GenericMatrix<TransMatrix>& tau)
{
   Matrix<typename TransMatrix::element_type> M;
   std::string name;
   if (p_in.lookup_with_property_name(section_req, name, allow_conversion()) >> M) {
      if (M.rows())
         p_out.take(name) << M*tau;
      else
         p_out.take(name) << M;
   }
}

/** Transform all coordinate sections according to the given transformation matrix.
    @param p object to transform
    @param tau transformation matrix for points
*/
template <typename Scalar, typename TransMatrix>
perl::Object transform(perl::Object p_in, const GenericMatrix<TransMatrix>& tau,
                       bool store_reverse_transformation=true)
{
   const auto tau_inv=inv(tau);

   perl::Object p_out("Polytope", mlist<Scalar>());

   transform_section(p_out, p_in, "VERTICES | POINTS", tau);
   transform_section(p_out, p_in, "LINEALITY_SPACE | INPUT_LINEALITY", tau);
   transform_section(p_out, p_in, "ZONOTOPE_INPUT_POINTS", tau);
   transform_section(p_out, p_in, "FACETS | INEQUALITIES", T(tau_inv));
   transform_section(p_out, p_in, "AFFINE_HULL | EQUATIONS", T(tau_inv));
   /* FIXME
      transform_section(p_out, p_in, "LINEAR_OBJECTIVE", T(tau_inv));
   */
   IncidenceMatrix<> VIF;
   if (p_in.lookup("VERTICES_IN_FACETS") >> VIF)
      p_out.take("VERTICES_IN_FACETS") << VIF;

   Array<std::string> labels;
   if (p_in.lookup("VERTEX_LABELS") >> labels) p_out.take("VERTEX_LABELS") << labels;
   if (p_in.lookup("FACET_LABELS") >> labels) p_out.take("FACET_LABELS") << labels;

   if (store_reverse_transformation) {
      Matrix<typename TransMatrix::element_type> tau_rev;
      if (p_in.get_attachment("REVERSE_TRANSFORMATION") >> tau_rev)
         tau_rev= tau_inv * tau_rev;
      else
         tau_rev= tau_inv;
      p_out.attach("REVERSE_TRANSFORMATION") << tau_rev;
   }
   return p_out;
}

} } // end namespace polymake

#endif // POLYMAKE_POLYTOPE_TRANSFORM_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
