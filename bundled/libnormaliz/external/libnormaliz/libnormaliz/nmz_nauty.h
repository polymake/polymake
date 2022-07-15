/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

//---------------------------------------------------------------------------
#ifndef LIBNORMALIZ_NMZ_NAUTY_HPP
#define LIBNORMALIZ_NMZ_NAUTY_HPP
//---------------------------------------------------------------------------
#include <vector>
#include <list>
#include <iostream>
#include <string>

namespace libnormaliz {
using std::vector;

namespace AutomParam {
enum Quality {
    combinatorial,
    rational,
    euclidean,
    ambient_gen,
    ambient_ineq,
    algebraic,
    graded,
    integral,
    input_gen,
    input_ineq
};

enum Type { rational_primal, rational_dual, integral_standard, matrix };  // can be used in IsoType;

enum Method {  // the type of data from which we compute the automorphisms

    // using generators and support hyperplanes
    EH,  // E extreme rays
    GH,  // G other "generators" like the Hilbert basis
         //
    EL,  // using extreme rays and given linear forms, NOT USED PRESENTLY
    GL,  // G and given linear forms  DITTO
         //
         // using only generators
    EE,  // extreme rays
    GG   // given generators
};
enum Goals { PermsPrimal, OrbitsPrimal, PermsDual, OrbitsDual, LinMaps, IsoClass };
}  // end namespace AutomParam

template <typename Integer>
struct nauty_result {
    vector<vector<key_t> > GenPerms;
    vector<vector<key_t> > LinFormPerms;
    vector<key_t> GenOrbits;
    vector<key_t> LinFormOrbits;
    mpz_class order;
    BinaryMatrix<Integer> CanType;
    vector<key_t> CanLabellingGens;
};

template <typename Integer>
nauty_result<Integer> compute_automs_by_nauty_Gens_LF(const Matrix<Integer>& Generators,
                                                      size_t nr_special_gens,
                                                      const Matrix<Integer>& LinForms,
                                                      const size_t nr_special_linforms,
                                                      AutomParam::Quality quality);

template <typename Integer>
nauty_result<Integer> compute_automs_by_nauty_FromGensOnly(const Matrix<Integer>& Generators,
                                                           size_t nr_special_gens,
                                                           const Matrix<Integer>& SpecialLinForms,
                                                           AutomParam::Quality quality);

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
