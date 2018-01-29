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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object pyramid(perl::Object p_in, const Scalar& z, perl::OptionSet options)
{
   const bool noc = options["no_coordinates"],
      relabel = !options["no_labels"],
      group = options["group"];

   if (z==0 && !noc)
      throw std::runtime_error("pyramid: z must be non-zero");
   if(group && !p_in.exists("GROUP"))
         throw std::runtime_error("pyramid: group of the base polytope needs to be provided in order to compute group of the pyramid.");

   int n_vertices=0;
   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "pyramid over " << p_in.name() << endl;




   if (noc || p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
      n_vertices=VIF.cols();
      const int n_facets=VIF.rows();
      const IncidenceMatrix<> VIF_out= (VIF | sequence(0,n_facets))     // original vertices + the new top vertex
         / sequence(0, n_vertices);     // original polytope becomes the bottom facet

      p_out.take("N_VERTICES") << n_vertices+1;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }
   if (noc) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const int dim=p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim+1;
      }
   } else {
      const bool pointed=p_in.give("POINTED");
      if (!pointed)
         throw std::runtime_error("pyramid: input polyhedron not pointed");

      const Matrix<Scalar> V=p_in.give("VERTICES");
      n_vertices=V.rows();
      const Vector<Scalar> z0= p_in.give("BOUNDED") ? p_in.give("VERTEX_BARYCENTER") : p_in.give("REL_INT_POINT");
      p_out.take("VERTICES") << (V | zero_vector<Scalar>()) /
         (z0 | z);
   }

   if(group){
      Array<Array<int>> gens = p_in.give("GROUP.VERTICES_ACTION.GENERATORS");

      for(auto i = entire(gens); !i.at_end(); ++i)
         (*i).resize(n_vertices+1,n_vertices);

      perl::Object a("group::PermutationAction");
      a.take("GENERATORS") << gens;

      perl::Object g("group::Group");
      g.set_description() << "canonical group induced by the group of the base polytope" << endl;
      g.set_name("canonicalGroup");
      p_out.take("GROUP") << g;
      p_out.take("GROUP.VERTICES_ACTION") << a;
   }

   if (relabel) {
      std::vector<std::string> labels = common::read_labels(p_in, "VERTEX_LABELS", n_vertices);
      labels.emplace_back("Apex");
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Make a pyramid over a polyhedron."
                          "# The pyramid is the convex hull of the input polyhedron //P// and a point //v//"
                          "# outside the affine span of //P//. For bounded polyhedra, the projection of //v//"
                          "# to the affine span of //P// coincides with the vertex barycenter of //P//."
                          "# @param Polytope P"
                          "# @param Scalar z is the distance between the vertex barycenter and //v//,"
                          "#   default value is 1."
                          "# @option Bool group compute the group induced by the GROUP of //P// and leaving the apex fixed."
                          "#  throws an exception if GROUP of //P// is not provided. default 0"
                          "# @option Bool no_coordinates don't compute new coordinates, produce purely combinatorial description."
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "#   label the new top vertex with \"Apex\"."
                          "# @return Polytope"
                          "# @example The following saves the pyramid of height 2 over the square to the variable $p."
                          "# The vertices are relabeled."
                          "# > $p = pyramid(cube(2),2);"
                          "# To print the vertices and vertex labels of the newly generated pyramid, do this:"
                          "# > print $p->VERTICES;"
                          "# | 1 -1 -1 0"
                          "# | 1 1 -1 0"
                          "# | 1 -1 1 0"
                          "# | 1 1 1 0"
                          "# | 1 0 0 2"
                          "# > print $p->VERTEX_LABELS;"
                          "# | 0 1 2 3 Apex",
                          "pyramid<Scalar>(Polytope<type_upgrade<Scalar>>; type_upgrade<Scalar>=1, {group => 0, no_coordinates => 0, no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
