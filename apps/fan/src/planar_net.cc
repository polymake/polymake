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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/Map.h"
#include "polymake/hash_set"
#include "polymake/list"
#include <cmath>

/* The overall idea is to produce a planar net of a 3-polytope from one spanning tree in the dual graph.
   Once the spanning tree is chosen and the root facet is drawn the layout is uniquely determined.
   It is not known if such a planar net always exists.

   The spanning tree is determined as follows.  Starting at the root facet we perform a breadth first search
   through the dual graph.  New facets are queued in the order in which they appear.  More precisely, the queue
   members are actually edges, such that one incident facet is already in the layout, while the other one is not.
   The current heuristic greedily places a facet if it does not overlap with the existing layout.  There is no
   backtracking (yet).  In our very many experiments so far, such a backtracking was never necessary.

   The current implementation always takes the facet numbered 0 as the root.

   This client is meant to be used by non-experts, too.  Therefore, there are more double checks than usual,
   and the code throws (hopefully) meaningful exceptions if something goes wrong.
*/

#define PLANAR_NET_DEBUG 0

namespace polymake { namespace fan {

namespace {

   typedef std::pair<int,int> directed_edge;
   typedef std::pair<int,int> vertex_facet_pair;

   // a tree is stored as a sequence of directed edges; works primally and dually
   typedef std::list<directed_edge> tree_type;

   // EXACT COMPUTATIONS (if Coord is an exact type)

   template <typename Coord>
   Vector<Coord> barycentric(const Vector<Coord>& x, const Matrix<Coord>& W) {
      /* x: point in d+1 homogeneous coordinates
         W: dx(d+1)-matrix of d points in homogeneous coordinates (linearly independent)
         leading 1 used for homogenization in both cases
         assumption: x is contained in the affine hull of the rows of W
         returns coefficients such that x is written as a linear combination of the rows of W, which sum to 1
      */
      // FIXME #867 copy operation because lin_solve<double> needs Matrix and not GenericMatrix
      return lin_solve(Matrix<Coord>(T(W)),x);
   }
   
   // find the orientation of the pair (a,b) induced by the cycle
   // a and b must be adjacent nodes in the cycle;
   // in the end the index points to a
   void determine_directed_edge(int& a, int& b, const Array<int>& cycle, int& index) {
      index=0;
      for ( ; cycle[index] != a; ++index); // now: cycle[index] == a
      int j=index+1;
      if ((j < cycle.size() && cycle[j] != b) || (j == cycle.size() && cycle[0] != b)) {
         std::swap(a,b);
         if (index > 0) {
            --index;
         } else {
            index=cycle.size()-1;
         }
      }
   }

   // create a set consisting of two elements
   Set<int> two_elements(const int a, const int b) {
      Set<int> S(a);
      S.insert(b);
      return S;
   }

   // INEXACT COMPUTATIONS
   // This can't be avoided (easily) as square roots and trigonometric functions are involved.

   // we try to compute exactly as long as we can
   template <typename Coord>
   inline double dist(const Vector<Coord>& x, const Vector<Coord>& y) {
      return sqrt(convert_to<double>(sqr(x-y)));
   }

   inline
   double norm(const Vector<double>& v) {
      return sqrt(pm::operators::sqr(v));
   }

   // counter clock-wise orientation of three affine points
   inline
   int ccw(const Vector<double>& a, const Vector<double>& b, const Vector<double>& c) {
      const double det_by_laplace=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
      return sign(det_by_laplace);
   }

   inline
   bool proper_segment_intersection(const Vector<double>& a, const Vector<double>& b,
                                    const Vector<double>& c, const Vector<double>& d) {
      return (ccw(a,b,c) * ccw(a,b,d) < 0) && (ccw(c,d,a) * ccw(c,d,b) < 0);
   }

   template <typename Coord>
   int overlap(const Vector<double>& point, const Vector<double>& previous_point,
               const Array< Array<int> >& vif, const Set<int>& marked, const Matrix<double>& net_vertices, const Map<vertex_facet_pair,int>& vf_map) {
      /* Check each facet h in the layout so far. */
      for (Entire< Set<int> >::const_iterator h=entire(marked); !h.at_end(); ++h) {
         /* There are two types of overlaps:
            (1) The point to be placed could be contained in the facet h ... */
         bool h_contained=true;
         Entire< Array<int> >::const_iterator iv=entire(vif[*h]);
         const int first_vertex = vf_map[vertex_facet_pair(*iv,*h)];
         Vector<double> pvh = net_vertices[first_vertex];
         Vector<double> nvh;
         // check each edge in h
         for (++iv; !iv.at_end(); ++iv) {
            nvh = net_vertices[vf_map[vertex_facet_pair(*iv,*h)]];
            if (ccw(pvh,nvh,point)<0) {
               // the point is on the negative side of that edge, hence ...
               h_contained=false;
               break;
            }
#if PLANAR_NET_DEBUG > 2
            cerr << " * h=" << *h << " iv=" << *iv << " contained=" << h_contained;
#endif
            pvh = nvh;
         }
         nvh = net_vertices[first_vertex];
         if (h_contained) {
            // check initial edge; pvh is now the last vertex on the facet h
            if (ccw(pvh,nvh,point)<0)
               h_contained=false;
#if PLANAR_NET_DEBUG > 2
            cerr << " * h=" << *h << " first_vertex=" << first_vertex << " contained=" << h_contained;
#endif
         }
         if (h_contained) {
#if PLANAR_NET_DEBUG > 2
            cerr << endl;
#endif
            return *h; // overlap with that facet
         }

         /*  (2) ... or two edges intersect.*/
         // check each edge in h
         for (iv=entire(vif[*h]); !iv.at_end(); ++iv) {
            nvh = net_vertices[vf_map[vertex_facet_pair(*iv,*h)]];
            if (proper_segment_intersection(previous_point,point, pvh,nvh)) {
#if PLANAR_NET_DEBUG > 2
               cerr << " * h=" << *h << " iv" << *iv << " intersection" << endl;
#endif
               return *h;
            }
         }
      }
      return -1; // OK, no overlap
   }

   template <typename Coord>
   bool layout_one_facet(const int f, // index of facet to be processed
                         const int ja, const int jb, // (indices of) points in the plane where layout starts
                         const std::list<int>& vertex_cycle, // indices of vertices on the facet in cyclic order starting at a and b
                         const Matrix<Coord>& V, // vertices of the 3-polytope
                         const Array< Array<int> >& vif, // vertices in facets, cyclically ordered
                         Set<int>& marked, // facets already layed out; f will be added if successful
                         int& cv, // number of vertices of the net already computed
                         Matrix<double>& net_vertices, // vertices of the net
                         Array< Set<int> >& net_facets, // facets of the net
                         Map<vertex_facet_pair,int>& vf_map, // which vertex on which facet ends up where
                         Array<int>& vf_map_inv // inverse of the previous
                         ) {
      std::list<int>::const_iterator vc_it=vertex_cycle.begin();

      const int ix=*vc_it; ++vc_it;
      const int iy=*vc_it; ++vc_it;
      const int iz=*vc_it;
      std::list<int> FirstThreeOnFacet;
      FirstThreeOnFacet.push_back(ix); FirstThreeOnFacet.push_back(iy); FirstThreeOnFacet.push_back(iz);

      Vector<double> // dehomogenize and subtract
         u = Vector<double>(V[ix].slice(1) - V[iy].slice(1)),
         v = Vector<double>(V[iz].slice(1) - V[iy].slice(1));
      double
         len_u=norm(u), len_v=norm(v);
      
      u /= len_u; v /= len_v; // scaling to unit lengths
      double beta=M_PI-acos(u*v); // exterior angle
      
      const Vector<double>&
         a(net_vertices[ja]),
         b(net_vertices[jb]),
         vec_ab(b-a);

      // scalar product of (b-a)/norm(b-a) and (1,0)
      double scp = vec_ab[0]/(norm(vec_ab));
      // beware of numerical atrocities
      if (scp>1.0) scp=1.0; else if (scp<-1.0) scp=-1.0;

      const double // angle between b-a and the horizontal axis
         alpha = vec_ab[1]>=0 ? acos(scp) : -acos(scp);

      Vector<double> c(b);
      c[0] += len_v * cos(alpha+beta);
      c[1] += len_v * sin(alpha+beta);

#if PLANAR_NET_DEBUG
      cerr << "layout of facet " << f
           << ": len_u=" << len_u << " beta=" << beta << " a=" << a << " b=" << b
           << " alpha=" << alpha << " c=" << c << endl;
#endif

      const int initial_cv=cv; // remember this for possible rollback later

      Set<int> this_net_facet; this_net_facet += ja; this_net_facet += jb;
      Vector<double> previous_vertex(net_vertices[jb]); // right vertex on the connecting edge
      for ( ; vc_it!=vertex_cycle.end(); ++vc_it) {
         const int idx=*vc_it;
         const Vector<double>
            bc(barycentric<Coord>(V[idx],V.minor(FirstThreeOnFacet,All))),
            next_vertex(bc[0]*a + bc[1]*b + bc[2]*c);
         
         /* If some facet contains the point next_vertex in its interior or in its boundary we need a rollback.
            Negative return value signals no overlap, otherwise index of overlapping facet. */
         const int h=overlap<Coord>(next_vertex,previous_vertex,vif,marked,net_vertices,vf_map);
         if (h>=0) {
#if PLANAR_NET_DEBUG > 1
            cerr << " next_vertex=" << next_vertex << " interferes with facet " << h << endl;
            const Array<int>& vif_h=vif[h];
            for (Entire< Array<int> >::const_iterator v=entire(vif_h); !v.at_end(); ++v)
               cerr << " " << *v << "/" << vf_map[vertex_facet_pair(*v,h)] << ": " << net_vertices[vf_map[vertex_facet_pair(*v,h)]] << endl;
#endif
            cv=initial_cv;
            return false;
         }

         previous_vertex = net_vertices[cv] = next_vertex;
         this_net_facet += cv;
         vf_map[vertex_facet_pair(idx,f)] = cv;  vf_map_inv[cv] = idx;
         ++cv;
      }
      net_facets[f] = this_net_facet;
      marked.insert(f);
      return true;
   }

