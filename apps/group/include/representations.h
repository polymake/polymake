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

#ifndef __GROUP_REPRESENTATIONS_H
#define __GROUP_REPRESENTATIONS_H

#include "polymake/Array.h"
#include "polymake/hash_map"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"

namespace polymake { namespace group {

namespace {

template<typename SetType>
class InducedAction {
protected:
   int degree;
   const Array<SetType>& domain;
   const hash_map<SetType, int>& index_of;

   Array<int> inverse_permutation(const Array<int>& perm) const {
      Array<int> inv_perm(perm.size());
      for (int i=0; i<perm.size(); ++i)
         inv_perm[perm[i]] = i;
      return inv_perm;
   }

public:
   InducedAction(int degree_,
                 const Array<SetType>& domain_,
                 const hash_map<SetType, int>& index_of_)
      : degree(degree_)
      , domain(domain_)
      , index_of(index_of_)
   {}

   int index_of_image(const Array<int>& perm,
                      const SetType& elt) const {
      SetType image;
      image.resize(perm.size());
      for (auto sit = entire(elt); !sit.at_end(); ++sit)
         image += perm[*sit];
      return index_of.at(image);
   }

   int index_of_inverse_image(const Array<int>& perm,
                              const SetType& elt) const {
      Array<int> inv_perm(inverse_permutation(perm));
      SetType inv_image;
      inv_image.resize(inv_perm.size());
      for (auto sit = entire(elt); !sit.at_end(); ++sit)
         inv_image += inv_perm[*sit];
      return index_of.at(inv_image);
   }

   int index_of_inverse_image(const Array<int>& perm,
                              const int elt_index) const {
      return index_of_inverse_image(perm, domain[elt_index]);
   }

   SparseMatrix<Rational> induced_rep(const Array<int>& perm) const {
      SparseMatrix<Rational> induced_rep(degree, degree);
      int col_index(0);
      for (auto dit = entire(domain); !dit.at_end(); ++dit, ++col_index) {
         induced_rep(index_of_image(perm, *dit), col_index) = 1;
      }
      return induced_rep;
   }
};

} // end anonymous namespace

} }

#endif // __GROUP_REPRESENTATIONS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

