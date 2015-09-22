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
#include "polymake/list"
#include <cmath>

/* This client is meant to be used by non-experts.  Therefore there are more double checks than usual,
   and the code throws (hopefully) meaningful exceptions if something goes wrong. */

#define PLANAR_NET_DEBUG 0

namespace polymake { namespace fan {

namespace {
   // EXACT COMPUTATIONS

   template <typename Coord>
   Vector<Coord> barycentric(const Vector<Coord>& x, const Matrix<Coord>& W) {
      /* x: point in d+1 homogeneous coordinates
         W: dx(d+1)-matrix of d points in homogeneous coordinates (linearly independent)
         leading 1 used for homogenization in both cases
         assumption: x is contained in the affine hull of the rows of W
         returns coefficients such that x is written as a linear combination of the rows of W, which sum to 1
      */
      return lin_solve(T(W),x);
   }

   // EXACT COMPUTATIONS BASED ON INEXACT INPUT
   // The intention is not to introduce more errors than strictly necessary.

   template <typename Coord>
   int orientation(const Vector<double>& u) {
      Matrix<Coord> M(3,3);
      M[0] = convert_to<Coord>(u); // leading 0
      M.col(0) = ones_vector<Coord>(3);
      M(2,1) = 1;
      return (sign(det(M)) >= 0 ? 1 : -1);
   }

   // INEXACT COMPUTATIONS
   // This can't be avoided (easily) as square roots and trigonometric functions are involved.

   template <typename Coord>
   inline double dist(const Vector<Coord>& x, const Vector<Coord>& y) {
      return sqrt(convert_to<double>(sqr(x-y)));
   }

