/*
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA  02110-1301, USA.

   ---
   Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

   ---
   Copyright (c) 2016-2021
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   Computes representatives of families of lines in a smooth cubic surface in P3 
   */

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

template <typename Addition>
BigObject rep_family_fixed_vertex(BigObject family)
{
  Matrix<Rational> vertices = family.give("VERTICES");
  const IncidenceMatrix<>& polytopes = family.give("MAXIMAL_POLYTOPES");
  const Set<Int>& far_vertices = family.give("FAR_VERTICES");
  const Int apex = *(sequence(0,vertices.rows()) - far_vertices).begin();

  vertices /= (unit_vector<Rational>(vertices.cols(),0));
  const Int vert_index1 = apex;
  const Int vert_index2 = vertices.rows()-1;

  Array<Set<Int>> rep_polytopes(5);
  auto rep_pol_it = rep_polytopes.begin();
  Int no_of_vertices_added = 0;
  for (auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone) {
    if (cone->size() == 3) {
      const Set<Int> dirs = (*cone) - apex;
      Int v_index = no_of_vertices_added ? vert_index1 : vert_index2;
      vertices.row(v_index).slice(range_from(1)) =
        accumulate( rows(vertices.minor( *cone, range_from(1))), operations::add());
      for (const Int d : dirs) {
        *rep_pol_it = scalar2set(v_index) + d;
        ++rep_pol_it;
      }
      ++no_of_vertices_added;
    } else {
      *rep_pol_it = *cone;
      ++rep_pol_it;
    }
  }
  *rep_pol_it = scalar2set(vert_index1) + vert_index2;
  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", vertices,
                   "MAXIMAL_POLYTOPES", rep_polytopes,
                   "WEIGHTS", ones_vector<Integer>(5));
}

template <typename Addition>
BigObject rep_family_moving_vertex(BigObject family)
{
  Matrix<Rational> vertices = family.give("VERTICES");
  const IncidenceMatrix<>& polytopes = family.give("MAXIMAL_POLYTOPES");
  const Set<Int>& far_vertices = family.give("FAR_VERTICES");
  const Int apex = *(sequence(0,vertices.rows()) - far_vertices).begin();

  const Int moving_dir = *(accumulate( rows(polytopes), operations::mul()) - apex).begin();
  vertices.row(apex) += vertices.row(moving_dir);

  Array<Set<Int>> rep_polytopes(4);
  auto rep_pol_it = rep_polytopes.begin();
  Set<Int> apex_set = scalar2set(apex);
  for (const Int d : far_vertices) {
    *rep_pol_it = apex_set + d;
    ++rep_pol_it;
  }

  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", vertices,
                   "MAXIMAL_POLYTOPES", rep_polytopes,
                   "WEIGHTS", ones_vector<Integer>(4));
}

template <typename Addition>
BigObject rep_family_fixed_edge(BigObject family)
{
  Matrix<Rational> vertices = family.give("VERTICES");
  const IncidenceMatrix<>& polytopes = family.give("MAXIMAL_POLYTOPES");
  const Set<Int>& far_vertices = family.give("FAR_VERTICES");

  Array<Set<Int>> rep_polytopes(5);
  auto rep_pol_it = rep_polytopes.begin();
  for (auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone) {
    if ((*cone).size() == 2) {
      *rep_pol_it = *cone; ++rep_pol_it;
    } else {
      Set<Int> dirs = (*cone) * far_vertices;
      Set<Int> apex = (*cone) - dirs;
      for (const Int d : dirs) {
        *rep_pol_it = apex + d;
        ++rep_pol_it;
      }
    }
  }
       
  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", vertices,
                   "MAXIMAL_POLYTOPES", rep_polytopes,
                   "WEIGHTS", ones_vector<Integer>(5));
}

template <typename Addition>
BigObject rep_family_moving_edge(BigObject family)
{
  Matrix<Rational> vertices = family.give("VERTICES");
  const IncidenceMatrix<>& polytopes = family.give("MAXIMAL_POLYTOPES");
  const Set<Int>& far_vertices = family.give("FAR_VERTICES");
  Vector<Int> sorted_apices ( sequence(0,vertices.rows()) - far_vertices);
  const Int vert_index1 = sorted_apices[0];
  const Int vert_index2 = sorted_apices[1];

  Array<Set<Int>> rep_polytopes(5);
  auto rep_pol_it = rep_polytopes.begin();
  Map<Int, Set<Int>> edge_sides;
  edge_sides[0] = Set<Int>();
  edge_sides[1] = Set<Int>();

  for (auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone, ++rep_pol_it) {
    if ((*cone).size() == 4) { // = moving edge
      *rep_pol_it = scalar2set(vert_index1) + vert_index2;
    } else {
      Set<Int> bounded_part = (*cone) - far_vertices;
      Set<Int> unbounded_part = (*cone) - bounded_part;
      for (Int mp = 0; mp <= 1; ++mp) {
        if (edge_sides[mp].size() == 0) {
          edge_sides[mp] = bounded_part; 
        }
        if (edge_sides[mp] == bounded_part) {
          *rep_pol_it =  unbounded_part + ((!mp)? vert_index1 : vert_index2);
          break;
        }
      }
    }
  }
  Vector<Rational> v1 = accumulate( rows(vertices.minor(edge_sides[0],All)), operations::add()) / 2;
  Vector<Rational> v2 = accumulate( rows(vertices.minor(edge_sides[1],All)), operations::add()) / 2;
  vertices.row(vert_index1) = v1;
  vertices.row(vert_index2) = v2;

  Set<Int> used_vertices = far_vertices + vert_index1 + vert_index2;
  return BigObject("Cycle", mlist<Addition>(),
                   "VERTICES", vertices.minor(used_vertices, All),
                   "MAXIMAL_POLYTOPES", IncidenceMatrix<>(rep_polytopes).minor(All, used_vertices),
                   "WEIGHTS", ones_vector<Integer>(5));
}

FunctionTemplate4perl("rep_family_fixed_vertex<Addition>(Cycle<Addition>)");
FunctionTemplate4perl("rep_family_moving_vertex<Addition>(Cycle<Addition>)");
FunctionTemplate4perl("rep_family_fixed_edge<Addition>(Cycle<Addition>)");
FunctionTemplate4perl("rep_family_moving_edge<Addition>(Cycle<Addition>)");

} }
