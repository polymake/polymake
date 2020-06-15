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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/Map.h"
#include "polymake/hash_set"
#include "polymake/list"
#include <cmath>

/* The overall idea is to produce a planar net of a 3-polytope from one spanning tree in the dual graph.  Once the
   spanning tree is chosen and the root facet is drawn the layout is uniquely determined.  It is not known if such a
   planar net always exists.

   The spanning tree is determined as follows.  Starting at the root facet we perform a breadth first search through the
   dual graph.  New facets are queued in the order in which they appear.  More precisely, the queue members are actually
   edges, such that one incident facet is already in the layout, while the other one is not.  The current heuristic
   greedily places a facet if it does not overlap with the existing layout.  There is no proper backtracking (yet).
   However, the current implementation tries all facets as root facet.

   This client is meant to be used by non-experts, too.  Therefore, there are more double checks than usual,
   and the code throws (hopefully) meaningful exceptions if something goes wrong.
*/

#define PLANAR_NET_DEBUG 0

namespace polymake { namespace fan {

namespace {
   constexpr double epsilon = 1e-10;

   typedef std::pair<Int, Int> directed_edge;
   typedef std::pair<Int, Int> vertex_facet_pair;

   // a tree is stored as a sequence of directed edges; works primally and dually
   typedef std::list<directed_edge> tree_type;

   // EXACT COMPUTATIONS (if Coord is an exact type)

