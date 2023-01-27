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

#include "polymake/client.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/integer_linalg.h"
#include "polymake/polytope/solve_MILP.h"
#include <algorithm>
#include <functional>


namespace polymake { namespace fulton {

   namespace {

   class CompareByLinearForm
   {
      private:
         const Vector<Rational> LinearForm;
      public:
         CompareByLinearForm(const Vector<Rational> l) : LinearForm(l) {}

         pm::cmp_value operator() (const Vector<Integer>& a, const Vector<Integer>& b) const
         {
            pm::cmp_value result = operations::cmp()(- (LinearForm * a), - (LinearForm * b));
            // break ties
            if (result == pm::cmp_eq) {
               pm::cmp_value lex_cmp_value = lex_compare(a, b);

               if (lex_cmp_value == pm::cmp_lt) return pm::cmp_gt;
               return pm::cmp_lt;
            }

            return result;
         }
   };

   template <typename Vec>
   Vector<Integer> component_positive_max(const Vector<Integer>& u, const GenericVector<Vec>& v) {
      return Vector<Integer>(attach_operation(attach_operation(u, zero_vector<Integer>(u.size()), operations::max()), v.top(), operations::max()));
   }

   Vector<Integer> maximal_decreasing_path(const Vector<Integer>& alpha,
         const Set<Vector<Integer>, CompareByLinearForm>& edge_directions)
   {
      Vector<Integer> min_alpha(alpha);
      bool subtraction_occured = true;

      while (subtraction_occured) {
         subtraction_occured = false;
         for (const auto &edge_direction : edge_directions) {
            if (accumulate(min_alpha - edge_direction, operations::min()) >= 0) {
               min_alpha -= edge_direction;
               subtraction_occured = true;
               break;
            }
         }
      }

      return min_alpha;
   }

   bool u_v_w_criterion(const Set<Vector<Integer>>& u_v, Set<Vector<Integer>, CompareByLinearForm>& G) {
      const Vector<Integer>& u(u_v.front());
      const Vector<Integer>& v(u_v.back());
      const Vector<Integer> z_u_v(component_positive_max(u, v));
      Int u_index = std::numeric_limits<Int>::max();
      Int v_index = std::numeric_limits<Int>::max();
      Int w_index = 0;
      for (const auto &w : G) {
         w_index +=1;

         if (w == u) {
            u_index = w_index;
            continue;
         }

         if (w == v) {
            v_index = w_index;
            continue;
         }

         const Vector<Integer> z_u_w(component_positive_max(u, w));
         const Vector<Integer> z_v_w(component_positive_max(v, w));
         auto uv_uw_min = accumulate(z_u_v - z_u_w, operations::min());
         auto uv_vw_min = accumulate(z_u_v - z_v_w, operations::min());

         if ( uv_uw_min > 0 && uv_vw_min > 0) return true;

         if (z_u_v == z_u_w && uv_vw_min > 0 && w_index < v_index)
            return true;


         if (uv_uw_min > 0 && z_u_v == z_v_w && w_index < u_index)
            return true;

         if (z_u_v == z_u_w && z_u_v == z_u_w &&
               w_index < u_index &&
               w_index < v_index) return true;
      }

      return false;
   }

   Int get_index_pos(const Set<Int>& sigma, const Int& index) {
      // finds the position of the index in the given set of indices
      Int pos = 0;
      for(const Int &i : sigma) {
         if (i < index) {
            pos += 1;
         }
      }
      return pos;
   }

   } // end anonymous namespace

