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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/graph/DoublyConnectedEdgeList.h"
#include "polymake/topaz/DomeVolumeVisitor.h"
#include "polymake/topaz/CoveringTriangulationVisitor.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace topaz {

using flip_sequence = std::list<Int>;
using DoublyConnectedEdgeList = graph::dcel::DoublyConnectedEdgeList;
using HalfEdge = graph::dcel::HalfEdge;

// compute horocycle at w, given the three lengths of a triangle (u,v,w) and the horocycles at u and v
// the determinant along (u,v) needs to be positive!
Vector<Rational> thirdHorocycle(const Vector<Rational>& horo_u, const Vector<Rational>& horo_v, const Rational& lambda_uv, const Rational& lambda_vw, const Rational& lambda_wu)
{
   if (horo_u[0]*horo_v[1] <= horo_u[1]*horo_v[0])
      throw std::runtime_error("thirdHorocycle: determinant not positive");
   return Vector<Rational>{ -( horo_u[0]*lambda_vw + horo_v[0]*lambda_wu ) / lambda_uv,
                            -( horo_u[1]*lambda_vw + horo_v[1]*lambda_wu ) / lambda_uv };
}

// compute horo matrix of (unflipped) zero edge given horo coordinates
Matrix<Rational> compute_horo(DoublyConnectedEdgeList& dcel, const Rational& p_inf, const Rational& zero_head)
{
   const HalfEdge* zero = dcel.getHalfEdge(0);

   Rational q = zero->getLength() / p_inf;
   Rational p = zero_head * q;

   return Matrix<Rational>{ { p_inf, Rational(0) },
                            { p, q } };
}

// Calculate the horo matrix of the (flipped) zeroth edge in dependence of the surrounding quadrangle,
// which is currently triangulated by (0,a,b) and (1,c,d).
void compute_horo_flipped(DoublyConnectedEdgeList& dcel, Matrix<Rational>& horo)
{
   const HalfEdge* zero = dcel.getHalfEdge(0);

   const Vector<Rational> horo_tail = horo[0];
   const Vector<Rational> horo_head = horo[1];

   // horocycle third_zero=[p_1,q_1] at corner (a,b)
   const HalfEdge* a = zero->getNext();
   const HalfEdge* b = a->getNext();
   const Vector<Rational> third_zero = thirdHorocycle(horo_tail, horo_head, zero->getLength(), a->getLength(), b->getLength());

   // horocycle third_one=[p_2,q_2] at corner (c,d)
   const HalfEdge* one = zero->getTwin();
   const HalfEdge* c = one->getNext();
   const HalfEdge* d = c->getNext();
   const Vector<Rational> third_one = thirdHorocycle(horo_head, -horo_tail, one->getLength(), c->getLength(), d->getLength());

   // horocycles at the tail and head of the flipped edge 0 are -[p_2,q_2] and [p_1,q_1] respectively.
   horo[0] = -third_one;
   horo[1] = third_zero;
}


//Compute VERTICES_IN_FACETS of a a secondary polytope
IncidenceMatrix<> secPolyVif(const Matrix<Rational>& rays, const IncidenceMatrix<>& cells)
{
   IncidenceMatrix<> M(rays.rows(), cells.rows() + rays.cols());
   for (Int i = 0 ; i < rays.rows(); ++i) {
      for (Int j = 0 ; j < cells.rows(); ++j) {
         if (cells[j].contains(i)) {
            M(i, j) = true;
         }
      }
      for (Int k = 0 ; k < rays.cols(); ++k) {
        if (rays(i, k) == 0) {
            M(i, cells.rows()+k) = true;
         }
      }
   }
   return M;
}


