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

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <deque>

#include "libnormaliz/signed_dec.h"
#include "libnormaliz/list_and_map_operations.h"

namespace libnormaliz {
using std::cout;
using std::endl;
using std::ifstream;

// first "hoolow" subfacet in their list coming from the same simplex in the triangulation
template <typename Integer>
void SignedDec<Integer>::first_subfacet(const dynamic_bitset& Subfacet,
                                        const bool compute_multiplicity,
                                        Matrix<Integer>& PrimalSimplex,
                                        mpz_class& MultPrimal,
                                        vector<Integer>& DegreesPrimal,
                                        Matrix<Integer>& ValuesGeneric) {
    int tn = 0;
    if (omp_in_parallel())
        tn = omp_get_ancestor_thread_num(omp_start_level + 1);

    size_t g = 0;  // select generators in subfacet
    // Matrix<Integer> DualSimplex[tn](dim,dim);
    for (size_t i = 0; i < nr_gen; ++i) {
        if (Subfacet[i] == 1) {
            DualSimplex[tn][g] = Generators[i];
            g++;
        }
    }
    DualSimplex[tn][dim - 1] = Generic;

    Integer MultDual;
    DualSimplex[tn].simplex_data(identity_key(dim), PrimalSimplex, MultDual, SimplexDataWork[tn], SimplexDataUnitMat, true);

    // DualSimplex[tn].simplex_data(identity_key(dim), PrimalSimplex, MultDual, true);

    if (compute_multiplicity) {
        DegreesPrimal = PrimalSimplex.MxV(GradingOnPrimal);
        mpz_class ProductOfHeights = 1;
        for (size_t i = 0; i < dim; ++i) {
            ProductOfHeights *= convertTo<mpz_class>(v_scalar_product(PrimalSimplex[i], DualSimplex[tn][i]));
        }
        MultPrimal = ProductOfHeights / convertTo<mpz_class>(MultDual);
    }
    else {  // we want to find a generic vector
        for (size_t i = 0; i < 2; i++)
            ValuesGeneric[i] = PrimalSimplex.MxV(CandidatesGeneric[i]);
    }
}

template <typename Integer>
void SignedDec<Integer>::next_subfacet(const dynamic_bitset& Subfacet_next,
                                       const dynamic_bitset& Subfacet_start,
                                       const Matrix<Integer>& PrimalSimplex,
                                       const bool compute_multiplicity,
                                       const mpz_class& MultPrimal,
                                       mpz_class& NewMult,
                                       const vector<Integer>& DegreesPrimal,
                                       vector<Integer>& NewDegrees,
                                       const Matrix<Integer>& ValuesGeneric,
                                       Matrix<Integer>& NewValues) {
    size_t new_vert;
    size_t old_place = 0;  // this is the place of i in the ascending sequence of generators in Subfacet_start
    size_t g = 0;
    for (size_t i = 0; i < nr_gen; ++i) {
        if (Subfacet_next[i] && !Subfacet_start[i])
            new_vert = i;
        if (!Subfacet_next[i] && Subfacet_start[i]) {
            old_place = g;
        }
        if (Subfacet_start[i])
            g++;
    }

    // We want to replace the "old" Generators[old_vert] corresponding to row old_place
    // in PrimalSimplex gy the "new" Generators[new_vert]

    // evaluate old linear forms on new vertex
    vector<Integer> lambda = PrimalSimplex.MxV(Generators[new_vert]);

    // We only need the new degrees. This is a Fourier-Motzkin step.

    if (compute_multiplicity) {  // we really want to compute multiplicity
        for (size_t i = 0; i < dim; ++i) {
            if (i == old_place)  // is already coprime
                continue;
            NewDegrees[i] = (lambda[i] * DegreesPrimal[old_place] - lambda[old_place] * DegreesPrimal[i]);
            if (!check_range(NewDegrees[i]))
                throw ArithmeticException("Overflow in degree computation. Starting with gigger integer class");
        }
        NewDegrees[old_place] = -DegreesPrimal[old_place];
        NewMult = MultPrimal;
        mpz_class MultFactor = convertTo<mpz_class>(lambda[old_place]);

        mpz_t raw_power;
        mpz_init(raw_power);
        mpz_pow_ui(raw_power, MultFactor.get_mpz_t(), (unsigned long)dim - 1);
        mpz_class MultPower(raw_power);
        NewMult *= MultPower;  // corresponds to the virtual  multiplication
                               // of dim-1 rows by lambbda[old_place]
        NewMult = Iabs(NewMult);
    }
    else {
        for (size_t k = 0; k < 2; ++k) {
            for (size_t i = 0; i < dim; ++i) {
                if (i == old_place)  // is already coprime
                    continue;
                NewValues[k][i] = (lambda[i] * ValuesGeneric[k][old_place] - lambda[old_place] * ValuesGeneric[k][i]);
            }
            NewValues[k][old_place] = -ValuesGeneric[k][old_place];
        }
    }
}

// This function tries to
// Find a generic element. For this purpose we exchage the role of the generic element and the grading.
// The point is to find an element that does not share a critical hyperplane with the grading. This is a
// syymetric relation. The function becomes 2 candidates in CandisatesGeneric and tries to form a suitable
// linear combination if this is possible at all. It is possible if there is no critical hyperplane (through
// the fraing that contains both candidates. Then it is a matter to find the linear combination
// that lies in none of the hyperplanes. If one is lucky, then one of the candidates is already generic in this sense.

template <typename Integer>
bool SignedDec<Integer>::FindGeneric() {
    bool success = true;

    vector<vector<bool> > IsGeneric(omp_get_max_threads(), vector<bool>(2, true));
    Matrix<Integer> Quot_tn(omp_get_max_threads(), 2);
    vector<Integer> Quot(2);

    long RelBound = 10000;
#ifdef NMZ_EXTENDED_TESTS
    if (test_small_pyramids)
        RelBound = 1;
#endif
    vector<deque<bool> > Relations(RelBound + 1, deque<bool>(RelBound + 1, true));  // deque because of parallelization

    if (verbose) {
        verboseOutput() << "Trying to find generic linear combination of " << endl;
        CandidatesGeneric.pretty_print(verboseOutput());
    }

    mpz_class Dummy_mpz;  // used in place of the multiplicities that are not computed here
    Matrix<Integer> Dummy_mat;
    vector<Integer> Dummy_vec;

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

#pragma omp parallel
    {
        Matrix<Integer> PrimalSimplex(dim, dim);
        Matrix<Integer> ValuesGeneric(2, dim);

        size_t ppos = 0;

        auto S = SubfacetsBySimplex->begin();
        size_t nr_subfacets_by_simplex = SubfacetsBySimplex->size();

        int tn = 0;
        if (omp_in_parallel())
            tn = omp_get_ancestor_thread_num(omp_start_level + 1);

#pragma omp for schedule(dynamic)
        for (size_t fac = 0; fac < nr_subfacets_by_simplex; ++fac) {
            if (skip_remaining)
                continue;

            for (; fac > ppos; ++ppos, ++S)
                ;
            for (; fac < ppos; --ppos, --S)
                ;

            try {
                if (verbose && fac % 10000 == 0 && fac > 0) {
#pragma omp critical(VERBOSE)
                    { verboseOutput() << fac << " simplices done " << endl; }
                }

                Matrix<Integer> NewValues;
                dynamic_bitset Subfacet_start;
                bool first = true;

                list<dynamic_bitset> SubfacetsOfSimplex;  // now we reproduce the subfacets of the hollow triangulation
                for (size_t i = 0; i < nr_gen; ++i) {     // coming from simplex S
                    if (S->second[i]) {
                        SubfacetsOfSimplex.push_back(S->first);
                        SubfacetsOfSimplex.back()[i] = 0;
                    }
                }

                for (auto& Subfacet : SubfacetsOfSimplex) {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    if (first) {
                        first = false;

                        first_subfacet(Subfacet, false, PrimalSimplex, Dummy_mpz, Dummy_vec, ValuesGeneric);
                        // computes the first simplex in this walk
                        Subfacet_start = Subfacet;
                        NewValues = ValuesGeneric;
                    }
                    else {
                        next_subfacet(Subfacet, Subfacet_start, PrimalSimplex, false, Dummy_mpz, Dummy_mpz, Dummy_vec, Dummy_vec,
                                      ValuesGeneric, NewValues);
                    }

                    for (size_t i = 0; i < dim; ++i) {
                        bool good = false;
                        for (size_t k = 0; k < 2; ++k) {
                            if (NewValues[k][i] != 0) {
                                good = true;
                                // cout << i << " " << k << endl;
                            }
                            else {
                                IsGeneric[tn][k] = false;
                            }
                        }
                        if (!good) {  // there is a linear form giving 0 on both candidates !
                            skip_remaining = true;
#pragma omp flush(skip_remaining)
                            if (verbose)
                                verboseOutput() << "Must increase coefficients" << endl;
                            success = false;
                            break;
                        }

                        if (NewValues[0][i] == 0 || NewValues[1][i] == 0)
                            continue;
                        if (NewValues[0][i] < 0)
                            continue;
                        if (NewValues[1][i] > 0)
                            continue;
                        // remaining case: pos at 0, neg at 1
                        Integer quot = 1 + (-NewValues[1][i]) / NewValues[0][i];
                        if (quot > Quot_tn[tn][0])
                            Quot_tn[tn][0] = quot;
                        quot = 1 + NewValues[0][i] / (-NewValues[1][i]);
                        if (quot > Quot_tn[tn][1])
                            Quot_tn[tn][1] = quot;

                        Integer g = libnormaliz::gcd(NewValues[0][i], NewValues[1][i]);
                        Integer r0 = (-NewValues[1][i]) / g;
                        if (r0 <= RelBound) {
                            Integer r1 = NewValues[0][i] / g;
                            if (r1 <= RelBound) {
                                long i0 = convertTo<long>(r0);
                                long i1 = convertTo<long>(r1);
                                Relations[i0][i1] = false;
                            }
                        }

                    }  // for i (coordinates)

                    if (!success)
                        break;

                }  // loop for given simplex

            } catch (const std::exception&) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
#pragma omp flush(skip_remaining)
            }

        }  // for fac

    }  // parallel

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    if (!success)
        return false;

