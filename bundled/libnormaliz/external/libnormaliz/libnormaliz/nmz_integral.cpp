#ifdef NMZ_COCOA
/*
 * Copyright (C) 2012-2014  Winfried Bruns, Christof Soeger
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

#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>

#include "libnormaliz/nmz_integrate.h"
#include "libnormaliz/cone.h"
#include "libnormaliz/full_cone.h"
#include "libnormaliz/vector_operations.h"
// #include "libnormaliz/map_operations.h"
#include "libnormaliz/dynamic_bitset.h"
#include "libnormaliz/list_and_map_operations.h"

using namespace CoCoA;

#include "../libnormaliz/my_omp.h"

namespace libnormaliz {

bool verbose_INT;

void processInputPolynomial(const string& poly_as_string,
                            const SparsePolyRing& R,
                            const SparsePolyRing& RZZ,
                            const bool& do_leadCoeff,
                            const long dim,
                            PolynomialData& PolData) {
    // "res" stands for "result"
    // resPrimeFactors are homogenized, the "nonhom" come from the original polynomial

    string input_string = poly_as_string;
    size_t semicolon = input_string.find(';');
    if (semicolon != string::npos) {  // remove semicolon
        input_string[semicolon] = ' ';
    }

    vector<string> inputFactors;
    while (true) {
        size_t sep = input_string.find('|');
        if (sep == string::npos)
            break;
        inputFactors.push_back(input_string.substr(0, sep));
        input_string = input_string.substr(sep + 1);
    }
    inputFactors.push_back(input_string);

    vector<RingElem> factorsRead;
    for (auto& inp : inputFactors)
        factorsRead.push_back(ReadExpr(R, inp));

    vector<long> rawMultiplicities;
    vector<RingElem> rawPrimeFactorsNonhom;

    if (verbose_INT)
        verboseOutput() << "Polynomial read" << endl;

    bool homogeneous = true;
    RingElem remainingFactor = one(R);
    for (auto& G : factorsRead) {
        // we factor the polynomials read and make them integral this way they
        // must further be homogenized and converted to polynomials with ZZ
        // coefficients (instead of inegral QQ) The homogenization is necessary
        // to allow substitutions over ZZ
        if (deg(G) == 0) {
            remainingFactor *= G;  // constants go into remainingFactor
            continue;              // this extra treatment would not be necessary
        }

        // homogeneous=(G==LF(G));
        vector<RingElem> compsG = homogComps(G);
        // we test for homogeneity. In case do_leadCoeff==true, polynomial
        // is replaced by highest homogeneous component
        if (G != compsG[compsG.size() - 1]) {
            homogeneous = false;
            if (do_leadCoeff) {
                G = compsG[compsG.size() - 1];
            }
        }

        factorization<RingElem> FF = factor(G);  // now the factorization and transfer to integer coefficients
        for (long j = 0; j < (long)FF.myFactors().size(); ++j) {
            rawPrimeFactorsNonhom.push_back(FF.myFactors()[j]);  // these are the factors of the polynomial to be integrated
            rawMultiplicities.push_back(FF.myMultiplicities()[j]);
        }
        remainingFactor *= FF.myRemainingFactor();
    }

    if (verbose_INT && do_leadCoeff && !homogeneous)
        verboseOutput() << "Polynomial is inhomogeneous. Replacing it by highest homogeneous component" << endl;

    PolData.homogeneous = homogeneous;

    // we collect the factors   from the varrious input factors
    // no way to work with a map since there is no order of RingElem

    vector<RingElem> primeFactorsNonhom;
    vector<RingElem> primeFactors;
    vector<long> multiplicities;

    for (size_t i = 0; i < rawPrimeFactorsNonhom.size(); ++i) {
        bool found = false;
        for (size_t j = 0; j < primeFactorsNonhom.size(); ++j) {
            if (rawPrimeFactorsNonhom[i] == primeFactorsNonhom[j]) {
                multiplicities[j] += rawMultiplicities[i];
                found = true;
                break;
            }
        }
        if (!found) {
            primeFactorsNonhom.push_back(rawPrimeFactorsNonhom[i]);
            primeFactors.push_back(makeZZCoeff(homogenize(rawPrimeFactorsNonhom[i]), RZZ));
            multiplicities.push_back(rawMultiplicities[i]);
        }
    }

    PolData.FF = ourFactorization(primeFactors, multiplicities, remainingFactor);    // assembels the data
    ourFactorization FFNonhom(primeFactorsNonhom, multiplicities, remainingFactor);  // for output

    long nf = PolData.FF.myFactors.size();  // No real need to make FFNonhom
    if (verbose_INT) {
        verboseOutput() << "Factorization" << endl;  // we show the factorization so that the user can check
        for (long i = 0; i < nf; ++i)
            verboseOutput() << FFNonhom.myFactors[i] << "  mult " << PolData.FF.myMultiplicities[i] << endl;
        verboseOutput() << "Remaining factor " << PolData.FF.myRemainingFactor << endl << endl;
    }

    PolData.F = one(R);  // the polynomial to be integrated with QQ coefficients
    for (const auto& G : factorsRead)
        PolData.F *= G;

    PolData.degree = deg(PolData.F);

    PolData.Factorial.resize(PolData.degree + dim);  // precomputed values
    for (long i = 0; i < PolData.degree + dim; ++i)
        PolData.Factorial[i] = factorial(i);

    PolData.FactQuot.resize(PolData.degree + dim);  // precomputed values
    for (long i = 0; i < PolData.degree + dim; ++i)
        PolData.FactQuot[i] = PolData.Factorial.back() / PolData.Factorial[i];

    PolData.dimension = dim;
}

BigRat IntegralUnitSimpl(const RingElem& F, const SparsePolyRing& P, const PolynomialData& PolData, const long& rank) {
    long dim = NumIndets(P);
    vector<long> v(dim);

    SparsePolyIter mon = BeginIter(F);        // go over the given polynomial
    map<vector<long>, RingElem> orderedMons;  // will take the ordered exponent vectors

    for (; !IsEnded(mon); ++mon) {
        exponents(v, PP(mon));  // this function gives the exponent vector back as v
        sort(v.begin() + 1, v.begin() + rank + 1);
        auto ord_mon = orderedMons.find(v);  // insert into map or add coefficient
        if (ord_mon != orderedMons.end()) {
            ord_mon->second += coeff(mon);
        }
        else {
            orderedMons.insert(pair<vector<long>, RingElem>(v, coeff(mon)));
        }
    }

    long deg;
    BigInt facProd, I;
    I = 0;
    for (const auto& ord_mon : orderedMons) {
        deg = 0;
        v = ord_mon.first;
        IsInteger(facProd, ord_mon.second);  // start with coefficient and multipliy by Factorials
        for (long i = 1; i <= rank; ++i) {
            deg += v[i];
            facProd *= PolData.Factorial[v[i]];
        }
        I += facProd * PolData.FactQuot[deg + rank - 1];  // maxFact/Factorial[deg+rank-1];
    }

    BigRat Irat;
    Irat = I;
    return Irat / PolData.Factorial.back();
}

template <typename Number>
BigRat substituteAndIntegrate(const vector<vector<Number> >& A,
                              const vector<Number>& degrees,
                              const BigInt& lcmDegs,
                              const SparsePolyRing& R,
                              const PolynomialData& PolData) {
    // applies linar substitution y --> y*(lcmDegs*A/degrees) to all factors in FF
    // where row A[i] is divided by degrees[i]
    // After substitution the polynomial is integrated over the unit simplex
    // and the integral is returned

    size_t i;
    size_t m = A.size();
    long rank = (long)m;  // we prefer rank to be of type long
    vector<RingElem> v(m, zero(R));

    BigInt quot;
    for (i = 0; i < m; i++) {
        quot = lcmDegs / degrees[i];
        v[i] = indets(R)[i + 1] * quot;
    }
    vector<RingElem> w = VxM(v, A);
    vector<RingElem> w1(w.size() + 1, zero(R));
    w1[0] = RingElem(R, lcmDegs);
    for (i = 1; i < w1.size(); ++i)  // we have to shift w since the (i+1)st variable
        w1[i] = w[i - 1];            // corresponds to coordinate i (counted from 0)

    // RingHom phi=PolyAlgebraHom(R,R,w1);

    RingElem G1(zero(R));
    list<RingElem> sortedFactors;
    for (i = 0; i < PolData.FF.myFactors.size(); ++i) {
        // G1=phi(FF.myFactors[i]);
        G1 = mySubstitution(PolData.FF.myFactors[i], w1);
        for (int nn = 0; nn < PolData.FF.myMultiplicities[i]; ++nn)
            sortedFactors.push_back(G1);
    }

    sortedFactors.sort(compareLength);

    RingElem G(one(R));

    for (const auto& sf : sortedFactors)
        G *= sf;

    // verboseOutput() << "Evaluating integral over unit simplex" << endl;
    // dynamic_bitset dummyInd;
    // vector<long> dummyDeg(degrees.size(),1);

    return (IntegralUnitSimpl(G, R, PolData, rank));  // orderExpos(G,dummyDeg,dummyInd,false)
}

template <typename Integer>
void readGens(Cone<Integer>& C, Matrix<long>& gens, const vector<long>& grading, bool check_ascending) {
    // get  from C for nmz_integrate functions

    size_t i, j;
    size_t nrows, ncols;
    nrows = C.getBasicTriangulation().second.nr_of_rows();
    ncols = C.getEmbeddingDim();
    gens.resize(nrows, ncols);

    for (i = 0; i < nrows; i++) {
        for (j = 0; j < ncols; j++) {
            convert(gens[i], C.getBasicTriangulation().second[i]);
        }
        if (check_ascending) {
            long degree, prevDegree = 1;
            degree = v_scalar_product(gens[i], grading);
            if (degree < prevDegree) {
                throw FatalException(" Degrees of generators not weakly ascending!");
            }
            prevDegree = degree;
        }
    }
}

void integrate(SignedDec<mpz_class>& SD, const bool do_virt_mult) {
    GlobalManager CoCoAFoundations;

    try {
        bool verbose_INTsave = verbose_INT;
        verbose_INT = SD.verbose;

        if (verbose_INT) {
            verboseOutput() << "==========================================================" << endl;
            verboseOutput() << "Integration over signed decomposition" << endl;
            verboseOutput() << "==========================================================" << endl << endl;
        }

        long dim = SD.dim;
        bool do_transformation = false;
        long rank = dim;  // we are in the full dimensional case or:
        if (SD.Embedding.nr_of_rows() > 0) {
            dim = SD.Embedding[0].size();
            do_transformation = true;
        }

        vector<mpz_class> grading = SD.GradingOnPrimal;  // to use the same names as in the standard integrate(...)
        mpz_class gradingDenom = v_gcd(grading);

        SparsePolyRing R = NewPolyRing_DMPI(RingQQ(), dim + 1, lex);
        SparsePolyRing RZZ = NewPolyRing_DMPI(RingZZ(), PPM(R));  // same indets and ordering as R

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        PolynomialData PolData;
        processInputPolynomial(SD.Polynomial, R, RZZ, do_virt_mult, dim, PolData);
        SD.DegreeOfPolynomial = PolData.degree;

        if (verbose_INT) {
            verboseOutput() << "********************************************" << endl;
            verboseOutput() << SD.size_hollow_triangulation << " simplicial cones to be evaluated" << endl;
            verboseOutput() << "********************************************" << endl;
        }

        size_t progress_step = 10;
        if (SD.size_hollow_triangulation >= 1000000)
            progress_step = 100;

        size_t nrSimplDone = 0;

        vector<AdditionPyramid<BigRat> > I_thread(omp_get_max_threads());
        vector<mpz_class> Collect_mpz(omp_get_max_threads(), 0);

        std::exception_ptr tmp_exception;
        bool skip_remaining = false;
        int omp_start_level = omp_get_level();

#pragma omp parallel
        {
            mpz_class det, det_dual;
            vector<mpz_class> degrees(rank);
            Matrix<mpz_class> A(rank, dim);
            Matrix<mpz_class> A_0(rank, rank);
            BigRat ISimpl;      // integral over a simplex
            mpz_class prodDeg;  // product of the degrees of the generators
            RingElem h(zero(R));
            mpz_class MinusOne = -1;

            vector<BigInt> degreesBigInt(rank);
            BigInt lcmDegsBigInt;
            vector<vector<BigInt> > ABigInt(A.nr_of_rows());
            for (size_t i = 0; i < A.nr_of_rows(); ++i)
                ABigInt[i].resize(A.nr_of_columns());
            BigInt prodDegBigInt;
            BigInt detBigInt;

            auto S = SD.SubfacetsBySimplex->begin();
            size_t nr_subfacets_by_simplex = SD.SubfacetsBySimplex->size();

            int tn = 0;
            if (omp_in_parallel())
                tn = omp_get_ancestor_thread_num(omp_start_level + 1);

            size_t ppos = 0;

#pragma omp for schedule(dynamic)
            for (size_t fac = 0; fac < nr_subfacets_by_simplex; ++fac) {
                if (skip_remaining)
                    continue;

                for (; fac > ppos; ++ppos, ++S)
                    ;
                for (; fac < ppos; --ppos, --S)
                    ;

                try {
                    list<dynamic_bitset> SubfacetsOfSimplex;  // now we reproduce the subfacets of the hollow triangulation
                    for (size_t i = 0; i < SD.nr_gen; ++i) {  // coming from simplex S
                        if (S->second[i]) {
                            SubfacetsOfSimplex.push_back(S->first);
                            SubfacetsOfSimplex.back()[i] = 0;
                        }
                    }

                    for (auto& Subfacet : SubfacetsOfSimplex) {
                        INTERRUPT_COMPUTATION_BY_EXCEPTION

                        size_t g = 0;  // select generators in subfacet
                        Matrix<mpz_class> DualSimplex(rank, rank);
                        for (size_t i = 0; i < SD.nr_gen; ++i) {
                            if (Subfacet[i] == 1) {
                                DualSimplex[g] = SD.Generators[i];
                                g++;
                            }
                        }
                        DualSimplex[rank - 1] = SD.Generic;

                        if (do_transformation) {
                            DualSimplex.simplex_data(identity_key(rank), A_0, det_dual, true);
                            A = A_0.multiplication(SD.Embedding);
                            degrees = A_0.MxV(grading);
                            det = A_0.vol();
                        }
                        else {
                            DualSimplex.simplex_data(identity_key(rank), A, det_dual, true);
                            degrees = A.MxV(grading);
                            det = A.vol();
                        }

                        long our_sign = 1;

                        mpz_class lcmDegs = 1;
                        prodDeg = 1;
                        for (int i = 0; i < rank; ++i) {
                            if (degrees[i] < 0) {
                                our_sign = -our_sign;
                                degrees[i] = -degrees[i];
                                v_scalar_multiplication(A[i], MinusOne);
                            }
                            degrees[i] /= gradingDenom;
                            lcmDegs = libnormaliz::lcm(lcmDegs, degrees[i]);
                            prodDeg *= degrees[i];
                        }
                        // cout << "-----------------" << endl;
                        // A.pretty_print(cout);
                        // cout << "-----------------" << endl;
                        // We transfer our data to CoCoALib types. This is not necessary if we come from long
                        // since CoCoALib allows multiplication by long etc. Not so for mpz_class
                        lcmDegsBigInt = BigIntFromMPZ(lcmDegs.get_mpz_t());
                        for (size_t i = 0; i < A.nr_of_rows(); ++i) {
                            for (size_t j = 0; j < A.nr_of_columns(); ++j) {
                                ABigInt[i][j] = BigIntFromMPZ(A[i][j].get_mpz_t());
                                // cout << ABigInt[i][j] << " ";
                            }
                            // cout << endl;
                        }
                        for (size_t i = 0; i < degrees.size(); ++i)
                            degreesBigInt[i] = BigIntFromMPZ(degrees[i].get_mpz_t());
                        prodDegBigInt = BigIntFromMPZ(prodDeg.get_mpz_t());
                        detBigInt = BigIntFromMPZ(det.get_mpz_t());

                        ISimpl = (detBigInt * substituteAndIntegrate(ABigInt, degreesBigInt, lcmDegsBigInt, RZZ, PolData)) /
                                 prodDegBigInt;
                        ISimpl *= our_sign;
                        ISimpl /= power(lcmDegsBigInt, PolData.degree);  // done here because lcmDegs not used globally

                        if (SD.approximate) {
                            BigInt Num = num(ISimpl);
                            BigInt Den = den(ISimpl);
                            Num *= BigIntFromMPZ(SD.approx_denominator.get_mpz_t());
                            Num /= Den;
                            Collect_mpz[tn] += mpz(Num);
                        }
                        else {
                            I_thread[tn].add(ISimpl);
                        }

                        // a little bit of progress report
                        if ((++nrSimplDone) % progress_step == 0 && verbose_INT)
#pragma omp critical(PROGRESS)
                            verboseOutput() << nrSimplDone << " simplicial cones done" << endl;

                    }  // S

                }  // try
                catch (const std::exception&) {
                    tmp_exception = std::current_exception();
                    skip_remaining = true;
#pragma omp flush(skip_remaining)
                }

            }  // fac

        }  // parallel
        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);

        BigRat I;  // accumulates the integral
        if (SD.approximate) {
            mpz_class Total = 0;
            for (size_t i = 0; i < Collect_mpz.size(); ++i) {
                Total += Collect_mpz[i];
            }
            BigInt Total_BigInt = BigIntFromMPZ(Total.get_mpz_t());
            I = Total_BigInt;
            I /= BigIntFromMPZ(SD.approx_denominator.get_mpz_t());
        }
        else {
            I = 0;
            for (size_t i = 0; i < I_thread.size(); ++i) {
                I += I_thread[i].sum();
            }
        }

        // I /= power(lcmDegs, PolData.degree);
        BigRat RFrat;
        IsRational(RFrat, PolData.FF.myRemainingFactor);  // from RingQQ to BigRat
        // cout << "RRRRRR " << RFrat << endl;
        I *= RFrat;

        // See comment below for this correction
        if (gradingDenom != 1) {
            I *= BigIntFromMPZ(gradingDenom.get_mpz_t());
        }

        string result = "Integral";
        if (do_virt_mult)
            result = "Virtual multiplicity";

        BigRat VM = I;

        if (do_virt_mult) {
            VM *= factorial(PolData.degree + rank - 1);
            SD.VirtualMultiplicity = mpq(VM);
        }
        else {
            BigRat I_fact = I * factorial(rank - 1);
            mpq_class Int_bridge = mpq(I_fact);
            nmz_float EuclInt = mpq_to_nmz_float(Int_bridge);
            // EuclInt *= C.euclidean_corr_factor(); // done in cone.cpp!
            SD.Integral = mpq(I);
            SD.RawEuclideanIntegral = EuclInt;
        }

        if (verbose_INT) {
            verboseOutput() << "********************************************" << endl;
            verboseOutput() << result << " is " << endl << VM << endl;
            verboseOutput() << "********************************************" << endl;
        }

        verbose_INT = verbose_INTsave;

    }  // try global
    catch (const CoCoA::ErrorInfo& err) {
        cerr << "***ERROR***  UNCAUGHT CoCoA error";
        ANNOUNCE(cerr, err);

        throw NmzCoCoAException("");
    }
}

template <typename Integer>
void integrate(Cone<Integer>& C, const bool do_virt_mult) {
    GlobalManager CoCoAFoundations;

    std::exception_ptr tmp_exception;

    try {
        long dim = C.getEmbeddingDim();
        // testPolynomial(C.getIntData().getPolynomial(),dim);

        bool verbose_INTsave = verbose_INT;
        verbose_INT = C.get_verbose();

        if (verbose_INT) {
            verboseOutput() << "==========================================================" << endl;
            verboseOutput() << "Integration" << endl;
            verboseOutput() << "==========================================================" << endl << endl;
        }

        vector<long> grading;
        convert(grading, C.getGrading());
        long gradingDenom;
        convert(gradingDenom, C.getGradingDenom());

        long rank = C.getRank();

        SparsePolyRing R = NewPolyRing_DMPI(RingQQ(), dim + 1, lex);
        SparsePolyRing RZZ = NewPolyRing_DMPI(RingZZ(), PPM(R));  // same indets and ordering as R

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        PolynomialData PolData;
        processInputPolynomial(C.getIntData().getPolynomial(), R, RZZ, do_virt_mult, dim, PolData);
        C.getIntData().setDegreeOfPolynomial(PolData.degree);

        Matrix<long> gens;
        readGens(C, gens, grading, false);
        if (verbose_INT)
            verboseOutput() << "Generators read" << endl;

        BigInt lcmDegs(1);
        for (size_t i = 0; i < gens.nr_of_rows(); ++i) {
            long deg = v_scalar_product(gens[i], grading);
            deg /= gradingDenom;
            lcmDegs = lcm(lcmDegs, deg);
        }

        size_t tri_size = C.getBasicTriangulation().first.size();  // also computes triangulation
        size_t k_start = 0, k_end = tri_size;

        for (size_t k = 0; k < tri_size; ++k)
            for (size_t j = 1; j < C.getBasicTriangulation().first[k].key.size(); ++j)
                if (!(C.getBasicTriangulation().first[k].key[j - 1] < C.getBasicTriangulation().first[k].key[j]))
                    throw FatalException("Key in triangulation not ordered");

        if (verbose_INT)
            verboseOutput() << "BasicTriangulation is ordered" << endl;

        size_t eval_size;
        if (k_start >= k_end)
            eval_size = 0;
        else
            eval_size = k_end - k_start;

        if (verbose_INT) {
            /* if (pseudo_par) {
                verboseOutput() << "********************************************" << endl;
                verboseOutput() << "Parallel block " << block_nr << endl;
            }*/
            verboseOutput() << "********************************************" << endl;
            verboseOutput() << eval_size << " simplicial cones to be evaluated" << endl;
            verboseOutput() << "********************************************" << endl;
        }

        size_t progress_step = 10;
        if (tri_size >= 1000000)
            progress_step = 100;

        size_t nrSimplDone = 0;

        vector<AdditionPyramid<BigRat> > I_thread(omp_get_max_threads());

        bool skip_remaining = false;

