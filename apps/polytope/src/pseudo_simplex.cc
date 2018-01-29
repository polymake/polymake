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
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Scalar>
void pseudo_simplex(perl::Object p, perl::Object lp, bool maximize)
{
   const Graph<> G=p.give("GRAPH.ADJACENCY");
   NodeMap<Undirected,bool> visited(G,false);

   const Vector<Scalar> objective=lp.give("LINEAR_OBJECTIVE");
   const Set<int> far_face=p.give("FAR_FACE");

   const Matrix<Scalar> V=p.give("VERTICES");
   const int n_vertices = V.rows();

   // start with an affine point
   int current_vertex=(sequence(0,n_vertices)-far_face).front();

   Scalar opt=objective * V[current_vertex];
   Set<int> optimal_face=scalar2set(current_vertex);
   visited[current_vertex]=true;

   bool better, unbounded=false;
   const int sense(maximize ? 1 : -1);

   do {
      better = false;
      // steepest ascent/descent
      for (Entire<Graph<>::out_edge_list>::const_iterator v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
         const int neighbor=v.to_node();
         if (visited[neighbor])  // this neighbor can't be better
            continue;
         if (!is_zero(V(neighbor,0))) {
            visited[neighbor]=true;
            const Scalar value = objective * V[neighbor];
            int diff=sign(value - opt);
            if (diff == sense) { // this one is better
               current_vertex = neighbor;
               opt = value;
               optimal_face = scalar2set(current_vertex);
               better = true;
            } else if (diff==0) { // also belongs to optimal_face
               optimal_face += neighbor;
            }
         } else { // check for an increasing/decreasing unbounded edge
            if (sign(objective * V[neighbor]) == sense) {
               unbounded = true;
               break;
            }
         }
      }
   } while (better && !unbounded);

   if (!unbounded) {
      // linear program is bounded, look for the entire optimal face
      std::list<int> optimal_vertices(optimal_face.begin(), optimal_face.end());

      while (!optimal_vertices.empty()) {
         current_vertex = optimal_vertices.front();
         optimal_vertices.pop_front();

         for (Entire<Graph<>::out_edge_list>::const_iterator v=entire(G.out_edges(current_vertex));  !v.at_end();  ++v) {
            const int neighbor=v.to_node();
            if (!visited[neighbor]) {
               visited[neighbor]=true;
               if (!is_zero(V(neighbor,0)) && objective * V[neighbor] == opt) {
                  optimal_face += neighbor;
                  optimal_vertices.push_back(neighbor);
               }
            }
         }
      }

      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << opt;

   } else {
      // linear program is unbounded
      // nonetheless: the optimal face is the subset of the far face
      optimal_face.clear();
      for (Entire< Set<int> >::const_iterator v=entire(far_face); !v.at_end(); ++v) {
         if (sign(objective * V[*v]) == sense)
            optimal_face.push_back(*v);
      }

      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
   }

   lp.take(maximize ? "MAXIMAL_FACE" : "MINIMAL_FACE") << optimal_face;
}

FunctionTemplate4perl("pseudo_simplex<Scalar> (Polytope<Scalar>, LinearProgram<Scalar>, $) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