    // cout << IsGeneric;
    // Quot_tn.pretty_print(cout);

    for (int i = 0; i < omp_get_max_threads(); ++i) {
        for (size_t j = 0; j < 2; ++j) {
            if (Quot_tn[i][j] > Quot[j])
                Quot[j] = Quot_tn[i][j];
            if (!IsGeneric[i][j])
                IsGeneric[0][j] = false;
        }
    }
    if (IsGeneric[0][0])
        GenericComputed = CandidatesGeneric[0];
    else {
        if (IsGeneric[0][1])
            GenericComputed = CandidatesGeneric[1];
    }
    if (GenericComputed.size() > 0) {
        if (verbose)
            verboseOutput() << "Generic on the nose" << endl;
        return true;
    }

    // Now we try to find a linear combination by checking the "syzygies" for one that is
    // not hit. Success is indicted by "found". The pair (i,j) gives the suitable
    // coefficients.
    bool found = false;
    vector<Integer> Coeff(2);
    for (long k = 2; k <= 2 * RelBound; ++k) {
        long i_start = 1;
        if (k > RelBound)
            i_start = k - RelBound + 1;
        long i_end = k - 1;
        if (i_end > RelBound)
            i_end = RelBound;
        for (long i = i_start; i <= i_end; ++i) {
            long j = k - i;
            if (libnormaliz::gcd(i, j) > 1)
                continue;
            if (Relations[i][j]) {
                Coeff[0] = convertTo<Integer>(i);
                Coeff[1] = convertTo<Integer>(j);
                found = true;
                break;
            }
        }  // j
        if (found)
            break;
    }
    if (found) {
        v_scalar_multiplication(CandidatesGeneric[0], Coeff[0]);
        v_scalar_multiplication(CandidatesGeneric[1], Coeff[1]);
        GenericComputed = CandidatesGeneric[0];
        GenericComputed = v_add(GenericComputed, CandidatesGeneric[1]);
        if (verbose)
            verboseOutput() << "Generic with coeff " << Coeff[0] << " " << Coeff[1] << endl;
        return true;
    }

