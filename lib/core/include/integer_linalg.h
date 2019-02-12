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


#ifndef POLYMAKE_HERMITE_NORMAL_FORM_H
#define POLYMAKE_HERMITE_NORMAL_FORM_H

#include <iostream>
#include "polymake/client.h"
#include "polymake/pair.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Bitset.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/numerical_functions.h"
#include "polymake/list"
#include "polymake/GenericStruct.h"

namespace pm {

template<typename E>
class HermiteNormalForm
  : public GenericStruct<HermiteNormalForm<E> > {

public:
   DeclSTRUCT( DeclTemplFIELD(hnf, Matrix<E>)
               DeclTemplFIELD(companion, SparseMatrix<E>)
               DeclTemplFIELD(rank, int) );
   
};


template <typename TMatrix, typename E>
int ranked_hermite_normal_form(const GenericMatrix<TMatrix, E>& M, Matrix<E>& hnf, SparseMatrix<E>& companion, bool reduced = true)
{
   SparseMatrix2x2<E> U;
   SparseMatrix<E> R, S;
   Matrix<E> N(M);

   const int rows = M.rows();
   const int cols = M.cols();

   R = unit_matrix<E>(cols);

   int current_row = 0, current_col = 0;
   int rank = -1;

   for (int i = 0; i<rows; ++i) {
      bool nonzero = true;

      // Find a non-zero entry and move it to here.
      if (N(i,current_col) == 0) {
         nonzero = false;
         for (int j = current_col; j<cols; ++j) {
           if (!is_zero(N(i,j))) {
             nonzero = true;
             U.i = current_col;
             U.j = j;
             U.a_ii = zero_value<E>();
             U.a_ij = one_value<E>();
             U.a_ji = one_value<E>();
             U.a_jj = zero_value<E>();
             R.multiply_from_right(U);
             N.multiply_from_right(U);
           }
         }
      }

      if (!nonzero) {
         ++current_row;
         continue;
      } else {
         rank = current_col;
      }

      // GCD part of algorithm.
      for (int j = current_col+1; j<cols; ++j) {
        if (!is_zero(N(i,j))) {
          U.i = current_col;
          U.j = j;
          ExtGCD<E> egcd = ext_gcd(N(i,current_col), N(i,j));
          U.a_ii = egcd.p;
          U.a_ji = egcd.q;
          U.a_ij = egcd.k2;
          U.a_jj = -egcd.k1;
          R.multiply_from_right(U);
          N.multiply_from_right(U);
        }
      }
      if (N(i,current_col)<0) {
         S = unit_matrix<E>(cols);
         S(current_col,current_col) = -1;
         R = R*S;
         N = N*S;
      }
      if (reduced) {
         for (int j=0; j<current_col; ++j) {
            U.i = j;
            U.j = current_col;
            E factor = N(i,j) % N(i,current_col);
            if (factor < 0) factor += N(i,current_col);
            factor = (N(i,j) - factor)/N(i,current_col);
            U.a_ii = 1;
            U.a_ji = -factor;
            U.a_ij = 0;
            U.a_jj = 1;
            R.multiply_from_right(U);
            N.multiply_from_right(U);
         }
      }
      ++current_col;
      if (current_col == cols) {
         break;
      }
   }
   
   ++rank;
   hnf = N;
   companion = R;

   return rank;
}


template <typename TMatrix, typename E>
HermiteNormalForm<E> hermite_normal_form(const GenericMatrix<TMatrix, E>& M, bool reduced = true)
{
   HermiteNormalForm<E> res;
   res.rank = ranked_hermite_normal_form(M, res.hnf, res.companion, reduced);
   return res;
}

//returns indices of a minimal rowspace basis of a matrix in an euclidean ring
template <typename TMatrix, typename E>
Set<int> basis_rows_integer(const GenericMatrix<TMatrix, E>& M)
{
  HermiteNormalForm<E> H = hermite_normal_form(M,false); //non-reduced form is faster, and we won't use the matrix later so big entrys should not be an issue.
  int pos = 0;
  Set<int> basis;
  for (auto cit = entire(cols(H.hnf)); !cit.at_end() && !is_zero(*cit); ++cit) {
    while (is_zero((*cit)[pos])) ++pos; //find uppermost non-null entry in this col
    basis += pos;
  }
  return basis;
}

//returns as rows a basis of the null space in an euclidean ring
template <typename TMatrix, typename E>
SparseMatrix<E> null_space_integer(const GenericMatrix<TMatrix, E>& M)
{
   Matrix<E> H;
   SparseMatrix<E> R;
   int r = ranked_hermite_normal_form(M, H, R);
   return T(R.minor(All, range(r, R.cols()-1)));
}

} // namespace pm

namespace polymake {

using pm::HermiteNormalForm;
using pm::null_space_integer;
using pm::basis_rows_integer;

}

#endif
