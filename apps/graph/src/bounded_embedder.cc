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
Coords max_norm(const Matrix<Coords>& V, int i, int j)
{
   return accumulate(attach_operation(V[i]-V[j], operations::abs_value()), operations::max());
}
   
template <typename Coords> inline
Coords square_norm(const Matrix<Coords>& V, int i, int j)
{
   return static_cast<Coords>(std::sqrt(double(sqr(V[i]-V[j]))));
}
}


template <typename Coords>
perl::ListReturn tentacle_graph(const Array<int>& tentacles, const Matrix<Coords>& metric)
{
   const int n_nodes = tentacles.size();
   Graph<> G(n_nodes);
   EdgeMap<Undirected,Coords> weights(G);
   for (int i=1; i<n_nodes; ++i)
      for (int j=0; j<i; ++j)
         weights(i,j)=metric(tentacles[i],tentacles[j]);

   perl::ListReturn results;
   results << G << weights;
   return results;
}


template <typename Coords>
Matrix<Coords> bounded_embedder(const Graph<>& BG, const Matrix<Coords>& V, const Set<int>& far_face,
                                const Array<int>& fixed_nodes, const Matrix<Coords>& fixed_coord,
                                bool use_max_norm)
{
   const int n_nodes = BG.nodes()+far_face.size();
   Matrix<Coords> GR(n_nodes,3);
   const int n_fixed_nodes=fixed_nodes.size();
   if (n_fixed_nodes<3)
      throw std::runtime_error("bounded_embedder: Less than three fixed nodes.");

   GR.minor(fixed_nodes,All) = fixed_coord;

#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
#endif

   // compute distances according to metric chosen

   Coords scale=zero_value<Coords>();
   if (use_max_norm) {
      for (Entire< Array<int> >::const_iterator fix1=entire(fixed_nodes); !fix1.at_end(); ++fix1) {
         Entire< Array<int> >::const_iterator fix2=fix1;
         while (!(++fix2).at_end())
            scale += max_norm(V,*fix1,*fix2)/square_norm(GR,*fix1,*fix2);
      }
   }
   else {
      for (Entire< Array<int> >::const_iterator fix1=entire(fixed_nodes); !fix1.at_end(); ++fix1) {
         Entire< Array<int> >::const_iterator fix2=fix1;
         while (!(++fix2).at_end())
            scale += square_norm(V,*fix1,*fix2)/square_norm(GR,*fix1,*fix2);
      }
   }
   scale /= n_fixed_nodes*(n_fixed_nodes-1)/2;

   EdgeMap<Undirected, Coords> BGmap(BG);   
   for (Entire<Edges< Graph<> > >::const_iterator e=entire(edges(BG)); !e.at_end(); ++e)
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
   const Set<int> inner_nodes = sequence(0,n_nodes) - Set<int>(entire(fixed_nodes))-far_face;
   const int n_inner_nodes = inner_nodes.size();
   Matrix<Coords> stress_matrix(n_inner_nodes, n_nodes);
   Matrix<Coords> rhs(n_inner_nodes,3);
   
   int m=0;
   for (Entire< Set <int> >::const_iterator n=entire(inner_nodes); !n.at_end(); ++n,++m) {
      for (Entire<Graph<>::out_edge_list>::const_iterator e=entire(BG.out_edges(*n)); !e.at_end(); ++e) {
         stress_matrix(m,*n) += 1/BGmap[*e];  // spring constant
         const int nn = e.to_node();
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
