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
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Graph.h"
#include "polymake/polytope/solve_LP.h"

/*
  http://www.uni-frankfurt.de/fb/fb12/mathematik/dm/personen/steffens/Dokumente/MV_Computation1.pdf   [1]
  http://www.uni-frankfurt.de/fb/fb12/mathematik/dm/personen/steffens/Dokumente/Diss1.pdf             [2]
*/

namespace polymake { namespace polytope {

template <typename E>
using matrix_list = Array<Matrix<E>>;
template <typename E>
using vector_list = Array<Vector<E>>;
using graph_list = Array<Graph<Undirected>>;

template<typename E>
Matrix<E> list2matrix(const std::vector<Matrix<E>>& v, const Int r, const Int c)
{
    Matrix<E> A(r, c);
    auto Arows = rows(A).begin();
    for (const auto& M : v) {
       Arows = copy_range(entire(rows(M)), Arows);
    }
    return A;
}

template<typename E>
E solve_lp_mixed_volume(const Matrix<E>& M, const Vector<E>& objective)
{
    Matrix<E> eqs(M.cols()-1, M.cols());
    for (Int j = 0; j < M.cols()-1; ++j)
       eqs.row(j) = unit_vector<E>(M.cols(),j+1);
    const auto S = solve_LP(eqs, M, objective, true);
    if (S.status != LP_status::valid)
       throw std::runtime_error("mixed_volume: wrong LP");
    return S.objective_value;
}

template<typename E>
Matrix<E> construct_A(const Int n, const Array<Int>& r, const matrix_list<E>& polytopes, const vector_list<E>& lifted_edges)
{
    std::vector<Matrix<E> > c;
    Int R = 0;
    for (Int i = 0; i < n; ++i) {
       Matrix<E> B = zero_matrix<E>(lifted_edges[i].size(),n+1);
       B.col(0) = lifted_edges[i];
       B.col(i+1) = ones_vector<E>(r[i]);
       c.push_back((polytopes[i]| B));
       R+=r[i];
    }
    Matrix<E> A = list2matrix(c, R, 2*n+2);
    Vector<E> vec = unit_vector<E>(A.cols(),polytopes[0].cols());
    A /= vec;
    return T(A.minor(All,sequence(1,A.cols()-1))); // slice ones and transpose
}

template<typename E>
bool lower_envelope_check(Matrix<E>& A, const Int n, const Int k, const Array<Int>& r, const Vector<E>& m)
{
    Int R = 0;
    assert(r.size() >= k);
    for (Int i = 0; i < k; ++i)
       R += r[i];
    const Vector<E> b = m | ones_vector<E>(n);
    const Vector<E> obj = unit_vector<E>(R+2,R+1);
    T(A).row(0) = -b;

    const Matrix<E> M(A.minor(sequence(0,n+k+1),~sequence(R+1,A.cols()-R-2)));
    return solve_lp_mixed_volume(M, obj)==0;
}

template<typename E>
E volume(const Int n, const Array<Int>& node, const Array<Int>& next, const matrix_list<E>& polytopes, const graph_list& graphs)
{
    Matrix<E> A(n, polytopes[0].cols()-1);
    for (Int j = 0; j < n; ++j) {
       auto it = entire(graphs[j].adjacent_nodes(node[j]));
       for (Int i = 0; i < next[j]; ++i)  //reset iterator
          ++it;
       A.row(j) = (polytopes[j].row(node[j]) - polytopes[j].row(*it)).slice(range_from(1));
    }
    E d = det(A);
    if (d == 0)
       throw std::runtime_error("mixed_volume: calculation failed, edge matrix is singular.");
    // check (2.9) in [2]. change the  Lift-functions
    return abs(d);
}

template <typename E>
E mixed_volume(const Array<BigObject>& summands)
{
   E vol(0);             // mixedVolume
   const Int n = summands.size();      // number of (input)polytopes
   matrix_list<E> polytopes(n);      // stores matrices s.t. the i-th entry is a discribtion of P_j by vertices
   vector_list<E> lifted_edges(n);
   graph_list graphs(n);         // stores all graphs from the input polytopes P_j
   Array<Int> node(n);
   Array<Int> next(n);
   Array<Int> r(n);              //stores the number of input Points

   //initialization:
   Int j = 0;
   Vector<E> Lift(n+1);
   Lift[0] = 0;
   for (const BigObject& s : summands) {
      const Matrix<E> m = s.give("VERTICES");
      polytopes[j] = m;
      r[j] = m.rows();
      for (Int k = 1; k <= n; ++k)        //LIFT
         Lift[k] = 1 + j*(1 - j*(1 - k*j)); //1 + j - j*j + k*j*j*j;
      lifted_edges[j] = m*Lift;
      const Graph<Undirected> graph=s.give("GRAPH.ADJACENCY");
      graphs[j]=graph;
      ++j;
   }
   if (n != polytopes[0].cols()-1)
      throw std::runtime_error("mixed_volume: dimension and number of input polytopes mismatch");
   Matrix<E> A = construct_A(n, r, polytopes, lifted_edges);
   A = ones_vector<E>(A.rows()) | A;

   // Algo:
   Vector<E> m = zero_vector<E>(n+1);
   Vector<E> temp;
   Int count;

   j = 0;
   for (Int i = 0; i < polytopes[j].rows(); ++i) {
      auto it = entire(graphs[j].adjacent_nodes(i));
      for (count = 0; count < polytopes[j].rows()-1; ++count) {
         if (*it > i) {
            temp = ((polytopes[j].row(i) + polytopes[j].row(*it))/2).slice(range_from(1))
                 | (lifted_edges[j][i] + lifted_edges[j][*it])/2;
            if (lower_envelope_check(A, n, j+1, r, Vector<E>(m+temp))) {
               node[j] = i;
               next[j] = count;
               if (j == n-1) {
                  vol += volume(n, node, next, polytopes, graphs);
               } else {
                  ++j;
                  m += temp;
                  count = polytopes[j].rows();  //jump out of loop
                  i = -1;
               }
            }
         }
         ++it;
         if (j > 0 && it.at_end()&& i == polytopes[j].rows()-1) {
            --j;
            i = node[j];
            it = entire(graphs[j].adjacent_nodes(i));
            count = next[j];
            for (Int k = 0; k < count; ++k) //reset iterator
               ++it;
            temp = ((polytopes[j].row(i) + polytopes[j].row(*it))/2).slice(range_from(1))
                 | (lifted_edges[j][i] + lifted_edges[j][*it])/2;
            m -= temp;
            ++it;
         }
         if (it.at_end())
            break;
      }
   }
   return vol;
}


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Produces the mixed volume of polytopes P<sub>1</sub>,P<sub>2</sub>,...,P<sub>n</sub>."
                          "# @param Polytope<Scalar> P1 first polytope"
                          "# @param Polytope<Scalar> P2 second polytope"
                          "# @param Polytope<Scalar> Pn last polytope"
                          "# @return Scalar mixed volume"
                          "# @example"
                          "# > print mixed_volume(cube(2),simplex(2));"
                          "# | 4",
                          "mixed_volume<Scalar>(Polytope<Scalar> +)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