    // the last resort: multiply one of the two vector by a large factor
    // so that the other vector cann be added without creating a zero for one
    // of the critical linear forms
    int k;
    if (Quot[0] <= Quot[1])
        k = 0;
    else
        k = 1;
    GenericComputed = CandidatesGeneric[1 - k];
    v_scalar_multiplication(CandidatesGeneric[k], Quot[k]);
    GenericComputed = v_add(GenericComputed, CandidatesGeneric[k]);
    if (verbose)
        verboseOutput() << "Generic Computed with factor " << Quot[k] << endl;

    return true;
}

//-------------------------------------------------------------------------

template <typename Integer>
bool SignedDec<Integer>::ComputeMultiplicity() {
    // vector<mpq_class> Collect(omp_get_max_threads());
    // vector<mpq_class> HelpCollect(omp_get_max_threads());
    // vector<int> CountCollect(omp_get_max_threads());

    if (decimal_digits > 0)
        approximate = true;
    approx_denominator = 1;
    if (approximate) {
        for (long i = 0; i < decimal_digits; ++i)
            approx_denominator *= 10;
    }
    vector<AdditionPyramid<mpq_class> > Collect(omp_get_max_threads());
    vector<mpz_class> Collect_mpz(omp_get_max_threads(), 0);
    bool success = true;

    if (verbose)
        verboseOutput() << "Generic " << Generic;

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

    for (size_t i = 0; i < Collect.size(); ++i) {
        Collect[i].set_capacity(8);
    }

#pragma omp parallel
    {
        Matrix<Integer> PrimalSimplex(dim, dim);
        Matrix<Integer> Dummy_mat;

        size_t ppos = 0;

        auto S = SubfacetsBySimplex->begin();
        size_t nr_subfacets_by_simplex = SubfacetsBySimplex->size();

        int tn = 0;
        if (omp_in_parallel())
            tn = omp_get_ancestor_thread_num(omp_start_level + 1);

#pragma omp for schedule(dynamic)
        for (size_t fac = 0; fac < nr_subfacets_by_simplex; ++fac) {
            if (skip_remaining)
                continue;

            for (; fac > ppos; ++ppos, ++S)
                ;
            for (; fac < ppos; --ppos, --S)
                ;

            try {
                if (verbose && fac % 10000 == 0 && fac > 0) {
#pragma omp critical(VERBOSE)
                    { verboseOutput() << fac << " simplices done " << endl; }
                }

                mpz_class NewMult;
                mpz_class MultPrimal;
                // dynamic_bitset Subfacet = S->first;

                vector<Integer> DegreesPrimal(dim);
                vector<Integer> NewDegrees(dim);
                dynamic_bitset Subfacet_start;
                bool first = true;

                list<dynamic_bitset> SubfacetsOfSimplex;  // now we reproduce the subfacets of the hollow triangulation
                for (size_t i = 0; i < nr_gen; ++i) {     // coming from simplex S
                    if (S->second[i]) {
                        SubfacetsOfSimplex.push_back(S->first);
                        SubfacetsOfSimplex.back()[i] = 0;
                    }
                }

                for (auto& Subfacet : SubfacetsOfSimplex) {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    if (first) {
                        first = false;
                        first_subfacet(Subfacet, true, PrimalSimplex, MultPrimal, DegreesPrimal, Dummy_mat);
                        // computes the first simplex in this walk

                        Subfacet_start = Subfacet;
                        NewMult = MultPrimal;
                        NewDegrees = DegreesPrimal;
                    }
                    else {
                        next_subfacet(Subfacet, Subfacet_start, PrimalSimplex, true, MultPrimal, NewMult, DegreesPrimal,
                                      NewDegrees, Dummy_mat, Dummy_mat);
                    }

                    for (size_t i = 0; i < dim; ++i) {
                        if (NewDegrees[i] == 0) {  // should never happen !!!!!!
                            success = false;
                            skip_remaining = true;
#pragma omp flush(skip_remaining)
                            if (verbose)
                                verboseOutput() << "Vector not generic" << endl;
                            break;
                        }
                    }

                    if (!success)
                        break;

                    mpz_class GradProdPrimal = 1;
                    for (size_t i = 0; i < dim; ++i)
                        GradProdPrimal *= convertTo<mpz_class>(NewDegrees[i]);
                    mpz_class NewMult_mpz = convertTo<mpz_class>(NewMult);
                    if (approximate) {
                        NewMult_mpz *= approx_denominator;
                        NewMult_mpz /= GradProdPrimal;
                        Collect_mpz[tn] += NewMult_mpz;
                    }

                    else {
                        mpq_class NewMult_mpq(NewMult_mpz);
                        NewMult_mpq /= GradProdPrimal;
                        Collect[tn].add(NewMult_mpq);
                    }
                }  // loop for given simplex

            } catch (const std::exception&) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
#pragma omp flush(skip_remaining)
            }

        }  // for fac

    }  // parallel

    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    vector<mpq_class> ThreadMult(Collect.size());
    mpq_class TotalVol;

    if (verbose)
        verboseOutput() << "Adding multiplicities of threads" << endl;

    if (approximate) {
        mpz_class TotalVol_mpz = 0;
        for (size_t tn = 0; tn < Collect_mpz.size(); ++tn)
            TotalVol_mpz += Collect_mpz[tn];
        TotalVol = TotalVol_mpz;
        TotalVol /= approx_denominator;
    }
    else {
        for (size_t tn = 0; tn < Collect.size(); ++tn) {
            ThreadMult[tn] = Collect[tn].sum();
        }
        TotalVol = vector_sum_cascade(ThreadMult);
    }
    /* for(size_t tn = 0; tn < Collect.size();++tn){
        TotalVol += Collect[tn].sum();
        // TotalVol += HelpCollect[tn];
    }*/

    /*
    mpz_class test_den = 1;
    for(long i=0; i<=100;++i)
        test_den *= 10;
    mpz_class mult_num = TotalVol.get_num();
    mpz_class mult_den = TotalVol.get_den();
    mult_num *= test_den;
    mult_num /= mult_den;
    cout << "Fixed test num " << endl;
    cout << mult_num << endl << endl;
    */

    multiplicity = TotalVol;
    if (verbose) {
        verboseOutput() << endl << "Mult (before NoGradingDenom correction) " << multiplicity << endl;
        verboseOutput() << "Mult (float) " << std::setprecision(12) << mpq_to_nmz_float(multiplicity) << endl;
    }

    return true;
}

