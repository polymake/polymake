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

#ifndef NORMALIZ_SIGNED_DEC_H
#define NORMALIZ_SIGNED_DEC_H

#include "libnormaliz/general.h"
#include "libnormaliz/matrix.h"
#include "libnormaliz/dynamic_bitset.h"

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

// Class for the computation of multiplicities via signed decompoasition

template <typename Integer>
class SignedDec {
    template <typename>
    friend class Full_Cone;

   public:
    bool verbose;

    vector<pair<dynamic_bitset, dynamic_bitset> >* SubfacetsBySimplex;
    size_t size_hollow_triangulation;
    size_t dim;
    size_t nr_gen;
    int omp_start_level;
    mpq_class multiplicity;
    mpz_class int_multiplicity;
    long decimal_digits;
    bool approximate;

    mpz_class approx_denominator;

    Integer GradingDenom;

    string Polynomial;
    // nmz_float EuclideanIntegral;
    mpq_class Integral, VirtualMultiplicity;
    nmz_float RawEuclideanIntegral;
    long DegreeOfPolynomial;

    Matrix<Integer> Generators;
    Matrix<Integer> Embedding;  // transformation on the primal side back to cone coordinates
    // Matrix<mpz_class> Genererators_mpz;
    vector<Integer> GradingOnPrimal;
    // Matrix<mpz_class> GradingOnPrimal_mpz;
    Matrix<Integer> CandidatesGeneric;
    vector<Integer> Generic;
    vector<Integer> GenericComputed;

    Matrix<Integer> SimplexDataUnitMat;
    vector<Matrix<Integer> > SimplexDataWork;
    vector<Matrix<Integer> > DualSimplex;

    void first_subfacet(const dynamic_bitset& Subfacet,
                        const bool compute_multiplicity,
                        Matrix<Integer>& PrimalSimplex,
                        mpz_class& MultPrimal,
                        vector<Integer>& DegreesPrimal,
                        Matrix<Integer>& ValuesGeneric);
    void next_subfacet(const dynamic_bitset& Subfacet_next,
                       const dynamic_bitset& Subfacet_start,
                       const Matrix<Integer>& PrimalSimplex,
                       const bool compute_multiplicity,
                       const mpz_class& MultPrimal,
                       mpz_class& NewMult,
                       const vector<Integer>& DegreesPrimal,
                       vector<Integer>& NewDegrees,
                       const Matrix<Integer>& ValuesGeneric,
                       Matrix<Integer>& NewValues);

    SignedDec();
    SignedDec(vector<pair<dynamic_bitset, dynamic_bitset> >& SFS,
              const Matrix<Integer>& Gens,
              const vector<Integer> Grad,
              const int osl);
    bool FindGeneric();
    bool ComputeMultiplicity();
    bool ComputeIntegral(const bool do_virt);
};

class HollowTriangulation {
    template <typename>
    friend class Full_Cone;

    vector<pair<dynamic_bitset, dynamic_bitset> > Triangulation_ind;  // triangulation encoded by bitsets

    size_t nr_gen, dim;

    bool verbose;

    size_t make_hollow_triangulation_inner(const vector<size_t>& Selection,
                                           const vector<key_t>& PatternKey,
                                           const dynamic_bitset& Pattern);
    size_t refine_and_process_selection(vector<size_t>& Selection,
                                        const vector<key_t>& PatternKey,
                                        const dynamic_bitset& Pattern,
                                        size_t& nr_subfacets);
    size_t extend_selection_pattern(vector<size_t>& Selection,
                                    const vector<key_t>& PatternKey,
                                    const dynamic_bitset& Pattern,
                                    size_t& nr_subfacets);
    size_t make_hollow_triangulation();

    HollowTriangulation(vector<pair<dynamic_bitset, dynamic_bitset> >& TriInd, const size_t d, const size_t ng, bool verb);
};

}  // namespace libnormaliz

#endif  // NMZ_CHUNK_H