   Set<Vector<Integer>> geometric_buchberger(const Vector<Rational>& ordering,
         const Set<Vector<Integer>>& initial_edge_directions,
         const Set<Int>& independant_indices)
   {
      if (initial_edge_directions.size() == 1) return initial_edge_directions;
      CompareByLinearForm comparator(ordering);
      Set<Vector<Integer>,
         CompareByLinearForm> final_edge_directions(comparator);

      auto zero = zero_vector<Integer>(ordering.size());

      for (const auto &edge_direction : initial_edge_directions){
         const Vector<Integer> u_plus(component_positive_max(edge_direction, zero));
         const Vector<Integer> u_minus(component_positive_max(- edge_direction, zero));

         if (comparator(u_plus, u_minus) == pm::cmp_gt) {
            final_edge_directions += edge_direction;
         } else {
            final_edge_directions += -1 * edge_direction;
         }
      }

      Set<Vector<Integer>> edges(entire(final_edge_directions));
      Set<Set<Vector<Integer>>> pairs = all_subsets_of_k(edges, 2);

      while (!pairs.empty()) {
         Set<Vector<Integer>> pair = pairs.front();
         pairs.pop_front();

         Vector<Integer> u(pair.front());
         Vector<Integer> v(pair.back());

         // disjoint positive support criterion
         SparseMatrix<Integer> u_plus = diag(component_positive_max(u, zero));
         Vector<Integer> v_plus(component_positive_max(v, zero));
         Vector<Integer> uv_plus_support = u_plus * v_plus;

         if (uv_plus_support == zero) continue;

         // triplet criterion
         if (u_v_w_criterion(pair, final_edge_directions)) continue;

         Vector<Integer> z(component_positive_max(pair.front(), pair.back()));
         Vector<Integer> alpha = z - u;
         Vector<Integer> beta = z - v;

         // **** completion procedure ****
         Vector<Integer> x = maximal_decreasing_path(alpha, final_edge_directions);
         Vector<Integer> y = maximal_decreasing_path(beta, final_edge_directions);

         // cancelation criterion
         // currently uncommenting check produces incorrect results
         SparseMatrix<Integer> x_diag = diag(x.slice(independant_indices));
         Vector <Integer> zero_trunc(independant_indices.size());
         // if  (x_diag * y.slice(independant_indices) != zero_trunc)   continue;

         Vector<Integer> r = x - y;

         if (r == zero) continue;

         const Vector<Integer> r_plus(component_positive_max(r, zero));
         const Vector<Integer> r_minus(component_positive_max(- r, zero));

         if (comparator(r_plus, r_minus) == pm::cmp_lt) negate(r);

         if (final_edge_directions.contains(r) == 0 && final_edge_directions.contains(- r) == 0) {
            for(const auto& edge_direction : final_edge_directions) {
               Set<Vector<Integer>> new_pair;
               new_pair += r;
               new_pair += edge_direction;
               pairs += new_pair;
            }
            final_edge_directions += r;
         }
      }

      return Set<Vector<Integer>>(entire(final_edge_directions));
   }

   Matrix<Integer> markov_basis(const Matrix<Integer>& lattice_basis)
   {
      Matrix<Integer> G = T(lattice_basis);
      G = hermite_normal_form(G).hnf;
      G = T(remove_zero_rows(T(G)));

      Int ambient_dim = rows(G).size();
      Int row_index = 0;
      Set<Int> dependant_indices;

      for (const auto &col : cols(G)) {
         while (col[row_index] == 0) {
            dependant_indices += row_index;
            row_index += 1;
         }
         row_index += 1;
      }

      dependant_indices += range(row_index, ambient_dim - 1);
      auto all_indices = range(0, ambient_dim - 1);

      while (!dependant_indices.empty()) {
         // choosing first index is not always optimal
         Int index = dependant_indices.back();

         Set<Int> sigma = range(0, ambient_dim - 1) - dependant_indices;
         auto M_minor = G.minor(sigma + index, All);
         Int column_size = M_minor.rows();
         Int row_size = M_minor.cols();

         Vector<Integer> c(column_size);
         Int index_pos = get_index_pos(sigma, index);
         c[index_pos] = -1;
         Matrix<Rational> inequalities(c | M_minor);
         auto MILP = polymake::polytope::solve_MILP(inequalities, ones_vector<Rational>(row_size + 1),
               range(0, row_size), false);

         if (MILP.status == polymake::polytope::LP_status::infeasible){
            Set<Int> sigma_complement = all_indices - sigma;
            Matrix<Rational> ineqs(unit_matrix<Integer>(ambient_dim + 1));
            Matrix<Rational> equalities(T(lattice_basis).row(index) | lattice_basis);
            equalities /= (zero_vector<Integer>(sigma_complement.size()) | unit_matrix<Integer>(ambient_dim).minor(sigma_complement, All));

            auto LP = polymake::polytope::solve_LP(ineqs, equalities, ones_vector<Rational>(ambient_dim + 1), false);

            Vector<Rational> ordering = LP.solution.slice(range(1, ambient_dim));
            Set<Vector<Integer>> edge_directions(cols(G));

            G = T(Matrix<Integer>(geometric_buchberger(ordering,
                        edge_directions,
                        sigma)));
         } else {
            Vector<Rational> lattice_coordinates = MILP.solution.slice(range(1, row_size));
            Set<Vector<Integer>> column_set(cols(G));
            column_set += G * lattice_coordinates;
            G.resize(ambient_dim, column_set.size());
            copy_range(entire(column_set), cols(G).begin());
         }
         dependant_indices.pop_back();
      }

      return T(G);
   }

