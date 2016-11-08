/* Copyright (c) 1997-2016
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


#include "polymake/SparseVector.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/hash_map"
#include "polymake/Set.h"
#include "polymake/group/permlib.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

/*
  A specialized method for calculating the symmetrized cocircuit
  equations corresponding to the trivial isotypical component.

  This amounts to summing all cocircuit equations corresponding to the orbit of each ridge.
 */
template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int> > 
symmetrized_cocircuit_equations_0_impl(int d,
                                       const Matrix<Scalar>& V,
                                       const IncidenceMatrix<>& VIF,
                                       const Array<Array<int>>& generators,
                                       const Array<SetType>& interior_ridge_reps,
                                       const Array<SetType>& facet_reps,
                                       perl::OptionSet options,
                                       bool partial_equations) 
{
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];

   const group::PermlibGroup sym_group(generators);

   hash_map<SetType, int> index_of_facet;
   int ct(-1);
   for (const auto& rep : facet_reps)
      index_of_facet[rep] = ++ct;

   const int 
      n_facets(index_of_facet.size()),
      n(V.rows());
   ListMatrix<SparseVector<int>> cocircuit_eqs(0, n_facets);

   ct = 0;
   time_t start_time, current_time;
   time(&start_time);
   for (const auto& ridge_rep : interior_ridge_reps) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      SparseVector<int> eq(n_facets);
      const Vector<Scalar> vals = V * (null_space(V.minor(ridge_rep, All))[0]);
      for (int i=0; i<vals.size(); ++i) {
         const int s = sign(vals[i]);
         if (s!=0) {
            SetType facet(ridge_rep);
            facet.resize(n);
            facet += i;
            const SetType facet_rep(sym_group.lex_min_representative(facet));
            if (!partial_equations || index_of_facet.exists(facet_rep))
               eq[index_of_facet.at(facet_rep)] += s * sym_group.setwise_stabilizer(facet).order();
         }
      }
      if (eq.size()) {
         if (reduce_rows) eq = common::divide_by_gcd(eq);
         cocircuit_eqs /= eq;
         if (filename.size()) wrap(outfile) << eq << endl;
      }
   }
   return cocircuit_eqs;
}


/*
  The analogous specialized method for the projection of foldable
  cocircuit equations to the trivial isotypical component.
 */
template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int>>
symmetrized_foldable_cocircuit_equations_0_impl(int d,
                                                const Matrix<Scalar>& V,
                                                const IncidenceMatrix<>& VIF,
                                                const Array<Array<int>>& generators,
                                                const Array<SetType>& interior_ridge_reps,
                                                const Array<SetType>& facet_reps,
                                                perl::OptionSet options,
                                                bool partial_equations) 
{
   const group::PermlibGroup sym_group(generators);

   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];

   hash_map<SetType, int> index_of_facet;
   int n_facet_reps(0);
   for (const auto& rep : facet_reps)
      index_of_facet[rep] = n_facet_reps++;
   
   ListMatrix<SparseVector<int>> cocircuit_eqs(0, 2*n_facet_reps);

   // use int instead of Rational to save time;
   //   we don't use row reductions (experimentally bad)
   SparseVector<int> eq_0_first, eq_1_first;

   // for each interior ridge rho and c in {0,1}:
   //   sum_{sigma > rho, orientation=+} x_{c,sigma} = sum_{sigma > rho, orientation=-} x_{1-c,sigma}
   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   for (const auto& rho : interior_ridge_reps) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      eq_0_first = SparseVector<int>(2*n_facet_reps);
      eq_1_first = SparseVector<int>(2*n_facet_reps); 
      const SparseVector<Scalar> nv = null_space(V.minor(rho, All)).row(0);
      int row_index(0); 
      for (typename Entire<Rows<Matrix<Scalar>>>::const_iterator vit = entire(rows(V)); !vit.at_end(); ++vit, ++row_index) {
         const int orientation = sign(nv * (*vit));
         if (orientation != 0) {
            SetType facet(rho);
            facet.resize(V.rows());
            facet += row_index;
            const SetType this_facet(sym_group.lex_min_representative(facet));
            if (partial_equations && !index_of_facet.exists(this_facet)) continue;
            const int 
               iof (index_of_facet[this_facet]),
               mult(sym_group.setwise_stabilizer(this_facet).order());
            if (orientation>0) {
               eq_0_first[2*iof] +=  mult;
               eq_1_first[2*iof+1] +=  mult;
            } else {
               eq_0_first[2*iof+1] +=  -mult;
               eq_1_first[2*iof] +=  -mult;
            }
         }
      }
      if (eq_0_first.size()) {
         if (reduce_rows) {
            eq_0_first = common::divide_by_gcd(eq_0_first);
            eq_1_first = common::divide_by_gcd(eq_1_first);
         }
         cocircuit_eqs /= eq_0_first;
         cocircuit_eqs /= eq_1_first;
         if (filename.size())
            wrap(outfile) << eq_0_first << "\n" << eq_1_first << endl;
      }
   }
   return cocircuit_eqs;     
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
