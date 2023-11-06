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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "polymake/Integer.h"
#include "polymake/SparseMatrix.h"

namespace polymake {

namespace common {

namespace flint {

template <typename MatrixType>
void matrix_to_fmpzmat(fmpz_mat_t& fmat, const GenericMatrix<MatrixType>& m)
{
   auto intmat = convert_to<Integer>(m);
   fmpz_mat_init(fmat,intmat.rows(),intmat.cols());
   for (auto r = entire<indexed>(rows(intmat));  !r.at_end();  ++r)
      for (auto e = entire<indexed>(*r);  !e.at_end();  ++e)
         fmpz_set_mpz(fmpz_mat_entry(fmat, r.index(), e.index()), e->get_rep());
}

inline SparseMatrix<Integer> matrix_from_fmpzmat(const fmpz_mat_t& fmat)
{
   SparseMatrix<Integer> smat(fmat->r,fmat->c);
   for (int i = 0; i < fmat->r; i++)
      for (int j = 0; j < fmat->c; j++)
         if (!fmpz_is_zero(fmpz_mat_entry(fmat, i,j))) {
            mpz_t tmp;
            mpz_init(tmp);
            fmpz_get_mpz(tmp, fmpz_mat_entry(fmat, i, j));
            smat(i,j) = Integer(std::move(tmp));
         }
   return smat;
}

}

template <typename MatrixType>
SparseMatrix<Integer> smith_normal_form_flint(const GenericMatrix<MatrixType>& m) {
   fmpz_mat_t in, res;
   fmpz_mat_init(res,m.rows(),m.cols());
   flint::matrix_to_fmpzmat(in, m);

   fmpz_mat_snf(res, in);
   
   fmpz_mat_clear(in);
   SparseMatrix<Integer> sparseres(flint::matrix_from_fmpzmat(res));
   fmpz_mat_clear(res);
   return sparseres;
}

} }


