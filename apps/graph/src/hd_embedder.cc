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
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/RandomSubset.h"
#include <cmath>

namespace polymake { namespace graph {

class HDEmbedder {
protected:
   const HasseDiagram& HD;
   const Vector<double>& label_width;
   int dim;
   typedef std::vector< std::vector<int> > layer_vector;
   layer_vector layers;
   double total_width, epsilon;
   Vector<double> node_x, weights, nb_x_sum, width_in_layer;

public:
   HDEmbedder(const HasseDiagram& HD_arg, const Vector<double>& label_width_arg)
      : HD(HD_arg), label_width(label_width_arg),
        dim(HD.dim()), layers(dim), node_x(HD.nodes()), weights(dim+1), nb_x_sum(HD.nodes()), width_in_layer(dim) {}

   Matrix<double> compute(const perl::OptionSet& options);

private:
   void init(const perl::OptionSet& options);

   int try_move_node(std::vector<int>::iterator n,
                     const std::vector<int>::iterator& n_first, const std::vector<int>::iterator& n_last,
                     const double delta, const double *w);

   inline void adjust_x(int node, const double x, const double *w);
   inline bool good_swap(int lnode, int rnode, const double delta, const double *w) const;
};

void HDEmbedder::init(const perl::OptionSet& options)
{
   options["eps"] >> epsilon;
   const RandomSeed seed(options["seed"]);
   UniformlyRandom<long> random(seed);

   total_width=label_width[0];
   // randomly toss the nodes in each layer
   layer_vector::iterator l=layers.begin();
   Vector<double>::iterator wd=width_in_layer.begin();
   for (int d=0; d<dim; ++d, ++l, ++wd) {
      const HasseDiagram::nodes_of_dim_set nodes=HD.nodes_of_dim(d);
      const int n=nodes.size();
      l->resize(n);
      copy(entire(random_permutation(nodes, random)), l->begin());
      accumulate_in(entire(select(label_width, nodes)), operations::max(), *wd);
      assign_max(total_width, (*wd)*n);
   }

   // place the nodes equidistantly and symmetrically around x=0
   // weights[d]=nodes_of_dim(d).size()/nodes_of_dim(d-1).size()
   // weight for out_edges = weights[d+1]
   // weight for in_edges = 1/weights[d]

   Vector<double>::iterator wt=weights.begin();
   double prev_width=1;
   for (int d=0; d<dim; ++d, ++wt) {
      const int layer_width=HD.nodes_of_dim(d).size();
      const double width=total_width/layer_width;
      double x=(width-total_width)/2;
      for (Entire< std::vector<int> >::iterator n=entire(layers[d]); !n.at_end(); ++n, x+=width)
         node_x[*n]=x;
      *wt=layer_width/prev_width;
      prev_width=layer_width;
   }
   *wt=1/prev_width;

   wt=weights.begin();
   for (int d=0; d<dim; ++d, ++wt) {
      for (Entire<HasseDiagram::nodes_of_dim_set>::const_iterator nodes=entire(HD.nodes_of_dim(d)); !nodes.at_end(); ++nodes) {
         const int n=*nodes;
         nb_x_sum[n]= accumulate(select(node_x, HD.out_adjacent_nodes(n)), operations::add())*wt[1] +
            accumulate(select(node_x, HD.in_adjacent_nodes(n)), operations::add())/wt[0];
      }
   }
}

Matrix<double> HDEmbedder::compute(const perl::OptionSet& options)
{
   init(options);
#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
#endif

   // try to decrease the total tension
   bool anyone_moved;
   do {
      anyone_moved=false;
      Vector<double>::const_iterator wd=width_in_layer.begin(), wt=weights.begin();
      for (Entire<layer_vector>::iterator l=entire(layers); !l.at_end(); ++l, ++wd, ++wt) {
         std::vector<int>::iterator n_first=l->begin(), n_last=l->end();
#if POLYMAKE_DEBUG
         if (debug_print) {
            cout << "layer " << (l-layers.begin()) << ":";
            for (std::vector<int>::iterator n=n_first; n!=n_last; ++n) {
               cout << ' ' << *n << '(' << node_x[*n] << ')';
            }
            cout << endl;
         }
#endif
         for (std::vector<int>::iterator n=n_first; n!=n_last; ++n) {
#if POLYMAKE_DEBUG
            const int node=*n;
#endif
            bool moved=try_move_node(n, n_first, n_last, *wd, &*wt);
            anyone_moved |= moved;
#if POLYMAKE_DEBUG
            if (moved)
               for (std::vector<int>::iterator n2=n_first+1; n2!=n_last; ++n2)
                  if (node_x[*n2]-node_x[n2[-1]]+epsilon < *wd)
                     cerr << "ERROR after moving " << node << ": dist(" << n2[-1] << "," << *n2 << ") < " << *wd << endl;
#endif
         }
      }
   } while (anyone_moved);

   Matrix<double> embedding(HD.nodes(), 2);
   const bool dual=options["dual"];
   double y=0;
   for (int d=(dual ? -1 : 0), d_step=(dual ? -1 : 1), d_last=(dual ? -dim-1 : dim);
        d!=d_last;  d+=d_step, y+=1) {
      for (Entire<HasseDiagram::nodes_of_dim_set>::const_iterator n=entire(HD.nodes_of_dim(d)); !n.at_end(); ++n) {
         embedding(*n,0)=node_x[*n];
         embedding(*n,1)=y;
      }
   }
   embedding(HD.top_node(), 1)=(dual ? -1 : y);
   embedding(HD.bottom_node(), 1)=(dual ? y : -1);
   return embedding;
}

int HDEmbedder::try_move_node(std::vector<int>::iterator n,
                              const std::vector<int>::iterator& n_first, const std::vector<int>::iterator& n_last,
                              const double delta, const double* wt)
{
   int node=*n;
   int moved=0;
   double x_min=nb_x_sum[node]/(HD.out_degree(node)*wt[1]+HD.in_degree(node)/wt[0]),
      diff_to_min=node_x[node]-x_min;
   if (diff_to_min>epsilon) {
      // try moving to the left
      while (true) {
         if (n--==n_first) {
            // the leftmost node, no neighbors to spring over
            assert(x_min >= -total_width/2-epsilon);
         } else {
            const int node2=*n;
            if (node_x[node2]+delta > x_min) {
               // gap between the neighbors is not large enough
               if (good_swap(node2, node, delta, wt)) {
                  adjust_x(node2, node_x[node2]+delta, wt);
                  n[1]=node2;
                  moved=1;
                  continue;
               } else {
                  if (moved || node_x[node2]+delta+epsilon < node_x[node]) {
                     adjust_x(node, node_x[node2]+delta, wt);
                     if (moved++) n[1]=node;
                  }
                  break;
               }
            }
         }
         if (moved) {
            n[1]=node;
            assign_min(x_min, node_x[n[2]]-delta);
         }
         adjust_x(node, x_min, wt);
         return true;
      }

   } else if (diff_to_min<-epsilon) {
      // try moving to the right
      while (true) {
         if (++n == n_last) {
            // the rightmost mode
            assert(x_min <= total_width/2+epsilon);
         } else {
            const int node2=*n;
            if (node_x[node2]-delta < x_min) {
               // gap between the neighbors is not large enough
               if (good_swap(node2, node, -delta, wt)) {
                  adjust_x(node2, node_x[node2]-delta, wt);
                  n[-1]=node2;
                  moved=1;
                  continue;
               } else {
                  if (moved || node_x[node2]-delta-epsilon > node_x[node]) {
                     adjust_x(node, node_x[node2]-delta, wt);
                     if (moved++) n[-1]=node;
                  }
                  break;
               }
            }
         }
         if (moved) {
            n[-1]=node;
            assign_max(x_min, node_x[n[-2]]+delta);
         }
         adjust_x(node, x_min, wt);
         return true;
      }
   }

   return moved;
}

void HDEmbedder::adjust_x(int node, const double x, const double* wt)
{
   const double dx=x-node_x[node];
   node_x[node]=x;
   for (Entire<Graph<Directed>::out_adjacent_node_list>::const_iterator nb=entire(HD.out_adjacent_nodes(node));
        !nb.at_end(); ++nb)
      nb_x_sum[*nb]+=dx/wt[1];
   for (Entire<Graph<Directed>::in_adjacent_node_list>::const_iterator nb=entire(HD.in_adjacent_nodes(node));
        !nb.at_end(); ++nb)
      nb_x_sum[*nb]+=dx*wt[0];
}

bool HDEmbedder::good_swap(int lnode, int rnode, const double delta, const double* wt) const
{
   return delta*( (delta+2*node_x[lnode])*( (HD.out_degree(lnode)-HD.out_degree(rnode))*wt[1] +
                                            (HD.in_degree(lnode)-HD.in_degree(rnode))/wt[0] )
                  - 2*(nb_x_sum[lnode]-nb_x_sum[rnode])) < 0;
}

Matrix<double> hd_embedder(perl::Object HD_obj, const Vector<double>& label_width, perl::OptionSet options)
{
   const HasseDiagram HD(HD_obj);
   HDEmbedder HDE(HD, label_width);
   return HDE.compute(options);
}

UserFunction4perl("# @category Visualization"
                  "# Create an embedding of the Hasse diagram as a layered graph."
                  "# The embedding algorithm tries to minimize the weighted sum of squares of edge lengths,"
                  "# starting from a random distribution. The weights are relative to the fatness of the layers."
                  "# The y-space between the layers is constant."
                  "# @param Array label_width estimates (better upper bounds) of the label width of each node."
                  "# The computed layout guarantees that the distances between the nodes in a layer are at least equal to"
                  "# the widest label in this layer."
                  "# @option Bool dual  the node representing the empty face is put on the topmost level"
                  "# @option Float eps  calculation accuracy."
                  "# @option Int seed  effects the initial placement of the nodes.",
                  &hd_embedder, "hd_embedder(FaceLattice $ { dual => undef, eps => 1e-4, seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