// the main function. returns the secondary polyhedron of the triangulation given by the surface,
// computed up to the given dual tree depth, and laying out the zeroth half edge as specified by the
// coordinated p_inf and zero_head.
Matrix<Rational> gkz_vectors(BigObject surface, Int depth)
{
   if (depth<0) {
      throw std::runtime_error("gkz_vectors: invalid depth");
   }

   const Vector<Rational> penner_coord = surface.give("PENNER_COORDINATES");
   const Array<flip_sequence> flip_words = surface.give("FLIP_WORDS");
   std::pair<Rational, Rational> first_horo = surface.give("SPECIAL_POINT");
   const Rational& p_inf = first_horo.first;
   const Rational& zero_head = first_horo.second;

   // Set up the surface from dcel:
   DoublyConnectedEdgeList dcel = surface.give("DCEL");
   dcel.setMetric(penner_coord);

   // Initialize vertex matrix of secondary polyhedron.
   const Int num_vertices = dcel.getNumVertices();
   Matrix<Rational> vert(flip_words.size(), num_vertices);

   // Loop that visits every maximal cone of the secondary fan.
   // We flip to the corresponding Delaunay triangulation, calculate the GKZ vector via the DomeVolumeVisitor,
   // flip back, and start over.
   for (Int i = 0 ; i < flip_words.size(); ++i) {
      // We compute the position of the zeroth half edge (index 0) in the upper half plane.
      Matrix<Rational> zero_horo = compute_horo(dcel, p_inf, zero_head);

      for (const Int flip_word : flip_words[i]) {
         if (flip_word == 0) {
            // For every flip of the zeroth (half-)edge, we calculate the new coords in dependence
            // of the surrounding quadrangle.
            compute_horo_flipped(dcel, zero_horo);
         }

         dcel.flipEdge(flip_word);
      }

      // After we applied the i-th flip sequence, we calculate the GKZvector of the corresponding
      // triangulation via the DomeVolumeVisitor.

      DomeBuilder dome(dcel, zero_horo);
      vert[i] = dome.computeGKZVector(depth);

      // Flip back to reference triangulation from triangulation "i".
      // The "true" parameter leads to unflipping of the edges in the reverse ordering,
      // yielding the same orientations as in the beginning.
      dcel.flipEdges(flip_words[i], true);
   }

   // Add the negative Orthant and 1's in the first column for homogenous coordinates.
   vert = ones_vector<Rational>( flip_words.size() ) | vert;

   return vert;
}



// return the covering triangulation of the k-th Delaunay triangulation
BigObject covering_triangulation(BigObject surface, Int flip_word_id, Int depth)
{
   if (depth < 0) {
      throw std::runtime_error("gkz_dome: invalid depth");
   }

   const Vector<Rational> penner_coord = surface.give("PENNER_COORDINATES");
   const Array<flip_sequence> flip_words = surface.give("FLIP_WORDS");
   if (flip_word_id < 0 || flip_word_id >= flip_words.size()) {
      throw std::runtime_error("gkz_dome: invalid index of Delaunay triangulation");
   }
   const flip_sequence& flip_word = flip_words[flip_word_id];
   std::pair<Rational, Rational> first_horo = surface.give("SPECIAL_POINT");
   const Rational& p_inf = first_horo.first;
   const Rational& zero_head = first_horo.second;

   // Set up the surface from dcel:
   DoublyConnectedEdgeList dcel = surface.give("DCEL");
   dcel.setMetric(penner_coord);

   // We compute the position of the zeroth half edge (index 0) in the upper half plane.
   Matrix<Rational> zero_horo = compute_horo(dcel, p_inf, zero_head);

   for (const Int flip : flip_word) {
      if (flip == 0) {
         // For every flip of the zeroth (half-)edge, we calculate the new coords in dependence
         // of the surrounding quadrangle.
         compute_horo_flipped(dcel, zero_horo);
      }
      dcel.flipEdge(flip);
   }

   // After we applied the i-th flip sequence, we calculate the covering triangulation via the CoveringTriangulationVisitor.
   CoveringBuilder cov(dcel, zero_horo, depth);
   return cov.computeCoveringTriangulation();
}


BigObject secondary_polyhedron(BigObject surface, Int d)
{
   if (d < 0) {
      throw std::runtime_error("secondary_polyhedron: invalid depth");
   }

   Matrix<Rational> vert = gkz_vectors(surface, d);
   Matrix<Rational> neg = zero_vector<Rational>() | -unit_matrix<Rational>(vert.cols()-1);
   vert /= neg;

   BigObject secfan = surface.give("SECONDARY_FAN");
   const Matrix<Rational> rays = secfan.give("RAYS");
   const IncidenceMatrix<> cells = secfan.give("MAXIMAL_CONES");
   return BigObject("polytope::Polytope<Float>",
                    "VERTICES", vert,
                    "VERTICES_IN_FACETS", secPolyVif(rays, cells));
}

InsertEmbeddedRule("REQUIRE_APPLICATION fan\n\n");
Function4perl(&gkz_vectors, "gkz_vectors(HyperbolicSurface, Int)");
Function4perl(&covering_triangulation, "covering_triangulation(HyperbolicSurface, Int, Int)");

UserFunction4perl("# @category Producing other objects\n"
                  "# Computes the secondary polyhedron of a hyperbolic surface up to a given depth\n"
                  "# of the spanning tree of the covering triangluation of the hypoerbolic plane."
                  "# @param HyperbolicSurface s"
                  "# @param Int depth"
                  "# @return polytope::Polytope<Float>",
                  &secondary_polyhedron, "secondary_polyhedron(HyperbolicSurface Int)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
