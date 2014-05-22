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
#include "polymake/Rational.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {
template<typename Scalar>
perl::Object cayley_embedding(perl::Object p_in1, perl::Object p_in2,
                              const Scalar& z, const Scalar& z_prime, 
                              perl::OptionSet options)
{
   const bool pointed=p_in1.give("POINTED") && p_in2.give("POINTED");
   if (!pointed)
      throw std::runtime_error("prism: at least one input polyhedron not pointed");

   if (z==z_prime)
      throw std::runtime_error("z and z' must be different");

   std::string has_VERTICES1, has_VERTICES2;
   const Matrix<Scalar> 
      V1=p_in1.give_with_property_name("VERTICES | POINTS", has_VERTICES1),
      V2=p_in2.give_with_property_name("VERTICES | POINTS", has_VERTICES2);
   bool VERTICES_out= has_VERTICES1=="VERTICES" && has_VERTICES2=="VERTICES";

   const int n_vertices1 = V1.rows(),
             n_vertices2 = V2.rows(),
                       d = V1.cols();
   if (V2.cols() != d)
      throw std::runtime_error("cayley_embedding: dimension mismatch");
   
   const Set<int> rays1=far_points(V1),
                  rays2=far_points(V2);

   if (VERTICES_out && !rays1.empty() && !rays2.empty()) VERTICES_out=false;

   const bool relabel=options["relabel"];
   if (!VERTICES_out && relabel)
      throw std::runtime_error("can't produce VERTEX_LABELS since VERTICES are unknown");

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Cayley embedding of " << p_in1.name() << " and " << p_in2.name() << endl;

   const Matrix<Scalar> V_out=
      rays1.empty() && rays2.empty()
      ? Matrix<Scalar>( (V1 | same_element_vector(z, n_vertices1)) /
                          (V2 | same_element_vector(z_prime, n_vertices2)) )
      : Matrix<Scalar>( (V1 | same_element_sparse_vector(~rays1, z, n_vertices1)) /
                          (V2 | same_element_sparse_vector(~rays2, z_prime, n_vertices2)) );

   p_out.take(VERTICES_out ? "VERTICES" : "POINTS") << V_out;
   p_out.take(VERTICES_out ? "LINEALITY_SPACE" : "INPUT_LINEALITY") << Matrix<Scalar>();

   if (relabel) {
      std::vector<std::string> labels(n_vertices1+n_vertices2);
      read_labels(p_in1, "VERTEX_LABELS", non_const(select(labels, sequence(0,n_vertices1))));
      read_labels(p_in2, "VERTEX_LABELS", non_const(select(labels, sequence(n_vertices1,n_vertices2))));
      const char tick='\'';
      for (std::vector<std::string>::iterator l=labels.begin()+n_vertices1, l_end=labels.end(); l!=l_end; ++l)
         (*l) += tick;
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Create a Cayley embedding of two polytopes (one of them must be pointed)."
                          "# The vertices of the first polytope //P// get an extra coordinate //z//"
                          "# and the vertices of the second polytope //P_prime// get //z_prime//."
                          "# "
                          "# Default values are //z//=1 and //z_prime//=-//z//."
                          "# "
                          "# The option //relabel// creates an additional section [[VERTEX_LABELS]]."
                          "# The vertices of //P// inherit the original labels unchanged;"
                          "# the vertex labels of //P_prime// get a tick (') appended."
                          "# @param Polytope P the first polytope"
                          "# @param Polytope P_prime the second polytope"
                          "# @param Scalar z the extra coordinate for the vertices of //P//"
                          "# @param Scalar z_prime the extra coordinate for the vertices of //P_prime//"
                          "# @option Bool relabel"
                          "# @return Polytope",
                          "cayley_embedding<_ExtraType, Scalar={ typechecks::is_ordered_field(_ExtraType) ? _ExtraType : Rational }>(Polytope, Polytope; _ExtraType=1, _ExtraType=(-$_[2]), { relabel => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
