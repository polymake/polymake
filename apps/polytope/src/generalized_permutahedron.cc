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
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/SparseVector.h"
#include "polymake/Vector.h"


namespace polymake { 
namespace polytope {

template<typename Scalar>
Map<Set<Int>, Scalar> building_set_ycoord_2_zcoord(const Map<Set<Int>, Scalar>& ycoord){
   Map<Set<Int>, Scalar> zcoord;
   Map<Int, Set<Set<Int>>> sorted_keys;
   // This allows us to skip determining the dimension and reduces the number
   // of containment checks later.
   for(const auto& h: ycoord){
      sorted_keys[h.first.size()] += h.first;
   }
   // A building set must contain all singletons, hence we can determine the
   // ground interval in the following way:
   Set<Int> all = sequence(0, sorted_keys[1].size());
   for(Int i=1; i<=all.size(); i++){
      for(const auto& zset : all_subsets_of_k(all, i)){
         zcoord[zset] = 0;
         for(Int j=1; j<=zset.size(); j++){
            for(const auto& yset: sorted_keys[j]){
               if(incl(yset, zset) <= 0){
                  zcoord[zset] += ycoord[yset];
               }
            }
         }
      }
   }
   return zcoord;
}

template<typename Scalar>
BigObject generalized_permutahedron(Int d, const Map<Set<Int>,Scalar>& z)
{
   BigObject p("Polytope", mlist<Scalar>());
   p.set_description() << "generalized permutahedron of dimension " << d << endl;

   // generate inequalities of the generalized permutahedron
   Matrix<Scalar> ineqs(z.size(), d+1);
   typename Rows<Matrix<Scalar>>::iterator r=rows(ineqs).begin();
   for (const auto& h : z) { 
      (*r)[0] = -h.second;
      for (const auto i : h.first){
         (*r)[i+1] = 1;
      }
      r++;
   }
   
   // generate equation of the generalized permutahedron (if existent)
   if (z.exists(range(0,d-1))){
      Matrix<Scalar> eq(1, d+1);
      eq[0][0] = -z[range(0,d-1)];
      for (int i = 1; i <= d; ++i){
         eq[0][i] = 1;
      }
      p.take("EQUATIONS") << eq;
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("INEQUALITIES") << ineqs;
   return p;
}

template<typename Scalar>
BigObject SIM_body(const Vector<Scalar>& alpha)
{
   const Int n = alpha.size();
   if (n < 1) {
      throw std::runtime_error("SIM-body: dimension must be at least 1");
   }
   Scalar tmp = 0;
   for(Int i = 0; i < n; i++) {
      if (alpha[n-1-i] < tmp) {
         throw std::runtime_error("SIM-body: input is not descending");
      }
      tmp = alpha[n-1-i];
   }
   Map<Set<Int>,Scalar> z;
   auto curr_set = entire(all_subsets(range(0,n)));
   curr_set++;  // first entry is ignored, it is the empty set
   while (!curr_set.at_end()) {
      Scalar sum = 0;
      // check whether n+1 is in the current set
      for (auto el = entire(*curr_set); !el.at_end(); el++){
         if (*el == n) {
            for(Int i = 0; i < (*curr_set).size()-1; i++) {
               sum += alpha[n-1-i];
            }
         }
      }
      z[*curr_set] = sum;
      curr_set++;
   }

   return generalized_permutahedron(n+1, z);
}

template<typename Scalar>
BigObject pitman_stanley(const Vector<Scalar>& y) {

   BigObject p("Polytope", mlist<Scalar>());

   const Int n = y.size();
   if (n < 1) {
      throw std::runtime_error("pitman_stanley: length of input must be at least 1");
   }

   // We do not check if the parameters are actually positive; negative values
   // are legal but that does not yield a Pitman-Stanley polytope.
   // Zeros just reduce the dimension; negative numbers may produce unbounded polyhedra.
   Vector<Scalar> t(n+1);
   t[0] = -y[0]; t[1] = 1;
   ListMatrix<Vector<Scalar>> ineqs(0,n+1);
   for (Int i=1; i<n; ++i) {
      ineqs /= t;
      t[0] -= y[i];
      t[i+1] = 1;
   }
   ineqs /= (zero_vector<Scalar>(n) | unit_matrix<Scalar>(n));

   ListMatrix<Vector<Scalar>> eqs(0,n+1);
   eqs /= t;
   
   p.take("CONE_AMBIENT_DIM") << n+1;
   p.take("INEQUALITIES") << ineqs;
   p.take("EQUATIONS") << eqs;
   
   return p;
}         


template<typename Scalar>
BigObject gelfand_tsetlin(const Vector<Scalar>& lambda, OptionSet options){
   bool projected = options["projected"];
   // Index the entries of the triangular array. Skip first row if redundant
   // dimensions should be omitted.
   Map<Int, Map<Int, Int>> unfolding;
   // The indexing counter has to start at 1, since the 0th coordinate is
   // reserved for homogenization.
   Int counter = 1, n = lambda.dim();
   Int starting_row = projected ? 1:0;
   for(Int i=starting_row ; i<n; i++){
      for(Int j=0; j<n-i; j++){
         unfolding[i][j] = counter++;
      }
   }
   Int dim = counter;
   // The equations and inequalities are only differences of two unit vectors
   // each, so it makes sense to use a sparse format.
   ListMatrix<SparseVector<Scalar>> equations(0,dim), inequalities(0,dim);

   // Make equations for first row, in case we don't want redundant dimensions
   // these are turned into inequalities.
   for(Int i=0; i<n-starting_row; i++){
      if(!projected){
         SparseVector<Scalar> equat = lambda[i]*unit_vector<Scalar>(dim, 0) - unit_vector<Scalar>(dim, unfolding[0][i]);
         equations /= equat;
      } else {
         SparseVector<Scalar> ineq_vert = lambda[i]*unit_vector<Scalar>(dim, 0) - unit_vector<Scalar>(dim, unfolding[1][i]);
         SparseVector<Scalar> ineq_diag = -lambda[i+1]*unit_vector<Scalar>(dim, 0) + unit_vector<Scalar>(dim, unfolding[1][i]);
         inequalities /= ineq_vert;
         inequalities /= ineq_diag;

      }
   }
   // Inequalities
   for(Int i=starting_row; i<n-1; i++){
      for(Int j=0; j<n-i-1; j++){
         SparseVector<Scalar> ineq_vert = unit_vector<Scalar>(dim, unfolding[i][j]) - unit_vector<Scalar>(dim, unfolding[i+1][j]);
         SparseVector<Scalar> ineq_diag = unit_vector<Scalar>(dim, unfolding[i+1][j]) - unit_vector<Scalar>(dim, unfolding[i][j+1]);
         inequalities /= ineq_vert;
         inequalities /= ineq_diag;
      }
   }

   // Write output
   BigObject result("Polytope", mlist<Scalar>());
   if(projected){
      result.set_description() << "Projected Gelfand-Tsetlin polytope for the sequence (" << lambda << ")" << endl;
   } else {
      result.set_description() << "Gelfand-Tsetlin polytope for the sequence (" << lambda << ")" << endl;
   }
   result.take("EQUATIONS") << equations;
   result.take("INEQUALITIES") << inequalities;
   return result;
}


template<typename Scalar>
Scalar gelfand_tsetlin_counting(const Vector<Scalar>& lambda, OptionSet options){
   Scalar result(1);
   bool lattice = options["lattice_points"];
   for(Int i=0; i<lambda.dim(); i++){
      for(Int j=i+1; j<lambda.dim(); j++){
         if(lattice){
            result *= lambda[i] - lambda[j] + j - i;
         } else {
            result *= lambda[i] - lambda[j];
         }
         result /= j-i;
      }
   }
   return result;
}


template<typename Scalar>
Array<Matrix<Scalar>> gelfand_tsetlin_diagrams(const Matrix<Scalar>& pts){
   Array<Matrix<Scalar>> result(pts.rows());
   Int size = pts.cols() - 1, n = 0;
   while(size > 0){
      size -= ++n;
   }
   if(size != 0){
      throw std::runtime_error("These are not (homogeneous) points of a Gelfand-Tsetlin polytope");
   }
   Int counter = 0;
   for(const auto& row : rows(pts)){
      Matrix<Scalar> entry(n,n);
      Int start = 1;
      for(Int j=n; j>0; j--){
         entry.row(n-j).slice(sequence(0,j)) = row.slice(sequence(start, j));
         start += j;
      }
      result[counter++] = entry;
   }
   return result;
}


UserFunctionTemplate4perl("# @category Coordinate conversions"
                  "# Convert the y-coordinate representation of a generalized permutahedron given"
                  "# via a building set into the z-coordinate representation."
                  "# "
                  "# See 6, 7, 8 of"
                  "# Postnikov, A. (2009) “Permutohedra, Associahedra, and Beyond,”"
                  "# International Mathematics Research Notices, Vol. 2009, No. 6, pp. 1026–1106"
                  "# Advance Access publication January 7, 2009"
                  "# doi:10.1093/imrn/rnn153"
                  "#"
                  "# @tparam Scalar"
                  "# @param Map<Set<Int>, Scalar> The y-coordinates of the building set"
                  "# @return Map<Set<Int>, Scalar> The z-coordinates",
                  "building_set_ycoord_2_zcoord<Scalar>(Map<Set<Int>, type_upgrade<Scalar>>)");


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce a generalized permutahedron via z<sub>I</sub> height function."
                  "# See Postnikov: Permutohedra, associahedra, and beyond, IMRN (2009); doi:10.1093/imrn/rnn153"
                  "# Note that opposed to Postnikov's paper, polymake starts counting at zero."
                  
                  "# @tparam Scalar"
                  "# @param Int d The dimension"
                  "# @param Map<Set<Int>,Scalar> z Values of the height functions for the different 0/1-directions, i.e. for h = height({1,2,4}) we have the inequality x1 + x2 + x4 >= h. The height value for the set containing all coordinates from 0 to d-1 is interpreted as equality. If any value is missing, it will be skipped. Also it is not checked, if the values are consistent for a height function."
                  "# @return Polytope"
                  "# @example To create a generalized permutahedron in 3-space use"
                  "# > $m = new Map<Set,Rational>;"
                  "# > $m->{new Set(0)} = 0;"
                  "# > $m->{new Set(1)} = 0;"
                  "# > $m->{new Set(2)} = 0;"
                  "# > $m->{new Set(0,1)} = 1/4;"
                  "# > $m->{new Set(0,2)} = 1/4;"
                  "# > $m->{new Set(1,2)} = 1/4;"
                  "# > $m->{new Set(0,1,2)} = 1;"
                  "# > $p = generalized_permutahedron(3,$m);",
                  "generalized_permutahedron<Scalar>($, Map<Set<Int>, type_upgrade<Scalar>>)");


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce an //n//-dimensional SIM-body as generalized permutahedron in //(n+1)//-space. SIM-bodies are defined in the article \"Duality and Optimality of Auctions for Uniform Distributions\" by Yiannis Giannakopoulos and Elias Koutsoupias, but the input needs to be descending instead of ascending, as used in \"Generalized Permutahedra and Optimal Auctions\" by Michael Joswig, Max Klimm and Sylvain Spitz."

