/* Copyright (c) 1997-2015
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


#include "polymake/Rational.h"
#include "polymake/SparseVector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/hash_map"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/group/group_domain.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/polytope/simplex_tools.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

typedef Set<int> SetType;

template<typename Scalar>
ListMatrix<SparseVector<int> > 
cocircuit_equations_impl(int d, 
                         const Matrix<Scalar>& points, 
                         const IncidenceMatrix<>& VIF, 
                         const Array<Set<int> >& interior_simplices, 
                         perl::OptionSet options,
                         bool partial_equations)
{
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   
   const int n = points.rows();
   int n_facets = 0;
   hash_map<SetType,int> index_of;
   for (Entire<Array<SetType> >::const_iterator iit = entire(interior_simplices); !iit.at_end(); ++iit)
      index_of[*iit] = n_facets++;

   ListMatrix<SparseVector<int> > cocircuit_eqs(0, n_facets);

   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   for (Entire<Subsets_of_k<const sequence&> >::const_iterator rit = entire(all_subsets_of_k(sequence(0,n), d)); !rit.at_end(); ++rit) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      const SetType rho = *rit;
      if (rank(points.minor(rho, All)) == rho.size() && is_interior(rho, VIF)) {
         SparseVector<int> eq(n_facets);
         const SparseVector<Scalar> nv = null_space(points.minor(*rit, All)).row(0);
         int row_index(0); 
         for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
            const int sigma = sign(nv * (*vit));
            if (sigma != 0) {
               const SetType s(*rit + scalar2set(row_index));
               if ((!partial_equations || index_of.exists(s))) {
                  eq[index_of[s]] = sigma;
               }
            }
         }
         if (eq.size()) {
            if (reduce_rows) eq = common::divide_by_gcd(eq);
            cocircuit_eqs /= eq;
            if (filename.size()) wrap(outfile) << eq << endl;
         }
      } 
   }
   return cocircuit_eqs;
}

template<typename Scalar>
ListMatrix<SparseVector<int> >
foldable_cocircuit_equations_impl(int d,
                                  const Matrix<Scalar>& points,
                                  const IncidenceMatrix<>& VIF,
                                  const Array<SetType>& interior_ridges, // FIXME: Map
                                  const Array<SetType>& facets,
                                  perl::OptionSet options,
                                  bool partial_equations)
{
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   
   hash_map<SetType,int> index_of_facet;
   int n_facets = 0; // number of full-dimensional simplices
   for (Entire<Array<SetType> >::const_iterator iit = entire(facets); !iit.at_end(); ++iit)
      index_of_facet[*iit] = n_facets++;

   ListMatrix<SparseVector<int> > cocircuit_eqs(0, 2*n_facets);

   // use int instead of Rational to save time;
   SparseVector<int> eq_0_first, eq_1_first;

   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   // for each interior ridge rho and c in {0,1}:
   //   sum_{sigma > rho, orientation=+} x_{c,sigma} = sum_{sigma > rho, orientation=-} x_{1-c,sigma}
   for (Entire<Array<SetType> >::const_iterator rit = entire(interior_ridges); !rit.at_end(); ++rit) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      const SetType rho = *rit;
      eq_0_first = SparseVector<int>(2*n_facets);
      eq_1_first = SparseVector<int>(2*n_facets); 
      const SparseVector<Scalar> nv = null_space(points.minor(*rit, All)).row(0);
      int row_index(0); 
      for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
         const int orientation = sign(nv * (*vit));
         if (orientation != 0) {
            const SetType this_facet(*rit + scalar2set(row_index));
            if (partial_equations && !index_of_facet.exists(this_facet)) continue;
            const int iof(index_of_facet[this_facet]);
            if (orientation>0) {
               eq_0_first[2*iof] =  1;
               eq_1_first[2*iof+1] =  -1;
            } else {
               eq_0_first[2*iof+1] =  -1;
               eq_1_first[2*iof] =  1;
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

 