   Matrix<Integer> markov_basis_from_matrix(const Matrix<Integer>& M, bool use_kernel) {
      if (use_kernel) {
         Matrix<Integer> L = null_space_integer(M);

         if (is_zero(L)){
            throw std::runtime_error("Null Space of Matrix is 0");
         }
         return markov_basis(L);
      }
      return markov_basis(M);
   }

   Matrix<Integer> markov_basis_with_options(const Matrix<Integer>& M, OptionSet options) {
      return markov_basis_from_matrix(M, options["use_kernel"]);
   }

   Matrix<Integer> markov_basis_from_polytope(BigObject p) {
      const Array<Matrix<Integer>> l = p.give("LATTICE_POINTS_GENERATORS");

      return markov_basis_from_matrix(T(l[0]), true);
   }

   Set<Vector<Integer>> markov_basis_from_generating_set(const Set<Vector<Integer>>& S){
      return Set<Vector<Integer>>(rows(markov_basis(Matrix<Integer>(S))));
   }

    UserFunction4perl("# Implementation of Project and Lift algorithm by Hemmecke and Malkin."
		      "# Given a spanning set of a lattice returns a markov basis."
		      "# @param Set<Vector<Integer>> S"
		      "# @return Set<Vector<Integer>>"
		      "# @example" 
		      "# > $s = new Set<Vector<Integer>>([1, -2, 1], [1, 1, -1]);"
		      "# > print markov_basis($s);"
		      "# | {<-2 1 0> <-1 -1 1> <0 -3 2>}",
		      &markov_basis_from_generating_set, "markov_basis(Set<Vector<Integer>>)");

    UserFunction4perl("# Implementation of Project and Lift algorithm by Hemmecke and Malkin."
		      "# Given a Matrix whose rows form a spanning set of a lattice return markov basis as rows of an Integer Matrix, "
		      "# or if use_kernel = true, returns a markov basis of integer kernel of given Matrix as rows of an Integer Matrix."
		      "# @param Matrix<Integer> M"
		      "# @option Bool use_kernel = false"
		      "# @return Matrix<Integer>"
		      "# @example"
		      "# > $M = new Matrix<Integer>([[1, 1, 2, -2], [-1, 3, -2, 1]]);"
		      "# > print markov_basis($M, {\"use_kernel\" => true});"
		      "# | 0 2 7 8"
		      "# | 1 1 3 4"
		      "# > $M = new Matrix<Integer>([[1, 1, 2, -2], [-1, 3, -2, 1]]);"
		      "# > print markov_basis($M);"
		      "# | 0 -4 0 1"
		      "# | 1 -7 2 0"
		      "# | 1 1 2 -2"
		      ,
		      &markov_basis_with_options, "markov_basis(Matrix<Integer>, {\"use_kernel\"=>false})");
    
    UserFunction4perl("# Implementation of Project and Lift algorithm by Hemmecke and Malkin."
		      "# Given a polytope return the markov basis of the lattice spanned by it's lattice points as rows of an Integer Matrix,"
		      "# @param  Polytope<Rational> P"
		      "# @return Matrix<Integer>"
		      "# @example"
		      "# > $P = new Polytope(VERTICES=>[[1, 1, 0], [1, 0, 1], [1, 1, 1], [1, 0, 0]]);"
		      "# > print markov_basis($P);"
		      "# | 1 -1 -1 1"
		      , &markov_basis_from_polytope, "markov_basis(Polytope<Rational>)");
  }
}

