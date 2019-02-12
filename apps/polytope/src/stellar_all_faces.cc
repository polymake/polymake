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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Graph.h"
#include "polymake/FacetList.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/polytope/bisector.h"
#include "polymake/vector"
#include "polymake/list"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object stellar_all_faces(perl::Object p_in, int end_dim)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("stellar_all_faces: input polytope must be bounded\n");

   Matrix<Scalar> V=p_in.give("VERTICES");
   const Matrix<Scalar> F=p_in.give("FACETS");
   const Matrix<Scalar> lineality_space=p_in.give("LINEALITY_SPACE");
   const Vector<Scalar> rel_int_point=p_in.give("REL_INT_POINT");
   FacetList VIF=p_in.give("VERTICES_IN_FACETS");
   perl::Object HD_obj =p_in.give("HASSE_DIAGRAM");
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& HD = p_in.give("HASSE_DIAGRAM");
   Graph<Undirected> DG=p_in.give("DUAL_GRAPH.ADJACENCY");

   const int dim = HD.rank()-1;
   if (end_dim < 0)
      end_dim += dim;
   if (end_dim >= dim || end_dim <= 0)
      throw std::runtime_error("end dimension out of range");

   NodeMap<Undirected, Vector<Scalar> > facet_normals(DG, rows(F).begin());

   int v_count = V.rows();
   int new_size = HD.nodes_of_rank(1).size();
   for (int d=dim-1; d>=end_dim; --d)
      new_size += HD.nodes_of_rank(d+1).size();
   V.resize(new_size, V.cols());

   // random access to the facets of VIF
   std::vector< FacetList::iterator > facets_of_VIF;
   facets_of_VIF.reserve(VIF.size());
   for (auto fl_it=entire(VIF); !fl_it.at_end(); ++fl_it)
      facets_of_VIF.push_back(fl_it);

   // iterate over dimensions dim-1..end_dim and over all faces of each dimension
   // a new vertex is added for each face
   for (int d=dim-1; d>=end_dim; --d) {
      FacetList new_VIF;
      std::vector< FacetList::iterator > facets_of_new_VIF;
      facets_of_new_VIF.reserve(VIF.size() * (d+1));  // lower bound for #facets after stell-sd of faces dim-1, .., d

      Graph<Undirected> new_DG;
      NodeMap<Undirected, Vector<Scalar> > new_facet_normals(new_DG);

      std::vector<int> corr_old_facet;
      corr_old_facet.reserve((d+1) * DG.nodes());
      Array< Set<int> > corr_new_facets(DG.nodes());

      int f_count = 0;
      for (const auto face_index : HD.nodes_of_rank(d+1)) {
         const Set<int>& face = HD.face(face_index);

         // produce all relevant inequalities = each neighbour (in the dual graph) of the nodes
         // corresponding to the facets of star(face) produces one inequality
         const Vector<Scalar> m = average( rows(V.minor(face, All)) ) - rel_int_point;
         Scalar t_max = Scalar(3);

         Set<int> star_facets;
         for (FacetList::superset_iterator incid_facets=VIF.findSupersets(face); !incid_facets.at_end(); ++incid_facets)
            star_facets += incid_facets.index();

         for (auto st_it=entire(star_facets); !st_it.at_end(); ++st_it) {
            const Set<int> neighbors = DG.adjacent_nodes(*st_it)-star_facets;

            for (auto n_it=entire(neighbors); !n_it.at_end(); ++n_it) {
               const int v= (*facets_of_VIF[*st_it] * *facets_of_VIF[*n_it]).front();
               const Vector<Scalar> inequ = bisector(facet_normals[*st_it], facet_normals[*n_it], V[v]);

               const Scalar denominator = inequ * m;
               if (denominator != 0) assign_min(t_max, -inequ * rel_int_point / denominator);
            }
         }

         // add new vertex
         const Scalar scale = (t_max + 1) / 2;
         V[v_count] = rel_int_point + m * scale;

         // add new facets
         // iterate over all neighbours (in the dual graph) of the nodes
         // corresponding to the facets of star(face) and add a facet for each one
         for (auto st_it=entire(star_facets); !st_it.at_end(); ++st_it) {
            const Set<int> neighbors = DG.adjacent_nodes(*st_it)-star_facets;

            for (auto n_it=entire(neighbors); !n_it.at_end(); ++n_it) {
               Set<int> new_facet = (*facets_of_VIF[*st_it]) * (*facets_of_VIF[*n_it]);
               new_facet += v_count;

               new_VIF.insert(new_facet);
               facets_of_new_VIF.push_back(--new_VIF.end());

               if (d>end_dim) {  // add node to new_DG and store the facet inequality
                  Vector<Scalar> equ = null_space(V.minor(new_facet,All))[0];
                  if (equ*rel_int_point < 0)
                     equ.negate();
                  new_facet_normals[new_DG.add_node()]=equ;

                  corr_old_facet.push_back(*st_it);
                  corr_new_facets[*st_it] += f_count;
               }

               ++f_count;
            }
         }
         ++v_count;

      }  // end of faces of dim d
        
      // update VIF
      VIF = new_VIF;
      facets_of_VIF = facets_of_new_VIF;

      if (d==end_dim)  // no update of DG
         break;

      // add adjacency information to new_DG and update DG
      FacetList ridges;
      std::vector< std::pair<int,int> > corresp_edge;
      corresp_edge.reserve(VIF.size()*(d+1)/2);  // lower bound for #ridges

      // iterate over new faces to determine adjacency
      for (auto f_it=entire(VIF); !f_it.at_end(); ++f_it) {
         Set<int> old_neighbors = DG.adjacent_nodes(corr_old_facet[ f_it.index() ]);
         old_neighbors += corr_old_facet[ f_it.index() ];

         for (auto n_it=entire(old_neighbors); !n_it.at_end(); ++n_it)
            for (auto cand=entire(corr_new_facets[*n_it]); !cand.at_end(); ++cand)
               if (f_it.index() < *cand) {
                  const Set<int> ridge = *f_it * *facets_of_VIF[*cand];

                  if (ridge.size() >= dim-1) {  // potential ridge
                     if (ridges.insertMax(ridge)) {  // new potential ridge has been inserted -> remember corresponding edge
                        const int index = (--ridges.end()).index();

                        if (size_t(index) >= corresp_edge.size())
                           corresp_edge.resize(index+1);
                        corresp_edge[index].first = f_it.index();
                        corresp_edge[index].second = *cand;
                     }
                  }
               }
      }  // end iterate over faces

      // add edges to new_DG
      for (auto r_it=entire(ridges); !r_it.at_end(); ++r_it)
         new_DG.edge(corresp_edge[r_it.index()].first,corresp_edge[r_it.index()].second);

      DG = new_DG;
      facet_normals = new_facet_normals;

   }  // end of d = dim-1..end_dim

   perl::Object p_out("Polytope", mlist<Scalar>());
   p_out.set_description() << "Stellar subdivision of " << p_in.name() << " over all proper faces of dimension >= " << end_dim << endl;
   p_out.take("VERTICES") << V;
   p_out.take("VERTICES_IN_FACETS") << VIF;
   p_out.take("LINEALITY_SPACE") << lineality_space;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Perform a stellar subdivision of all proper faces, starting with the facets."
                          "# "
                          "# Parameter //d// specifies the lowest dimension of the faces to be divided."
                          "# It can also be negative, then treated as the co-dimension."
                          "# Default is 1, that is, the edges of the polytope."
                          "# @param Polytope P, must be bounded"
                          "# @param Int d the lowest dimension of the faces to be divided;"
                          "#   negative values: treated as the co-dimension; default value: 1."
                          "# @return Polytope"
                          "# @author Nikolaus Witte",
                          "stellar_all_faces<Scalar>(Polytope<Scalar>; $=1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
