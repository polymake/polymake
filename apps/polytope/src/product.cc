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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {
namespace {

struct product_label {
   typedef std::string first_argument_type;
   typedef std::string second_argument_type;
   typedef std::string result_type;

   result_type operator() (const first_argument_type& l1, const second_argument_type& l2) const
   {
      return l1 + '*' + l2;
   }
};

template<typename Scalar>
Matrix<Scalar>
product_coord(const Matrix<Scalar>& V1, const Matrix<Scalar>& V2,
              int& n_vertices1, int& n_vertices2, int& n_vertices_out, int n_rays,
              const Set<int>& rays1, const Set<int>& rays2)
{
   if (!n_vertices_out)
      n_vertices1=V1.rows(), n_vertices2=V2.rows(),
         n_vertices_out=(n_vertices1 - rays1.size()) * (n_vertices2 - rays2.size()) + n_rays;
   const int dim=V1.cols()+V2.cols()-1;
  
   Matrix<Scalar> V_out(n_vertices_out, dim);
   if (n_rays==0) {
      copy(entire(product(rows(V1), rows(V2.minor(All,range(1,V2.cols()-1))), operations::concat())),
           rows(V_out).begin());
   } else {
      // affine vertices
      copy(entire(product(rows(V1.minor(~rays1,All)), rows(V2.minor(~rays2,range(1,V2.cols()-1))), operations::concat())),
           rows(V_out).begin());
      // rays from P1
      V_out.minor(sequence(n_vertices_out-n_rays, rays1.size()), sequence(0,V1.cols())) =
         V1.minor(rays1,All);
      // rays from P2
      V_out.minor(sequence(n_vertices_out-rays2.size(), rays2.size()), sequence(V1.cols()-1, V2.cols())) =
         V2.minor(rays2,All);
   }
   return V_out;
}
  
} // end unnamed namespace

template<typename Scalar>
perl::Object product(perl::Object p_in1, perl::Object p_in2, perl::OptionSet options)
{
   int n_vertices1=0, n_vertices2=0, n_vertices_out=0, n_rays=0;

   const bool noc=options["no_coordinates"],
      relabel=options["relabel"];

   Set<int> rays1, rays2;
   if (!noc) {
      p_in1.give("FAR_FACE") >> rays1;
      p_in2.give("FAR_FACE") >> rays2;
      n_rays=rays1.size()+rays2.size();
   }

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Product of " << p_in1.name() << " and " << p_in2.name() << endl;

   if (noc || p_in1.exists("VERTICES_IN_FACETS") && p_in2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF1=p_in1.give("VERTICES_IN_FACETS"),
         VIF2=p_in2.give("VERTICES_IN_FACETS");
      n_vertices1=VIF1.cols();  n_vertices2=VIF2.cols();
      n_vertices_out= (n_vertices1 - rays1.size()) * (n_vertices2 - rays2.size()) + n_rays;

      Set<int> far_facet1=accumulate(cols(VIF1.minor(All,rays1)), operations::mul()),
         far_facet2=accumulate(cols(VIF2.minor(All,rays2)), operations::mul());
      if (far_facet1.size() > 1) far_facet1.clear();            // if size()>1 then FAR_FACE is not a facet
      if (far_facet2.size() > 1) far_facet2.clear();

      const bool has_far_facet= !far_facet1.empty() && !far_facet2.empty();
      const int n_facets1=VIF1.rows() - far_facet1.size(),
         n_facets2=VIF2.rows() - far_facet2.size(),
         n_facets_out=n_facets1 + n_facets2 + has_far_facet;
      IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);
      if (n_rays==0) {
         copy(entire(pm::product(cols(VIF1), cols(VIF2), operations::concat())),
              cols(VIF_out).begin());
      } else {
         // affine vertices come first
         copy(entire(pm::product(cols(VIF1.minor(~far_facet1,~rays1)), cols(VIF2.minor(~far_facet2,~rays2)),
                                 operations::concat())), cols(VIF_out).begin());
         // rays from P1 and P2
         VIF_out.minor(sequence(0, n_facets_out-has_far_facet), sequence(n_vertices_out-n_rays, n_rays)) =
            diag_1(VIF1.minor(~far_facet1,rays1), VIF2.minor(~far_facet2,rays2));
         if (has_far_facet)
            VIF_out[n_facets_out-1]=sequence(n_vertices_out-n_rays, n_rays);
      }
   
      p_out.take("N_VERTICES") << n_vertices_out;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (noc && p_in1.exists("COMBINATORIAL_DIM") && p_in2.exists("COMBINATORIAL_DIM")) {
      const int dim1=p_in1.give("COMBINATORIAL_DIM"),
         dim2=p_in2.give("COMBINATORIAL_DIM");
      p_out.take("COMBINATORIAL_DIM") << dim1+dim2;
   }

   if (!noc) {

      const bool pointed=p_in1.give("POINTED") && p_in2.give("POINTED");
      if (!pointed)
         throw std::runtime_error("product: input polyhedron not pointed");

      std::string given1, given2;
      const Matrix<Scalar> V1=p_in1.give_with_property_name("VERTICES | POINTS", given1),
         V2=p_in2.give_with_property_name("VERTICES | POINTS", given2);
      const bool VERTICES_out= given1=="VERTICES" && given2=="VERTICES";

      const Matrix<Scalar> V_out=product_coord(V1, V2, n_vertices1, n_vertices2, n_vertices_out, n_rays, rays1, rays2);

      p_out.take(VERTICES_out ? "VERTICES" : "POINTS") << V_out;
      if ( VERTICES_out ) {
         const Matrix<Scalar> empty;
         p_out.take("LINEALITY_SPACE") << empty;
      }
   }

   if (relabel) {
      std::vector<std::string> labels1(n_vertices1), labels2(n_vertices2),
         labels_out(n_vertices_out);
      read_labels(p_in1, "VERTEX_LABELS", labels1);
      read_labels(p_in2, "VERTEX_LABELS", labels2);
      if (n_rays==0) {
         copy(entire(pm::product(labels1, labels2, product_label())), labels_out.begin());
      } else {
         std::vector<std::string>::iterator l=labels_out.begin();
         l=copy(entire(pm::product(select(labels1,~rays1), select(labels2,~rays2), product_label())), l);
         const std::string all("all");
         l=copy(entire(attach_operation(select(labels1,rays1), constant(all), product_label())), l);
         copy(entire(attach_operation(constant(all), select(labels2,rays2), product_label())), l);
      }
      p_out.take("VERTEX_LABELS") << labels_out;
   }
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polytope as the product of two given polytopes //P1// and //P2//."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @option Bool no_coordinates only combinatorial information is handled"
                          "# @option Bool relabel creates an additional section [[VERTEX_LABELS]];"
                          "#   the label of a new vertex corresponding to v<sub>1</sub> &oplus; v<sub>2</sub> will"
                          "#   have the form LABEL_1*LABEL_2."
                          "# @return Polytope"
                          "# @example The following builds the product of a square and an interval while relabeling,"
                          "# and then prints a nice representation of its vertices."
                          "# > $p = product(cube(2),cube(1),relabel=>1);"
                          "# > print labeled($p->VERTICES,$p->VERTEX_LABELS);"
                          "# | 0*0:1 -1 -1 -1 0*1:1 -1 -1 1 1*0:1 1 -1 -1 1*1:1 1 -1 1 2*0:1 -1 1 -1 2*1:1 -1 1 1 3*0:1 1 1 -1 3*1:1 1 1 1",
                          "product<Scalar>(Polytope<type_upgrade<Scalar>>, Polytope<type_upgrade<Scalar>>; { no_coordinates => 0, relabel => undef })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
