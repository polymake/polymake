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

#pragma once

#include "polymake/group/representations.h"
#include "polymake/Bitset.h"

namespace polymake { namespace group {



namespace {

class QuotientedInducedAction 
   : public InducedAction<Bitset> {
   using SetType = Bitset;
protected:
   PermlibGroup G;

   void set_entry(SparseMatrix<Rational>& rep, const SetType& image, Int col_index) const
   {
      ++rep(index_of.at(G.lex_min_representative(image)), col_index);
   }

public:
   QuotientedInducedAction(Int degree, 
                           const Array<SetType>& domain,
                           const hash_map<SetType, Int>& index_of, 
                           const Array<Array<Int> >& generators)
      : InducedAction<SetType>(degree, domain, index_of)
      , G(generators) {}
};


// template<typename InducedAction, typename RowType>
// SparseMatrix<Rational> isotypic_projector_impl(const RowType& character,
//                                                const InducedAction& induced_action,
//                                                Int degree,
//                                                const Array<Set<Array<Int>>>& conjugacy_classes,
//                                                Int order)
// {
//    SparseMatrix<Rational> isotypic_projector(degree, degree);
//    for (Int i = 0; i < conjugacy_classes.size(); ++i) {
//       for (auto cit = entire(conjugacy_classes[i]); !cit.at_end(); ++cit) {
//          isotypic_projector += 
//             character[i] // FIXME: conjugate here, once complex character tables are implemented
//             * induced_action.rep(*cit);
//       }
//    }

//    //    chi(G.identity())/G.order() * sum([chi(g).conjugate() * rep.rho(g) for g in G])
//    return isotypic_projector * character[0] / order;
// }

// template<typename SparseMatrixType, typename InducedAction>
// IncidenceMatrix<> isotypic_supports_impl(const SparseMatrixType& S,
//                                          const Matrix<Rational>& character_table,
//                                          const InducedAction& IA,
//                                          const Array<Set<Array<Int>>>& conjugacy_classes,
//                                          Int order,
//                                          Int degree)
// {
//    const Int n_irreps = character_table.rows();
//    IncidenceMatrix<> supp(n_irreps, S.rows());
//    for (Int i = 0; i < n_irreps; ++i) {
//       const SparseMatrix<Rational> proj = isotypic_projector_impl(character_table[i], IA, degree, conjugacy_classes, order);
//       Int j = 0;
//       for (auto rit = entire(rows(S)); !rit.at_end(); ++rit, ++j) {
//          for (suto e = entire(*rit); !e.at_end(); ++e) {
//             if (proj.col(e.index()) != zero_vector<Rational>(degree)) {
//                supp(i,j) = 1;
//                break; // it's only necessary to prove support once
//             }
//          }
//       }
//    }
//    return supp;
// }

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
