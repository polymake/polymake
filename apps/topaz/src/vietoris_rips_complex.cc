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

#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/topaz/Filtration.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace topaz {

BigObject vietoris_rips_complex(const Matrix<Rational>& dist, Rational step)
{
  BigObject NG = call_function("neighborhood_graph", dist, step);
  BigObject vr_complex = call_function("clique_complex", NG);
  vr_complex.set_description() << "Vietoris Rips complex of the input point set." << endl;
  return vr_complex;
}

// this does not use the existing clique/flag complex functionality, as we need to calculate degrees for each face.
template <typename Coeff>
Filtration<SparseMatrix<Coeff>> vietoris_rips_filtration(const Matrix<double>& dist, const Array<Int>& point_degs, double step, Int k)
{
  using pair = std::pair<Int, Int>;

  Int n = dist.rows();

  if (k>=n) k=n-1; //TODO this is ugly

  Map<Set<Int>, pair> deg_idx; //first pair entry is degree in filtration, second is index in bd matrix

  Int size = 0;
  for (Int i = 1; i <= k+1; ++i)
    size += Int(Integer::binom(n, i)); //number of all subsets of [n] with <=k members
  Array<Cell> F(size); //TODO: IntType thingy

  Int cell_index = 0; //index in filtration

  Array<SparseMatrix<Coeff>> bd(k+1);
  bd[0] = SparseMatrix<Coeff>(n,1);

  for (Int index = 0; index < n; ++index) {
    bd[0][index][0] = 1;
    deg_idx[Set<Int>{index}] = pair(point_degs[index], index); //get point degrees from input array
    Cell c(point_degs[index], 0, index);
    F[cell_index++] = c;
  }
  Set<Int> facet = sequence(0, n); //the full-dimensional simplex

  for (Int d = 1; d <= k; ++d) { //iterate over dimensions to max_dim
    bd[d] = SparseMatrix<Coeff>(Int(Integer::binom(n,d+1)), Int(Integer::binom(n,d)));

    Int index = 0; //index in bd matrix

    for (auto i = entire(all_subsets_of_k(facet,d+1)); !i.at_end(); ++i) { //iterate over all simplices of dimension d

      Int sgn = 1;  //entry in bd matrix simply alternates as simplices are enumerated lexicographically
      Int max = 0; //maximal degree of boundary simplices
      Set<Int> simplex(*i);

      for (auto s=entire(all_subsets_of_k(simplex,d)); !s.at_end(); ++s){ //iterate over boundary and find maximal degree

        pair p = deg_idx[*s];

        Int s_deg = p.first;
        if (s_deg > max) max = s_deg;

        bd[d][index][p.second] = sgn; // set bd matrix entry
        sgn *= -1;
      }
      if (d == 1) { //for edges, calculate degree from distance matrix
         const double l = double(dist(simplex.front(), simplex.back()));
        assign_max(max, Int(ceil(l / step)));
      }

      deg_idx[simplex] =  pair(max,index);
      Cell c(max, d, index);
      F[cell_index++] = c;
      ++index;
    }
  }
  return Filtration<SparseMatrix<Coeff> >(F,bd);
}


UserFunction4perl("# @category Producing a simplicial complex from other objects"
                  "# Computes the __Vietoris Rips complex__ of a point set. The set is passed as its so-called \"distance matrix\", whose (i,j)-entry is the distance between point i and j. This matrix can e.g. be computed using the distance_matrix function. The points corresponding to vertices of a common simplex will all have a distance less than //delta// from each other."
                  "# @param Matrix D the \"distance matrix\" of the point set (can be upper triangular)"
                  "# @param Rational delta"
                  "# @return SimplicialComplex"
                  "# @example The VR-complex from 3 points (vertices of a triangle with side lengths 3, 3, and 5) for different delta:"
                  "# > print vietoris_rips_complex(new Matrix([[0,3,3],[0,0,5],[0,0,0]]), 2)->FACETS;"
                  "# | {0}"
                  "# | {1}"
                  "# | {2}"
                  "# > print vietoris_rips_complex(new Matrix([[0,3,3],[0,0,5],[0,0,0]]), 4)->FACETS;"
                  "# | {0 1}"
                  "# | {0 2}"
                  "# > print vietoris_rips_complex(new Matrix([[0,3,3],[0,0,5],[0,0,0]]), 6)->FACETS;"
                  "# | {0 1 2}"
                  ,&vietoris_rips_complex, "vietoris_rips_complex($$)");


UserFunctionTemplate4perl("# @category Other"
                          "# Constructs the k-skeleton of the Vietrois Rips filtration of a point set. The set is passed as its so-called \"distance matrix\", whose (i,j)-entry is the distance between point i and j. This matrix can e.g. be computed using the distance_matrix function. The other inputs are an integer array containing the degree of each point, the desired distance step size between frames, and the dimension up to which to compute the skeleton. Redundant points will appear as separate vertices of the complex. Setting k to |S| will compute the entire VR-Complex for each frame."
                          "# @param Matrix D the \"distance matrix\" of the point set (can be upper triangular)"
                          "# @param Array<Int> deg the degrees of input points"
                          "# @param Float step_size "
                          "# @param Int k dimension of the resulting filtration"
                          "# @tparam Coeff desired coefficient type of the filtration"
                          "# @return Filtration<SparseMatrix<Coeff, NonSymmetric> >",
                          "vietoris_rips_filtration<Coeff>($$$$)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
