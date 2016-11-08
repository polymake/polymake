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
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Rational.h"
#include "polymake/RandomSpherePoints.h"
#include "polymake/graph/HasseDiagram.h"
#include <cmath>

namespace polymake { namespace polytope {
namespace {

template <typename Coord> inline
Coord weight(int a, int b, Coord*) { return Coord(a)/Coord(b); }

inline
Rational weight(int a, int b, Rational*) { return Rational(a,b); }

template <typename Coord, typename AdjVert>
Coord calc_weight(const Matrix<Coord>& Vd, const GenericMatrix<AdjVert>& adjVert_in,
                  const int i, const int dim, const int nop,
                  RandomSpherePoints<>& random_source,
                  const double eps)
{
   // get the perpendicular vectors for actual vertex
   const Matrix<Coord> adjVert=repeat_row(Vd[i], adjVert_in.rows())-adjVert_in;

   int countwhile = 0;
   int old_out = 0;
   int out = 0;
   double step_dist;

   // check more points on the sphere until weights "converge"
   do {
      //reinitialize some values
      step_dist = 0;
      countwhile++;

      // check how many points are in- and outside the cone
      old_out = out;

      RandomSpherePoints<>::const_iterator rand_point_it(random_source.begin());
      for (int i = 0; i < ((countwhile == 1)?50*nop:nop); ++i) {
         const Vector<Coord> point(Coord(1) | *rand_point_it);
         for (typename Entire< Rows< Matrix<Coord> > >::const_iterator ri2= entire(rows(adjVert)); !ri2.at_end(); ++ri2 )
            if ( point*(*ri2) < 0) {
               out++;
               break;
            }
      }

      step_dist = (countwhile!=1)?(double(out)/countwhile)/(double(old_out)/(countwhile-1)):2;

   } while (abs(step_dist-1) >= eps);

   return weight((countwhile+49)*nop-out, (countwhile+49)*nop, (Coord*)0);
}
}

template <typename Coord>
Matrix<Coord> all_steiner_points(perl::Object p, perl::OptionSet options)
{
   //FIXME: check dim-type!
   const int dim=p.call_method("DIM");
   const Matrix<Coord> V=p.give("VERTICES");
   const Graph<> G=p.give("GRAPH.ADJACENCY");
   const graph::HasseDiagram HDiagram=p.give("HASSE_DIAGRAM");

   RandomSpherePoints<> random_source(dim, RandomSeed(options["seed"]));

   const double eps=options["eps"];

   const int n_stp = HDiagram.nodes() - HDiagram.node_range_of_dim(1).size() - HDiagram.node_range_of_dim(0).size() - 1;
   Matrix<Coord> steiner_points(n_stp,dim+1);
   typename Rows< Matrix<Coord> >::iterator si=rows(steiner_points).begin();

   for (int d = dim; d >= 2; --d) {
      const int nop = 10*(1<<d);

      //iterate over the faces of current dimension d
      for (Entire<graph::HasseDiagram::nodes_of_dim_set>::iterator fi=entire(HDiagram.node_range_of_dim(d)); !fi.at_end(); ++fi, ++si) {

         const Set<int>& face = HDiagram.face(*fi);
         Vector<Coord> weights(face.size());
         Coord sw(0);
         typename Vector<Coord>::iterator wi=weights.begin();

         //iterate over the vertices of the face
         for ( Entire< Set<int> >::const_iterator vi=entire(face); !vi.at_end(); ++vi, ++wi ) {
            const Coord w=calc_weight(V, V.minor(G.adjacent_nodes(*vi)*face, All),
                                      *vi, dim, nop, random_source, eps);
            sw+=w;
            *wi=w;
         }

         weights /= sw;
         *si = weights*V.minor(face,All);
      }
   }

   return steiner_points;
}

template <typename Coord>
Vector<Coord> steiner_point(perl::Object p, perl::OptionSet options)
{
   //FIXME: check dim-type!
   const int dim=p.call_method("DIM");
   const Matrix<Coord> V=p.give("VERTICES");
   const Graph<> G=p.give("GRAPH.ADJACENCY");

   RandomSpherePoints<> random_source(dim, RandomSeed(options["seed"]));

   const double eps=options["eps"];
   const int nop = 10*(1<<dim);

   Vector<Coord> weights(V.rows());
   Coord sw(0);
   typename Vector<Coord>::iterator wi=weights.begin();

   for (Entire< Nodes< Graph<> > >::const_iterator n=entire(nodes(G)); !n.at_end(); ++n, ++wi) {
      const Coord w=calc_weight(V, V.minor(n.adjacent_nodes(),All),
                                *n, dim, nop, random_source, eps);
      sw+=w;
      *wi=w;
   }

   weights/=sw;
   return weights*V;
}

UserFunctionTemplate4perl("# @category Geometry"
                          "# Compute the Steiner points of all faces of a polyhedron //P// using a"
                          "# randomized approximation of the angles."
                          "# //P// must be [[BOUNDED]]."
                          "# @param Polytope P"
                          "# @option Float eps controls the accuracy of the angles computed"
                          "# @option Int seed controls the outcome of the random number generator;"
                          "#   fixing a seed number guarantees the same outcome."
                          "# @return Matrix"
                          "# @author Thilo Rörig",
                          "all_steiner_points<Coord>(Polytope<Coord> { seed => undef, eps => 0.1 })");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Compute the Steiner point of a polyhedron //P// using a randomized"
                          "# approximation of the angles."
                          "# @param Polytope P"
                          "# @option Float eps controls the accuracy of the angles computed"
                          "# @option Int seed controls the outcome of the random number generator;"
                          "#   fixing a seed number guarantees the same outcome."
                          "# @return Vector"
                          "# @author Thilo Rörig",
                          "steiner_point<Coord>(Polytope<Coord> { seed => undef, eps => 0.1 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
