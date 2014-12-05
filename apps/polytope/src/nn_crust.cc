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
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {
namespace {

template<typename Scalar>
inline
bool obtuse_angle(const Vector<Scalar>& x, const Vector<Scalar>& y, const Vector<Scalar>& z)
{
   return (x-y)*(z-y)<0;
}
}

template<typename Scalar>
void nn_crust(perl::Object p)
{
   const Matrix<Scalar> S=p.give("SITES");
   const int n=S.rows();
   const Graph<> D=p.give("DELAUNAY_GRAPH.ADJACENCY");
   Graph<> G(n);
      
   // first step: find nearest neighbors; if there are more than one: take any of them
   Array<int> near(n,-1);
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(D)); !e.at_end(); ++e) {
      const int x=e.from_node(), y=e.to_node();
      if (near[x]<0 || sqr(S.row(x)-S.row(y))<sqr(S.row(x)-S.row(near[x])))
         near[x]=y;
      if (near[y]<0 || sqr(S.row(x)-S.row(y))<sqr(S.row(y)-S.row(near[y])))
         near[y]=x;
   }
   for (int i=0; i<n; ++i)
      G.edge(i,near[i]);
      
   p.take("NN_GRAPH.ADJACENCY") << G;
      
   // second step: find nearest neighbor among those with obtuse angle
   Array<int> obtuse_near(n,-1);
   for (Entire< Edges< Graph<> > >::const_iterator e=entire(edges(D)); !e.at_end(); ++e) {
      const int x=e.from_node(), y=e.to_node();
      if (G.degree(x)<2 && y!=near[x] && obtuse_angle<Scalar>(S.row(y),S.row(x),S.row(near[x]))
          && (obtuse_near[x]<0 || sqr(S.row(x)-S.row(y))<sqr(S.row(x)-S.row(obtuse_near[x]))))
         obtuse_near[x]=y;
      if (G.degree(y)<2 && x!=near[y] && obtuse_angle<Scalar>(S.row(x),S.row(y),S.row(near[y]))
          && (obtuse_near[y]<0 || sqr(S.row(x)-S.row(y))<sqr(S.row(y)-S.row(obtuse_near[y]))))
         obtuse_near[y]=x;
   }
   for (int i=0; i<n; ++i)
      if (obtuse_near[i]>=0) G.edge(i,obtuse_near[i]);
   
   p.take("NN_CRUST_GRAPH.ADJACENCY") << G;
}

FunctionTemplate4perl("nn_crust<Scalar>(VoronoiDiagram<Scalar>) : void");

} } 

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
