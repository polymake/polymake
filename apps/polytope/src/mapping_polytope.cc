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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

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
perl::Object mapping_polytope(perl::Object p_in1, perl::Object p_in2, perl::OptionSet options)
{
   const bool bounded=p_in1.give("BOUNDED") && p_in2.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("mapping_polytope: input polyhedra must be bounded");
   const int p=p_in1.give("CONE_DIM"),
      q=p_in2.give("CONE_DIM"),
      p2=p_in1.give("CONE_AMBIENT_DIM"),
      q2=p_in2.give("CONE_AMBIENT_DIM");
   if (p != p2 || q != q2)
      throw std::runtime_error("mapping_polytope: input polytopes must be full-dimensional");

   const Matrix<Scalar> V=p_in1.give("VERTICES"),
      H=p_in2.give("FACETS");
   const int n=V.rows(), m=H.rows();

   perl::ObjectType t=p_in1.type();
   // type check versus other polytope implicitly done by server
   perl::Object p_out(t);
   p_out.set_description() << "Mapping polytope of " << p_in1.name() << " and " << p_in2.name() << endl;

   Matrix<Scalar> H_out(m*n,(q-1)*p+1);

   int i = 0, j = 0, l = 0;
   for (typename Entire< Rows< Matrix<Scalar> > >::const_iterator vi = entire(rows(V)); !vi.at_end(); ++vi, ++i) {
      j = 0;
      for (typename Entire< Rows< Matrix<Scalar> > >::const_iterator fi = entire(rows(H)); !fi.at_end(); ++fi, ++j) {
         H_out(l,0) = (*fi)[0];
         for (int k = 0; k < q-1; ++k) {
            H_out[l].slice(sequence(k*p+1,p)) = (*fi)[k+1]*(*vi);
         }
         ++l;
      }
   }
   p_out.take("FACETS") << H_out;
   p_out.take("AFFINE_HULL") << Matrix<Scalar>();
   p_out.take("FEASIBLE") << true;
   p_out.take("BOUNDED") << true;

   const bool relabel=options["relabel"];
   if (relabel) {
      std::vector<std::string> vertex_labels(n), facet_labels(m), labels_out(m*n);
      read_labels(p_in1, "VERTEX_LABELS", vertex_labels);
      read_labels(p_in2, "FACET_LABELS", facet_labels);

      copy(entire(pm::product(vertex_labels, facet_labels, product_label())), labels_out.begin());
      p_out.take("FACET_LABELS") << labels_out;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                  "# Construct a new polytope as the __mapping polytope__ of two polytopes //P1// and //P2//."
                  "# The mapping polytope is the set of all affine maps from R<sup>p</sup> to R<sup>q</sup>, that map //P1// into //P2//."
                  "# "
                  "# The label of a new facet corresponding to v<sub>1</sub> and h<sub>1</sub> will have the form"
                  "# \"v<sub>1</sub>*h<sub>1</sub>\"."
                  "# @param Polytope P1"
                  "# @param Polytope P2"
                  "# @option Bool relabel"
                  "# @return Polytope",
                  "mapping_polytope<Scalar> (Polytope<Scalar> Polytope<Scalar> { relabel => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