   template <typename Coord>
   Vector<Coord> barycentric(const Vector<Coord>& x, const Matrix<Coord>& W)
   {
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
   void determine_directed_edge(Int& a, Int& b, const Array<Int>& cycle, Int& index)
   {
      index=0;
      for ( ; cycle[index] != a; ++index); // now: cycle[index] == a
      Int j = index+1;
      if ((j < cycle.size() && cycle[j] != b) || (j == cycle.size() && cycle[0] != b)) {
         std::swap(a,b);
         if (index > 0) {
            --index;
         } else {
            index=cycle.size()-1;
         }
      }
   }

   // INEXACT COMPUTATIONS
   // This can't be avoided (easily) as square roots and trigonometric functions are involved.

   // we try to compute exactly as long as we can
   template <typename Coord>
   double dist(const Vector<Coord>& x, const Vector<Coord>& y)
   {
      return sqrt(convert_to<double>(sqr(x-y)));
   }

   double norm(const Vector<double>& v)
   {
      return sqrt(sqr(v));
   }

   // counter clock-wise orientation of three affine points
   Int ccw(const Vector<double>& a, const Vector<double>& b, const Vector<double>& c)
   {
      const double det_by_laplace=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
      if (fabs(det_by_laplace) < epsilon) {
         return 0;
      }
      return sign(det_by_laplace);
   }

   bool proper_segment_intersection(const Vector<double>& a, const Vector<double>& b,
                                    const Vector<double>& c, const Vector<double>& d)
   {
      return (ccw(a,b,c) * ccw(a,b,d) < 0) && (ccw(c,d,a) * ccw(c,d,b) < 0);
   }

#if 0  // unused now
   bool point_in_facet(const Vector<double>& point, const Int facet,
                       const Array<Array<Int>>& vif,
                       const Matrix<double>& net_vertices,
                       const Map<vertex_facet_pair,Int>& vf_map)
   {
#if PLANAR_NET_DEBUG > 2
      cerr << " [point=" << point << " in facet " << facet<< ":";
#endif
      auto iv=entire(vif[facet]);
      const Int first_vertex = vf_map[vertex_facet_pair(*iv,facet)];
      Vector<double> pvh = net_vertices[first_vertex];
      Vector<double> nvh;
      // check each edge in facet
      for (++iv; !iv.at_end(); ++iv) {
#if PLANAR_NET_DEBUG > 2
         cerr << " " << *iv;
#endif
         nvh = net_vertices[vf_map[vertex_facet_pair(*iv,facet)]];
         if (ccw(pvh,nvh,point)<0) {
            // the point is on the negative side of that edge, hence ...
#if PLANAR_NET_DEBUG > 2
            cerr << " (" << pvh << ", " << nvh << ", " << point << ")*]";
#endif
            
            return false;
         }
         pvh = nvh;
      }
      // check final edge; pvh is now the last vertex on the facet h
      nvh = net_vertices[first_vertex];
      if (ccw(pvh,nvh,point)<0) {
#if PLANAR_NET_DEBUG > 2
         cerr << "final (" << pvh << ", " << nvh << ", " << point << ")*]";
#endif
         return false;
      }
#if PLANAR_NET_DEBUG > 2
      cerr << "]" << endl;
#endif
      return true;
   }
#endif

   bool point_versus_edges_of_facet(const Vector<double>& point, const Vector<double>& previous_point, const Int facet,
                                    const Array< Array<Int>>& vif, const Matrix<double>& net_vertices, const Map<vertex_facet_pair, Int>& vf_map)
   {
  
#if PLANAR_NET_DEBUG > 2
      cerr << " [point=" << point << " versus edges of facet " << facet<< ":";
#endif
      Vector<double> pvh = net_vertices[vf_map[vertex_facet_pair(vif[facet].back(),facet)]];
      Vector<double> nvh;
      for (auto iv=entire(vif[facet]); !iv.at_end(); ++iv) {
#if PLANAR_NET_DEBUG > 2
         cerr << " " << *iv;
#endif
         nvh = net_vertices[vf_map[vertex_facet_pair(*iv,facet)]];
         if (proper_segment_intersection(previous_point,point, pvh,nvh)) {
#if PLANAR_NET_DEBUG > 2
            cerr << " *] pp=" << previous_point << " p= " << point << " pvh=" << pvh << " nvh=" << nvh ;
#endif
            return true;
         }
         pvh=nvh;
      }
#if PLANAR_NET_DEBUG > 2
      cerr << "]" << endl;
#endif
      return false;
   }
   
   template <typename Coord>
   Int overlap(const Vector<double>& point, const Vector<double>& previous_point,
               const Array< Array<Int>>& vif, const Set<Int>& marked, const Matrix<double>& net_vertices, const Map<vertex_facet_pair, Int>& vf_map)
   {
      /* Check each facet h in the layout so far. */
      for (auto h=entire(marked); !h.at_end(); ++h) {
         /* There are two types of overlaps:
            (1) The point to be placed could be contained in the facet h ... */
         /* this would also trigger an edge-intersection except for some corner-cases which are ignored for now
            to deal with these properly the edge-intersection needs to distinguish between neighboring edges
            and other edges */
         // if (point_in_facet(point,*h,vif,net_vertices,vf_map)) return *h;
         /* (2) ... or two edges intersect.*/
         if (point_versus_edges_of_facet(point,previous_point,*h,vif,net_vertices,vf_map)) return *h;
      }
      return -1; // OK, no overlap
   }

   template <typename Coord>
   bool layout_one_facet(const Int f, // index of facet to be processed
                         const Int ja, const Int jb, // (indices of) points in the plane where layout starts
                         const std::list<Int>& vertex_cycle, // indices of vertices on the facet in cyclic order starting at a and b
                         const Matrix<Coord>& V, // vertices of the 3-polytope
                         const Array< Array<Int> >& vif, // vertices in facets, cyclically ordered
                         Set<Int>& marked, // facets already layed out; f will be added if successful
                         Int& cv, // number of vertices of the net already computed
                         Matrix<double>& net_vertices, // vertices of the net
                         Array<Set<Int>>& net_facets, // facets of the net
                         Map<vertex_facet_pair, Int>& vf_map, // which vertex on which facet ends up where
                         Array<Int>& vf_map_inv // inverse of the previous
                         ) {
      std::list<Int>::const_iterator vc_it = vertex_cycle.begin();

      const Int ix = *vc_it; ++vc_it;
      const Int iy = *vc_it; ++vc_it;
      const Int iz = *vc_it;
      std::list<Int> FirstThreeOnFacet;
      FirstThreeOnFacet.push_back(ix); FirstThreeOnFacet.push_back(iy); FirstThreeOnFacet.push_back(iz);

      Vector<double> // dehomogenize and subtract
         u = Vector<double>((V[ix] - V[iy]).slice(range_from(1))),
         v = Vector<double>((V[iz] - V[iy]).slice(range_from(1)));
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

      const Int initial_cv = cv; // remember this for possible rollback later

      Set<Int> this_net_facet{ja, jb};
      Vector<double> previous_vertex(net_vertices[jb]); // right vertex on the connecting edge

      while (true) {
         Int idx;
         Vector<double> next_vertex;

         if (vc_it != vertex_cycle.end()) {
            idx = *vc_it;
            Vector<double> bc(barycentric<Coord>(V[idx],V.minor(FirstThreeOnFacet,All)));
            next_vertex = bc[0]*a + bc[1]*b + bc[2]*c;
         } else {
            // go back to the first vertex to make sure the last edge is checked
            idx = ix;
            next_vertex = a;
         }

#if PLANAR_NET_DEBUG > 1
         cerr << " idx=" << idx << " ";
#endif

         /* If some facet contains the point next_vertex in its interior or in its boundary we need a rollback.
            Negative return value signals no overlap, otherwise index of overlapping facet. */
         const Int h = overlap<Coord>(next_vertex, previous_vertex, vif, marked, net_vertices, vf_map);
         if (h >= 0) {
#if PLANAR_NET_DEBUG > 1
            cerr << " next_vertex=" << next_vertex << " interferes with facet " << h << endl;
            const Array<Int>& vif_h = vif[h];
            for (auto v=entire(vif_h); !v.at_end(); ++v)
               cerr << " " << *v << "/" << vf_map[vertex_facet_pair(*v,h)] << ": " << net_vertices[vf_map[vertex_facet_pair(*v,h)]] << endl;
#endif
            cv=initial_cv;
            return false;
         }

         // bail out here to avoid overwriting the information for the first vertex
         if (ix == idx)
            break;

         vc_it++;

         previous_vertex = net_vertices[cv] = next_vertex;
         this_net_facet += cv;
         vf_map[vertex_facet_pair(idx,f)] = cv;  vf_map_inv[cv] = idx;
         ++cv;
      }
      net_facets[f] = this_net_facet;
      marked.insert(f);
      return true;
   }

   template <typename Coord>
   void initialize_root_facet(const Int f, // this will be the root facet
                              const Int ia, const Int ib,
                              const Int ja, const Int jb, // (indices of) points in the plane where layout starts
                              std::list<Int>& vertex_cycle, // indices of vertices on the facet in cyclic order starting at a and b
                              const Matrix<Coord>& V, // vertices of the 3-polytope
                              const Array<Array<Int>>& vif, // vertices in facets, cyclically ordered
                              Set<Int>& marked, // facets already layed out; f will be added if successful
                              Int& cv, // number of vertices of the net already computed
                              Matrix<double>& net_vertices, // vertices of the net
                              Array<Set<Int>>& net_facets, // facets of the net
                              Map<vertex_facet_pair, Int>& vf_map, // which vertex on which facet ends up where
                              Array<Int>& vf_map_inv, // inverse of the previous
                              tree_type& dual_tree
                              ) {
      // initialize data
      vertex_cycle.clear();
      for (Int i=0; i < vif[f].size(); ++i)
         vertex_cycle.push_back(vif[f][i]);
      marked.clear();
      vf_map.clear(); vf_map[vertex_facet_pair(ia,f)] = ja; vf_map[vertex_facet_pair(ib,f)] = jb; vf_map_inv[ja] = ia; vf_map_inv[jb] = ib;
      dual_tree.clear();
      
      // true edge length in 3-space
      double unit_length=dist<Coord>(V[ia],V[ib]);

      const Vector<double>
         a = zero_vector<double>(2), // first vertex gets mapped to origin
         b = unit_length * unit_vector<double>(2,0); // second vertex at proper distance
      net_vertices[ja]=a;
      net_vertices[jb]=b;

      layout_one_facet(f, ja, jb, vertex_cycle, V, vif, marked, cv, net_vertices, net_facets, vf_map, vf_map_inv);
   }

   // COMBINATORIAL

   void queue_neighbors(const Int f, const Graph<>& DG, const Set<Int>& marked, std::list<directed_edge>& unprocessed_edges) {
      const Set<Int> neighbors = DG.adjacent_nodes(f);
#if PLANAR_NET_DEBUG > 2
      cerr << "queue_neighbors: f=" << f;
#endif
      for (auto n_it = entire(neighbors); !n_it.at_end(); ++n_it)
         if (!marked.contains(*n_it)) {
#if PLANAR_NET_DEBUG > 2
            cerr << " " << *n_it;
#endif
            unprocessed_edges.push_back(directed_edge(f,*n_it));
         }
#if PLANAR_NET_DEBUG > 2
      cerr << endl;
#endif
   }
   
}

template <typename Coord>
BigObject planar_net(BigObject p)
{
   const Matrix<Coord> V=p.give("VERTICES");
   const Int d = p.give("CONE_DIM");
   
   if (V.cols() != 4 || d!=4)
      throw std::runtime_error("planar_net: requires full-dimensional 3-polytope");

   const Array<Array<Int>> vif=p.give("VIF_CYCLIC_NORMAL");
   /* The following is also the number of maximal cells of the planar net.
      Throughout we keep the labeling of the facets the same. */
   const Int n_facets = vif.size(); 
   Array<Set<Int>> net_facets(n_facets);

   /* Total number of vertices in the planar net = 2 * (f_1 - f_2 + 1);
      since each edge is drawn twice, except for those in a spanning tree
      of the dual graph.  Euler yields f_1 - f_2 + 1 = f_0 - 1. */
   const Int n_net_vertices = 2*(V.rows()-1);
   Matrix<double> net_vertices(n_net_vertices,2); // Euclidean
   Int cv = 0; // no vertices in the planar net so far

   // Recalls which vertex in which facet of the 3-polytope has which row index in the matrix above.
   Map<vertex_facet_pair, Int> vf_map;
   Array<Int> vf_map_inv(n_net_vertices);

   const Graph<> DG=p.give("DUAL_GRAPH.ADJACENCY");
   tree_type dual_tree;

#if PLANAR_NET_DEBUG
   cerr << "SEARCHING FOR SPANNING TREE IN DUAL GRAPH"
        << " n_facets=" << n_facets << " n_net_vertices=" << n_net_vertices << endl;
#endif
         
   // Try each facet as the root of the spanning tree in the dual graph.
   for (Int root_facet = 0; root_facet < n_facets && cv < n_net_vertices ; ++root_facet) {
      Int
         ia = vif[root_facet][0], // indices of first ...
         ib = vif[root_facet][1], // ... and second polytope vertex for the layout
         ja = 0, jb = 1; // corresponding indices of the vertices in the planar net
         cv = 2; // number of vertices in the planar net so far

#if PLANAR_NET_DEBUG
      cerr << "*** root_facet=" << root_facet << " {" << vif[root_facet] << "}"
           << " ia=" << ia << " ib=" << ib
           << " ja=" << ja << " jb=" << jb << endl;
#endif
      
      std::list<Int> f_vertex_cycle;
      Set<Int> marked;  // no facets processed yet; start with the root facet
      initialize_root_facet(root_facet, ia, ib, ja, jb, f_vertex_cycle, V, vif, marked, cv, net_vertices, net_facets, vf_map, vf_map_inv, dual_tree);

      std::list<directed_edge> unprocessed_edges;
      queue_neighbors(root_facet, DG, marked, unprocessed_edges);

      Int f = root_facet;
      do {
         // pick next edge, but beware that in the mean time both facets could have been processed
         Int g;
         bool found = false; // no suitable g found yet
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
         if (!found) { // this root facet does not work with our (deterministic) heuristics
 #if PLANAR_NET_DEBUG > 1
            cerr << " failed" << endl;
#endif
            break;
         }
         // now f has been processed, and its neighbor g is next

         const Array<Int>& vif_g(vif[g]);

         const Set<Int>
            f_vertex_set(vif[f]),
            g_vertex_set(vif_g),
            common_vertices(f_vertex_set * g_vertex_set);
         
         if (common_vertices.size() != 2)
            throw std::runtime_error("planar_net: dual edge does not correspond to a primal one");
         
         // the two common vertices
         ia=common_vertices.front();
         ib=common_vertices.back();
         
         Int i;
         determine_directed_edge(ia,ib,vif_g,i);
         
         // vertex_cycle starts with first ia, then ib
         std::list<Int> g_vertex_cycle;
         for (Int k = i; k < vif_g.size(); ++k) g_vertex_cycle.push_back(vif_g[k]);
         for (Int k = 0; k < i; ++k) g_vertex_cycle.push_back(vif_g[k]);
         
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
   }

   if (cv < n_net_vertices)
      throw std::runtime_error("planar_net: heuristic failed, all root facets tried. KEEP THIS POLYTOPE!!!");

   // compute the primal edges which need flaps
#if PLANAR_NET_DEBUG
   cerr << "COMPUTING FLAPS" << endl;
#endif
   hash_set<Set<Int>> dual_tree_set;
   for (tree_type::iterator it=dual_tree.begin(); it!=dual_tree.end(); ++it) {
      dual_tree_set.insert(Set<Int>{it->first, it->second});
   }

   tree_type flaps;
   for (auto e=entire(edges(DG));  !e.at_end();  ++e) {
      // flaps are directed from facet f to facet g, where f has the smaller index
      Int f = e.from_node(), g = e.to_node();
      if (e.from_node() > e.to_node())
         std::swap(f, g);

#if PLANAR_NET_DEBUG
      cerr << "considering f=" << f << " g=" << g;
#endif
      if (dual_tree_set.exists(Set<Int>{f, g})) {
#if PLANAR_NET_DEBUG
         cerr << endl;
#endif
         continue;
      }

      const Array<Int>& vif_f = vif[f];
      const Set<Int>
         f_vertex_set(vif_f),
         g_vertex_set(vif[g]),
         common_vertices(f_vertex_set * g_vertex_set);
      Int
         ia = common_vertices.front(),
         ib = common_vertices.back(),
         i; // not used here
#if PLANAR_NET_DEBUG
      cerr << " vif_f=" << vif_f << " ia=" << ia << " ib=" << ib << endl;
#endif
      determine_directed_edge(ia,ib,vif_f,i);
      // now ia comes before ib in the cyclic order of the vertices of the facet f
      flaps.push_back(directed_edge(vf_map[vertex_facet_pair(ia,f)],vf_map[vertex_facet_pair(ib,f)]));
   }

   BigObject net("PlanarNet", mlist<Coord>(),
                 "VERTICES", ones_vector<double>(n_net_vertices) | net_vertices,
                 "MAXIMAL_POLYTOPES", net_facets,
                 "VF_MAP", vf_map,
                 "VF_MAP_INV", vf_map_inv,
                 "DUAL_TREE", Array<directed_edge>(dual_tree),
                 "FLAPS", Array<directed_edge>(flaps),
                 "POLYTOPE", p);

   net.set_description() << "planar net of " << p.name() << endl;

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
