/* Copyright (c) 1997-2023
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
#include "polymake/Set.h"
#include "polymake/Graph.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/list"
#include "polymake/vector"
#include <cmath>

namespace polymake { namespace graph {
namespace {

template <typename Coords> inline
Coords max_norm(const Matrix<Coords>& V, Int i, Int j)
{
   return accumulate(attach_operation(V[i]-V[j], operations::abs_value()), operations::max());
}
   
template <typename Coords> inline
Coords square_norm(const Matrix<Coords>& V, Int i, Int j)
{
   return static_cast<Coords>(std::sqrt(double(sqr(V[i] - V[j]))));
}
}


template <typename Coords>
ListReturn tentacle_graph(const Array<Int>& tentacles, const Matrix<Coords>& metric)
{
   const Int n_nodes = tentacles.size();
   Graph<> G(n_nodes);
   EdgeMap<Undirected,Coords> weights(G);
   for (Int i = 1; i < n_nodes; ++i)
      for (Int j = 0; j < i; ++j)
         weights(i, j) = metric(tentacles[i], tentacles[j]);

   ListReturn results;
   results << G << weights;
   return results;
}


template <typename Coords>
Matrix<Coords> bounded_embedder(const Graph<>& BG, const Matrix<Coords>& V, const Set<Int>& far_face,
                                const Array<Int>& fixed_nodes, const Matrix<Coords>& fixed_coord,
                                bool use_max_norm)
{
   const Int n_nodes = BG.nodes() + far_face.size();
   Matrix<Coords> GR(n_nodes,3);
   const Int n_fixed_nodes = fixed_nodes.size();
   if (n_fixed_nodes < 3)
      throw std::runtime_error("bounded_embedder: Less than three fixed nodes.");

   GR.minor(fixed_nodes,All) = fixed_coord;

#if POLYMAKE_DEBUG
   const bool debug_print = get_debug_level() > 1;
#endif

   // compute distances according to metric chosen

   Coords scale=zero_value<Coords>();
   if (use_max_norm) {
      for (auto fix1 = entire(fixed_nodes); !fix1.at_end(); ++fix1) {
         auto fix2 = fix1;
         while (!(++fix2).at_end())
            scale += max_norm(V,*fix1,*fix2)/square_norm(GR,*fix1,*fix2);
      }
   }
   else {
      for (auto fix1 = entire(fixed_nodes); !fix1.at_end(); ++fix1) {
         auto fix2 = fix1;
         while (!(++fix2).at_end())
            scale += square_norm(V,*fix1,*fix2)/square_norm(GR,*fix1,*fix2);
      }
   }
   scale /= double(n_fixed_nodes)*(double(n_fixed_nodes)-1)/2;

   EdgeMap<Undirected, Coords> BGmap(BG);   
   for (auto e=entire(edges(BG)); !e.at_end(); ++e)
      BGmap[*e] = use_max_norm ? max_norm(V, e.from_node(), e.to_node())/scale :  square_norm(V, e.from_node(), e.to_node())/scale;

#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "Vertices:\n" << V << "\n"
              "Graph:\n" << BG << "\n"
              "EdgeMap:\n " << BGmap << " \n "
              "fixed_nodes:\n" << GR.minor(fixed_nodes,All) << "\n"
              "scale: " << scale << endl;
   }
#endif
   
   // compute coordinates of remaining nodes
   const Set<Int> inner_nodes = sequence(0, n_nodes) - Set<Int>(entire(fixed_nodes)) - far_face;
   const Int n_inner_nodes = inner_nodes.size();
   Matrix<Coords> stress_matrix(n_inner_nodes, n_nodes);
   Matrix<Coords> rhs(n_inner_nodes,3);
   
   Int m = 0;
   for (auto n = entire(inner_nodes); !n.at_end(); ++n,++m) {
      for (auto e = entire(BG.out_edges(*n)); !e.at_end(); ++e) {
         stress_matrix(m,*n) += 1/BGmap[*e];  // spring constant
         const Int nn = e.to_node();
         if ( inner_nodes.contains(nn) )
            stress_matrix(m,nn) = -1/BGmap[*e];  // spring constant
         else
            rhs[m] += GR[nn]/BGmap[*e];
      }
   }

   GR.minor(inner_nodes,All) = inv(stress_matrix.minor(All,inner_nodes)) * rhs;
#if POLYMAKE_DEBUG
   if (debug_print) {
      cout << "inner_nodes: " << inner_nodes << "\n"
              "stress_matrix:\n" << stress_matrix << "\n"
              "rhs:\n" << rhs << "\n"
              "stress_matrix_inv:\n" << inv(stress_matrix.minor(All,inner_nodes)) << "\n"
              "result:\n" << GR << endl;
   }
#endif

   return GR.minor(~far_face,All);
}

FunctionTemplate4perl("bounded_embedder($ Matrix $$ Matrix; $=1)");
FunctionTemplate4perl("tentacle_graph($ Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