#pragma omp parallel
        {
            long det;  //  rank = C.getBasicTriangulation().first[0].key.size();
            vector<long> degrees(rank);
            Matrix<long> A(rank, dim);
            BigRat ISimpl;   // integral over a simplex
            BigInt prodDeg;  // product of the degrees of the generators
            RingElem h(zero(R));

#pragma omp for schedule(dynamic)
            for (size_t k = k_start; k < k_end; ++k) {
                if (skip_remaining)
                    continue;

                try {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    convert(det, C.getBasicTriangulation().first[k].vol);
                    for (long i = 0; i < rank; ++i)  // select submatrix defined by key
                        A[i] = gens[C.getBasicTriangulation().first[k].key[i]];

                    degrees = A.MxV(grading);
                    prodDeg = 1;
                    for (long i = 0; i < rank; ++i) {
                        degrees[i] /= gradingDenom;
                        prodDeg *= degrees[i];
                    }

                    // We apply the transformation formula for integrals -- but se ebelow for the correctin if the lattice
                    // height of 0 over the simplex is different from 1
                    ISimpl = (det * substituteAndIntegrate(A.get_elements(), degrees, lcmDegs, RZZ, PolData)) / prodDeg;
                    I_thread[omp_get_thread_num()].add(ISimpl);

                    // a little bit of progress report
                    if ((++nrSimplDone) % progress_step == 0 && verbose_INT)
#pragma omp critical(PROGRESS)
                        verboseOutput() << nrSimplDone << " simplicial cones done" << endl;

                } catch (const std::exception&) {
                    tmp_exception = std::current_exception();
                    skip_remaining = true;
#pragma omp flush(skip_remaining)
                }

            }  // triang

        }  // parallel
        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);

        BigRat I;  // accumulates the integral
        I = 0;
        for (size_t i = 0; i < I_thread.size(); ++i)
            I += I_thread[i].sum();

        I /= power(lcmDegs, PolData.degree);
        BigRat RFrat;
        IsRational(RFrat, PolData.FF.myRemainingFactor);  // from RingQQ to BigRat
        I *= RFrat;

        // We integrate over the polytope P which is the intersection of the cone
        // with the hyperplane at degree 1. Our transformation formula
        // is only correct if assumes that P hathe same lattice volume as
        // the convex hull of P and 0. Lattice volume comes from the effective lattice.
        // Therefore we need a correction factor if the restriction of the absolute
        // grading to the effective lattice is (grading on eff latt)/g with g>1.
        // this amounts to multiplying the integral by g.

        vector<Integer> test_grading = C.getSublattice().to_sublattice_dual_no_div(C.getGrading());
        Integer corr_factor = v_gcd(test_grading);
        if (corr_factor != gradingDenom) {
            mpz_class corr_mpz = convertTo<mpz_class>(corr_factor);
            // I*=BigInt(corr_mpz.get_mpz_t());
            I *= BigIntFromMPZ(corr_mpz.get_mpz_t());
        }

        string result = "Integral";
        if (do_virt_mult)
            result = "Virtual multiplicity";

        BigRat VM = I;

        if (do_virt_mult) {
            VM *= factorial(PolData.degree + rank - 1);
            C.getIntData().setVirtualMultiplicity(mpq(VM));
        }
        else {
            BigRat I_fact = I * factorial(rank - 1);
            mpq_class Int_bridge = mpq(I_fact);
            nmz_float EuclInt = mpq_to_nmz_float(Int_bridge);
            EuclInt *= C.euclidean_corr_factor();
            C.getIntData().setIntegral(mpq(I));
            C.getIntData().setEuclideanIntegral(EuclInt);
        }

        if (verbose_INT) {
            verboseOutput() << "********************************************" << endl;
            verboseOutput() << result << " is " << endl << VM << endl;
            verboseOutput() << "********************************************" << endl;
        }

        verbose_INT = verbose_INTsave;
    }  // try
    catch (const CoCoA::ErrorInfo& err) {
        cerr << "***ERROR***  UNCAUGHT CoCoA error";
        ANNOUNCE(cerr, err);

        throw NmzCoCoAException("");
    }
}