                  "# @tparam Scalar"
                  "# @param Vector<Scalar> alpha Vector with the parameters (a<sub>1</sub>,...,a<sub>n</sub>) s.t. a<sub>1</sub> >= ... >= a<sub>n</sub> >= 0."
                  "# @return Polytope"
                  "# @example To produce a 2-dimensional SIM-body, use for example the following code. Note that the polytope lives in 3-space, so we project it down to 2-space by eliminating the last coordinate."
                  "# > $p = SIM_body(new Vector(sequence(3,1)));"
                  "# > $s = new Polytope(POINTS=>$p->VERTICES->minor(All,~[$p->CONE_DIM]));",
                  "SIM_body<Scalar>(Vector<type_upgrade<Scalar>>)");


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce a Pitman-Stanley polytope of dimension //n//-1."
                  "# See Pitman and Stanley, Discrete Comput Geom 27 (2002); doi:10.1007/s00454-002-2776-6"
                  
                  "# @tparam Scalar"
                  "# @param Vector<Scalar> y Vector of //n// positive parameters."
                  "# @return Polytope"
                  "# @example Pitman-Stanley polytopes are combinatorial cubes:"
                  "# > $p = pitman_stanley(new Vector([1,1,2,3]));"
                  "# > print $p->F_VECTOR;"
                  "# | 8 12 6",
                  "pitman_stanley<Scalar>(Vector<type_upgrade<Scalar>>)");


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce a Gelfand-Tsetlin polytope for a given sequence."
                  "# "
                  "# See Postnikov: Permutohedra, associahedra, and beyond, IMRN (2009); doi:10.1093/imrn/rnn153"
                  "# Theorem 15.1."
                  "# "
                  "# @tparam Scalar"
                  "# @param Vector<Scalar> lambda Vector encoding a descending sequence of numbers."
                  "# @option Bool projected Omit the redundant first row of equations to reduce dimension, default=false"
                  "# @return Polytope"
                  "# @example [require bundled:libnormaliz] Create the Gelfand-Tsetlin polytope for the sequence (6,4,2,1)"
                  "# > $lambda = new Vector(6,4,2,1);"
                  "# > $pgt = gelfand_tsetlin($lambda,projected=>1);"
                  "# > $gt = gelfand_tsetlin($lambda,projected=>0);"
                  "# > print $gt->LATTICE_VOLUME;"
                  "# | 14400"
                  "# > print $pgt->LATTICE_VOLUME;"
                  "# | 14400",
                  "gelfand_tsetlin<Scalar>(Vector<type_upgrade<Scalar>>; {projected => false})");


