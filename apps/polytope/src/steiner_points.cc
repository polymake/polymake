/* Copyright (c) 1997-2021
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
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Graph.h"
#include "polymake/Rational.h"
#include "polymake/RandomPoints.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include <cmath>

namespace polymake { namespace polytope {
namespace {

template <typename Coord>
Coord weight(Int a, Int b) { return Coord(a)/Coord(b); }

template <>
Rational weight<Rational>(Int a, Int b) { return Rational(a,b); }

template <typename Coord, typename AdjVert>
Coord calc_weight(const Matrix<Coord>& Vd, const GenericMatrix<AdjVert>& adjVert_in,
                  const Int vertex_index, const Int dim, const Int nop,
                  RandomSpherePoints<>& random_source,
                  const double eps)
{
   // get the perpendicular vectors for actual vertex
   const Matrix<Coord> adjVert=repeat_row(Vd[vertex_index], adjVert_in.rows())-adjVert_in;

   Int countwhile = 0;
   Int old_out = 0;
   Int out = 0;
   double step_dist;

   // check more points on the sphere until weights "converge"
   do {
      //reinitialize some values
      step_dist = 0;
      countwhile++;

      // check how many points are in- and outside the cone
      old_out = out;

      RandomSpherePoints<>::const_iterator rand_point_it(random_source.begin());
      for (Int i = 0; i < ((countwhile == 1)?50*nop:nop); ++i) {
         const Vector<Coord> point(Coord(1) | *rand_point_it);
         for (auto ri2= entire(rows(adjVert)); !ri2.at_end(); ++ri2)
            if (point*(*ri2) < 0) {
               ++out;
               break;
            }
      }

      step_dist = countwhile != 1 ? (double(out)/double(countwhile))/(double(old_out)/double(countwhile-1)) : 2;

   } while (abs(step_dist-1) >= eps);

   return weight<Coord>((countwhile+49)*nop-out, (countwhile+49)*nop);
}

}

template <typename Coord>
Matrix<Coord> all_steiner_points(BigObject p, OptionSet options)
{
   //FIXME: check dim-type!
   const Int dim = p.call_method("DIM");
   const Matrix<Coord> V=p.give("VERTICES");
   const Graph<> G=p.give("GRAPH.ADJACENCY");
   BigObject HD_obj = p.give("HASSE_DIAGRAM");
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HDiagram(HD_obj);

   RandomSpherePoints<> random_source(dim, RandomSeed(options["seed"]));

   const double eps = options["eps"];

   const Int n_stp = HDiagram.nodes() - HDiagram.nodes_of_rank(2).size() - HDiagram.nodes_of_rank(1).size()-1;
   Matrix<Coord> steiner_points(n_stp,dim+1);
   auto si=rows(steiner_points).begin();

   for (Int d = dim; d >= 2; --d) {
      const Int nop = 10*(1L<<d);

      // iterate over the faces of current dimension d
      for (const auto f : HDiagram.nodes_of_rank(d+1)) {
         const Set<Int>& face = HDiagram.face(f);
         Vector<Coord> weights(face.size());
         Coord sw(0);
         auto wi=weights.begin();

         //iterate over the vertices of the face
         for (const auto vertex : face) {
            const Coord w=calc_weight(V, V.minor(G.adjacent_nodes(vertex)*face, All),
                                      vertex, dim, nop, random_source, eps);
            sw+=w;
            *wi=w;
            ++wi;
         }

         weights /= sw;
         *si = weights * V.minor(face, All);
         ++si;
      }
   }

   return steiner_points;
}

template <typename Coord>
Vector<Coord> steiner_point(BigObject p, OptionSet options)
{
   //FIXME: check dim-type!
   const Int dim = p.call_method("DIM");
   const Matrix<Coord> V=p.give("VERTICES");
   const Graph<> G=p.give("GRAPH.ADJACENCY");

   RandomSpherePoints<> random_source(dim, RandomSeed(options["seed"]));

   const double eps=options["eps"];
   const Int nop = 10*(1L<<dim);

   Vector<Coord> weights(V.rows());
   Coord sw(0);
   typename Vector<Coord>::iterator wi=weights.begin();

   for (auto n=entire(nodes(G)); !n.at_end(); ++n, ++wi) {
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
