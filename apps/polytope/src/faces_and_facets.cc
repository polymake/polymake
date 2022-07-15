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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/hash_map"
#include "polymake/Graph.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject facet(BigObject p_in, Int facet_number, OptionSet options)
{
   const IncidenceMatrix<> RIF = p_in.give("RAYS_IN_FACETS");
   const Graph<> DG = p_in.give("DUAL_GRAPH.ADJACENCY");
   const Int n_rays = RIF.cols(), n_facets=RIF.rows();

   if (facet_number < 0 || facet_number >= n_facets)
      throw std::runtime_error("facet number out of range");

   IncidenceMatrix<> RIF_out=RIF.minor(DG.adjacent_nodes(facet_number), RIF[facet_number]);

   BigObject p_out(p_in.type());
   p_out.take("RAYS_IN_FACETS") << RIF_out;
   p_out.set_description() << "facet " << facet_number << " of " << p_in.name() << endl;

   if (options["no_coordinates"]) {
      if (p_in.exists("CONE_DIM")) {
         const Int dim = p_in.give("CONE_DIM");
         p_out.take("CONE_DIM") << dim-1;
      }
   } else {
      const Matrix<Scalar> R=p_in.give("RAYS"),
                             LS=p_in.give("LINEALITY_SPACE");

      p_out.take("RAYS") << R.minor(RIF[facet_number],All);
      p_out.take("LINEALITY_SPACE") << LS;
   }

   if (!options["no_labels"]) {
      const std::vector<std::string> labels = common::read_labels(p_in, "RAY_LABELS", n_rays);
      Array<std::string> labels_out(select(labels,RIF[facet_number]));
      p_out.take("RAY_LABELS") << labels_out;
   }

   return p_out;
}

std::pair<Set<Int>, Set<Int>>
face_pair(BigObject p_in, const Set<Int>& some_rays)
{
   const IncidenceMatrix<> RIF = p_in.give("RAYS_IN_FACETS");
   const Int n_facets = RIF.rows(), n_rays = RIF.cols();
   Set<Int> facets_containing_given_rays, rays_in_facets_found;
   if (some_rays.empty()) {
      facets_containing_given_rays=sequence(0,n_facets);
   } else {
      auto i=entire(some_rays);
      facets_containing_given_rays=RIF.col(*i);
      for (++i; !i.at_end(); ++i) {
         facets_containing_given_rays*=RIF.col(*i);
      }
   }
   if (facets_containing_given_rays.empty()) {
      rays_in_facets_found=sequence(0,n_rays);
   } else {
      auto i = entire(facets_containing_given_rays);
      rays_in_facets_found = RIF.row(*i);
      for (++i; !i.at_end(); ++i) {
         rays_in_facets_found*=RIF.row(*i);
      }
   }
   return { rays_in_facets_found, facets_containing_given_rays };
}


template <typename Scalar>
BigObject face(BigObject p_in, const Set<Int>& some_rays, OptionSet options)
{
   const IncidenceMatrix<> RIF = p_in.give("RAYS_IN_FACETS");
   const Int n_facets = RIF.rows(), n_rays = RIF.cols();
   std::pair<Set<Int>, Set<Int>> fp = face_pair(p_in, some_rays);
   
   const Set<Int>& rays_of_face = fp.first;
   const Set<Int>& facets_containing_face = fp.second;
   Set<Int> facet_candidates = sequence(0, n_facets) - facets_containing_face; // facet indices
   FacetList facets_of_face;
   for (auto f = entire(facet_candidates); !f.at_end(); ++f)
      facets_of_face.insertMax(rays_of_face * RIF.row(*f)); // sets of ray indices
   const Int n_rays_of_face = rays_of_face.size(), n_facets_of_face = facets_of_face.size();

   Int idx = 0;
   hash_map<Int, Int> relabel(n_rays_of_face);
   for (auto v = entire(rays_of_face); !v.at_end(); ++v)
      relabel[*v]=idx++;

   IncidenceMatrix<> rif_face(n_facets_of_face, n_rays_of_face);
   idx = 0;
   for (auto f = entire(lex_ordered(facets_of_face)); !f.at_end(); ++f, ++idx)
      for (auto v = entire(*f); !v.at_end(); ++v)
         rif_face(idx,relabel[*v])=1;
   
   BigObject p_out(p_in.type());
   p_out.take("RAYS_IN_FACETS") << rif_face;

   if (!options["no_coordinates"]) {
      const Matrix<Scalar> R=p_in.give("RAYS"),
                          LS=p_in.give("LINEALITY_SPACE");

      p_out.take("RAYS") << R.minor(rays_of_face,All);
      p_out.take("LINEALITY_SPACE") << LS;
   }
   
   if (!options["no_labels"]) {
      const std::vector<std::string> labels = common::read_labels(p_in, "RAY_LABELS", n_rays);
      Array<std::string> labels_out(select(labels, rays_of_face));
      p_out.take("RAY_LABELS") << labels_out;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                  "# Extract the given //facet// of a polyhedron and write it as a new polyhedron."
                  "# @param Cone P"
                  "# @param Int facet"
                  "# @option Bool no_coordinates don't copy the coordinates, produce purely combinatorial description."
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                  "# @return Cone"
                  "# @example To create a cone from the vertices of the zeroth facet of the 3-cube, type this:"
                  "# > $p = facet(cube(3),0);",
                  "facet<Scalar>(Cone<Scalar> $ {no_coordinates => 0, no_labels => 0})");

UserFunction4perl("# @category Other"
                  "# For a given set S of rays compute the smallest face F of a cone P containing them all; see also //face//."
                  "# @param Cone P"
                  "# @param Set S"
                  "# @return Pair<Set,Set> where the first is the set of vertices of F, while the second is the set of facets containing F."
                  "# @example computes the dimension of the face of the 3-cube which is spanned by the vertices 0 and 1"
                  "# > $c=cube(3);"
                  "# > print rank($c->VERTICES->minor(face_pair($c,[0,1])->first(),All))-1;"
                  "# | 1",
                  &face_pair,"face_pair(Cone $)");

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                  "# For a given set S of rays compute the smallest face F of a cone P containing them all; see also //face_pair//."
                  "# @param Cone P"
                  "# @param Set S"
                  "# @option Bool no_coordinates don't copy the coordinates, produce purely combinatorial description."
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                  "# @return Cone"
                  "# @example To create a cone from the vertices of the zeroth facet of the 3-cube, type this:"
                  "# > $p = face(cube(3),[0,1]);",
                  "face<Scalar>(Cone<Scalar> $ {no_coordinates => 0, no_labels => 0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
