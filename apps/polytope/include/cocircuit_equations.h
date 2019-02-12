/* Copyright (c) 1997-2018
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
#include "polymake/common/lattice_tools.h"
#include "polymake/group/representations.h"
#include "polymake/group/isotypic_components.h"
#include "polymake/group/action.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

namespace {

template<typename SparseSetType>
using IndexOfType = hash_map<SparseSetType, int>;

/*
  We make cocircuit equations available as two data types:
  hash_map<SetType, int> and SparseVector<int>.

  Since the SparseVector<int> needs the additional data structure
  index_of that maps (facet) sets to indices, we abstract the data
  type to a class that we specialize as needed.
*/

// The generic instance is for CocircuitEquationType = hash_map<SetType, int>
template<typename CocircuitEquationType, typename SetType>
class CocircuitEquation {
   CocircuitEquationType cce;

public:
   CocircuitEquation() = default;

   void set(const SetType& facet, int sigma)
   {
      cce[facet] = sigma;
   }

   const CocircuitEquationType& equation() const
   {
      return cce;
   }
};

// The specialized one is for SparseVector<int>
template<typename SetType>
class CocircuitEquation<SparseVector<int>, SetType> {
   SparseVector<int> cce;
   const IndexOfType<SetType>& index_of;

 public:
   CocircuitEquation(const IndexOfType<SetType>& index_of_)
      : cce(index_of_.size())
      , index_of(index_of_)
   {}

   void set(const SetType& facet, int sigma)
   {
      cce[index_of.at(facet)] = sigma;
   }

   const SparseVector<int>& equation() const
   {
      return cce;
   }
};

// the following function implements the calculation of the cocircuit
// equation corresponding to a given ridge. It is templated on the
// desired data type.
template <typename Scalar, typename SetType, typename CocircuitEquationType>
void
cocircuit_equation_of_ridge_impl_impl(const Matrix<Scalar>& points,
                                      const SetType& ridge,
                                      CocircuitEquationType& eq) {
   const SparseVector<Scalar> nv = null_space(points.minor(ridge, All)).row(0);
   int row_index(0);
   for (auto vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
      const int sigma = sign(nv * (*vit));
      if (sigma != 0) {
         SetType facet(ridge);
         facet += row_index;
         eq.set(facet, sigma);
      }
   }
}

// next, the two variants that are supposed to be called from other functions.
template<typename Scalar, typename SetType>
auto
cocircuit_equation_of_ridge_impl(const Matrix<Scalar>& points,
                                 const SetType& ridge,
                                 const IndexOfType<SetType>& index_of)
{
   CocircuitEquation<SparseVector<int>, SetType> eq(index_of);
   cocircuit_equation_of_ridge_impl_impl(points, ridge, eq);
   return eq.equation();
}

template<typename Scalar, typename SetType>
auto
cocircuit_equation_of_ridge_impl(const Matrix<Scalar>& points,
                                 const SetType& ridge)
{
   CocircuitEquation<group::SparseSimplexVector<SetType>, SetType> eq;
   cocircuit_equation_of_ridge_impl_impl(points, ridge, eq);
   return eq.equation();
}

} // end anonymous namespace

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int>>
cocircuit_equations_impl(int d,
                         const Matrix<Scalar>& points,
                         const IncidenceMatrix<>& VIF,
                         const Array<SetType>& interior_ridge_simplices,
                         const Array<SetType>& interior_simplices,
                         perl::OptionSet options)
{
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);

   int n_facets = 0;
   IndexOfType<SetType> index_of;
   for (const auto& s : interior_simplices)
      index_of[s] = n_facets++;

   ListMatrix<SparseVector<int>> cocircuit_eqs(0, n_facets);

   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   for (const auto& ir : interior_ridge_simplices) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      SparseVector<int> eq(cocircuit_equation_of_ridge_impl(points, ir, index_of));
      if (eq.size()) {
         if (reduce_rows) eq = common::divide_by_gcd(eq);
         cocircuit_eqs /= eq;
         if (filename.size()) wrap(outfile) << eq << endl;
      }
   }
   return cocircuit_eqs;
}

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<int> >
foldable_cocircuit_equations_impl(int d,
                                  const Matrix<Scalar>& points,
                                  const IncidenceMatrix<>& VIF,
                                  const Array<SetType>& interior_ridge_simplices, // FIXME: Map
                                  const Array<SetType>& max_interior_simplices,
                                  perl::OptionSet options,
                                  bool partial_equations)
{
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);

   IndexOfType<SetType> index_of;
   int n_facets = 0; // number of full-dimensional simplices
   for (const auto& s: max_interior_simplices)
      index_of[s] = n_facets++;

   ListMatrix<SparseVector<int>> cocircuit_eqs(0, 2*n_facets);

   // use int instead of Rational to save time;
   SparseVector<int> eq_0_first, eq_1_first;

   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   // for each interior ridge rho and c in {0,1}:
   //   sum_{sigma > rho, orientation=+} x_{c,sigma} = sum_{sigma > rho, orientation=-} x_{1-c,sigma}
   for (const auto& ir: interior_ridge_simplices) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      eq_0_first = SparseVector<int>(2*n_facets);
      eq_1_first = SparseVector<int>(2*n_facets);
      const SparseVector<Scalar> nv = null_space(points.minor(ir, All)).row(0);
      int row_index(0);
      for (auto vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
         const int orientation = sign(nv * (*vit));
         if (orientation != 0) {
            const SetType this_facet(ir + scalar2set(row_index));
            if (partial_equations && !index_of.exists(this_facet)) continue;
            const int iof(index_of.at(this_facet));
            if (orientation>0) {
               eq_0_first[2*iof] =  1;
               eq_1_first[2*iof+1] =  -1;
            } else {
               eq_0_first[2*iof+1] =  -1;
               eq_1_first[2*iof] =  1;
            }
         }
      }
      if (!eq_0_first.size()) continue;
      if (reduce_rows) {
         eq_0_first = common::divide_by_gcd(eq_0_first);
         eq_1_first = common::divide_by_gcd(eq_1_first);
      }
      cocircuit_eqs /= eq_0_first;
      cocircuit_eqs /= eq_1_first;
      if (filename.size())
         wrap(outfile) << eq_0_first << "\n" << eq_1_first << endl;
   }
   return cocircuit_eqs;
}



} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
