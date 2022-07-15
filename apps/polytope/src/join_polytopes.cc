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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject join_polytopes(BigObject p1, BigObject p2, OptionSet options)
{
   const bool bounded1 = p1.give("BOUNDED"),
              bounded2 = p2.give("BOUNDED");
   if (!bounded1 || !bounded2)
      throw std::runtime_error("join_polytopes: input polyhedron not BOUNDED");

   const bool noc = options["no_coordinates"];

   BigObject p_out("Polytope", mlist<Scalar>());
   p_out.set_description() << "Join of " << p1.name() << " and " << p2.name() << endl;

   // initializations prevent wrong warnings "may be used uninitialized" with some broken gcc's
   Int n1 = 0;
   Int n2 = 0;
   Int n_vertices_out = 0;

   if (noc || p1.exists("VERTICES_IN_FACETS") && p2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF1 = p1.give("VERTICES_IN_FACETS"),
                              VIF2 = p2.give("VERTICES_IN_FACETS");
      n1 = VIF1.cols();  n2 = VIF2.cols();
      n_vertices_out = n1+n2;

      const IncidenceMatrix<> VIF_out = diag_1(VIF1, VIF2);
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (!noc) {
      const Matrix<Scalar> 
      v1=p1.give("VERTICES"),
      v2=p2.give("VERTICES");

      n1=v1.rows();
      n2=v2.rows();
      n_vertices_out = n1+n2;

      // TODO: should work with (v1 | same_element_vector(Scalar(-1),n1) ...)
      // check when ContainerChain refactoring completed
      const Matrix<Scalar> V_out = 
         (v1 | -ones_vector<Scalar>(n1) | zero_matrix<Scalar>(n1, v2.cols()-1)) /
         (ones_vector<Scalar>(n2) | zero_matrix<Scalar>(n2, v1.cols()-1) | v2);
      p_out.take("VERTICES") << V_out;
   }

   if (options["group"]) {
      Array<Array<Int>> gens1 = p1.give("GROUP.VERTICES_ACTION.GENERATORS");
      Array<Array<Int>> gens2 = p2.give("GROUP.VERTICES_ACTION.GENERATORS");
      Int g1 = gens1.size();
      Array<Array<Int>> gens_out(g1 + gens2.size());

      for (Int i = 0; i < g1; ++i) {
         gens_out[i] = gens1[i].append(range(n1,n_vertices_out-1));
      }
      for (Int i = g1; i < gens_out.size(); ++i) {
         gens_out[i] = Array<Int>(range(0,n1-1)).append(gens2[i-g1]);
         for (Int j = n1; j < n_vertices_out; ++j)
            gens_out[i][j]+=n1;
      }

      BigObject a("group::PermutationAction", "GENERATORS", gens_out);
      BigObject g("group::Group", "canonicalGroup");
      g.set_description() << "canonical group induced by the group of the base polytopes" << endl;
      p_out.take("GROUP") << g;
      p_out.take("GROUP.VERTICES_ACTION") << a;
   }

   p_out.take("N_VERTICES") << n_vertices_out;
   return p_out;
}

template <typename Scalar>
BigObject free_sum_impl(BigObject p1, BigObject p2, const std::string object_prefix, const std::string linear_span_name, Int first_coord, OptionSet options)
{
   if ( (  (object_prefix=="CONE" && 
            !p1.exists("RAYS | INPUT_RAYS") &&
            !p2.exists("RAYS | INPUT_RAYS")) ||
           (object_prefix=="VECTOR" &&
            !p1.exists("VECTORS") &&
            !p2.exists("VECTORS")) ) &&
        ! (object_prefix=="CONE" && 
           p1.exists("FACETS | INEQUALITIES") && 
           p2.exists("FACETS | INEQUALITIES") ) )
      throw std::runtime_error("free_sum is not defined for combinatorially given objects");

   const bool force_centered=options["force_centered"];

   const bool bounded=p1.give("BOUNDED") && p2.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("free_sum: input not bounded");

   const bool centered=p1.give("CENTERED") && p2.give("CENTERED");
   if (!centered && force_centered)
      throw std::runtime_error("free_sum: input polyhedron not centered. If you want to continue, you may use the option 'force_centered=>0'");
   const bool noc=options["no_coordinates"];

   BigObject p_out(p1.type());
   p_out.set_description() << "Free sum of "<< p1.name() << " and " << p2.name() << endl;

   Int n1;
   Int n2;
   Int n_vertices_out = 0;

   if (noc || object_prefix=="CONE" && p1.exists("VERTICES_IN_FACETS") && p2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> 
         VIF1=p1.give("VERTICES_IN_FACETS"),
         VIF2=p2.give("VERTICES_IN_FACETS");
      n1=VIF1.cols();  n2=VIF2.cols();
      n_vertices_out = n1+n2;
      
      const Int n_facets1 = VIF1.rows(),
                n_facets2 = VIF2.rows(),
             n_facets_out = n_facets1 * n_facets2;

      IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out, product(rows(VIF1), rows(VIF2), operations::concat()).begin());

      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if (!noc) {
      const std::string rays_section = (object_prefix=="CONE" ? "RAYS" : "VECTORS");
      const Matrix<Scalar> 
         v1=p1.give(rays_section),
         v2=p2.give(rays_section);

      n1=v1.rows();
      n2=v2.rows();
      n_vertices_out = n1+n2;

      const Matrix<Scalar> V_out = 
         (v1 | zero_matrix<Scalar>(v1.rows(), v2.cols()-1)) /
         (ones_vector<Scalar>(v2.rows()) | zero_matrix<Scalar>(v2.rows(), v1.cols()-1) | v2.minor(All, range_from(1)));
      p_out.take(rays_section) << V_out;
   }
   const std::string n_section = object_prefix == "CONE" ? "N_VERTICES" : "N_VECTORS";
   p_out.take(n_section) << n_vertices_out;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polyhedron as the join of two given bounded ones."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @option Bool no_coordinates produces a pure combinatorial description."
                          "# @option Bool group Compute the canonical group induced by the groups on //P1// and //P2//"
                          "#   Throws an exception if the GROUPs of the input polytopes are not provided. default 0"
                          "# @return Polytope"
                          "# @example To join two squares, use this:"
                          "# > $p = join_polytopes(cube(2),cube(2));"
                          "# > print $p->VERTICES;"
                          "# | 1 -1 -1 -1 0 0"
                          "# | 1 1 -1 -1 0 0"
                          "# | 1 -1 1 -1 0 0"
                          "# | 1 1 1 -1 0 0"
                          "# | 1 0 0 1 -1 -1"
                          "# | 1 0 0 1 1 -1"
                          "# | 1 0 0 1 -1 1"
                          "# | 1 0 0 1 1 1",
                          "join_polytopes<Scalar>(Polytope<Scalar> Polytope<Scalar>, {no_coordinates => 0, group => 0})");

FunctionTemplate4perl("free_sum_impl<Scalar=Rational>($$$$$ {force_centered=>1, no_coordinates=> 0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