CyclRatFunct evaluateFaceClasses(const vector<vector<CyclRatFunct> >& GFP, map<vector<long>, RingElem>& faceClasses) {
    // computes the generating rational functions
    // for the denominator classes collected from proper faces and returns the sum

    SparsePolyRing R = owner(faceClasses.begin()->second);
    CyclRatFunct H(zero(R));
    // vector<CyclRatFunct> h(omp_get_max_threads(),CyclRatFunct(zero(R)));
    // vector<CyclRatFunct> h(1,CyclRatFunct(zero(R)));

    long mapsize = faceClasses.size();
    if (verbose_INT) {
        // verboseOutput() << "--------------------------------------------" << endl;
        verboseOutput() << "Evaluating " << mapsize << " face classes" << endl;
        // verboseOutput() << "--------------------------------------------" << endl;
    }
#pragma omp parallel
    {
        auto den = faceClasses.begin();
        long mpos = 0;
        CyclRatFunct h(zero(R));

#pragma omp for schedule(dynamic)
        for (long dc = 0; dc < mapsize; ++dc) {
            for (; mpos < dc; ++mpos, ++den)
                ;
            for (; mpos > dc; --mpos, --den)
                ;
            // verboseOutput() << "mpos " << mpos << endl;

            h = genFunct(GFP, den->second, den->first);
            h.simplifyCRF();
            if (false) {  // verbose_INT
#pragma omp critical(VERBOSE)
                {
                    verboseOutput() << "Class ";
                    for (size_t i = 0; i < den->first.size(); ++i)
                        verboseOutput() << den->first[i] << " ";
                    verboseOutput() << "NumTerms " << NumTerms(den->second) << endl;

                    // verboseOutput() << "input " << den->second << endl;
                }
            }

// h.showCoprimeCRF();
#pragma omp critical(ADDCLASSES)
            H.addCRF(h);
        }

    }  // parallel
    faceClasses.clear();
    H.simplifyCRF();
    return (H);
}