UserFunctionTemplate4perl("# @category Geometry"
                  "# Compute the volume of the Gelfand-Tsetlin polytope associated to the vector lambda."
                  "# "
                  "# See Postnikov: Permutohedra, associahedra, and beyond, IMRN (2009); doi:10.1093/imrn/rnn153"
                  "# Theorem 15.1."
                  "# "
                  "# Note that this volume is the volume of the polytope in its embedding space, in case that all entries of lambda are different."
                  "# "
                  "# @tparam Scalar"
                  "# @param Vector<Scalar> lambda Vector encoding a descending sequence of numbers."
                  "# @option Bool lattice The same formula may be used to count lattice points, default=false" 
                  "# @return Scalar the volume"
                  "# @example Illustrating the differences between the volumes for the sequence (6,4,2,1)"
                  "# > $lambda = new Vector(6,4,2,1);"
                  "# > $pgt = gelfand_tsetlin($lambda,projected=>1);"
                  "# > $gt = gelfand_tsetlin($lambda,projected=>0);"
                  "# > print $gt->VOLUME;"
                  "# | 0"
                  "# > print $gt->FULL_DIM;"
                  "# | false"
                  "# > print $pgt->VOLUME;"
                  "# | 20"
                  "# > print $pgt->FULL_DIM;"
                  "# | true"
                  "# > print gelfand_tsetlin_counting($lambda);"
                  "# | 20"
                  "# > print $gt->N_LATTICE_POINTS;"
                  "# | 360"
                  "# > print gelfand_tsetlin_counting($lambda, lattice_points=>1);"
                  "# | 360",
                  "gelfand_tsetlin_counting<Scalar>(Vector<type_upgrade<Scalar>>; {lattice_points => false})");


