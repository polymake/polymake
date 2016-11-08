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
      perl::Object rep_family_fixed_vertex(perl::Object family) {
         Matrix<Rational> vertices = family.give("VERTICES");
         const IncidenceMatrix<> &polytopes = family.give("MAXIMAL_POLYTOPES");
         const Set<int> &far_vertices = family.give("FAR_VERTICES");
         const int apex = *(sequence(0,vertices.rows()) - far_vertices).begin();

         vertices /= (unit_vector<Rational>(vertices.cols(),0));
         const int vert_index1 = apex;
         const int vert_index2 = vertices.rows()-1;

         Array<Set<int> > rep_polytopes(5);
         auto rep_pol_it = entire(rep_polytopes);
         int no_of_vertices_added = 0;
         for(auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone) {
            if( (*cone).size() == 3) {
               const Set<int> dirs = (*cone) - apex;
               int v_index = no_of_vertices_added? vert_index1 : vert_index2;
               vertices.row(v_index).slice(~scalar2set(0)) = 
                  accumulate( rows(vertices.minor( *cone, ~scalar2set(0))), operations::add());
               for(auto d : dirs) {
                  *rep_pol_it = (scalar2set(v_index) + d);
                  ++rep_pol_it;
               }
               no_of_vertices_added++;
            }
            else {
               *rep_pol_it = *cone;
               ++rep_pol_it;
            }
         }
         *rep_pol_it = (scalar2set(vert_index1) + vert_index2);
         perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
         result.take("VERTICES") << vertices;
         result.take("MAXIMAL_POLYTOPES") << rep_polytopes;
         result.take("WEIGHTS") << ones_vector<Integer>(5);
         return result;

      }

   template <typename Addition>
      perl::Object rep_family_moving_vertex(perl::Object family) {
         Matrix<Rational> vertices = family.give("VERTICES");
         const IncidenceMatrix<> &polytopes = family.give("MAXIMAL_POLYTOPES");
         const Set<int> &far_vertices = family.give("FAR_VERTICES");
         const int apex = *(sequence(0,vertices.rows()) - far_vertices).begin();

         const int moving_dir = *(accumulate( rows(polytopes), operations::mul()) - apex).begin();
         vertices.row(apex) += vertices.row(moving_dir);

         Array<Set<int> > rep_polytopes(4);
         auto rep_pol_it = entire(rep_polytopes);
         Set<int> apex_set = scalar2set(apex);
         for(auto d = entire(far_vertices); !d.at_end(); ++d, ++rep_pol_it) {
            *rep_pol_it = (apex_set + *d);
         }

         perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
         result.take("VERTICES") << vertices;
         result.take("MAXIMAL_POLYTOPES") << rep_polytopes;
         result.take("WEIGHTS") << ones_vector<Integer>(4);
         return result;
      }

   template <typename Addition>
      perl::Object rep_family_fixed_edge(perl::Object family) {
         Matrix<Rational> vertices = family.give("VERTICES");
         const IncidenceMatrix<> &polytopes = family.give("MAXIMAL_POLYTOPES");
         const Set<int> &far_vertices = family.give("FAR_VERTICES");

         Array<Set<int> > rep_polytopes(5);
         auto rep_pol_it = entire(rep_polytopes);
         for(auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone) {
            if( (*cone).size() == 2) {
               *rep_pol_it = *cone; ++rep_pol_it;
            }
            else {
               Set<int> dirs = (*cone) * far_vertices;
               Set<int> apex = (*cone) - dirs;
               for(auto d : dirs) {
                  *rep_pol_it = apex + d;
                  ++rep_pol_it;
               }
            }
         }
         
         perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
         result.take("VERTICES") << vertices;
         result.take("MAXIMAL_POLYTOPES") << rep_polytopes;
         result.take("WEIGHTS") << ones_vector<Integer>(5);
         return result;

      }

   template <typename Addition>
      perl::Object rep_family_moving_edge(perl::Object family) {
         Matrix<Rational> vertices = family.give("VERTICES");
         const IncidenceMatrix<> &polytopes = family.give("MAXIMAL_POLYTOPES");
         const Set<int> &far_vertices = family.give("FAR_VERTICES");
         Vector<int> sorted_apices ( sequence(0,vertices.rows()) - far_vertices);
         const int vert_index1 = sorted_apices[0];
         const int vert_index2 = sorted_apices[1];

         Array<Set<int> > rep_polytopes(5);
         auto rep_pol_it = entire(rep_polytopes);
         Map<int, Set<int> > edge_sides;
            edge_sides[0] = Set<int>();
            edge_sides[1] = Set<int>();

         for(auto cone = entire(rows(polytopes)); !cone.at_end(); ++cone, ++rep_pol_it) {
            if( (*cone).size() == 4) { // = moving edge
               *rep_pol_it = scalar2set(vert_index1) + vert_index2;
            }
            else {
               Set<int> bounded_part = (*cone) - far_vertices;
               Set<int> unbounded_part = (*cone) - bounded_part;
               for(int mp = 0; mp <= 1; mp++) {
                  if(edge_sides[mp].size() == 0) {
                     edge_sides[mp] = bounded_part; 
                  }
                  if(edge_sides[mp] == bounded_part) {
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

         perl::Object result(perl::ObjectType::construct<Addition>("Cycle"));
         Set<int> used_vertices = far_vertices + vert_index1 + vert_index2;
         result.take("VERTICES") << vertices.minor( used_vertices,All);
         result.take("MAXIMAL_POLYTOPES") << (IncidenceMatrix<>(rep_polytopes)).minor(All, used_vertices);
         result.take("WEIGHTS") << ones_vector<Integer>(5);
         return result;
      }

   FunctionTemplate4perl("rep_family_fixed_vertex<Addition>(Cycle<Addition>)");
   FunctionTemplate4perl("rep_family_moving_vertex<Addition>(Cycle<Addition>)");
   FunctionTemplate4perl("rep_family_fixed_edge<Addition>(Cycle<Addition>)");
   FunctionTemplate4perl("rep_family_moving_edge<Addition>(Cycle<Addition>)");

}}

