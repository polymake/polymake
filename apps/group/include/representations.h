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

#ifndef __GROUP_REPRESENTATIONS_H
#define __GROUP_REPRESENTATIONS_H

#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"

namespace polymake { namespace group {

namespace {

template<typename SetType>
class InducedAction {
protected:
   int degree;
   const Array<SetType>& domain;
   const Map<SetType, int>& index_of;

   Array<int> inverse_permutation(const Array<int>& perm) const {
      Array<int> inv_perm(perm.size());
      for (int i=0; i<perm.size(); ++i)
         inv_perm[perm[i]] = i;
      return inv_perm;
   }

public:
   InducedAction(int degree, 
                 const Array<SetType>& domain, 
                 const Map<SetType, int>& index_of)
      : degree(degree)
      , domain(domain)
      , index_of(index_of) 
   {}

   int index_of_image(const Array<int>& perm,
                      const SetType& elt) const {
      SetType image;
      image.resize(perm.size());
      for (typename Entire<SetType>::const_iterator sit = entire(elt); !sit.at_end(); ++sit)
         image += perm[*sit];
      return index_of[image];
   }

   int index_of_inverse_image(const Array<int>& perm,
                              const SetType& elt) const {
      Array<int> inv_perm(inverse_permutation(perm));
      SetType inv_image;
      inv_image.resize(inv_perm.size());
      for (typename Entire<SetType>::const_iterator sit = entire(elt); !sit.at_end(); ++sit)
         inv_image += inv_perm[*sit];
      return index_of[inv_image];
   }

   int index_of_inverse_image(const Array<int>& perm,
                              const int elt_index) const {
      return index_of_inverse_image(perm, domain[elt_index]);
   }

   SparseMatrix<Rational> rep(const Array<int>& perm) const {
      SparseMatrix<Rational> rep(degree, degree);
      int col_index(0);
      for (typename Entire<Array<SetType> >::const_iterator dit = entire(domain); !dit.at_end(); ++dit, ++col_index) {
         rep(index_of_image(perm, *dit), col_index) = 1;
      }
      return rep;
   }
};


template<typename InducedAction, typename RowType>
SparseMatrix<Rational> isotypic_projector_impl(const RowType& character,
                                               const InducedAction& induced_action,
                                               int degree,
                                               const Array<Set<Array<int> > >& conjugacy_classes,
                                               int order)
{
   SparseMatrix<Rational> isotypic_projector(degree, degree);
   for (int i=0; i<conjugacy_classes.size(); ++i) {
      for (Entire<Set<Array<int> > >::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
         isotypic_projector += 
            character[i] // FIXME: conjugate here, once complex character tables are implemented
            * induced_action.rep(*cit);
      }
   }

   //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
   return isotypic_projector * character[0] / order;
}

template<typename InducedAction, typename RowType>
ListMatrix<SparseVector<Rational> > 
isotypic_basis_impl(const RowType& character,
                    const InducedAction& induced_action,
                    int degree,
                    const Array<Set<Array<int> > >& conjugacy_classes,
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
         for (Entire<Set<Array<int> > >::const_iterator cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
            for (int j=0; j<degree; ++j)
               if (induced_action.index_of_inverse_image(*cit, j) == k)
                  new_row[j] += character[i]; // FIXME: conjugate here, once complex character tables are implemented
         }
      }
      add_row_if_rowspace_increases(isotypic_basis, new_row, kernel_so_far);
   }

   return isotypic_basis * character[0] / order;
}

template<typename SparseMatrixType, typename InducedAction>
IncidenceMatrix<> isotypic_supports_impl(const SparseMatrixType& S,
                                         const Matrix<Rational>& character_table,
                                         const InducedAction& IA,
                                         const Array<Set<Array<int> > >& conjugacy_classes,
                                         int order,
                                         int degree)
{
   const int n_irreps = character_table.rows();
   IncidenceMatrix<> supp(S.rows(), n_irreps);
   for (int i=0; i<n_irreps; ++i) {
      const SparseMatrix<Rational> image = isotypic_projector_impl(character_table[i], IA, degree, conjugacy_classes, order) * T(S);
      int j(0);
      for (Entire<Cols<SparseMatrix<Rational> > >::const_iterator cit = entire(cols(image)); !cit.at_end(); ++cit, ++j)
         if (*cit != zero_vector<Rational>(degree))
            supp(j,i) = 1;
   }
   return supp;
}

} // end anonymous namespace

} }

#endif // __GROUP_REPRESENTATIONS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

