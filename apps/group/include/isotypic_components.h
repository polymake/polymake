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

#ifndef __GROUP_ISOTYPIC_COMPONENTS_H
#define __GROUP_ISOTYPIC_COMPONENTS_H

#include "polymake/group/action_datatypes.h"

#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/linalg.h"
#include "polymake/group/orbit.h"

namespace polymake { namespace group {


namespace {

template<typename Perm>
SparseMatrix<Rational> permutation_matrix(const Perm& perm,
                                          const Array<int>& coordinate_permutation)
{
   SparseMatrix<Rational> permutation_matrix(perm.size(), perm.size());
   int i(0);
   for (typename Entire<Perm>::const_iterator pit = entire(perm); !pit.at_end(); ++pit, ++i)
      permutation_matrix(coordinate_permutation[*pit],
                         coordinate_permutation[i]) = 1;
   return permutation_matrix;
}

template<typename Perm>
int inverse_perm_at(const Perm& perm,
                    int k)
{
   assert(k < perm.size());
   int i(0);
   for (typename Entire<Perm>::const_iterator pit = entire(perm); !pit.at_end(); ++pit, ++i)
      if (*pit == k)
         return i;
   std::ostringstream msg;
   wrap(msg) << "The array " << perm << " is not a permutation.";
   throw std::runtime_error(msg.str());
}

   /*
template<typename InducedAction, typename RowType>
SparseMatrix<Rational> 
isotypic_projector_impl(const RowType& character,
                        const InducedAction& induced_action,
                        int degree,
                        const ConjugacyClasses& conjugacy_classes,
                        int order)
{
   SparseMatrix<Rational> isotypic_projector(degree, degree);
   for (int i=0; i<conjugacy_classes.size(); ++i) {
      for (Entire<ConjugacyClass>::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
         isotypic_projector += 
            character[i] // FIXME: conjugate here, once complex character tables are implemented
            * induced_action.induced_rep(*cit);
      }
   }

   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   return isotypic_projector * character[0] / order;
}
   */

template<typename RowType>
SparseMatrix<Rational> 
isotypic_projector_impl(const RowType& character,
                        const ConjugacyClasses& conjugacy_classes,
                        const Array<int>& permutation_to_orbit_order,
                        int order)
{
   const int degree(permutation_to_orbit_order.size());
   SparseMatrix<Rational> isotypic_projector(degree, degree);
   for (int i=0; i<conjugacy_classes.size(); ++i) {
      for (Entire<ConjugacyClass>::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
         isotypic_projector += 
            character[i] // FIXME: conjugate here, once complex character tables are implemented
            * permutation_matrix(*cit, permutation_to_orbit_order);
      }
   }
   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   isotypic_projector *= character[0] / order;
   return isotypic_projector;
}

template<typename InducedAction, typename RowType>
ListMatrix<SparseVector<Rational> >
isotypic_basis_impl(const RowType& character,
                    const InducedAction& induced_action,
                    int degree,
                    const ConjugacyClasses& conjugacy_classes,
                    int order)
{
   ListMatrix<SparseVector<Rational> >
      isotypic_basis(0, degree),
      kernel_so_far(unit_matrix<Rational>(degree));
   // we fill the matrix row-wise. The entire matrix is
   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   // and rep.rho(g) (k, g^{-1}(k)) = 1.
   for (int k=0; k<degree; ++k) {
      SparseVector<Rational> new_row(degree);
      for (int i=0; i<conjugacy_classes.size(); ++i) {
         for (Entire<ConjugacyClass>::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
            for (int j=0; j<degree; ++j)
               if (induced_action.index_of_inverse_image(*cit, j) == k) {
                  new_row[j] += 
                     character[i]; // FIXME: conjugate here, once complex character tables are implemented
               }
         }
      }
      add_row_if_rowspace_increases(isotypic_basis, new_row, kernel_so_far);
   }

   return isotypic_basis * character[0] / order;
}

template<typename RowType>
ListMatrix<SparseVector<Rational> >
isotypic_basis_impl(const RowType& character,
                    const ConjugacyClasses& conjugacy_classes,
                    const Array<int>& permutation_to_orbit_order,
                    int order)
{
   const int degree(permutation_to_orbit_order.size());
   ListMatrix<SparseVector<Rational> >
      isotypic_basis(0, degree),
      kernel_so_far(unit_matrix<Rational>(degree));
   // we fill the matrix row-wise. The entire matrix is
   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   // and rep.rho(g) (k, g^{-1}(k)) = 1.
   int k(0);
   for (Entire<Array<int> >::const_iterator pit = entire(permutation_to_orbit_order); !pit.at_end(); ++pit, ++k) {
      SparseVector<Rational> new_row(degree);
      for (int i=0; i<conjugacy_classes.size(); ++i) {
         for (Entire<ConjugacyClass>::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
            new_row[permutation_to_orbit_order[inverse_perm_at(*cit,k)]] += 
               character[i]; // FIXME: conjugate here, once complex character tables are implemented
         }
      }
      add_row_if_rowspace_increases(isotypic_basis, new_row, kernel_so_far);
   }

   return isotypic_basis * (character[0] / order);
}


template<typename SparseMatrixType>
IncidenceMatrix<> 
isotypic_supports_impl(const SparseMatrixType& S,
                       const Matrix<Rational>& character_table,
                       const ConjugacyClasses& conjugacy_classes,
                       const Array<int>& permutation_to_orbit_order,
                       int order)
{
   const int n_irreps = character_table.rows();
   const int degree = permutation_to_orbit_order.size();
   IncidenceMatrix<> supp(S.rows(), n_irreps);
   for (int i=0; i<n_irreps; ++i) {
      const SparseMatrix<Rational> image = isotypic_projector_impl(character_table[i], conjugacy_classes, permutation_to_orbit_order, order) * T(S);
      int j(0);
      for (auto cit = entire(cols(image)); !cit.at_end(); ++cit, ++j)
         if (!is_zero(*cit))
            supp(j,i) = 1;
   }
   return supp;
}

} // end anonymous namespace

} }

#endif // __GROUP_ISOTYPIC_COMPONENTS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

