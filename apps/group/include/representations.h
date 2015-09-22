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

   void set_entry(SparseMatrix<Rational>& rep, const SetType& image, int col_index) const {
      rep(index_of[image], col_index) = 1;
   }

public:
   InducedAction(int degree, 
                 const Array<SetType>& domain, 
                 const Map<SetType, int>& index_of)
      : degree(degree)
      , domain(domain)
      , index_of(index_of) 
   {}

   SparseMatrix<Rational> rep(const Array<int>& perm) const {
      SparseMatrix<Rational> rep(degree, degree);
      int col_index(0);
      for (typename Entire<Array<SetType> >::const_iterator dit = entire(domain); !dit.at_end(); ++dit, ++col_index) {
         SetType image;
         image.resize(perm.size());
         for (typename Entire<SetType>::const_iterator sit = entire(*dit); !sit.at_end(); ++sit)
            image += perm[*sit];
         set_entry(rep, image, col_index);
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

