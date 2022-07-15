/* Copyright (c) 1997-2022
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

#pragma once
#include <polymake/GenericSet.h>
#include <polymake/Set.h>
#include <polymake/TropicalNumber.h>
#include <polymake/Matrix.h>
#include <polymake/IncidenceMatrix.h>
#include <polymake/Vector.h>
#include <polymake/Map.h>
#include <polymake/GF2.h>
#include <polymake/tropical/arithmetic.h>
#include <polymake/tropical/thomog.h>
#include <polymake/linalg.h>
#include <polymake/graph/Lattice.h>

#include <limits.h>

#ifndef POLYMAKE_TROPICAL_PATCHWORK_H
#define POLYMAKE_TROPICAL_PATCHWORK_H

namespace polymake { namespace tropical {

typedef unsigned long Binary;
typedef unsigned long Orthant;

// COMBINATORICS:

// rows correspond to facets of the hypersurface,
// columns to monomials,
// such that those are the monomials, where the 
// tropical polynomial is minimized/maximized
// by a point in the rel. interior of a facet
// (i.e., those correspond to the vertices of the
// respective dual edge in the dual subdivision)
template <typename Addition>
IncidenceMatrix<NonSymmetric> optimal_monomials(
      const Matrix<Int>&                      monomials,
      const Vector<TropicalNumber<Addition>>& coefficients,
      const IncidenceMatrix<NonSymmetric>&    facets,
      const Matrix<Rational>&                 vertices)
{
   const Int n_monoms = monomials.rows();
   const Int n_facets = facets.rows();

   // for each facet - compute a point in its relative interior
   Matrix<Rational> rel_int_points(n_facets, vertices.cols());
   for (Int r = 0; r < n_facets; ++r) {
      for (auto v = entire(facets.row(r)); !v.at_end(); ++v)
         rel_int_points.row(r) += vertices.row(*v);
      rel_int_points.row(r).dehomogenize();
   }
   rel_int_points = rel_int_points.minor(All, ~scalar2set(0));

   // for each facet - compute the monomials where the optimum is attained:
   IncidenceMatrix<NonSymmetric> opt(n_facets, n_monoms);
   for (Int c = 0; c < n_facets; ++c) {
      Vector<TropicalNumber<Addition>> substed{ monomials * rel_int_points[c] + Vector<Rational>(coefficients) };
      opt.row(c) = extreme_value_and_index<Addition>(substed).second;
   }

   return opt;
}

// Regarding 'orthant' as a 0/1-vector,
// and 'monomial' as an exponentvector,
// this computes orthant*monomial.
Int count_exponents(Orthant orthant, const Vector<Int>& monomial)
{
   Int result = 0;
   orthant <<= 1;
   for (Int index = 0; orthant != 0; orthant >>= 1, ++index) {
      if ((orthant & 1) != 0)
         result += monomial[index];
   }
   return result;
}

// symmetrize 'signs' to 'orthant'
Array<bool> signs_in_orthant(
      const Array<bool>& signs,
      const Matrix<Int>& monomials,
      const Orthant&     orthant)
{
   Array<bool> S(monomials.rows());
   for (Int m = 0; m < monomials.rows(); ++m)
     S[m] = ((count_exponents(orthant, monomials[m])%2 != 0) != signs[m]);
   return S;
}

// false iff all signs agree
inline
bool signs_differ(
      const Array<bool>& signs,
      const Set<Int>&    opt)
{
   if (signs.size() < 1)
      return true;
   auto tmp(entire(opt));
   const bool first_sign(signs[*tmp]);
   for (++tmp; !tmp.at_end(); ++tmp) {
      if (signs[*tmp] != first_sign)
         return true;
   }
   return false;
}

// The real phase structure induced by 'signs'.
// Rows correspond to facets of the tropical hypersurface,
// columns to orthants.
template <typename Addition>
IncidenceMatrix<NonSymmetric> real_phase(
      const Array<bool>&                      signs,
      const Matrix<Int>&                      monomials,
      const Vector<TropicalNumber<Addition>>& coefficients,
      const Matrix<Rational>&                 vertices,
      const IncidenceMatrix<NonSymmetric>&    facets)
{
   if (signs.size() != monomials.rows())
      throw std::runtime_error("dimension mismatch between signs and monomials");
   const Int n_facets             = facets.rows();
   const Int ambient_dim          = monomials.cols() - 1;
   const unsigned long n_orthants = 1UL << ambient_dim;

   const IncidenceMatrix<NonSymmetric> opt = optimal_monomials(monomials, coefficients, facets, vertices);

   IncidenceMatrix<NonSymmetric> ret_real_phase(n_facets, n_orthants);
   for (Orthant o = 0; o < n_orthants; ++o) {
      const Array<bool> S = signs_in_orthant(signs, monomials, o);
      for (Int f = 0; f < n_facets; ++f) {
         if (signs_differ(S, opt[f]))
            ret_real_phase(f, o) = true;
      }
   }

   return ret_real_phase;
}

// indices of those facets of the hypersurface present in orthant:
Set<Int> real_facets_in_orthant(
      const Orthant&                       orthant,
      const IncidenceMatrix<NonSymmetric>& facets,
      const Matrix<Int>&                   monomials,
      const Array<bool>&                   signs,
      const IncidenceMatrix<NonSymmetric>& opt)
{
   // sign distribution for this orthant:
   Array<bool> S = signs_in_orthant(signs, monomials, orthant);

   // facets for this orthand:
   Set<Int> real_facets;
   for (Int f = 0; f < facets.rows(); ++f)
      if (signs_differ(S, opt.row(f)))
         real_facets += f;

   return real_facets;
}

// indices of those facets of the hypersurface present in each orthant:
template <typename Addition>
IncidenceMatrix<NonSymmetric> real_facets(
      const Array<bool>&                      signs,
      const Matrix<Int>&                      monomials,
      const Vector<TropicalNumber<Addition>>& coefficients,
      const Matrix<Rational>&                 vertices,
      const IncidenceMatrix<NonSymmetric>&    facets)
{
   if (signs.size() != monomials.rows())
      throw std::runtime_error("dimension mismatch between signs and monomials");
   const Int dim = monomials.cols()-1;
   const unsigned long n_orthants = 1UL << dim;

   IncidenceMatrix<NonSymmetric> rfacets(n_orthants, facets.rows());
   IncidenceMatrix<NonSymmetric> opt = optimal_monomials(monomials, coefficients, facets, vertices);
   for (Orthant o = 0; o < n_orthants; ++o)
      rfacets[o] = real_facets_in_orthant(o, facets, monomials, signs, opt);

   return rfacets;
}

// REALIZATION:

// translate vertices into positive orthant (and dehomogenize):
template <typename Addition>
Matrix<Rational> move_to_positive(const Matrix<Rational>& vertices, const Set<Int>& far_vertices)
{
   Set<Int> finite_vertices = range(0, vertices.rows()-1)-far_vertices;
   Matrix<Rational> vertices_moved = (-Addition::orientation())*vertices.minor(All, range(2, vertices.cols()-1));
   for (Int c = 0; c < vertices_moved.cols(); ++c) {
      Rational min = std::numeric_limits<Rational>::infinity();
      for (auto r = entire(finite_vertices); !r.at_end(); r++) {
         if (vertices_moved[*r][c] < min)
            min = vertices_moved[*r][c];
      }
      for (auto r = entire(finite_vertices); !r.at_end(); r++)
         vertices_moved[*r][c] += 1 - min;
   }
   vertices_moved = vertices.col(0)|vertices_moved;
   return vertices_moved;
}

// find the max. cells in the dual subdivision which are dual to the vertices:
template <typename Addition>
IncidenceMatrix<NonSymmetric> dual_facets(
      const Matrix<Rational>&                 vertices,
      const Set<Int>&                         far_vertices,
      const Matrix<Int>&                      monomials,
      const Vector<TropicalNumber<Addition>>& coefficients)
{
   IncidenceMatrix<NonSymmetric> duals(vertices.rows(), monomials.rows());

   Matrix<Rational> sub = monomials*T(vertices.minor(All, ~scalar2set(0))) + repeat_col(Vector<Rational>(coefficients), vertices.rows());
   for (Int v = 0; v < sub.cols(); ++v) {
      if (!far_vertices.contains(v))
         duals.row(v) = (tropical::extreme_value_and_index<Addition>( Vector<TropicalNumber<Addition>>(sub.col(v)) )).second;
   }

   return duals;
}

// move vertices to barycenters of resp. facets in the dual subdivision:
template <typename Addition>
Matrix<Rational> move_to_bary(
      const Matrix<Rational>&                 vertices,
      const Matrix<Int>&                      monomials,
      const Vector<TropicalNumber<Addition>>& coefficients,
      const Set<Int>&                         far_vertices)
{
   Matrix<Rational> vertices_moved(vertices.rows(), vertices.cols()-2);
   IncidenceMatrix<NonSymmetric> duals = dual_facets(vertices, far_vertices, monomials, coefficients);

   for (Int v = 0; v < vertices_moved.rows(); ++v) {
      if (far_vertices.contains(v))
         vertices_moved.row(v) = (-Addition::orientation())*(vertices.row(v)).slice(range(2, vertices.cols()-1));
      else
         vertices_moved.row(v) = barycenter(Matrix<Rational>(monomials.minor(duals.row(v), ~scalar2set(0))));
   }

   return vertices.col(0)|vertices_moved;
}

// realize the real part of the hypersurface in IR^dim:
template <typename Addition>
BigObject real_part_realize(
      const Matrix<Int>&                               monomials,
      const Vector<TropicalNumber<Addition>>&          coefficients,
      const Matrix<Rational>&                          vertices,
      const IncidenceMatrix<NonSymmetric>&             cells,
      const Set<Int>& far_vertices,
      const IncidenceMatrix<NonSymmetric>& viro_cells,
      const std::string&                               method)
{
   const Int dim = monomials.cols()-1;
   const unsigned long n_orthants = 1UL << dim;
   const Int n_vertices = vertices.rows();

   // find far vertices pointing in coordinate direction:
   Set<Int> far_vertices_cd;
   for (auto i = entire(far_vertices); !i.at_end(); ++i) {
      if (support(vertices.row(*i)).size() == 1)
         far_vertices_cd += *i;
   }

   // dehomogenize and move vertices into positive orthant:
   Matrix<Rational> vertices_moved;
   if (method == "rigid")
      vertices_moved = move_to_positive<Addition>(vertices, far_vertices);
   else if (method == "uniform")
      vertices_moved = move_to_bary(vertices, monomials, coefficients, far_vertices);
   else
      throw std::runtime_error("Unknown realization method.");

   // build the complex:
   Matrix<Rational> points(0, vertices_moved.cols()-1);
   IncidenceMatrix<NonSymmetric> input_polytopes(0, n_vertices*n_orthants);
   Int current_row = 0; // ... of input_polytopes
   for (Orthant O = 0; O < n_orthants; ++O) {

      // reflect vertices to current orthant:
      Matrix<Rational> vertices_reflected(vertices_moved);
      for (Int i = 0; i < dim; ++i) {
         if (O & (1UL << i))
            vertices_reflected.col(i+1).negate();
      }
      points /= vertices_reflected;

      // build maximal cells:
      input_polytopes.resize(input_polytopes.rows() + viro_cells[O].size(), input_polytopes.cols());
      for (auto c = entire(viro_cells[O]); !c.at_end(); ++c, ++current_row) {

         // find orthants c meets (i.e. mirror current orthant along all combinations of coordinate direction rays in c):
         Set<Orthant> orthants = scalar2set(O);
         for (auto cd = entire(cells.row(*c)*far_vertices_cd); !cd.at_end(); ++cd) {
            Set<Int> tmp(orthants);
            for (auto o = entire(tmp); !o.at_end(); ++o) {
               Int index = *(support(vertices_reflected.row(*cd)).begin())-1;
               Orthant o_new = (*o)^(1UL << index);
               orthants += o_new;
            }
         }

         // build cell from all (incl. reflected) vertices:
         for (auto o = entire(orthants); !o.at_end(); o++) {
            for (auto v = entire(cells[*c]-far_vertices_cd); !v.at_end(); ++v)
               input_polytopes(current_row, n_vertices*(*o) + *v) = true;
         }
      }
   }

   return BigObject("fan::PolyhedralComplex<Rational>",
                    "POINTS", points,
                    "INPUT_POLYTOPES", input_polytopes);
}

// Z2 CHAIN COMPLEX:

// Whether orthants 'start' and 'dest' can "see" each other via coordinate 'directions'.
// This is the case iff, as sets, their symmetric difference is either
// contained in 'directions', or doesn't intersect it at all.
bool is_reachable(
      const Int     ambient_dim,
      const Orthant start,
      const Orthant dest,
      const Binary  directions)
{
   const bool foo = directions == (directions | (start ^ dest));
   return foo || (directions == (directions | (((~start)%(1<<(ambient_dim+1))) ^ dest)));
}

// The characteristic vector of a set of integers,
// encoded as a binary number.
Binary set2binary(const Set<Int>& directions)
{
   Binary binary = 0;
   for (auto i = entire(directions); !i.at_end(); ++i)
      binary = binary | (1<<(*i));
   return binary;
}

// Reachable orthants, from 'representative', within 'orthants', via 'directions'.
Set<Orthant> reachable(
      const Int           ambient_dim,
      const Orthant       representative,
      const Set<Orthant>& orthants,
      const Binary        directions)
{
   Set<Orthant> r = Set<Orthant>();
   for (auto o = entire(orthants); !o.at_end(); ++o) {
      if (is_reachable(ambient_dim, representative<<1, (*o)<<1, directions))
         r += *o;
   }
   return r;
}

// Z_2 chain complex of a patchworked hypersurface, build from its dual subdivision.
Array<SparseMatrix<GF2>> chain_complex_from_dualsub(
      const Array<bool>&                                     signs,
      const graph::Lattice<graph::lattice::BasicDecoration>& dual_hasse,
      const Matrix<Rational>&                                dual_vertices)
{
   const Int ambient_dim = dual_vertices.cols() - 2;
   const Int dim = ambient_dim - 1;
   const Int n_orthants  = 1<<ambient_dim;
   const Int n_nodes     = dual_hasse.nodes();
   const Matrix<Int> monoms = (Matrix<Int>)dual_vertices.minor(All,~scalar2set(0));

   // phase structure on nodes of the dual hasse diagram:
   IncidenceMatrix<NonSymmetric> dual_phase(n_nodes-1, n_orthants);
   for (Orthant o = 0; (Int)o < n_orthants; ++o) {
      const Array<bool> S = signs_in_orthant(signs, (Matrix<Int>)dual_vertices.minor(All,~scalar2set(0)), o);
      for (Int n = 1; n < n_nodes-1; ++n)
         if (signs_differ(S, dual_hasse.face(n)))
            dual_phase(n, o) = true;
   }

   // dual ray directions on nodes of the dual hasse diagram:
   Array<Set<Int>> directions(n_nodes);
   for (Int n = 1; n < n_nodes; ++n) {
      directions[n] = range(0,ambient_dim);
      for (auto v = entire(dual_hasse.face(n)); !v.at_end(); ++v)
         directions[n] -= support(monoms[*v]);
   }

   Array<SparseMatrix<GF2>> chain_complex(dim);

   for (Int d = dim; d > 0; d--) { // loop relevant dimensions

      Int rank = ambient_dim - d + 1;

      // boundary matrix for current dimension d:
      ListMatrix<SparseVector<GF2>> boundary;

      // loop hasse nodes of corresponding rank:
      for (auto F = entire(dual_hasse.nodes_of_rank(rank)); !F.at_end(); ++F) {

         // directions (as indizes) of rays in F:
         Binary F_directions = set2binary(directions[*F]);

         // orthants in which copies of F appear:
         Set<Orthant> F_o = (Set<Orthant>)(dual_phase[*F]);

         while (!F_o.empty()) { // 1 loop <=> 1 "connected" copy of F

            // orthants reachable from current "connected" copy of F
            Set<Orthant> F_r = reachable(ambient_dim, *entire(F_o), F_o, F_directions);

            // build boundary row for current "connected" copy of F:
            SparseVector<GF2> F_boundary(n_nodes * n_orthants);
            for (auto f = entire(dual_hasse.out_adjacent_nodes(*F)); !f.at_end(); ++f) {
               Set<Int> f_directions = directions[*f];
               Set<Orthant> f_o(F_r);
               while (!f_o.empty()) {
                  Orthant f_representative = *entire(f_o);
                  Set<Orthant> f_r = reachable(ambient_dim, f_representative, f_o, set2binary(f_directions));
                  F_boundary[(*f) * n_orthants + f_representative] = 1;
                  f_o -= f_r;
               }
            }

            // add boundary row of current "connected copy" to boundary matrix of current dimension:
            boundary /= F_boundary;

            // exclude already processed orthants from next loop:
            F_o -= F_r;

         }

      }

      // convert boundary to sparsematrix and remove empty columns:
      chain_complex[d-1] = SparseMatrix<GF2>(boundary);
      chain_complex[d-1].squeeze_cols();

   }

   return chain_complex;

}
} }

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
// vim: set shiftwidth=3:
