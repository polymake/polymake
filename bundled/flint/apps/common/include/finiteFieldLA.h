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

#include <flint/nmod_mat.h>
#include <flint/fmpz.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "polymake/Integer.h"
#include "polymake/Matrix.h"


namespace polymake {
    namespace common {
        namespace flint {
            template <typename MatrixType>
            void matrix_to_nmod_mat_t(nmod_mat_t& result, const GenericMatrix<MatrixType>& m, const Int p) {
                auto intmat = convert_to<Integer>(m);
                nmod_mat_init(result, intmat.rows(), intmat.cols(), p);

                for (auto r = entire<indexed>(rows(intmat));  !r.at_end();  ++r) {
                    for (auto e = entire<indexed>(*r);  !e.at_end();  ++e) {
                        long int entry = *e % p;
                        if (entry < 0) {
                            entry += p;
                        }
                        nmod_mat_entry(result, r.index(), e.index()) = entry;
                    }
                }
            }
        } // namespace flint
        template <typename MatrixType>
        mp_limb_signed_t rank_mod_p(const GenericMatrix<MatrixType>& m, const Int p) {
            nmod_mat_t in;
            flint::matrix_to_nmod_mat_t(in, m, p);
            mp_limb_signed_t rank = nmod_mat_rank(in);
            nmod_mat_clear(in);
            return rank;
        }

    } // namespace common
} // namespace polymake