struct denomClassData {
    vector<long> degrees;
    size_t simplDue;
    size_t simplDone;
};

CyclRatFunct evaluateDenomClass(const vector<vector<CyclRatFunct> >& GFP, pair<denomClassData, vector<RingElem> >& denomClass) {
    // computes the generating rational function
    // for a denominator class and returns it

    SparsePolyRing R = owner(denomClass.second[0]);

    if (verbose_INT) {
#pragma omp critical(PROGRESS)
        {
            verboseOutput() << "--------------------------------------------" << endl;
            verboseOutput() << "Evaluating denom class ";
            for (size_t i = 0; i < denomClass.first.degrees.size(); ++i)
                verboseOutput() << denomClass.first.degrees[i] << " ";
            verboseOutput() << "NumTerms " << NumTerms(denomClass.second[0]) << endl;
            // verboseOutput() << denomClass.second << endl;
            verboseOutput() << "--------------------------------------------" << endl;
        }
    }

    CyclRatFunct h(zero(R));
    h = genFunct(GFP, denomClass.second[0], denomClass.first.degrees);

    denomClass.second[0] = 0;  // to save memory
    h.simplifyCRF();
    return (h);
}

void transferFacePolys(deque<pair<vector<long>, RingElem> >& facePolysThread, map<vector<long>, RingElem>& faceClasses) {
    // verboseOutput() << "In Transfer " << facePolysThread.size() << endl;
    for (size_t i = 0; i < facePolysThread.size(); ++i) {
        auto den_found = faceClasses.find(facePolysThread[i].first);
        if (den_found != faceClasses.end()) {
            den_found->second += facePolysThread[i].second;
        }
        else {
            faceClasses.insert(facePolysThread[i]);
            if (false) {  // verbose_INT
#pragma omp critical(VERBOSE)
                {
                    verboseOutput() << "New face class " << faceClasses.size() << " degrees ";
                    for (size_t j = 0; j < facePolysThread[i].first.size(); ++j)
                        verboseOutput() << facePolysThread[i].first[j] << " ";
                    verboseOutput() << endl << flush;
                }
            }
        }  // else
    }
    facePolysThread.clear();
}