   template <typename Coord>
   void layout_one_facet(const int f, // index of facet to be processed
                         const int ja, const int jb, // (indices of) points in the plane where layout starts
                         const std::list<int>& vertex_cycle, // indices of vertices on the facet in cyclic order starting at a and b
                         const Matrix<Coord>& V, // vertices of the 3-polytope
                         int& cv, // number of vertices of the net already computed
                         Matrix<double>& net_vertices, // vertices of the net
                         Array< Set<int> >& net_facets, // facets of the net
                         Map<std::pair<int,int>,int>& vf_map
                         ) {
      std::list<int>::const_iterator vc_it=vertex_cycle.begin();

      const int ix=*vc_it; ++vc_it;
      const int iy=*vc_it; ++vc_it;
      const int iz=*vc_it;
      std::list<int> FirstThreeOnFacet;
      FirstThreeOnFacet.push_back(ix); FirstThreeOnFacet.push_back(iy); FirstThreeOnFacet.push_back(iz);

      Vector<double>
         u=Vector<double>(V[ix]-V[iy]),
         v=Vector<double>(V[iz]-V[iy]);
      double
         unit_length=sqrt(pm::operators::sqr(u)),
         len_v=sqrt(pm::operators::sqr(v));
      
      u /= unit_length; v /= len_v; // scaling to unit lengths
      double beta=M_PI-acos(u*v); // exterior angle
      
      const Vector<double>&
         a(net_vertices[ja]),
         b(net_vertices[jb]);

      // scalar product of (b-a)/norm(b-a) and (1,0)
      double scp = (b[1]-a[1])/unit_length;
      // beware of numerical atrocities
      if (scp>1.0) scp=1.0;
      if (scp<-1.0) scp=-1.0;

      const double
         alpha = acos(scp) * orientation<Coord>(b-a);

      Vector<double> c(b);
      c[1] += unit_length * cos(alpha+beta);
      c[2] += unit_length * sin(alpha+beta);

#if PLANAR_NET_DEBUG
      cerr << "layout of facet " << f
           << ": unit_length=" << unit_length << " alpha=" << alpha << " beta=" << beta
           << " a=" << a << " b=" << b << " c=" << c
           << " orientation=" << orientation<Coord>(b-a)
           << endl;
#endif

      Set<int> this_net_facet; this_net_facet += ja; this_net_facet += jb;
      for ( ; vc_it!=vertex_cycle.end(); ++vc_it) {
         const int idx=*vc_it;
         Vector<double> bc(barycentric<Coord>(V[idx],V.minor(FirstThreeOnFacet,All)));
         net_vertices[cv] = bc[0]*a + bc[1]*b + bc[2]*c;
         this_net_facet += cv;
         vf_map[std::pair<int,int>(idx,f)] = cv;
         ++cv;
      }
      net_facets[f] = this_net_facet;
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
   Matrix<double> net_vertices(n_net_vertices,3); // homogeneous

   // recalls which vertex in which facet has which row index in the matrix above
   Map<std::pair<int,int>,int> vf_map;

   const Graph<> DG=p.give("DUAL_GRAPH.ADJACENCY");

   int f=0; // some arbitrary facet, plays the root of the spanning tree

   int
      ia=vif[f][0], // indices of first ...
      ib=vif[f][1]; // ... and second vertex for the layout

   int ja=0, jb=1;

   // true edge length in 3-space
   double unit_length=dist<Coord>(V[ia],V[ib]);

   const Vector<double>
      a = unit_vector<double>(3,0), // first vertex gets mapped to origin
      b = a + unit_length * unit_vector<double>(3,1); // second vertex at proper distance
   net_vertices[ja]=a;
   net_vertices[jb]=b;
   int cv=2;

   // keep track where those vertices end up
   vf_map[std::pair<int,int>(ia,f)] = ja;
   vf_map[std::pair<int,int>(ib,f)] = jb;

   std::list<int> f_vertex_cycle;
   for (int i=0; i<vif[f].size(); ++i) f_vertex_cycle.push_back(vif[f][i]);

   layout_one_facet(f, ja, jb, f_vertex_cycle, V, cv, net_vertices, net_facets, vf_map);

   std::list< std::pair<int,int> > unprocessed_edges;
   Bitset marked;  // facets already processed
   marked.insert(f);

#if PLANAR_NET_DEBUG
   cerr << "f=" << f << " {" << vif[f] << "}"
        << " ia=" << ia << " ib=" << ib
        << " ja=" << ja << " jb=" << jb
        << " marked=" << marked
        << endl;
   cerr << "* " << vf_map << endl;
#endif

   do {
      // collect edges to unprocessed neighbors of f
      const Set<int> neighbors = DG.adjacent_nodes(f);  
      for (Entire< Set<int> >::const_iterator n_it = entire(neighbors); !n_it.at_end(); ++n_it)
         if (!marked.contains(*n_it))
            unprocessed_edges.push_back(std::pair<int,int>(f,*n_it));
      
      // pick next edge, but beware that in the mean time both facets could have been processed
      int g;
      bool found=false; // no suitable g found yet
      while ( !unprocessed_edges.empty() ) {
         std::pair<int,int> fg_pair=unprocessed_edges.front();
         unprocessed_edges.pop_front();
         g=fg_pair.second;
         if (!marked.contains(g)) {
            f=fg_pair.first;
            found=true;
            break;
         }
      }
      if (!found)
         throw std::runtime_error("planar_net: run out of edges, attempt failed"); 
      // now f has been processed, and its neighbor g is next

      const Array<int>& vif_g(vif[g]);

      Set<int>
         f_vertex_set(vif[f]),
         g_vertex_set(vif_g),
         common_vertices(f_vertex_set * g_vertex_set);

      if (common_vertices.size() != 2)
         throw std::runtime_error("planar_net: dual edge does not correspond to a primal one");

      // the two common vertices
      ia=common_vertices.front();
      ib=common_vertices.back();
      
      // check correct ordering on g: first ia then ib; otherwise swap
      int i=0;
      for ( ; vif_g[i] != ia; ++i);
      int j=i+1;
      if ((j < vif_g.size() && vif_g[j] != ib) || (j == vif_g.size() && vif_g[0] != ib)) {
         int tmp=ia; ia=ib; ib=tmp;
         if (i > 0) {
            i=i-1;
         } else {
            i=vif_g.size()-1;
         }
      }

      // vertex_cycle starts with first ia, then ib
      std::list<int> g_vertex_cycle;
      for (int k=i; k<vif_g.size(); ++k) g_vertex_cycle.push_back(vif_g[k]);
      for (int k=0; k<i; ++k) g_vertex_cycle.push_back(vif_g[k]);

      vf_map[std::pair<int,int>(ia,g)] = ja = vf_map[std::pair<int,int>(ia,f)];
      vf_map[std::pair<int,int>(ib,g)] = jb = vf_map[std::pair<int,int>(ib,f)];

#if PLANAR_NET_DEBUG
      cerr << "f=" << f << " g=" << g << " {" << vif_g << "}"
        << " ia=" << ia << " ib=" << ib
        << " ja=" << ja << " jb=" << jb
        << " marked=" << marked
        << endl;            
#endif

      layout_one_facet(g, ja, jb, g_vertex_cycle, V, cv, net_vertices, net_facets, vf_map);

#if PLANAR_NET_DEBUG
      cerr << "vf_map=" << vf_map << endl;
#endif

      // FIXME: check for overlap and backtrack; probably it suffices to reset cv

      if (cv == n_net_vertices) break; // done

      marked.insert(g);
      f=g; // continue with g next
   } while (true);

   perl::Object net("PolyhedralComplex<Float>");
   net.set_description() << "planar net of " << p.name() << endl;
   net.take("POINTS") << net_vertices;
   net.take("MAXIMAL_CELLS") << net_facets;

   // FIXME: add further data as attachments

   return net;
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes a planar net of the 3-polytope //p//."
                          "# CURRENT RESTRICTIONS (to be fixed soon):"
                          "# * planarity is not guaranteed"
                          "# * does not work for Coord==Float"
                          "# @param Polytope p"
                          "# @return PolyhedralComplex",
                          "planar_net<Coord>(Polytope<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
