/* Copyright (c) 1997-2023
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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {
namespace {

struct product_label {
   typedef std::string first_argument_type;
   typedef std::string second_argument_type;
   typedef std::string result_type;

   result_type operator() (const first_argument_type& v, const second_argument_type& f) const
   {
      return 'v' + v + '*' + 'F' + f;
   }
};
}

template <typename Scalar>
BigObject mapping_polytope(BigObject p_in1, BigObject p_in2, OptionSet options)
{
   const bool bounded=p_in1.give("BOUNDED") && p_in2.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("mapping_polytope: input polyhedra must be bounded");
   const Int p = p_in1.give("CONE_DIM"),
             q = p_in2.give("CONE_DIM"),
            p2 = p_in1.give("CONE_AMBIENT_DIM"),
            q2 = p_in2.give("CONE_AMBIENT_DIM");
   if (p != p2 || q != q2)
      throw std::runtime_error("mapping_polytope: input polytopes must be full-dimensional");

   const Matrix<Scalar> V = p_in1.give("VERTICES"),
                        H = p_in2.give("FACETS");
   const Int n = V.rows(), m = H.rows();

   BigObjectType t=p_in1.type();
   // type check versus other polytope implicitly done by server
   BigObject p_out(t);
   p_out.set_description() << "Mapping polytope of " << p_in1.name() << " and " << p_in2.name() << endl;

   Matrix<Scalar> H_out(m*n,(q-1)*p+1);

   Int i = 0, j = 0, l = 0;
   for (auto vi = entire(rows(V)); !vi.at_end(); ++vi, ++i) {
      j = 0;
      for (auto fi = entire(rows(H)); !fi.at_end(); ++fi, ++j) {
         H_out(l,0) = (*fi)[0];
         for (Int k = 0; k < q-1; ++k) {
            H_out[l].slice(sequence(k*p+1,p)) = (*fi)[k+1]*(*vi);
         }
         ++l;
      }
   }
   p_out.take("FACETS") << H_out;
   p_out.take("AFFINE_HULL") << Matrix<Scalar>(0, H_out.cols());
   p_out.take("FEASIBLE") << true;
   p_out.take("BOUNDED") << true;

   const bool relabel=!options["no_labels"];
   if (relabel) {
      const std::vector<std::string> vertex_labels = common::read_labels(p_in1, "VERTEX_LABELS", n);
      const std::vector<std::string> facet_labels = common::read_labels(p_in2, "FACET_LABELS", m);

      std::vector<std::string> labels_out(m*n);
      copy_range(entire(pm::product(vertex_labels, facet_labels, product_label())), labels_out.begin());
      p_out.take("FACET_LABELS") << labels_out;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polytope as the __mapping polytope__ of two polytopes //P1// and //P2//."
                          "# The mapping polytope is the set of all affine maps from R<sup>p</sup> to R<sup>q</sup>, that map //P1// into //P2//."
                          "# Mapping polytopes are also called Hom-polytopes; cf. Bogart, Contois & Gubeladze, doi:10.1007/s00209-012-1053-5."
                          "# "
                          "# The label of a new facet corresponding to v<sub>1</sub> and h<sub>1</sub> will have the form"
                          "# \"v<sub>1</sub>*h<sub>1</sub>\"."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @option Bool no_labels Do not assign [[FACET_LABELS]]. default: 0"
                          "# @return Polytope"
                          "# @example [prefer cdd] [require bundled:cdd] Let us look at the mapping polytope of the unit interval and the standard unimodular triangle."
                          "# > $I=simplex(1); $T=simplex(2); $Hom_IT=mapping_polytope($I,$T);"
                          "# The dimension equals (dim I + 1) * dim T."
                          "# > print $Hom_IT->DIM"
                          "# | 4"
                          "# > print rows_labeled($Hom_IT->FACETS,$Hom_IT->FACET_LABELS);"
                          "# | v0*F0:1 -1 0 -1 0"
                          "# | v0*F1:0 1 0 0 0"
                          "# | v0*F2:0 0 0 1 0"
                          "# | v1*F0:1 -1 -1 -1 -1"
                          "# | v1*F1:0 1 1 0 0"
                          "# | v1*F2:0 0 0 1 1"
                          "# > print $Hom_IT->N_VERTICES;"
                          "# | 9"
                          "# This is how to turn, e.g., the first vertex into a linear map."
                          "# > $v=$Hom_IT->VERTICES->[0];"
                          "# > $M=new Matrix([$v->slice([1..2]),$v->slice([3..4])]);"
                          "# > print $I->VERTICES * transpose($M);"
                          "# | 0 0"
                          "# | 0 1"
                          "# The above are coordinates in R^2, without the homogenization commonly used in polymake."
                          ,
                          "mapping_polytope<Scalar> (Polytope<Scalar> Polytope<Scalar> { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
