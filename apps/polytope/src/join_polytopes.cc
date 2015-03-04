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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object join_polytopes(perl::Object p1, perl::Object p2, perl::OptionSet options)
{
   const bool bounded=p1.give("BOUNDED") && p2.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("join_polytopes: input polyhedron not BOUNDED");

   const bool noc=options["noc"];

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Join of " << p1.name() << " and " << p2.name() << endl;

   int n1;
   int n2;
   int n_vertices_out=0;

   if (noc || p1.exists("VERTICES_IN_FACETS") && p2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF1=p1.give("VERTICES_IN_FACETS"),
         VIF2=p2.give("VERTICES_IN_FACETS");
      n1=VIF1.cols();  n2=VIF2.cols();
      n_vertices_out= n1 + n2;
      
      const int n_facets1=VIF1.rows(),
         n_facets2=VIF2.rows(),
         n_facets_out=n_facets1 + n_facets2;

      IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);

      const pm::SameElementIncidenceMatrix<true> S1(n_facets1,n2);
      const pm::SameElementIncidenceMatrix<true> S2(n_facets2,n1);
      VIF_out = (VIF1 | S1)/(S2 |VIF2);
      
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   
   if(!noc){
      const Matrix<Scalar> 
      v1=p1.give("VERTICES"),
      v2=p2.give("VERTICES");

      n1=v1.rows();
      n2=v2.rows();
      n_vertices_out = n1+n2;

      const Matrix<Scalar> V_out = 
         (v1 | same_element_vector(Scalar(-1),n1) | zero_matrix<Scalar>(n1,v2.cols()-1)) /
         (ones_vector<Scalar>(n2) | zero_matrix<Scalar>(n2,v1.cols()-1) | v2);
      p_out.take("VERTICES") << V_out;
      p_out.take("LINEALITY_SPACE") << Matrix<Scalar>();
   }

   p_out.take("N_VERTICES") << n_vertices_out;
   return p_out;
}

template <typename Scalar>
perl::Object free_sum(perl::Object p1, perl::Object p2, perl::OptionSet options)
{
   const bool bounded=p1.give("BOUNDED") && p2.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("free_sum: input polyhedron not bounded");

   const bool centered=p1.give("CENTERED") && p2.give("CENTERED");
   if (!centered)
      throw std::runtime_error("free_sum: input polyhedron not centered");
   const bool noc=options["noc"];

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Free sum of "<< p1.name() << " and " << p2.name() << endl;

   int n1;
   int n2;
   int n_vertices_out=0;

   if (noc || p1.exists("VERTICES_IN_FACETS") && p2.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF1=p1.give("VERTICES_IN_FACETS"),
         VIF2=p2.give("VERTICES_IN_FACETS");
      n1=VIF1.cols();  n2=VIF2.cols();
      n_vertices_out= n1 + n2;
      
      const int n_facets1=VIF1.rows(),
         n_facets2=VIF2.rows(),
         n_facets_out=n_facets1 * n_facets2;

      IncidenceMatrix<> VIF_out(n_facets_out, n_vertices_out);

      copy(entire(product(rows(VIF1), rows(VIF2), operations::concat())), rows(VIF_out).begin());

      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   if(!noc){
      const Matrix<Scalar> 
         v1=p1.give("VERTICES"),
         v2=p2.give("VERTICES");

      n1=v1.rows();
      n2=v2.rows();
      n_vertices_out = n1+n2;

      const Matrix<Scalar> V_out = 
         (v1 | zero_matrix<Scalar>(v1.rows(), v2.cols()-1)) /
         (ones_vector<Scalar>(v2.rows()) | zero_matrix<Scalar>(v2.rows(), v1.cols()-1) | v2.minor(All, ~scalar2set(0)));
      p_out.take("VERTICES") << V_out;
      p_out.take("LINEALITY_SPACE") << Matrix<Scalar>();
   }
   
p_out.take("N_VERTICES") << n_vertices_out;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polyhedron as the join of two given bounded ones."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @return Polytope",
                          "join_polytopes<Scalar>(Polytope<Scalar> Polytope<Scalar>, {noc => 0})");

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polyhedron as the free sum of two given bounded ones."
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @return Polytope",
                          "free_sum<Scalar>(Polytope<Scalar> Polytope<Scalar>, {noc => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
