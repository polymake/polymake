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

#pragma once

#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/linalg.h"
#include "polymake/group/orbit.h"
#include "polymake/group/group_tools.h"

namespace polymake { namespace group {

template <typename Perm>
SparseMatrix<Rational> permutation_matrix(const Perm& perm,
                                          const Array<Int>& coordinate_permutation)
{
   SparseMatrix<Rational> permutation_matrix(degree(perm), degree(perm));
   Int i = 0;
   for (auto pit = entire(perm); !pit.at_end(); ++pit, ++i)
      permutation_matrix(coordinate_permutation[*pit],
                         coordinate_permutation[i]) = 1;
   return permutation_matrix;
}

template <typename Perm>
Int inverse_perm_at(const Perm& perm, Int k)
{
   assert(k < perm.size());
   Int i = 0;
   for (auto pit = entire(perm); !pit.at_end(); ++pit, ++i)
      if (*pit == k)
         return i;
   std::ostringstream msg;
   wrap(msg) << "The array " << perm << " is not a permutation.";
   throw std::runtime_error(msg.str());
}

/*
template <typename InducedAction, typename RowType>
SparseMatrix<Rational> 
isotypic_projector_impl(const RowType& character,
                        const InducedAction& induced_action,
                        Int degree,
                        const ConjugacyClasses& conjugacy_classes,
                        Int order)
{
   SparseMatrix<Rational> isotypic_projector(degree, degree);
   for (Int i = 0; i<conjugacy_classes.size(); ++i) {
      for (auto cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
         isotypic_projector += 
            character[i] // FIXME: conjugate here, once complex character tables are implemented
            * induced_action.induced_rep(*cit);
      }
   }

   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   return isotypic_projector * character[0] / order;
}
*/

template <typename E>   
auto matrix_rep(const Array<E>& g, const Array<Int>& permutation_to_orbit_order)
{
   return permutation_matrix(g, permutation_to_orbit_order);
}
   
template <typename Scalar>
auto matrix_rep(const Matrix<Scalar>& g, const Array<Int>&)
{
   return g;
}
   
/*
    Implement the projector to the isotypic component given by the character //chi// using the formula

    pi_chi = chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
 */
template <typename RowType, typename Element, typename Scalar>
SparseMatrix<CharacterNumberType> 
isotypic_projector_impl(const RowType& chi,
                        const ConjugacyClasses<Element>& conjugacy_classes,
                        const Array<Int>& permutation_to_orbit_order,
                        Int group_order,
                        const Scalar& )
{
   const Int deg(degree(conjugacy_classes[0][0]));
   SparseMatrix<CharacterNumberType> isotypic_projector(deg, deg);
   for (Int i = 0; i < conjugacy_classes.size(); ++i) {
      if (is_zero(chi[i])) continue;
      for (const auto& cc: conjugacy_classes[i]) {
         isotypic_projector += 
            chi[i] // FIXME: conjugate here, once complex character tables are implemented
            * matrix_rep(cc, permutation_to_orbit_order);
      }
   }
   isotypic_projector *= chi[0] / group_order;
   return isotypic_projector;
}

template <typename RowType, typename Element>
SparseMatrix<double> 
isotypic_projector_impl(const RowType& chi,
                        const ConjugacyClasses<Element>& conjugacy_classes,
                        const Array<Int>& permutation_to_orbit_order,
                        Int group_order,
                        const double& )
{
   const Int deg = degree(conjugacy_classes[0][0]);
   SparseMatrix<double> isotypic_projector(deg, deg);
   for (Int i = 0; i < conjugacy_classes.size(); ++i) {
      if (is_zero(chi[i])) continue;
      for (const auto& cc: conjugacy_classes[i]) {
         isotypic_projector +=
            chi[i] // FIXME: conjugate here, once complex character tables are implemented
            * matrix_rep(cc, permutation_to_orbit_order);
      }
   }
   isotypic_projector *= chi[0] / double(group_order);
   return isotypic_projector;
}
      
template <typename InducedAction, typename RowType, typename Element>
ListMatrix<SparseVector<CharacterNumberType> >
isotypic_basis_impl(const RowType& character,
                    const InducedAction& induced_action,
                    Int degree,
                    const ConjugacyClasses<Element>& conjugacy_classes,
                    Int order)
{
   ListMatrix<SparseVector<CharacterNumberType>>
      isotypic_basis(0, degree),
      kernel_so_far(unit_matrix<CharacterNumberType>(degree));
   // we fill the matrix row-wise. The entire matrix is
   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   // and rep.rho(g) (k, g^{-1}(k)) = 1.
   for (Int k = 0; k < degree; ++k) {
      SparseVector<CharacterNumberType> new_row(degree);
      for (Int i = 0; i < conjugacy_classes.size(); ++i) {
         for (const auto& cc: conjugacy_classes[i]) {
            for (Int j = 0; j < degree; ++j)
               if (induced_action.index_of_inverse_image(cc, j) == k) {
                  new_row[j] += 
                     character[i]; // FIXME: conjugate here, once complex character tables are implemented
               }
         }
      }
      add_row_if_rowspace_increases(isotypic_basis, new_row, kernel_so_far);
   }

   return isotypic_basis * character[0] / order;
}

template <typename RowType, typename Element>
ListMatrix<SparseVector<CharacterNumberType> >
isotypic_basis_impl(const RowType& character,
                    const ConjugacyClasses<Element>& conjugacy_classes,
                    const Array<Int>& permutation_to_orbit_order,
                    Int order)
{
   const Int deg = degree(conjugacy_classes[0][0]);
   ListMatrix<SparseVector<CharacterNumberType>>
      isotypic_basis(0, deg),
      kernel_so_far(unit_matrix<CharacterNumberType>(deg));
   // we fill the matrix row-wise. The entire matrix is
   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   // and rep.rho(g) (k, g^{-1}(k)) = 1.
   for (Int k = 0; k < deg; ++k) {
      SparseVector<CharacterNumberType> new_row(deg);
      for (Int i = 0; i < conjugacy_classes.size(); ++i) {
         if (is_zero(character[i])) continue;
         for (auto cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
            new_row[permutation_to_orbit_order[inverse_perm_at(*cit,k)]] += 
               character[i]; // FIXME: conjugate here, once complex character tables are implemented
         }
      }
      add_row_if_rowspace_increases(isotypic_basis, new_row, kernel_so_far);
   }

   return isotypic_basis * (character[0] / order);
}


template <typename SparseMatrixType, typename Element>
IncidenceMatrix<> 
isotypic_supports_impl(const SparseMatrixType& S,
                       const Matrix<CharacterNumberType>& character_table,
                       const ConjugacyClasses<Element>& conjugacy_classes,
                       const Array<Int>& permutation_to_orbit_order,
                       Int order)
{
   const Int n_irreps = character_table.rows();
   IncidenceMatrix<> supp(S.rows(), n_irreps);
   for (Int i = 0; i < n_irreps; ++i) {
      const SparseMatrix<CharacterNumberType> image = isotypic_projector_impl(character_table[i], conjugacy_classes, permutation_to_orbit_order, order, CharacterNumberType()) * T(S);
      Int j = 0;
      for (auto cit = entire(cols(image)); !cit.at_end(); ++cit, ++j)
         if (!is_zero(*cit))
            supp(j,i) = 1;
   }
   return supp;
}

} }



// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

