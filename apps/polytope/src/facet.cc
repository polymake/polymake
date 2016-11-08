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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

perl::Object facet(perl::Object p_in, int facet_number, perl::OptionSet options)
{
   const IncidenceMatrix<> VIF=p_in.give("RAYS_IN_FACETS");
   const Graph<> DG=p_in.give("DUAL_GRAPH.ADJACENCY");
   const int n_vertices=VIF.cols(), n_facets=VIF.rows();

   if (facet_number < 0 || facet_number >= n_facets)
      throw std::runtime_error("facet number out of range");

   IncidenceMatrix<> VIF_out=VIF.minor(DG.adjacent_nodes(facet_number), VIF[facet_number]);

   perl::Object p_out(p_in.type());
   p_out.take("RAYS_IN_FACETS") << VIF_out;
   p_out.set_description() << "facet " << facet_number << " of " << p_in.name() << endl;

   if (options["no_coordinates"]) {
      if (p_in.exists("CONE_DIM")) {
         const int dim=p_in.give("CONE_DIM");
         p_out.take("CONE_DIM") << dim-1;
      }
   } else {
      const Matrix<Rational> V=p_in.give("RAYS"),
                             F=p_in.give("FACETS"),
                             AH=p_in.give("LINEAR_SPAN"),
                             LS=p_in.give("LINEALITY_SPACE");

      //FIXME: This client should be enhanced: it is not necessary to know the rays. 
      //One could also just write INEQUALITIES and EQUATIONS, where
      //taking all facets but the chosen facet and all affine hull equations + the chosen facet.
      p_out.take("RAYS") << V.minor(VIF[facet_number],All);
      p_out.take("LINEALITY_SPACE") << LS;

      // the following line is not allowed according to #519: LINEAR_SPAN may not be written without FACETS 
      // p_out.take("LINEAR_SPAN") << AH / F[facet_number];
   }

   if (!options["no_labels"]) {
      Array<std::string> labels(n_vertices);
      read_labels(p_in, "RAY_LABELS", labels);
      Array<std::string> labels_out(select(labels,VIF[facet_number]));
      p_out.take("RAY_LABELS") << labels_out;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Extract the given //facet// of a polyhedron and write it as a new polyhedron."
                  "# @param Cone P"
                  "# @param Int facet"
                  "# @option Bool no_coordinates don't copy the coordinates, produce purely combinatorial description."
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                  "# @return Cone"
                  "# @example To create a cone from the vertices of the zeroth facet of the 3-cube, type this:"
                  "# > $p = facet(cube(3),0);",
                  &facet,"facet(Cone $ {no_coordinates => 0, no_labels => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