UserFunctionTemplate4perl("# @category Geometry"
                  "# Turn points from a Gelfand-Tsetlin polytope into triangular arrays."
                  "# "
                  "# See Postnikov: Permutohedra, associahedra, and beyond, IMRN (2009); doi:10.1093/imrn/rnn153"
                  "# Theorem 15.1."
                  "# "
                  "# Note that we assume the points to come with a homogenizing coordinate."
                  "# "
                  "# @tparam Scalar"
                  "# @param Vector<Scalar> lambda Vector encoding a descending sequence of numbers."
                  "# @return Array<Matrix<Scalar>> List of triangular arrays"
                  "# @example Small example with tree lattice points"
                  "# > $lambda = new Vector(3,2,2);"
                  "# > $gt = gelfand_tsetlin($lambda,projected=>0);"
                  "# > print $gt->N_LATTICE_POINTS;"
                  "# | 3"
                  "# > print $gt->LATTICE_POINTS;"
                  "# | 1 3 2 2 2 2 2"
                  "# | 1 3 2 2 3 2 2"
                  "# | 1 3 2 2 3 2 3"
                  "# > print gelfand_tsetlin_diagrams($gt->LATTICE_POINTS);"
                  "# | <3 2 2"
                  "# | 2 2 0"
                  "# | 2 0 0"
                  "# | >"
                  "# | <3 2 2"
                  "# | 3 2 0"
                  "# | 2 0 0"
                  "# | >"
                  "# | <3 2 2"
                  "# | 3 2 0"
                  "# | 3 0 0"
                  "# | >",
                  "gelfand_tsetlin_diagrams<Scalar>(Matrix<type_upgrade<Scalar>>)");

} // namespace polytope
} // namespace polymake

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