template <typename Integer>
SignedDec<Integer>::SignedDec(vector<pair<dynamic_bitset, dynamic_bitset> >& SFS,
                              const Matrix<Integer>& Gens,
                              const vector<Integer> Grad,
                              const int osl) {
    SubfacetsBySimplex = &(SFS);
    Generators = Gens;
    GradingOnPrimal = Grad;
    nr_gen = Generators.nr_of_rows();
    dim = Generators[0].size();
    omp_start_level = osl;
    multiplicity = 0;
    int_multiplicity = 0;
    approximate = false;

    SimplexDataUnitMat = Matrix<Integer>(dim);
    SimplexDataWork.resize(omp_get_max_threads(), Matrix<Integer>(dim, 2 * dim));
    DualSimplex.resize(omp_get_max_threads(), Matrix<Integer>(dim, dim));
}

template <typename Integer>
SignedDec<Integer>::SignedDec() {
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template class SignedDec<long>;
#endif
template class SignedDec<long long>;
template class SignedDec<mpz_class>;

//--------------------------------------------------------------------------

const size_t HollowTriBound = 10000000;  // bound for the number of simplices computed in a pattern
                                         // evaluated for hollow triangulation

// const size_t SubFacetsJobsBound = 20; // bound for number of stored "subfacet jobs" = remove_twin jobs
const size_t MiniblockBound = 10000;

size_t HollowTriangulation::make_hollow_triangulation_inner(const vector<size_t>& Selection,
                                                            const vector<key_t>& PatternKey,
                                                            const dynamic_bitset& Pattern) {
    if (verbose) {
        verboseOutput() << "Evaluating " << Selection.size() << " simplices ";
        if (PatternKey.size() == 0)
            verboseOutput() << endl;
        else {
            vector<key_t> block_start, block_end;
            block_start.push_back(PatternKey[0]);
            for (size_t k = 1; k < PatternKey.size(); ++k) {
                if (PatternKey[k] > PatternKey[k - 1] + 1) {
                    block_end.push_back(PatternKey[k - 1]);
                    block_start.push_back(PatternKey[k]);
                }
            }
            block_end.push_back(PatternKey.back());
            verboseOutput() << "for ";
            for (size_t k = 0; k < block_start.size(); ++k) {
                if (block_end[k] == block_start[k])
                    verboseOutput() << block_end[k] << " ";
                else
                    verboseOutput() << block_start[k] << "-" << block_end[k] << " ";
            }
            verboseOutput() << endl;
        }
    }

    list<pair<dynamic_bitset, size_t> > Subfacets;
    bool restricted = false;
    if (PatternKey.size() > 0)
        restricted = true;

    vector<key_t> NonPattern;  // NonPattern is the complement of Pattern before the highest selected gen
    if (restricted) {
        for (size_t i = 0; i < PatternKey.back(); ++i) {
            if (!Pattern[i])
                NonPattern.push_back(i);
        }
    }

    size_t nr_tri = Selection.size();

    long nr_threads = omp_get_max_threads();
    size_t block_size = nr_tri / nr_threads;
    block_size++;

    vector<list<pair<dynamic_bitset, size_t> > > SubBlock(nr_threads);
    vector<int> CountMiniblocks(nr_threads, 1);

    int threads_needed = nr_tri / block_size;
    if (threads_needed * block_size < nr_tri)
        threads_needed++;

    size_t clean_up_point = 2 + (HollowTriBound / MiniblockBound) / (2 * threads_needed);

    bool skip_remaining = false;
    std::exception_ptr tmp_exception;

#pragma omp parallel for
    for (int q = 0; q < threads_needed; ++q) {
        if (skip_remaining)
            continue;

        try {
            size_t block_start = q * block_size;
            if (block_start > nr_tri)
                block_start = 0;
            size_t block_end = block_start + block_size;
            if (block_end > nr_tri)
                block_end = nr_tri;

            size_t nr_subblocks = (block_end - block_start) / MiniblockBound;
            nr_subblocks++;

            list<pair<dynamic_bitset, size_t> > MiniBlock;
            for (size_t k = 0; k < nr_subblocks; ++k) {
                size_t subblock_start = block_start + k * MiniblockBound;
                size_t subblock_end = subblock_start + MiniblockBound;
                if (subblock_end > block_end)
                    subblock_end = block_end;

                // #pragma omp critical(HOLLOW_PROGRESS)
                // if(verbose && nr_subblocks*nr_threads > 100)
                //    verboseOutput() << "Block " << q+1 << " Subblock " << k+1 << " of " << nr_subblocks << endl;

                INTERRUPT_COMPUTATION_BY_EXCEPTION
                for (size_t p = subblock_start; p < subblock_end; ++p) {
                    size_t pp = Selection[p];
                    if (!restricted) {
                        for (size_t j = 0; j < nr_gen; ++j) {           // we make copies in which we delete
                            if (Triangulation_ind[pp].first[j] == 1) {  // one entry each
                                MiniBlock.push_back(make_pair(Triangulation_ind[pp].first, pp));  // nr_done serves as a signature
                                MiniBlock.back().first[j] = 0;  // that allows us to recognize subfacets
                            }                                   // that arise from the same simplex in T
                        }
                    }
                    else {
                        bool done = false;
                        for (size_t j = 0; j < NonPattern.size(); ++j) {
                            if (Triangulation_ind[pp].first[NonPattern[j]]) {
                                MiniBlock.push_back(make_pair(Triangulation_ind[pp].first, pp));
                                MiniBlock.back().first[NonPattern[j]] = 0;
                                done = true;
                                break;
                            }
                        }

                        if (done)
                            continue;

                        for (size_t j = PatternKey.back() + 1; j < nr_gen; ++j) {
                            if (Triangulation_ind[pp].first[j] == 1) {
                                MiniBlock.push_back(make_pair(Triangulation_ind[pp].first, pp));
                                MiniBlock.back().first[j] = 0;
                                // cout << "+++Pattern " << j << endl;
                            }
                        }
                    }
                }
                remove_twins_in_first(MiniBlock);
                SubBlock[q].splice(SubBlock[q].end(), MiniBlock);
                if (CountMiniblocks[q] % clean_up_point == 0) {
                    remove_twins_in_first(SubBlock[q]);
                    CountMiniblocks[q] = 0;
                }
                CountMiniblocks[q]++;
            }

            remove_twins_in_first(SubBlock[q]);  // true

        } catch (const std::exception&) {
            tmp_exception = std::current_exception();
            skip_remaining = true;
#pragma omp flush(skip_remaining)
        }
    }
    if (!(tmp_exception == 0))
        std::rethrow_exception(tmp_exception);

    int step = 2;
    bool merged = true;
    skip_remaining = false;
    while (merged) {
        merged = false;
        // if(verbose && Selection.size() > 200000)
        //     verboseOutput() << "Merging hollow triangulation, step size " << step << endl;
#pragma omp parallel for
        for (int k = 0; k < nr_threads; k += step) {
            if (skip_remaining)
                continue;
            try {
                INTERRUPT_COMPUTATION_BY_EXCEPTION

                if (nr_threads > k + step / 2) {
                    SubBlock[k].merge(SubBlock[k + step / 2]);
                    merged = true;
                }
            } catch (const std::exception&) {
                tmp_exception = std::current_exception();
                skip_remaining = true;
#pragma omp flush(skip_remaining)
            }
        }
        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);
        step *= 2;
    }
    Subfacets.swap(SubBlock[0]);
    remove_twins_in_first(Subfacets, true);

    size_t nr_subfacets = Subfacets.size();

    for (auto F = Subfacets.begin(); F != Subfacets.end();) {  // encode subfacets as a single bitset associated to
        size_t s = F->second;                                  // simplex
        dynamic_bitset diff = Triangulation_ind[s].first;
        diff -= F->first;
        Triangulation_ind[s].second |= diff;
        F = Subfacets.erase(F);
    }

    return nr_subfacets;
}