libnormaliz::HilbertSeries nmzHilbertSeries(const CyclRatFunct& H, mpz_class& commonDen) {
    size_t i;
    vector<RingElem> HCoeff0 = ourCoeffs(H.num, 0);  // we must convert the coefficients
    BigInt commonDenBI(1);                           // and find the common denominator
    vector<BigRat> HCoeff1(HCoeff0.size());
    for (i = 0; i < HCoeff0.size(); ++i) {
        IsRational(HCoeff1[i], HCoeff0[i]);  // to BigRat
        commonDenBI = lcm(den(HCoeff1[i]), commonDenBI);
    }

    commonDen = mpz(commonDenBI);  // convert it to mpz_class

    BigInt HC2;
    vector<mpz_class> HCoeff3(HCoeff0.size());
    for (i = 0; i < HCoeff1.size(); ++i) {
        HC2 = num(HCoeff1[i] * commonDenBI);  // to BigInt
        HCoeff3[i] = mpz(HC2);                // to mpz_class
    }

    vector<long> denomDeg = denom2degrees(H.denom);
    libnormaliz::HilbertSeries HS(HCoeff3, count_in_map<long, long>(denomDeg));
    HS.simplify();
    return (HS);
}

bool compareDegrees(const STANLEYDATA_int& A, const STANLEYDATA_int& B) {
    return (A.degrees < B.degrees);
}