   // COMBINATORIAL

   void queue_neighbors(const int f, const Graph<>& DG, const Set<int>& marked, std::list<directed_edge>& unprocessed_edges) {
      const Set<int> neighbors = DG.adjacent_nodes(f);  
      for (Entire< Set<int> >::const_iterator n_it = entire(neighbors); !n_it.at_end(); ++n_it)
         if (!marked.contains(*n_it))
            unprocessed_edges.push_back(directed_edge(f,*n_it));
   }
   
}

template <typename Coord>
perl::Object planar_net(perl::Object p)
{
   const Matrix<Coord> V=p.give("VERTICES");
   const int d=p.give("CONE_DIM");
   
   if (V.cols() != 4 || d!=4)
      throw std::runtime_error("planar_net: requires full-dimensional 3-polytope");

   const Array< Array<int> > vif=p.give("VIF_CYCLIC_NORMAL");
   /* The following is also the number of maximal cells of the planar net.
      Throughout we keep the labeling of the facets the same. */
   const int n_facets = vif.size(); 
   Array< Set<int> > net_facets(n_facets);

   /* Total number of vertices in the planar net = 2 * (f_1 - f_2 + 1);
      since each edge is drawn twice, except for those in a spanning tree
      of the dual graph.  Euler yields f_1 - f_2 + 1 = f_0 - 1. */
   const int n_net_vertices = 2*(V.rows()-1);
   Matrix<double> net_vertices(n_net_vertices,2); // Euclidean

   // Recalls which vertex in which facet of the 3-polytope has which row index in the matrix above.
   Map<vertex_facet_pair,int> vf_map;
   Array<int> vf_map_inv(n_net_vertices);

   const Graph<> DG=p.give("DUAL_GRAPH.ADJACENCY");

   int f=0; // some arbitrary facet, plays the root of the spanning tree in the dual graph
   tree_type dual_tree;

   int
      ia=vif[f][0], // indices of first ...
      ib=vif[f][1]; // ... and second vertex for the layout

   int ja=0, jb=1;

   // true edge length in 3-space
   double unit_length=dist<Coord>(V[ia],V[ib]);

   const Vector<double>
      a = zero_vector<double>(2), // first vertex gets mapped to origin
      b = unit_length * unit_vector<double>(2,0); // second vertex at proper distance
   net_vertices[ja]=a;
   net_vertices[jb]=b;
   int cv=2;

   // keep track where those vertices end up
   vf_map[vertex_facet_pair(ia,f)] = ja;  vf_map_inv[ja] = ia;
   vf_map[vertex_facet_pair(ib,f)] = jb;  vf_map_inv[jb] = ib;

   std::list<int> f_vertex_cycle;
   for (int i=0; i<vif[f].size(); ++i) f_vertex_cycle.push_back(vif[f][i]);

   Set<int> marked;  // no facets processed yet; start with the root facet
   layout_one_facet(f, ja, jb, f_vertex_cycle, V, vif, marked, cv, net_vertices, net_facets, vf_map, vf_map_inv);

   std::list<directed_edge> unprocessed_edges;

#if PLANAR_NET_DEBUG
   cerr << "CONSTRUCTING LAYOUT VIA DUAL TREE\n"
        << "root facet f=" << f << " {" << vif[f] << "}"
        << " ia=" << ia << " ib=" << ib
        << " ja=" << ja << " jb=" << jb
        << " marked=" << marked << " #=" << marked.size()
        << " cv=" << cv << " vf_map=" << vf_map << endl;
#endif

   queue_neighbors(f,DG,marked,unprocessed_edges);

   do {
      // pick next edge, but beware that in the mean time both facets could have been processed
      int g;
      bool found=false; // no suitable g found yet
#if PLANAR_NET_DEBUG > 1
      cerr << "searching facet pair";
#endif
      while ( !unprocessed_edges.empty() ) {
         const directed_edge fg_pair=unprocessed_edges.front();
         unprocessed_edges.pop_front();
#if PLANAR_NET_DEBUG > 1
         cerr << " (" << fg_pair << ")";
#endif
         g=fg_pair.second;
         if (!marked.contains(g)) {
            f=fg_pair.first;
            found=true;
#if PLANAR_NET_DEBUG > 1
            cerr << " *" << endl;
#endif
            break;
         }
      }
      if (!found)
         throw std::runtime_error("planar_net: run out of edges, attempt failed"); 
      // now f has been processed, and its neighbor g is next

      const Array<int>& vif_g(vif[g]);

      const Set<int>
         f_vertex_set(vif[f]),
         g_vertex_set(vif_g),
         common_vertices(f_vertex_set * g_vertex_set);

      if (common_vertices.size() != 2)
         throw std::runtime_error("planar_net: dual edge does not correspond to a primal one");

      // the two common vertices
      ia=common_vertices.front();
      ib=common_vertices.back();
      
      int i;
      determine_directed_edge(ia,ib,vif_g,i);

      // vertex_cycle starts with first ia, then ib
      std::list<int> g_vertex_cycle;
      for (int k=i; k<vif_g.size(); ++k) g_vertex_cycle.push_back(vif_g[k]);
      for (int k=0; k<i; ++k) g_vertex_cycle.push_back(vif_g[k]);

      vf_map[vertex_facet_pair(ia,g)] = ja = vf_map[vertex_facet_pair(ia,f)];
      vf_map[vertex_facet_pair(ib,g)] = jb = vf_map[vertex_facet_pair(ib,f)];

#if PLANAR_NET_DEBUG
      cerr << "processing f=" << f << " g=" << g << " {" << vif_g << "}"
           << " ia=" << ia << " ib=" << ib
           << " ja=" << ja << " jb=" << jb
           << " marked=" << marked << " #=" << marked.size()
           << " cv=" << cv << " vf_map=" << vf_map
           << " dual_tree=" << dual_tree
           << endl;            
#endif

      if (layout_one_facet(g, ja, jb, g_vertex_cycle, V, vif, marked, cv, net_vertices, net_facets, vf_map, vf_map_inv)) {
         dual_tree.push_back(directed_edge(f,g));
         queue_neighbors(g,DG,marked,unprocessed_edges);
      }

   } while (cv < n_net_vertices);

   // compute the primal edges which need flaps
#if PLANAR_NET_DEBUG
   cerr << "COMPUTING FLAPS" << endl;
#endif
   hash_set< Set<int> > dual_tree_set;
   for (tree_type::iterator it=dual_tree.begin(); it!=dual_tree.end(); ++it) {
      dual_tree_set.insert(two_elements(it->first,it->second));
   }

   tree_type flaps;
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(DG));  !e.at_end();  ++e) {
      // flaps are directed from facet f to facet g, where f has the smaller index
      int f,g;
      if (e.from_node() < e.to_node()) {
         f=e.from_node(); g=e.to_node();
      } else {
         f=e.to_node(); g=e.from_node();
      }
#if PLANAR_NET_DEBUG
      cerr << "considering f=" << f << " g=" << g;
#endif
      if (dual_tree_set.exists(two_elements(f,g))) {
#if PLANAR_NET_DEBUG
         cerr << endl;
#endif
         continue;
      }

      const Array<int>& vif_f=vif[f];
      const Set<int>
         f_vertex_set(vif_f),
         g_vertex_set(vif[g]),
         common_vertices(f_vertex_set * g_vertex_set);
      int
         ia=common_vertices.front(),
         ib=common_vertices.back(),
         i; // not used here
#if PLANAR_NET_DEBUG
      cerr << " vif_f=" << vif_f << " ia=" << ia << " ib=" << ib << endl;
#endif
      determine_directed_edge(ia,ib,vif_f,i);
      // now ia comes before ib in the cyclic order of the vertices of the facet f
      flaps.push_back(directed_edge(vf_map[vertex_facet_pair(ia,f)],vf_map[vertex_facet_pair(ib,f)]));
   }

   perl::Object net("PlanarNet");
   net.set_description() << "planar net of " << p.name() << endl;

   net.take("VERTICES") << (ones_vector<double>(n_net_vertices) | net_vertices);
   net.take("MAXIMAL_POLYTOPES") << net_facets;

   net.take("VF_MAP") << vf_map;
   net.take("VF_MAP_INV") << vf_map_inv;
   net.take("DUAL_TREE") << Array<directed_edge>(dual_tree);
   net.take("FLAPS") << Array<directed_edge>(flaps);

   net.take("POLYTOPE") << p;

   return net;
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes a planar net of the 3-polytope //p//."
                          "# Note that it is an open problem if such a planar net always exists."
                          "# * PROGRAM MIGHT TERMINATE WITH AN EXCEPTION *"
                          "# If it does, please, notify the polymake team!  Seriously."
                          "# @param Polytope p"
                          "# @return PlanarNet",
                          "planar_net<Coord>(Polytope<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