//--------------------------------------------------------------------------

size_t HollowTriangulation::refine_and_process_selection(vector<size_t>& Selection,
                                                         const vector<key_t>& PatternKey,
                                                         const dynamic_bitset& Pattern,
                                                         size_t& nr_subfacets) {
    vector<size_t> Refinement;
    key_t select_gen = PatternKey.back();

    vector<key_t> NonPattern;
    for (size_t i = 0; i < PatternKey.back(); ++i) {
        if (!Pattern[i])
            NonPattern.push_back(i);
    }

    dynamic_bitset TwoInNonPattern(Selection.size());
    for (size_t i = 0; i < Selection.size(); ++i) {              // At all places in PatternKey we want a 1
        if (!Triangulation_ind[Selection[i]].first[select_gen])  // and at most one more before
            continue;                                            // the largest entry in PatternKey
        size_t nr_ones = 0;
        bool good = true;
        for (size_t j = 0; j < NonPattern.size(); ++j) {
            if (Triangulation_ind[Selection[i]].first[NonPattern[j]])
                nr_ones++;
            if (nr_ones > 1) {
                TwoInNonPattern[i] = 1;
                good = false;
                break;
            }
        }
        if (good)
            Refinement.push_back(Selection[i]);
    }

    if (Refinement.size() >= HollowTriBound
#ifdef NMZ_EXTENDED_TESTS
        || (test_small_pyramids && Refinement.size() >= 10)
#endif
    )
        extend_selection_pattern(Refinement, PatternKey, Pattern, nr_subfacets);
    else {
        if (Refinement.size() > 0) {
            // struct timeval begin, end;
            // gettimeofday(&begin, 0);
            nr_subfacets += make_hollow_triangulation_inner(Refinement, PatternKey, Pattern);
            /* gettimeofday(&end, 0);
            long seconds = end.tv_sec - begin.tv_sec;
            long microseconds = end.tv_usec - begin.tv_usec;
            double elapsed = seconds + microseconds*1e-6;
            printf("Time measured: %.3f seconds.\n", elapsed); */
        }
    }

    vector<size_t> NewSelection;
    for (size_t i = 0; i < Selection.size(); ++i) {
        if (!TwoInNonPattern[i])
            NewSelection.push_back(Selection[i]);
    }
    // cout << "Sieving " << Selection.size() << " -- " << NewSelection.size() << endl;
    swap(Selection, NewSelection);

    return nr_subfacets;
}

