/* Copyright (c) 1997-2022
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
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

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

template <typename Scalar>
Matrix<Scalar>
product_vertices(const Matrix<Scalar>& V1, const Matrix<Scalar>& V2,
                 Int& n_vertices1, Int& n_vertices2, Int& n_vertices_out, Int n_rays,
                 const Set<Int>& rays1, const Set<Int>& rays2)
{
   if (!n_vertices_out)
      n_vertices1=V1.rows(), n_vertices2=V2.rows(),
         n_vertices_out=(n_vertices1 - rays1.size()) * (n_vertices2 - rays2.size()) + n_rays;
   const Int dim = V1.cols()+V2.cols()-1;

   Matrix<Scalar> V_out(n_vertices_out, dim);
   if (n_rays==0) {
      copy_range(entire(product(rows(V1), rows(V2.minor(All,range(1,V2.cols()-1))), operations::concat())),
                 rows(V_out).begin());
   } else {
      // affine vertices
      copy_range(entire(product(rows(V1.minor(~rays1,All)), rows(V2.minor(~rays2,range(1,V2.cols()-1))), operations::concat())),
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

template<typename Scalar>
Matrix<Scalar>
product_facets(const Matrix<Scalar>& F1, const Matrix<Scalar>& F2)
{
   return ( F1 | zero_matrix<Scalar>(F1.rows(), F2.cols()-1) ) /
      ( F2.col(0) | zero_matrix<Scalar>(F2.rows(), F1.cols()-1) | F2.minor(All, sequence(1,F2.cols()-1)) );
}

//creates an array of length n, permuting blocks of block_size using perm
Array<Int> permute_blocks(Int n, const Array<Int>& perm, Int block_size)
{
   Array<Int> out(n);
   for (Int i = 0; i < perm.size(); ++i)
      for (Int j = 0; j < block_size; ++j)
         out[i*block_size+j] = perm[i]*block_size + j;
   return out;
}

//creates an array of length n, permuting each of the blocks using perm
Array<Int> permute_inside_blocks(Int n, const Array<Int>& perm, Int n_blocks)
{
   Array<Int> out(n);
   const Int block_size = perm.size();
   for (Int i = 0; i < n_blocks; ++i)
      for (Int j = 0; j < block_size; ++j)
         out[i*block_size+j] = perm[j] + i*block_size;
   return out;
}

} // end unnamed namespace

template <typename Scalar>
BigObject product(BigObject p_in1, BigObject p_in2, OptionSet options)
{
   Int n_vertices1 = 0, n_vertices2 = 0, n_vertices_out = 0, n_rays = 0;

   const bool noc = options["no_coordinates"],
              nov = options["no_vertices"],
              nof = options["no_facets"],
          relabel = !options["no_labels"];

   bool f_present = p_in1.exists("FACETS | INEQUALITIES") && p_in2.exists("FACETS | INEQUALITIES");
   bool v_present = ( p_in1.exists("VERTICES | POINTS") && p_in2.exists("VERTICES | POINTS") ) ||
      ( !noc && !nov && ( nof || !f_present) ); //these are the conditions under which both VERTICES have to be computed (if not existent yet)

   Set<Int> rays1, rays2;
   if (v_present) {
      p_in1.give("FAR_FACE") >> rays1; //this requires presence of VERTICES in p1 and p2
      p_in2.give("FAR_FACE") >> rays2;
      n_rays=rays1.size()+rays2.size();
   }

   BigObject p_out("Polytope", mlist<Scalar>());
   p_out.set_description() << "Product of " << p_in1.name() << " and " << p_in2.name() << endl;

   if (noc || v_present &&  p_in1.exists("VERTICES_IN_FACETS") && p_in2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF1=p_in1.give("VERTICES_IN_FACETS"),
         VIF2=p_in2.give("VERTICES_IN_FACETS");
      n_vertices1=VIF1.cols();  n_vertices2=VIF2.cols();
      n_vertices_out= (n_vertices1 - rays1.size()) * (n_vertices2 - rays2.size()) + n_rays;

      Set<Int> far_facet1 = accumulate(cols(VIF1.minor(All,rays1)), operations::mul()),
               far_facet2 = accumulate(cols(VIF2.minor(All,rays2)), operations::mul());
      if (far_facet1.size() > 1) far_facet1.clear();            // if size()>1 then FAR_FACE is not a facet
      if (far_facet2.size() > 1) far_facet2.clear();

      const bool has_far_facet= !far_facet1.empty() && !far_facet2.empty();
      const Int n_facets1 = VIF1.rows() - far_facet1.size(),
         n_facets2 = VIF2.rows() - far_facet2.size(),
         n_facets_out = n_facets1 + n_facets2 + has_far_facet;
      IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);
      if (n_rays==0) {
         copy_range(entire(pm::product(cols(VIF1), cols(VIF2), operations::concat())),
                    cols(VIF_out).begin());
      } else {
         // affine vertices come first
         copy_range(entire(pm::product(cols(VIF1.minor(~far_facet1,~rays1)), cols(VIF2.minor(~far_facet2,~rays2)),
                                       operations::concat())),
                    cols(VIF_out).begin());
         // rays from P1 and P2
         VIF_out.minor(sequence(0, n_facets_out-has_far_facet), sequence(n_vertices_out-n_rays, n_rays)) =
            diag_1(VIF1.minor(~far_facet1,rays1), VIF2.minor(~far_facet2, rays2));
         if (has_far_facet)
            VIF_out[n_facets_out-1]=sequence(n_vertices_out-n_rays, n_rays);
      }

      p_out.take("N_VERTICES") << n_vertices_out;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (noc && p_in1.exists("COMBINATORIAL_DIM") && p_in2.exists("COMBINATORIAL_DIM")) {
      const Int dim1 = p_in1.give("COMBINATORIAL_DIM"),
                dim2 = p_in2.give("COMBINATORIAL_DIM");
      p_out.take("COMBINATORIAL_DIM") << dim1+dim2;
   }

   if (!noc) {

      if(!nov && v_present){ // at least one of the polytopes has V given or nof is active
         const bool pointed=p_in1.give("POINTED") && p_in2.give("POINTED");
         if (!pointed)
            throw std::runtime_error("product: input polyhedron not pointed");

         std::string given1, given2;
         const Matrix<Scalar> V1=p_in1.give_with_property_name("VERTICES | POINTS", given1),
            V2=p_in2.give_with_property_name("VERTICES | POINTS", given2);
         const bool VERTICES_out= given1=="VERTICES" && given2=="VERTICES";

         const Matrix<Scalar> V_out=product_vertices(V1, V2, n_vertices1, n_vertices2, n_vertices_out, n_rays, rays1, rays2);

         p_out.take(VERTICES_out ? Str("VERTICES") : Str("POINTS")) << V_out;
      }

      if(!nof && (nov || f_present)){
         std::string given1, given2;
         const Matrix<Scalar> F1=p_in1.give_with_property_name("FACETS | INEQUALITIES", given1),
            F2=p_in2.give_with_property_name("FACETS | INEQUALITIES", given2);
         const bool FACETS_out= given1=="FACETS" && given2=="FACETS";

         const Matrix<Scalar> F_out=product_facets(F1, F2);


         const Matrix<Scalar> H1=p_in1.give("AFFINE_HULL | EQUATIONS"),
            H2=p_in2.give("AFFINE_HULL | EQUATIONS");
         const Matrix<Scalar> H_out=product_facets(H1, H2);

         p_out.take(FACETS_out ? Str("FACETS") : Str("INEQUALITIES")) << F_out;
         p_out.take(FACETS_out ? Str("AFFINE_HULL") : Str("EQUATIONS")) << H_out;
      }
   }

   if (options["group"]) {
      BigObject g("group::Group", "canonicalGroup");
      g.set_description() << "canonical group induced by the group of the base polytopes" << endl;

      Array<Array<Int>> gens1, gens2;

      if (p_in1.lookup("GROUP.VERTICES_ACTION.GENERATORS") >> gens1 &&
          p_in2.lookup("GROUP.VERTICES_ACTION.GENERATORS") >> gens2) {
         if (n_vertices1 == 0)
            n_vertices1 = gens1[0].size();
         if (n_vertices2 == 0)
            n_vertices2 = gens2[0].size();
         if (n_vertices_out == 0)
            n_vertices_out = n_vertices1 + n_vertices2;

         Int g1 = gens1.size();

         Array<Array<Int>> gens_out(g1 + gens2.size());

         // each "block" of vertices of p_out corresponds to one vertex of p1
         for (Int i = 0; i < g1; ++i) {
            gens_out[i] = permute_blocks(n_vertices_out, gens1[i], n_vertices2);
         }
         // the vertices inside a "block" correspond to the vertices of p2
         for (Int i = g1; i < gens_out.size(); ++i) {
            gens_out[i] = permute_inside_blocks(n_vertices_out, gens2[i-g1], n_vertices1);
         }

         BigObject a("group::PermutationAction", "GENERATORS", gens_out);
         p_out.take("GROUP") << g;
         p_out.take("GROUP.VERTICES_ACTION") << a;
      }
      else if (p_in1.lookup("GROUP.FACETS_ACTION.GENERATORS") >> gens1 &&
               p_in2.lookup("GROUP.FACETS_ACTION.GENERATORS") >> gens2) {

         Int n_facets1 = gens1[0].size();
         Int n_facets_out = n_facets1 + gens2[0].size();

         Int g1 = gens1.size();
         Array<Array<Int>> gens_out(g1 + gens2.size());

         for (Int i = 0; i < g1; ++i) {
            gens_out[i] = gens1[i].append(range(n_facets1,n_facets_out-1));
         }
         for (Int i = g1; i < gens_out.size(); ++i) {
            gens_out[i] = Array<Int>(range(0, n_facets1-1)).append(gens2[i-g1]);
            for (Int j = n_facets1; j < n_facets_out; ++j)
               gens_out[i][j]+=n_facets1;
         }

         BigObject a("group::PermutationAction", "GENERATORS", gens_out);
         p_out.take("GROUP") << g;
         p_out.take("GROUP.FACETS_ACTION") << a;
      }
      else
         throw std::runtime_error("GROUP action of both input polytopes must be provided.");
   }

   if (relabel && v_present) {
      const std::vector<std::string> labels1 = common::read_labels(p_in1, "VERTEX_LABELS", n_vertices1);
      const std::vector<std::string> labels2 = common::read_labels(p_in2, "VERTEX_LABELS", n_vertices2);
      std::vector<std::string> labels_out(n_vertices_out);
      if (n_rays==0) {
         copy_range(entire(pm::product(labels1, labels2, product_label())), labels_out.begin());
      } else {
         auto l=labels_out.begin();
         l=copy_range(entire(pm::product(select(labels1,~rays1), select(labels2,~rays2), product_label())), l);
         const std::string all("all");
         l=copy_range(entire(attach_operation(select(labels1,rays1), same_value(all), product_label())), l);
         copy_range(entire(attach_operation(same_value(all), select(labels2,rays2), product_label())), l);
      }
      p_out.take("VERTEX_LABELS") << labels_out;
   }
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polytope as the product of two given polytopes //P1// and //P2//."
                          "# As little additional properties of the input polytopes as possible are computed."
                          "# You can control this behaviour using the option flags."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @option Bool no_coordinates only combinatorial information is handled"
                          "# @option Bool no_vertices do not compute vertices"
                          "# @option Bool no_facets do not compute facets"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytopes, if present."
                          "#   the label of a new vertex corresponding to v<sub>1</sub> &oplus; v<sub>2</sub> will"
                          "#   have the form LABEL_1*LABEL_2. default: 0"
                          "# @option Bool group Compute the canonical group of the product, as induced by the groups on"
                          "#    FACETS of VERTICES of //P1// and //P2//. If neither FACETS_ACTION nor VERTICES_ACTION of the"
                          "#    GROUPs of the input polytopes are provided, an exception is thrown. default 0"
                          "# @return Polytope"
                          "# @example The following builds the product of a square and an interval,"
                          "# without computing vertices of neither the input nor the output polytopes."
                          "# > $p = product(cube(2),cube(1), no_vertices=>1);",
                          "product<Scalar>(Polytope<type_upgrade<Scalar>>, Polytope<type_upgrade<Scalar>>; { no_coordinates => 0, no_vertices=>0, no_facets=>0, no_labels => 0, group=>0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
