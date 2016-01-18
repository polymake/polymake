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
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/Map.h"
#include "polymake/hash_map"
#include "polymake/PowerSet.h"
#include "polymake/group/group_domain.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/polytope/simplex_tools.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

typedef Set<int> SetType;
typedef common::boost_dynamic_bitset BBitset;

template<typename Scalar, typename MapType>
SparseVector<int>
cocircuit_equation_of_impl(const Matrix<Scalar>& points, 
                           const SetType& ridge,
                           const MapType& index_of)
{
   SparseVector<int> eq(index_of.size());
   const SparseVector<Scalar> nv = null_space(points.minor(ridge, All)).row(0);
   int row_index(0); 
   for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
      const int sigma = sign(nv * (*vit));
      if (sigma != 0) {
         const SetType s(ridge + scalar2set(row_index));
         eq[index_of[s]] = sigma;
      }
   }
   return eq;
}

namespace {

} // end anonymous namespace

template<typename Scalar>
ListMatrix<SparseVector<int> > 
cocircuit_equations_impl(int d, 
                         const Matrix<Scalar>& points, 
                         const IncidenceMatrix<>& VIF, 
                         const Array<Set<int> >& interior_ridge_simplices, 
                         const Array<Set<int> >& interior_simplices, 
                         perl::OptionSet options)
{
   const bool reduce_rows = options["reduce_rows"];
   const int log_frequency = options["log_frequency"];
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   
   int n_facets = 0;
   Map<SetType,int> index_of;
   for (Entire<Array<SetType> >::const_iterator iit = entire(interior_simplices); !iit.at_end(); ++iit)
      index_of[*iit] = n_facets++;

   ListMatrix<SparseVector<int> > cocircuit_eqs(0, n_facets);

   int ct(0);
   time_t start_time, current_time;
   time(&start_time);
   for (Entire<Array<SetType> >::const_iterator rit = entire(interior_ridge_simplices); !rit.at_end(); ++rit) {
      if (log_frequency && (++ct % log_frequency == 0)) {
         time(&current_time);
         cerr << ct << " " << difftime(current_time, start_time) << endl;
      }
      SparseVector<int> eq(cocircuit_equation_of_impl(points, *rit, index_of));
      if (eq.size()) {
         if (reduce_rows) eq = common::divide_by_gcd(eq);
         cocircuit_eqs /= eq;
         if (filename.size()) wrap(outfile) << eq << endl;
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
   
   Map<SetType,int> index_of_facet;
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


namespace {

typedef Array<Array<Array<int> > > ConjugacyClasses;
typedef Array<Array<int> > ConjugacyClass;

typedef hash_map<BBitset, Rational> EquationAsMap;
typedef hash_map<BBitset, int> IndexOfOrbit;

inline
BBitset 
_apply(const Array<int>& perm,
       const BBitset& s)
{
   BBitset img(s.capacity());
   for (Entire<BBitset>::const_iterator sit = entire(s); !sit.at_end(); ++sit)
      img += perm[*sit];
   return img;
}

inline
EquationAsMap _apply(const Array<int>& perm,
                     const EquationAsMap& equation)
{
   EquationAsMap img;
   for (Entire<EquationAsMap>::const_iterator eit = entire(equation); !eit.at_end(); ++eit)
      img[_apply(perm, eit->first)] = eit->second;
   return img;
}

template<typename Scalar>
EquationAsMap
cocircuit_equation_as_map_impl(const Matrix<Scalar>& points, 
                               const BBitset& ridge)
{
   EquationAsMap eq;
   const SparseVector<Scalar> nv = null_space(points.minor(ridge, All)).row(0);
   int row_index(0); 
   for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator vit = entire(rows(points)); !vit.at_end(); ++vit, ++row_index) {
      const int sigma = sign(nv * (*vit));
      if (sigma != 0) {
         BBitset facet(ridge);
         facet += row_index;
         eq[facet] = sigma;
      }
   }
   return eq;
}

void 
make_index_of_facet (const Set<BBitset>& facets,
                     IndexOfOrbit& index_of_facet,
                     const ConjugacyClasses& conjugacy_classes)
{
   int index(0);
   for (Entire<Set<BBitset> >::const_iterator fit = entire(facets); !fit.at_end(); ++fit) {
      if (index_of_facet.exists(*fit)) continue;
      for (Entire<ConjugacyClasses>::const_iterator hccit = entire(conjugacy_classes); !hccit.at_end(); ++hccit) {
         for (Entire<ConjugacyClass>::const_iterator hit = entire(*hccit); !hit.at_end(); ++hit) {
            const BBitset f_prime = _apply(*hit, *fit);
            if (index_of_facet.exists(f_prime)) continue;
            index_of_facet[f_prime] = index++;
         }
      }
   }
}

ListMatrix<SparseVector<Rational> >
EquationsAsMap_2_SparseMatrix (const std::vector<EquationAsMap>& eqs,
                               IndexOfOrbit& index_of_facet)
{
   const int n_facets = index_of_facet.size();
   ListMatrix<SparseVector<Rational> > 
      pcceqs(0, n_facets),
      kernel_so_far(unit_matrix<Rational>(n_facets));

   for (Entire<std::vector<EquationAsMap> >::const_iterator eqsit = entire(eqs); !eqsit.at_end(); ++eqsit) {
      SparseVector<Rational> new_eq(n_facets);
      for (Entire<EquationAsMap>::const_iterator eit = entire(*eqsit); !eit.at_end(); ++eit) 
         new_eq[index_of_facet[eit->first]] = eit->second;
      add_row_if_rowspace_increases(pcceqs, new_eq, kernel_so_far);
   }
   return pcceqs;
}

} // end anonymous namespace


template<typename Scalar>
ListMatrix<SparseVector<Rational> >
projected_cocircuit_equations_impl(const Matrix<Scalar>& points,
                                   const Set<int>& _ridge_rep,
                                   const Set<int>& isotypic_components,
                                   IndexOfOrbit& index_of_facet,
                                   const Matrix<Rational>& character_table,
                                   const ConjugacyClasses& conjugacy_classes)
{
   const BBitset ridge_rep(points.rows(), _ridge_rep.begin(), _ridge_rep.end());
   const EquationAsMap cocircuit_equation = cocircuit_equation_as_map_impl(points, ridge_rep);

   /* 
      for all h in G, calculate the cocircuit equation 

      |Gamma| kappa^{h tau}
      =
       chi_i(id)
       sum_{ Delta in supp(kappa^ tau)} c_Delta
       sum_{j in cC(Gamma)} chi_i(j)
       sum_{g in C} e_{g cdot Delta}

       where  tau = ridge_rep  and  i = *iit, so that  chi_i(j) = character_table(*iit, j)
   */

   hash_set<BBitset> ridge_orbit;
   std::vector<EquationAsMap> eqs;
   Set<BBitset> facets;
   for (Entire<ConjugacyClasses>::const_iterator hccit = entire(conjugacy_classes); !hccit.at_end(); ++hccit) {
      for (Entire<ConjugacyClass>::const_iterator hit = entire(*hccit); !hit.at_end(); ++hit) {
         const BBitset tau_prime = _apply(*hit, ridge_rep);
         if (ridge_orbit.exists(tau_prime)) continue;
         ridge_orbit += tau_prime;
         
         EquationAsMap new_eq;
         for (int j=0; j<conjugacy_classes.size(); ++j) {
            for (Entire<ConjugacyClass>::const_iterator git = entire(conjugacy_classes[j]); !git.at_end(); ++git) {
               for (Entire<EquationAsMap>::const_iterator eit = entire(cocircuit_equation); !eit.at_end(); ++eit) {
                  const BBitset Delta_prime = _apply(*git, _apply(*hit, eit->first));
                  facets += Delta_prime;
                  Rational coeff(0);
                  for (Entire<Set<int> >::const_iterator iit = entire(isotypic_components); !iit.at_end(); ++iit)
                     coeff += character_table(*iit,j) * character_table(*iit,0);
                  new_eq[Delta_prime] += coeff * eit->second;
               }
            }
         }
         eqs.push_back(new_eq);
      }
   }

   make_index_of_facet(facets, index_of_facet, conjugacy_classes);
   return EquationsAsMap_2_SparseMatrix(eqs, index_of_facet);
}


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