//--------------------------------------------------------------------------

size_t HollowTriangulation::extend_selection_pattern(vector<size_t>& Selection,
                                                     const vector<key_t>& PatternKey,
                                                     const dynamic_bitset& Pattern,
                                                     size_t& nr_subfacets) {
    if (Selection.size() == 0)
        return nr_subfacets;

    size_t start_gen;
    if (PatternKey.size() == 0)
        start_gen = 0;
    else
        start_gen = PatternKey.back() + 1;

    int total_nr_gaps = nr_gen - dim + 1;  // in a subfacet
    int gaps_already = (start_gen + 1) - PatternKey.size();
    gaps_already--;  // one of the non-pattern places can be set. We stay on the safe size
    int nr_further_gaps = total_nr_gaps - gaps_already;
    size_t last_gen = start_gen + nr_further_gaps + 1;
    if (last_gen >= nr_gen)
        last_gen = nr_gen - 1;

    for (size_t i = start_gen; i <= last_gen; ++i) {
        vector<key_t> PatternKeyRefinement = PatternKey;
        PatternKeyRefinement.push_back(i);

        dynamic_bitset PatternRefinement = Pattern;
        PatternRefinement[i] = 1;
        if (verbose) {
            vector<key_t> block_start, block_end;
            block_start.push_back(PatternKeyRefinement[0]);
            for (size_t k = 1; k < PatternKeyRefinement.size(); ++k) {
                if (PatternKeyRefinement[k] > PatternKeyRefinement[k - 1] + 1) {
                    block_end.push_back(PatternKeyRefinement[k - 1]);
                    block_start.push_back(PatternKeyRefinement[k]);
                }
            }
            block_end.push_back(PatternKeyRefinement.back());
            verboseOutput() << "Select ";
            for (size_t k = 0; k < block_start.size(); ++k) {
                if (block_end[k] == block_start[k])
                    verboseOutput() << block_end[k] << " ";
                else
                    verboseOutput() << block_start[k] << "-" << block_end[k] << " ";
            }
            verboseOutput() << endl;
        }

        refine_and_process_selection(Selection, PatternKeyRefinement, PatternRefinement, nr_subfacets);

        if (Selection.size() == 0)
            return nr_subfacets;
    }

    return nr_subfacets;
}

//--------------------------------------------------------------------------

size_t HollowTriangulation::make_hollow_triangulation() {
    Triangulation_ind.shrink_to_fit();

    sort(Triangulation_ind.begin(), Triangulation_ind.end());

    vector<key_t> PatternKey;
    dynamic_bitset Pattern(nr_gen);
    size_t nr_subfacets = 0;

    for (auto& T : Triangulation_ind)
        T.second.resize(nr_gen);

    vector<size_t> All(Triangulation_ind.size());
    for (size_t i = 0; i < All.size(); ++i)
        All[i] = i;

    if (Triangulation_ind.size() < HollowTriBound)
        nr_subfacets = make_hollow_triangulation_inner(All, PatternKey, Pattern);
    else
        extend_selection_pattern(All, PatternKey, Pattern, nr_subfacets);

    return nr_subfacets;
}

HollowTriangulation::HollowTriangulation(vector<pair<dynamic_bitset, dynamic_bitset> >& TriInd,
                                         const size_t d,
                                         const size_t ng,
                                         bool verb) {
    swap(Triangulation_ind, TriInd);
    nr_gen = ng;
    dim = d;
    verbose = verb;
}

}  // namespace libnormaliz