bool compareFaces(const SIMPLINEXDATA_INT& A, const SIMPLINEXDATA_INT& B) {
    return (A.card > B.card);
}

void prepare_inclusion_exclusion_simpl(const STANLEYDATA_int& S,
                                       const vector<pair<dynamic_bitset, long> >& inExCollect,
                                       vector<SIMPLINEXDATA_INT>& inExSimplData) {
    size_t dim = S.key.size();
    vector<key_type> key = S.key;
    for (size_t i = 0; i < dim; ++i)
        key[i];

    dynamic_bitset intersection(dim), Excluded(dim);

    Excluded.set();
    for (size_t j = 0; j < dim; ++j)  // enough to test the first offset (coming from the zero vector)
        if (S.offsets[0][j] == 0)
            Excluded.reset(j);

    map<dynamic_bitset, long> inExSimpl;  // local version of nExCollect

    for (const auto& F : inExCollect) {
        // verboseOutput() << "F " << F.first << endl;
        bool still_active = true;
        for (size_t i = 0; i < dim; ++i)
            if (Excluded[i] && !F.first.test(key[i])) {
                still_active = false;
                break;
            }
        if (!still_active)
            continue;
        intersection.reset();
        for (size_t i = 0; i < dim; ++i) {
            if (F.first.test(key[i]))
                intersection.set(i);
        }
        auto G = inExSimpl.find(intersection);
        if (G != inExSimpl.end())
            G->second += F.second;
        else
            inExSimpl.insert(pair<dynamic_bitset, long>(intersection, F.second));
    }

    SIMPLINEXDATA_INT HilbData;
    inExSimplData.clear();
    vector<long> degrees;

    for (const auto& G : inExSimpl) {
        if (G.second != 0) {
            HilbData.GenInFace = G.first;
            HilbData.mult = G.second;
            HilbData.card = G.first.count();
            degrees.clear();
            for (size_t j = 0; j < dim; ++j)
                if (G.first.test(j))
                    degrees.push_back(S.degrees[j]);
            HilbData.degrees = degrees;
            HilbData.denom = degrees2denom(degrees);
            inExSimplData.push_back(HilbData);
        }
    }

    sort(inExSimplData.begin(), inExSimplData.end(), compareFaces);

    /* for(size_t i=0;i<inExSimplData.size();++i)
        verboseOutput() << inExSimplData[i].GenInFace << " ** " << inExSimplData[i].card << " || " << inExSimplData[i].mult << "
    ++ "<< inExSimplData[i].denom <<  endl; verboseOutput() << "InEx prepared" << endl; */
}

template <typename Integer>
void readInEx(Cone<Integer>& C, vector<pair<dynamic_bitset, long> >& inExCollect, const size_t nrGen) {
    size_t inExSize = C.getInclusionExclusionData().size(), keySize;
    long mult;
    dynamic_bitset indicator(nrGen);
    for (size_t i = 0; i < inExSize; ++i) {
        keySize = C.getInclusionExclusionData()[i].first.size();
        indicator.reset();
        for (size_t j = 0; j < keySize; ++j) {
            indicator.set(C.getInclusionExclusionData()[i].first[j]);
        }
        mult = C.getInclusionExclusionData()[i].second;
        inExCollect.push_back(pair<dynamic_bitset, long>(indicator, mult));
    }
}

template <typename Integer>
void readDecInEx(Cone<Integer>& C,
                 const long& dim, /* list<STANLEYDATA_int_INT>& StanleyDec, */
                 vector<pair<dynamic_bitset, long> >& inExCollect,
                 const size_t nrGen) {
    // rads Stanley decomposition and InExSata from C

    if (C.isComputed(ConeProperty::InclusionExclusionData)) {
        readInEx(C, inExCollect, nrGen);
    }

    // STANLEYDATA_int_INT newSimpl;
    // ong i=0;
    // newSimpl.key.resize(dim);

    long test;

    auto SD = C.getStanleyDec_mutable().first.begin();
    auto SD_end = C.getStanleyDec_mutable().first.end();

    for (; SD != SD_end; ++SD) {
        // swap(newSimpl.key,SD->key);
        test = -1;
        for (long i = 0; i < dim; ++i) {
            if ((long)SD->key[i] <= test) {
                throw FatalException("Key of simplicial cone not ascending or out of range");
            }
            test = SD->key[i];
        }

        /* swap(newSimpl.offsets,SD->offsets);
        StanleyDec.push_back(newSimpl);
        SD=C.getStanleyDec_mutable().erase(SD);*/
    }
    // C.resetStanleyDec();
}

