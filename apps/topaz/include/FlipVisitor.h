/* Copyright (c) 1997-2020
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
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/graph/graph_iterators.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"

namespace polymake { namespace topaz {

// TODO: REMOVE! using Cone = Set<Vector<Rational>>;
// TODO: REMOVE! using Indexed_Cones = Map<Cone, Int>;
// TODO: REMOVE! using flip_sequence = std::list<Int>;
// TODO: REMOVE! using Fan_Vertices = Map<Vector<Rational>, Int>;
// TODO: REMOVE! using Fan_Max_Cells = std::list<Set<Int>>;


class FlipVisitor : public graph::NodeVisitor<> {
   friend class DoublyConnectedEdgeList;
   friend class SecondaryFan;
public:
   using Cone = Set<Vector<Rational>>;
   using Indexed_Cones = Map<Cone, Int>;
   using flip_sequence = std::list<Int>;
   using Fan_Vertices = Map<Vector<Rational>, Int>;
   using Fan_Max_Cells = std::list<Set<Int>>;
   using DoublyConnectedEdgeList = graph::DoublyConnectedEdgeList;
private:

   // the graph we want to iterate through, built during iterations
   Graph<Directed>& delaunay_graph;

   // the base triangulation
   DoublyConnectedEdgeList& dcel;

   // collect all cones, IDs as its corresponding node in the delaunay_graph
   Indexed_Cones cones;

   // for each node of the graph we save the list of indices to flip to the corresponding dcel
   Map<Int, flip_sequence> flipIds_to_node;

   // a set of all vertices of the fan, each of which is labeled - we only need this to define a polyhedral complex
   Fan_Vertices fan_vertices;

   // a list where each entry is a Set that represents a maximal cone of the fan, e.g. {0,1,2,4} is the cone with the rays with labels 1,2,3 and 5; NOTE THE INDEX SHIFT! (we ignore the origin)
   Fan_Max_Cells fan_cells;

   // counter for the number vertices of the fan, equal to 1 + number of rays
   Int fan_num_vert;

   // number of punctures +1
   Int dim;

   // we store the ray indices of the facets at the coordinate hyperplane boundary for the extension to a complete fan by the all -1 vector
   Fan_Max_Cells boundary_facets;

public:

   // this is needed for the BFS++ to not just stop in depth one
   static constexpr bool visit_all_edges = true;

   FlipVisitor(Graph<Directed>& G, DoublyConnectedEdgeList& dcel_)
      : delaunay_graph(G)
      , dcel(dcel_)
   {
      // this is the dimension of the fan +1, or equivalently the number of punctures +1
      dim = dcel.DelaunayInequalities().cols();

      // the flip word of the first cone is obtained by finding the triangulation that is Delaunay for all weights = 1
      flip_sequence start_flips = dcel.flipToDelaunayAlt( ones_vector<Rational>(dim) );
      flipIds_to_node[0] = start_flips;

      Cone first_cone = dcel.coneRays();
      // add the first cone, from the starting dcel
      cones[ first_cone ] = 0;

      // the origin is always a vertex of the fan, for purposes of calculations we need to make sure that it is mapped to 0 by the Map "fan_vertices"
      Vector<Rational> origin(dim);
      origin[0] = 1;
      fan_vertices[ origin ] = 0;

      // since we just added the first cone we set the vertex-counter to 1
      fan_num_vert = 1;

      // updating fan_vertices, fan_num_vert and fan_cells resp. the first cone
      add_cone(first_cone);

      // flip back to input triangulation
      dcel.flipEdges(start_flips, true);
   }

   bool operator()(Int n)
   {
      return operator()(n, n);
   }

   /* Preconditions:
      1) the node n_to is part of the Delaunay graph,
      2) the flip sequence that transforms the base DCEL into the one corresponding to n_to ("flipIds_to_node[ n_to ]") is known and
      3) so is the corresponding cone in Indexed_Cones.

      After this operation all the neighbors of n_to are in the graph (with the corr. edges) with their flip sequences and
      cones respectively
   */
   bool operator()(Int n_from, Int n_to)
   {
      if (visited.contains(n_to)) return false;

      // we flip the start-triangulation T(0) to the triangulation T(n_to) corresponding to node n_to
      dcel.flipEdges(flipIds_to_node[n_to]);

      // calculate the secondary cone of triangulation n_to
      BigObject p("polytope::Polytope<Rational>",
                  "INEQUALITIES", dcel.DelaunayInequalities());
      IncidenceMatrix<> rays_in_facets = p.give("VERTICES_IN_FACETS");
      Matrix<Rational> rays = p.give("VERTICES");
      Matrix<Rational> facets = p.give("FACETS");

      // We compute a point outside each valid facet of the n_to-cone, the parameter 0 < epsilon < 1 is the distance of the point to its corresponding facet.
      // The point shall be contained in a top-dimensional cone that meets the n_to-cone in this facet.
      // If this is not the case, we consider a new point with distance epsilon^2 and try again
      for (Int i = 0 ; i < facets.rows() ; ++i) {
         flip_sequence new_flips{};

         // we store the facets at the coordinate hyperplanes for purposes of the fan estension to a complete fan
         if (dcel.nonZeros(facets[i]) == 1 && facets[i][0] == 0) {
            Set<Int> boundary_rays_ids;
            for (const auto it : rays_in_facets[i]) {
               if (rays[it][0] == 0) {
                  boundary_rays_ids += fan_vertices[ dcel.normalize(rays[it]) ]-1;
               }
            }
            boundary_facets.push_back(boundary_rays_ids);
         }

         if (dcel.validFacet(facets[i])) {
            Set<Vector<Rational>> facet_rays;
            for (const auto it : rays_in_facets[i]) {
               facet_rays += dcel.normalize(rays[it]);
            }

            Cone new_cone{};
            Rational epsilon{1,10};
            bool cone_is_neighbor = false;
            while (!cone_is_neighbor) {
               Vector<Rational> neighbor_point = neighborConePoint(facets[i], facet_rays, epsilon);

               // we use the flip algorithm to determine a flip sequence that makes the triangulation Delaunay w.r.t. the weights given by neighbor_point
               new_flips = dcel.flipToDelaunayAlt(neighbor_point);
               // calculate cone,  and check if really neighbored in facet[i]; if not  take epsilon^2 and start over
               new_cone = dcel.coneRays();
               if (incl(facet_rays, new_cone) == -1) {
                  cone_is_neighbor = true;
               } else {
                  dcel.flipEdges(new_flips, true);
                  epsilon = epsilon * epsilon;
               }
            }
            // add a new node to the graph and save all the corresponding data ( flip sequence, add_cone, cones )
            if (!cones.exists(new_cone) && new_cone.size() > dim-1) {
               const Int new_id = delaunay_graph.add_node();
               delaunay_graph.add_edge(n_to, new_id);

               flip_sequence new_flipIds{ flipIds_to_node[n_to] };
               new_flipIds.insert(new_flipIds.end(), new_flips.begin(), new_flips.end());
               flipIds_to_node[new_id] = new_flipIds;

               cones[new_cone] = new_id;
               add_cone(new_cone);
            }
         }
         // flip back to T(n_to)
         dcel.flipEdges(new_flips, true);
      }

      // flip back to T(0)
      dcel.flipEdges( flipIds_to_node[n_to] , true );

      visited += n_to;
      return true;
   }


   // when adding a cone we update the input data for the fan, namely the vertices and the maximal cells

   void add_cone(Cone new_cone)
   {
      Set<Int> fan_cell;
      for (const auto& it : new_cone) {
         // case: the vertex is new
         if (!fan_vertices.exists(it)) {
            fan_vertices[it] = fan_num_vert;
            fan_cell += fan_num_vert-1; // the -1 is an index shift, the fan_cells do not consider vertex 0 with index 0 and we relabel the verticesby -1
            ++fan_num_vert;
         }
         // case: the vertex is already known by some previous cone
         else {
            if (fan_vertices[it] != 0) fan_cell += fan_vertices[it]-1;
         }
      }
      fan_cells.push_back(fan_cell);
   }

   Int getfan_num_vert() const
   {
      return fan_num_vert;
   }

   Int getdim() const
   {
      return dim;
   }

   const Fan_Max_Cells& getfan_cells() const
   {
      return fan_cells;
   }

   const Fan_Vertices& getfan_vertices() const
   {
      return fan_vertices;
   }

   const Fan_Max_Cells& getboundary_facets() const
   {
      return boundary_facets;
   }

   const Map<Int, flip_sequence>& getflipIds_to_node() const
   {
      return flipIds_to_node;
   }


   // Given a facet of a 0-pointed cone via its inner normal vector & a set of the rays of this facet we return a point outside the cone near to the facet

   Vector<Rational> neighborConePoint(const Vector<Rational>& facet_normal, const Set<Vector<Rational>>& facet_vertices, const Rational& epsilon)
   {
      Rational eps(epsilon);
      Vector<Rational> point(dim);
      Vector<Rational> sum(dim);
      for (const auto& it : facet_vertices) {
         if (it[0] == 0) sum += it;
      }
      bool positive = false;
      do {
         point = 1/eps * sum;
         point = point - eps * facet_normal;
         positive = true;
         for (Int i = 1; i < point.size(); ++i) {
            if (point[i] <= 0) {
               positive = false;
               eps = eps*eps;
               break;
            }
         }
      } while (!positive);
      return point;
   }

   friend std::pair<Matrix<Rational>, Array<Set<Int>>> DCEL_secondary_fan_input(DoublyConnectedEdgeList& dcel);
   friend Indexed_Cones DCEL_secondary_fan(DoublyConnectedEdgeList& dcel);
   friend Matrix<Rational> DCEL_secondary_fan_input_vertices(DoublyConnectedEdgeList& dcel);
   friend Array<Set<Int>> DCEL_secondary_fan_input_cells(DoublyConnectedEdgeList& dcel);

}; // end class flip visitor

// the flip algorithm, we flip edges that are non-Delaunay w.r.t. the weights as long as there are some

FlipVisitor::flip_sequence flipToDelaunay(graph::DoublyConnectedEdgeList& dcel, const Vector<Rational>& weights)
{
   FlipVisitor::flip_sequence flip_ids{};
   Int non_delaunay = dcel.is_Delaunay(weights);
   while (non_delaunay != -1) {
      dcel.flipEdge(non_delaunay);
      flip_ids.push_back(non_delaunay);
      non_delaunay = dcel.is_Delaunay(weights);
   }
   return flip_ids;
}

} //end topaz namespace
} //end polymake namespace

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