template <typename Integer>
void generalizedEhrhartSeries(Cone<Integer>& C) {
    GlobalManager CoCoAFoundations;

    try {
        bool verbose_INTsave = verbose_INT;
        verbose_INT = C.get_verbose();

        if (verbose_INT) {
            verboseOutput() << "==========================================================" << endl;
            verboseOutput() << "Weighted Ehrhart series " << endl;
            verboseOutput() << "==========================================================" << endl << endl;
        }

        long i, j;

        vector<long> grading;
        convert(grading, C.getGrading());
        long gradingDenom;
        convert(gradingDenom, C.getGradingDenom());
        long rank = C.getRank();
        long dim = C.getEmbeddingDim();

        // processing the input polynomial

        SparsePolyRing R = NewPolyRing_DMPI(RingQQ(), dim + 1, lex);
        SparsePolyRing RZZ = NewPolyRing_DMPI(RingZZ(), PPM(R));  // same indets and ordering as R
        const RingElem& t = indets(RZZ)[0];

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        PolynomialData PolData;
        processInputPolynomial(C.getIntData().getPolynomial(), R, RZZ, false, dim, PolData);
        C.getIntData().setDegreeOfPolynomial(PolData.degree);

        if (rank == 0) {
            vector<RingElem> compsF = homogComps(PolData.F);
            CyclRatFunct HRat(compsF[0]);
            mpz_class commonDen;  // common denominator of coefficients of numerator of H
            libnormaliz::HilbertSeries HS(nmzHilbertSeries(HRat, commonDen));
            C.getIntData().setWeightedEhrhartSeries(make_pair(HS, commonDen));
            C.getIntData().computeWeightedEhrhartQuasiPolynomial();
            C.getIntData().setVirtualMultiplicity(0);
            return;
        }

        Matrix<long> gens;
        readGens(C, gens, grading, true);
        if (verbose_INT)
            verboseOutput() << "Generators read" << endl;
        long maxDegGen = v_scalar_product(gens[gens.nr_of_rows() - 1], grading) / gradingDenom;

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        // list<STANLEYDATA_int_INT> StanleyDec;
        vector<pair<dynamic_bitset, long> > inExCollect;
        readDecInEx(C, rank, inExCollect, gens.nr_of_rows());
        if (verbose_INT)
            verboseOutput() << "Stanley decomposition (and in/ex data) read" << endl;

        list<STANLEYDATA_int>& StanleyDec = C.getStanleyDec_mutable().first;

        size_t dec_size = StanleyDec.size();

        // Now we sort the Stanley decomposition by denominator class (= degree class)

        auto S = StanleyDec.begin();

        vector<long> degrees(rank);
        Matrix<long> A(rank, dim);

        // prepare sorting by computing degrees of generators

        BigInt lcmDets(1);  // to become the lcm of all dets of simplicial cones

        for (; S != StanleyDec.end(); ++S) {
            INTERRUPT_COMPUTATION_BY_EXCEPTION

            for (i = 0; i < rank; ++i)  // select submatrix defined by key
                A[i] = gens[S->key[i]];
            degrees = A.MxV(grading);
            for (i = 0; i < rank; ++i)
                degrees[i] /= gradingDenom;  // must be divisible
            S->degrees = degrees;
            lcmDets = lcm(lcmDets, S->offsets.nr_of_rows());
        }

        if (verbose_INT)
            verboseOutput() << "lcm(dets)=" << lcmDets << endl;

        StanleyDec.sort(compareDegrees);

        if (verbose_INT)
            verboseOutput() << "Stanley decomposition sorted" << endl;

        vector<pair<denomClassData, vector<RingElem> > > denomClasses;
        denomClassData denomClass;
        vector<RingElem> ZeroVectRingElem;
        for (int j = 0; j < omp_get_max_threads(); ++j)
            ZeroVectRingElem.push_back(zero(RZZ));

        vector<map<vector<long>, RingElem> > faceClasses(
            omp_get_max_threads());  // denominator classes for the faces
                                     // contrary to denomClasses these cannot be sorted beforehand

        vector<deque<pair<vector<long>, RingElem> > > facePolys(
            omp_get_max_threads());  // intermediate storage
                                     // contribution of faces first collected here, then transferred to faceClasses

        // we now make class 0 to get started
        S = StanleyDec.begin();
        denomClass.degrees = S->degrees;  // put degrees in class
        denomClass.simplDone = 0;
        denomClass.simplDue = 1;  // already one simplex to be done
        denomClasses.push_back(pair<denomClassData, vector<RingElem> >(denomClass, ZeroVectRingElem));
        size_t dc = 0;
        S->classNr = dc;  // assignment of class 0 to first simpl in sorted order

        auto prevS = StanleyDec.begin();

        for (++S; S != StanleyDec.end(); ++S, ++prevS) {
            if (S->degrees == prevS->degrees) {     // compare to predecessor
                S->classNr = dc;                    // assign class to simplex
                denomClasses[dc].first.simplDue++;  // number of simplices in class ++
            }
            else {
                denomClass.degrees = S->degrees;  // make new class
                denomClass.simplDone = 0;
                denomClass.simplDue = 1;
                denomClasses.push_back(pair<denomClassData, vector<RingElem> >(denomClass, ZeroVectRingElem));
                dc++;
                S->classNr = dc;
            }
        }

        if (verbose_INT)
            verboseOutput() << denomClasses.size() << " denominator classes built" << endl;

        vector<vector<CyclRatFunct> > GFP;  // we calculate the table of generating functions
        vector<CyclRatFunct> DummyCRFVect;  // for\sum i^n t^ki vor various values of k and n
        CyclRatFunct DummyCRF(zero(RZZ));
        for (j = 0; j <= PolData.degree; ++j)
            DummyCRFVect.push_back(DummyCRF);
        for (i = 0; i <= maxDegGen; ++i) {
            GFP.push_back(DummyCRFVect);
            for (j = 0; j <= PolData.degree; ++j)
                GFP[i][j] = genFunctPower1(RZZ, i, j);
        }

        CyclRatFunct H(zero(RZZ));  // accumulates the series

        if (verbose_INT) {
            verboseOutput() << "********************************************" << endl;
            verboseOutput() << dec_size << " simplicial cones to be evaluated" << endl;
            verboseOutput() << "********************************************" << endl;
        }

        size_t progress_step = 10;
        if (dec_size >= 1000000)
            progress_step = 100;

        size_t nrSimplDone = 0;

        std::exception_ptr tmp_exception;

        bool skip_remaining = false;
        int omp_start_level = omp_get_level();

#pragma omp parallel
        {
            long degree_b, i;
            long det;
            bool evaluateClass;
            vector<long> degrees;
            Matrix<long> A(rank, dim);
            auto S = StanleyDec.begin();

            RingElem h(zero(RZZ));           // for use in a simplex
            CyclRatFunct HClass(zero(RZZ));  // for single class

            size_t s, spos = 0;
#pragma omp for schedule(dynamic)
            for (s = 0; s < dec_size; ++s) {
                if (skip_remaining)
                    continue;

                for (; spos < s; ++spos, ++S)
                    ;
                for (; spos > s; --spos, --S)
                    ;

                try {
                    INTERRUPT_COMPUTATION_BY_EXCEPTION

                    int tn;
                    if (omp_get_level() == omp_start_level)
                        tn = 0;
                    else
                        tn = omp_get_ancestor_thread_num(omp_start_level + 1);

                    det = S->offsets.nr_of_rows();
                    degrees = S->degrees;

                    for (i = 0; i < rank; ++i)  // select submatrix defined by key
                        A[i] = gens[S->key[i]];

                    vector<SIMPLINEXDATA_INT> inExSimplData;
                    if (inExCollect.size() != 0)
                        prepare_inclusion_exclusion_simpl(*S, inExCollect, inExSimplData);

                    h = 0;
                    long iS = S->offsets.nr_of_rows();  // compute numerator for simplex being processed
                    for (i = 0; i < iS; ++i) {
                        degree_b = v_scalar_product(degrees, S->offsets[i]);
                        degree_b /= det;
                        h += power(t, degree_b) * affineLinearSubstitutionFL(PolData.FF, A.get_elements(), S->offsets[i], det,
                                                                             RZZ, degrees, lcmDets, inExSimplData, facePolys[tn]);
                    }

                    evaluateClass = false;  // necessary to evaluate class only once

                    // #pragma omp critical (ADDTOCLASS)
                    {
                        denomClasses[S->classNr].second[tn] += h;
#pragma omp critical(ADDTOCLASS)
                        {
                            denomClasses[S->classNr].first.simplDone++;

                            if (denomClasses[S->classNr].first.simplDone == denomClasses[S->classNr].first.simplDue)
                                evaluateClass = true;
                        }
                    }
                    if (evaluateClass) {
                        for (int j = 1; j < omp_get_max_threads(); ++j) {
                            denomClasses[S->classNr].second[0] += denomClasses[S->classNr].second[j];
                            denomClasses[S->classNr].second[j] = 0;
                        }

                        // denomClasses[S->classNr].second=0;  // <-------------------------------------
                        HClass = evaluateDenomClass(GFP, denomClasses[S->classNr]);
#pragma omp critical(ACCUMULATE)
                        { H.addCRF(HClass); }
                    }

                    // different strategy for faces, classes cllected by threads

                    if (facePolys[tn].size() >= 20) {
                        transferFacePolys(facePolys[tn], faceClasses[tn]);
                        if (faceClasses[tn].size() > 20) {
                            HClass = evaluateFaceClasses(GFP, faceClasses[tn]);
#pragma omp critical(ACCUMULATE)
                            { H.addCRF(HClass); }
                        }
                    }

#pragma omp critical(PROGRESS)  // a little bit of progress report
                    {
                        if ((++nrSimplDone) % progress_step == 0 && verbose_INT)
                            verboseOutput() << nrSimplDone << " simplicial cones done  "
                                            << endl;  // nrActiveFaces-nrActiveFacesOld << " faces done" << endl;
                        // nrActiveFacesOld=nrActiveFaces;
                    }

                } catch (const std::exception&) {
                    tmp_exception = std::current_exception();
                    skip_remaining = true;
#pragma omp flush(skip_remaining)
                }

            }  // Stanley dec

        }  // parallel

        if (!(tmp_exception == 0))
            std::rethrow_exception(tmp_exception);

        // collect the contribution of proper fases from inclusion/exclusion as far as not done yet

        for (int i = 0; i < omp_get_max_threads(); ++i) {
            transferFacePolys(facePolys[i], faceClasses[i]);
            if (!faceClasses[i].empty())
                H.addCRF(evaluateFaceClasses(GFP, faceClasses[i]));
        }

        // now we must return to rational coefficients

        CyclRatFunct HRat(zero(R));
        HRat.denom = H.denom;
        HRat.num = makeQQCoeff(H.num, R);

        HRat.num *= PolData.FF.myRemainingFactor;
        HRat.num /= power(lcmDets, PolData.degree);

        HRat.showCoprimeCRF();

        mpz_class commonDen;  // common denominator of coefficients of numerator of H
        libnormaliz::HilbertSeries HS(nmzHilbertSeries(HRat, commonDen));
        HS.set_nr_coeff_quasipol(C.getIntData().getWeightedEhrhartSeries().first.get_nr_coeff_quasipol());
        HS.set_expansion_degree(C.getIntData().getWeightedEhrhartSeries().first.get_expansion_degree());
        HS.set_period_bounded(C.getIntData().getWeightedEhrhartSeries().first.get_period_bounded());

        C.getIntData().setWeightedEhrhartSeries(make_pair(HS, commonDen));

        C.getIntData().computeWeightedEhrhartQuasiPolynomial();

        if (C.getIntData().isWeightedEhrhartQuasiPolynomialComputed()) {
            mpq_class genMultQ;
            long deg = C.getIntData().getWeightedEhrhartQuasiPolynomial()[0].size() - 1;
            long virtDeg = C.getRank() + C.getIntData().getDegreeOfPolynomial() - 1;
            if (deg == virtDeg)
                genMultQ = C.getIntData().getWeightedEhrhartQuasiPolynomial()[0][virtDeg];
            genMultQ *= ourFactorial(virtDeg);
            genMultQ /= C.getIntData().getWeightedEhrhartQuasiPolynomialDenom();
            C.getIntData().setVirtualMultiplicity(genMultQ);
        }

        verbose_INT = verbose_INTsave;

        return;
    }  // try
    catch (const CoCoA::ErrorInfo& err) {
        cerr << "***ERROR***  UNCAUGHT CoCoA error";
        ANNOUNCE(cerr, err);

        throw NmzCoCoAException("");
    }
}

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template void integrate(Cone<long>& C, const bool do_virt_mult);
#endif  // NMZ_MIC_OFFLOAD
template void integrate(Cone<long long>& C, const bool do_virt_mult);
template void integrate(Cone<mpz_class>& C, const bool do_virt_mult);

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
// template void integrate(SignedDec<long>& C, const bool do_virt_mult);
#endif  // NMZ_MIC_OFFLOAD
// template void integrate(SignedDec<long long>& C, const bool do_virt_mult);
// template void integrate(SignedDec<mpz_class>& C, const bool do_virt_mult);

#ifndef NMZ_MIC_OFFLOAD  // offload with long is not supported
template void generalizedEhrhartSeries<long>(Cone<long>& C);
#endif  // NMZ_MIC_OFFLOAD
template void generalizedEhrhartSeries<long long>(Cone<long long>& C);
template void generalizedEhrhartSeries<mpz_class>(Cone<mpz_class>& C);

}  // namespace libnormaliz

#endif  // NMZ_COCOA
