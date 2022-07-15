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

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(push, target(mic))
#endif

#include <cassert>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include "libnormaliz/general.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/list_and_map_operations.h"
#include "libnormaliz/integer.h"
// #include "libnormaliz/convert.h"

#include "libnormaliz/matrix.h"

#ifdef NMZ_FLINT
#include "flint/flint.h"
#include "flint/fmpz_poly.h"
#endif

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::cout;
using std::endl;
using std::flush;
using std::istringstream;
using std::ostringstream;
using std::pair;

//---------------------------------------------------------------------------

template <typename Integer>
Integer permutations(const size_t& a, const size_t& b) {
    unsigned long i;
    Integer P = 1;
    for (i = a + 1; i <= b; i++) {
        P *= i;
    }
    return P;
}

#ifdef NMZ_FLINT
void flint_poly(fmpz_poly_t flp, const vector<mpz_class>& nmzp) {
    slong n = (slong)nmzp.size();
    fmpz_poly_fit_length(flp, n);
    for (size_t i = 0; i < nmzp.size(); ++i) {
        fmpz_poly_set_coeff_mpz(flp, (slong)i, nmzp[i].get_mpz_t());
    }
}

void nmz_poly(vector<mpz_class>& nmzp, const fmpz_poly_t flp) {
    size_t n = (size_t)fmpz_poly_length(flp);
    nmzp.resize(n);
    mpz_t c;
    mpz_init(c);
    for (size_t i = 0; i < nmzp.size(); ++i) {
        fmpz_poly_get_coeff_mpz(c, flp, i);
        nmzp[i] = mpz_class(c);
    }
    mpz_clear(c);
}
#endif

template <typename Integer>
vector<Integer> poly_mult(const vector<Integer>& a, const vector<Integer>& b) {
    size_t a_size = a.size();
    size_t b_size = b.size();

    if (a_size * b_size > 1000 && a_size > 10 && b_size > 10) {
        // omp_set_nested(1);
        return karatsubamult(a, b);
        // omp_set_nested(0);
    }

    vector<Integer> p(a_size + b_size - 1);
    size_t i, j;
    for (i = 0; i < a_size; ++i) {
        if (a[i] == 0)
            continue;
        for (j = 0; j < b_size; ++j) {
            if (b[j] == 0)
                continue;
            p[i + j] += a[i] * b[j];
        }
    }
    return p;
}

#ifdef NMZ_FLINT
template <>
vector<mpz_class> poly_mult(const vector<mpz_class>& a, const vector<mpz_class>& b) {
    size_t a_size = a.size();
    size_t b_size = b.size();

    vector<mpz_class> p(a_size + b_size - 1);
    fmpz_poly_t flp1, flp2;
    fmpz_poly_init(flp1);
    fmpz_poly_init(flp2);

    flint_poly(flp1, a);
    flint_poly(flp2, b);
    fmpz_poly_mul(flp1, flp1, flp2);
    nmz_poly(p, flp1);

    fmpz_poly_clear(flp1);
    fmpz_poly_clear(flp2);

    return p;
}
#endif

// division with remainder, a = q * b + r, deg(r) < deg(b), needs |leadcoef(b)| = 1
template <typename Integer>
void poly_div(vector<Integer>& q, vector<Integer>& r, const vector<Integer>& a, const vector<Integer>& b) {
    assert(b.back() != 0);                    // no unneeded zeros
    assert(b.back() == 1 || b.back() == -1);  // then division is always possible
    r = a;
    remove_zeros(r);
    size_t b_size = b.size();
    int degdiff = r.size() - b_size;  // degree differenz
    if (r.size() < b_size) {
        q = vector<Integer>();
    }
    else {
        q = vector<Integer>(degdiff + 1);
    }
    Integer divisor;
    size_t i = 0;

    while (r.size() >= b_size) {
        divisor = r.back() / b.back();
        q[degdiff] = divisor;
        // r -= divisor * t^degdiff * b
        for (i = 0; i < b_size; ++i) {
            r[i + degdiff] -= divisor * b[i];
        }
        remove_zeros(r);
        degdiff = r.size() - b_size;
    }

    return;
}

#ifdef NMZ_FLINT
template <>
void poly_div(vector<mpz_class>& q, vector<mpz_class>& r, const vector<mpz_class>& a, const vector<mpz_class>& b) {
    assert(b.back() != 0);                    // no unneeded zeros
    assert(b.back() == 1 || b.back() == -1);  // then division is always possible

    fmpz_poly_t flpa, flpb, flpq, flpr;
    fmpz_poly_init(flpa);
    fmpz_poly_init(flpb);
    fmpz_poly_init(flpq);
    fmpz_poly_init(flpr);

    flint_poly(flpa, a);
    flint_poly(flpb, b);

    fmpz_poly_divrem(flpq, flpr, flpa, flpb);
    nmz_poly(q, flpq);
    nmz_poly(r, flpr);

    fmpz_poly_clear(flpa);
    fmpz_poly_clear(flpb);
    fmpz_poly_clear(flpq);
    fmpz_poly_clear(flpr);

    return;
}
#endif

template <typename Integer>
vector<Integer> cyclotomicPoly(long n) {
    // the static variable is initialized only once and then stored
    static map<long, vector<Integer> > CyclotomicPoly = map<long, vector<Integer> >();
    if (CyclotomicPoly.count(n) == 0) {  // it was not computed so far
        vector<Integer> poly, q, r;
        for (long i = 1; i <= n; ++i) {
            // compute needed and uncomputed factors
            if (n % i == 0 && CyclotomicPoly.count(i) == 0) {
                // compute the i-th poly by dividing X^i-1 by the
                // d-th cycl.poly. with d divides i
                poly = vector<Integer>(i + 1);
                poly[0] = -1;
                poly[i] = 1;                    // X^i - 1
                for (long d = 1; d < i; ++d) {  // <= i/2 should be ok
                    if (i % d == 0) {
                        poly_div(q, r, poly, CyclotomicPoly[d]);
                        assert(r.empty());
                        poly = q;
                    }
                }
                CyclotomicPoly[i] = poly;
                // cout << i << "-th cycl. pol.: " << CyclotomicPoly[i];
            }
        }
    }
    assert(CyclotomicPoly.count(n) > 0);
    return CyclotomicPoly[n];
}

#ifdef NMZ_FLINT
template <>
vector<mpz_class> cyclotomicPoly(long n) {
    // the static variable is initialized only once and then stored
    static map<long, vector<mpz_class> > CyclotomicPoly = map<long, vector<mpz_class> >();
    if (CyclotomicPoly.count(n) == 0) {  // it was not computed so far
        vector<mpz_class> poly;
        fmpz_poly_t cyc;
        fmpz_poly_init(cyc);
        fmpz_poly_cyclotomic(cyc, (ulong)n);
        nmz_poly(poly, cyc);
        CyclotomicPoly[n] = poly;
        fmpz_poly_clear(cyc);
        // cout << i << "-th cycl. pol.: " << CyclotomicPoly[i];
    }
    assert(CyclotomicPoly.count(n) > 0);
    return CyclotomicPoly[n];
}
#endif

long lcm_of_keys(const map<long, denom_t>& m) {
    long l = 1;
    for (const auto& it : m) {
        if (it.second != 0)
            l = lcm(l, it.first);
    }
    return l;
}

// compute the hsop numerator by multiplying the HS with a denominator
// of the form product of (1-t^i)
void HilbertSeries::compute_hsop_num() const {
    // get the denominator as a polynomial by mutliplying the (1-t^i) terms
    vector<mpz_class> hsop_denom_poly = vector<mpz_class>(1, 1);
    long factor;
    for (auto& it : hsop_denom) {
        factor = it.first;
        denom_t& denom_i = it.second;
        poly_mult_to(hsop_denom_poly, factor, denom_i);
    }
    // cout << "new denominator as polynomial: " << hsop_denom_poly << endl;
    vector<mpz_class> quot, remainder, cyclo_poly;
    // first divide the new denom by the cyclo polynomials
    for (auto& it : cyclo_denom) {
        for (long i = 0; i < it.second; i++) {
            cyclo_poly = cyclotomicPoly<mpz_class>(it.first);
            // cout << "the cyclotomic polynomial is " << cyclo_poly << endl;
            // TODO: easier polynomial division possible?
            poly_div(quot, remainder, hsop_denom_poly, cyclo_poly);
            // cout << "the quotient is " << quot << endl;
            hsop_denom_poly = quot;
            assert(remainder.size() == 0);
        }
    }
    // multiply with the old numerator
    hsop_num = poly_mult(hsop_denom_poly, cyclo_num);
}

//---------------------------------------------------------------------------

void HilbertSeries::initialize() {
    is_simplified = false;
    shift = 0;
    verbose = false;
    nr_coeff_quasipol = -1;  // all coefficients
    expansion_degree = -1;
    period_bounded = true;
}

// Constructor, creates 0/1
HilbertSeries::HilbertSeries() {
    num = vector<mpz_class>(1, 0);
    // denom just default constructed
    initialize();
}

// Constructor, creates num/denom, see class description for format
HilbertSeries::HilbertSeries(const vector<num_t>& numerator, const vector<denom_t>& gen_degrees) {
    num = vector<mpz_class>(1, 0);
    add(numerator, gen_degrees);
    initialize();
}

// Constructor, creates num/denom, see class description for format
HilbertSeries::HilbertSeries(const vector<mpz_class>& numerator, const map<long, denom_t>& denominator) {
    num = numerator;
    denom = denominator;
    initialize();
}

/*
// Constructor, string as created by to_string_rep
HilbertSeries::HilbertSeries(const string& str) {
    from_string_rep(str);
    initialize();
}
*/

void HilbertSeries::reset() {
    num.clear();
    num.push_back(0);
    denom.clear();
    denom_classes.clear();
    shift = 0;
    is_simplified = false;
}

void HilbertSeries::set_nr_coeff_quasipol(long nr_coeff) {
    nr_coeff_quasipol = nr_coeff;
}

long HilbertSeries::get_nr_coeff_quasipol() const {
    return nr_coeff_quasipol;
}

void HilbertSeries::set_period_bounded(bool on_off) const {  // period_bounded is mutable
    period_bounded = on_off;
}

bool HilbertSeries::get_period_bounded() const {
    return period_bounded;
}
// add another HilbertSeries to this
void HilbertSeries::add(const vector<num_t>& num, const vector<denom_t>& gen_degrees) {
    vector<denom_t> sorted_gd(gen_degrees);
    sort(sorted_gd.begin(), sorted_gd.end());
    if (gen_degrees.size() > 0)
        assert(sorted_gd[0] > 0);  // TODO InputException?
    poly_add_to(denom_classes[sorted_gd], num);
    if (denom_classes.size() > DENOM_CLASSES_BOUND)
        collectData();
    is_simplified = false;
}

// add another HilbertSeries to this
HilbertSeries& HilbertSeries::operator+=(const HilbertSeries& other) {
    // add denom_classes
    for (auto& denom_class : other.denom_classes) {
        poly_add_to(denom_classes[denom_class.first], denom_class.second);
    }
    // add accumulated data
    vector<mpz_class> num_copy(other.num);
    performAdd(num_copy, other.denom);
    return (*this);
}

void HilbertSeries::performAdd(const vector<num_t>& numerator, const vector<denom_t>& gen_degrees) const {
    map<long, denom_t> other_denom;
    size_t i, s = gen_degrees.size();
    for (i = 0; i < s; ++i) {
        assert(gen_degrees[i] > 0);
        other_denom[gen_degrees[i]]++;
    }
    // convert numerator to mpz
    vector<mpz_class> other_num(numerator.size());
    convert(other_num, numerator);
    performAdd(other_num, other_denom);
}

// modifies other_num!!
void HilbertSeries::performAdd(vector<mpz_class>& other_num, const map<long, denom_t>& oth_denom) const {
    map<long, denom_t> other_denom(oth_denom);  // TODO redesign, dont change other_denom
    // adjust denominators
    denom_t diff;
    for (auto& it : denom) {  // augment other
        denom_t& ref = other_denom[it.first];
        diff = it.second - ref;
        if (diff > 0) {
            ref += diff;
            poly_mult_to(other_num, it.first, diff);
        }
    }
    for (auto& it : other_denom) {  // augment this
        denom_t& ref = denom[it.first];
        diff = it.second - ref;
        if (diff > 0) {
            ref += diff;
            poly_mult_to(num, it.first, diff);
        }
    }
    assert(denom == other_denom);

    // now just add the numerators
    poly_add_to(num, other_num);
    remove_zeros(num);
    is_simplified = false;
}

void HilbertSeries::collectData() const {
    if (denom_classes.empty())
        return;
    if (verbose)
        verboseOutput() << "Adding " << denom_classes.size() << " denominator classes..." << flush;
    for (auto& denom_class : denom_classes) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        performAdd(denom_class.second, denom_class.first);
    }
    denom_classes.clear();
    if (verbose)
        verboseOutput() << " done." << endl;
}

// simplify, see class description
void HilbertSeries::simplify() const {
    if (is_simplified)
        return;
    collectData();
    /*    if (verbose) {
            verboseOutput() << "Hilbert series before simplification: "<< endl << *this;
        }*/
    vector<mpz_class> q, r, poly;  // polynomials
    // In denom_cyclo we collect cyclotomic polynomials in the denominator.
    // During this method the Hilbert series is given by num/(denom*cdenom)
    // where denom | cdenom are exponent vectors of (1-t^i) | i-th cyclotminc poly.
    map<long, denom_t> cdenom;

    map<long, denom_t> save_denom = denom;
    vector<mpz_class> save_num = num;
    map<long, denom_t>::reverse_iterator rit;
    long i;
    for (rit = denom.rbegin(); rit != denom.rend(); ++rit) {
        // check if we can divide the numerator by (1-t^i)
        i = rit->first;
        denom_t& denom_i = rit->second;
        poly = coeff_vector<mpz_class>(i);
        while (denom_i > 0) {
            poly_div(q, r, num, poly);
            if (r.size() == 0) {  // numerator is divisable by poly
                num = q;
                denom_i--;
            }
            else {
                break;
            }
        }
        if (denom_i == 0)
            continue;

        // decompose (1-t^i) into cyclotomic polynomial
        for (long d = 1; d <= i / 2; ++d) {
            if (i % d == 0)
                cdenom[d] += denom_i;
        }
        cdenom[i] += denom_i;
        // the product of the cyclo. is t^i-1 = -(1-t^i)
        if (denom_i % 2 == 1)
            v_scalar_multiplication(num, mpz_class(-1));
    }  // end for
    denom.clear();

    auto it = cdenom.begin();
    while (it != cdenom.end()) {
        // check if we can divide the numerator by i-th cyclotomic polynomial

        INTERRUPT_COMPUTATION_BY_EXCEPTION

        i = it->first;
        denom_t& cyclo_i = it->second;
        poly = cyclotomicPoly<mpz_class>(i);
        while (cyclo_i > 0) {
            poly_div(q, r, num, poly);
            if (r.empty()) {  // numerator is divisable by poly
                num = q;
                cyclo_i--;
            }
            else {
                break;
            }
        }

        if (cyclo_i == 0) {
            cdenom.erase(it++);
        }
        else {
            ++it;
        }
    }
    // done with canceling
    // save this representation
    cyclo_num = num;
    cyclo_denom = cdenom;

    // now collect the cyclotomic polynomials in (1-t^i) factors
    it = cdenom.find(1);
    if (it != cdenom.end())
        dim = it->second;
    else
        dim = 0;
    period = lcm_of_keys(cdenom);
    if (period_bounded && period > 10 * PERIOD_BOUND) {
        if (verbose) {
            errorOutput() << "WARNING: Period is too big, the representation of the Hilbert series may have more than dimension "
                             "many factors in the denominator!"
                          << endl;
        }
        denom = save_denom;
        num = save_num;
    }
    else {
        while (true) {
            // create a (1-t^k) factor in the denominator out of all cyclotomic poly.

            INTERRUPT_COMPUTATION_BY_EXCEPTION

            long k = 1;
            bool empty = true;
            vector<mpz_class> existing_factor(1, 1);  // collects the existing cyclotomic gactors in the denom
            for (auto& it : cdenom) {                 // with multiplicvity 1
                if (it.second > 0) {
                    empty = false;
                    k = libnormaliz::lcm(k, it.first);
                    existing_factor = poly_mult(existing_factor, cyclotomicPoly<mpz_class>(it.first));
                    it.second--;
                }
            }
            if (empty)
                break;
            denom[k]++;
            vector<mpz_class> new_factor = coeff_vector<mpz_class>(k);
            vector<mpz_class> quotient, dummy;
            poly_div(quotient, dummy, new_factor, existing_factor);
            assert(dummy.empty());  // assert remainder r is 0
            num = poly_mult(num, quotient);
        }
    }

    /*    if (verbose) {
            verboseOutput() << "Simplified Hilbert series: " << endl << *this;
        }*/
    if (!hsop_denom.empty()) {
        compute_hsop_num();
    }
    else {
        if (denom.empty()) {  // this takes care of the exceptional case in wgich the series
            hsop_num = num;   // is a polynomial
        }
    }
    is_simplified = true;
    computeDegreeAsRationalFunction();
    quasi_poly.clear();
}

void HilbertSeries::computeDegreeAsRationalFunction() const {
    simplify();
    long num_deg = num.size() - 1 + shift;
    long denom_deg = 0;
    for (auto& it : denom) {
        denom_deg += it.first * it.second;
    }
    degree = num_deg - denom_deg;
}

long HilbertSeries::getDegreeAsRationalFunction() const {
    simplify();
    return degree;
}

long HilbertSeries::getPeriod() const {
    simplify();
    return period;
}

bool HilbertSeries::isHilbertQuasiPolynomialComputed() const {
    return is_simplified && !quasi_poly.empty();
}

void HilbertSeries::resetHilbertQuasiPolynomial() {
    quasi_poly.clear();
}

const vector<vector<mpz_class> >& HilbertSeries::getHilbertQuasiPolynomial() const {
    computeHilbertQuasiPolynomial();
    if (quasi_poly.empty())
        throw NotComputableException("HilbertQuasiPolynomial");
    return quasi_poly;
}

mpz_class HilbertSeries::getHilbertQuasiPolynomialDenom() const {
    computeHilbertQuasiPolynomial();
    if (quasi_poly.empty())
        throw NotComputableException("HilbertQuasiPolynomial");
    return quasi_denom;
}

void HilbertSeries::computeHilbertQuasiPolynomial() const {
    if (isHilbertQuasiPolynomialComputed() || nr_coeff_quasipol == 0)
        return;
    simplify();

    vector<long> denom_vec = to_vector(denom);
    if (nr_coeff_quasipol > (long)denom_vec.size()) {
        if (verbose)
            verboseOutput() << "Number of coeff of quasipol too large. Reset to deault value." << endl;
        nr_coeff_quasipol = -1;
    }

    if (period_bounded && period > PERIOD_BOUND) {
        if (verbose) {
            errorOutput() << "WARNING: We skip the computation of the Hilbert-quasi-polynomial because the period " << period
                          << " is too big!" << endl;
            errorOutput() << "Rerun with NO_PERIOD_BOUND" << endl;
        }
        return;
    }
    if (verbose && period > 1) {
        verboseOutput() << "Computing Hilbert quasipolynomial of period " << period << " ..." << flush;
    }
    long i, j;
    // period und dim encode the denominator
    // now adjust the numerator
    long num_size = num.size();
    vector<mpz_class> norm_num(num_size);  // normalized numerator
    for (i = 0; i < num_size; ++i) {
        norm_num[i] = num[i];
    }
    map<long, denom_t>::reverse_iterator rit;
    long d;
    vector<mpz_class> r;
    for (rit = denom.rbegin(); rit != denom.rend(); ++rit) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        d = rit->first;
        // nothing to do if it already has the correct t-power
        if (d != period) {
            // norm_num *= (1-t^p / 1-t^d)^denom[d]
            // first by multiply: norm_num *= (1-t^p)^denom[d]
            poly_mult_to(norm_num, period, rit->second);

            // then divide: norm_num /= (1-t^d)^denom[d]
            for (i = 0; i < rit->second; ++i) {
                poly_div(norm_num, r, norm_num, coeff_vector<mpz_class>(d));
                assert(r.empty());  // assert remainder r is 0
            }
        }
    }

    // determine the common period of the coefficients that will be computed and printed
    long reduced_period;
    if (nr_coeff_quasipol >= 0) {
        reduced_period = 1;
        for (long j = 0; j < nr_coeff_quasipol; ++j)
            reduced_period = lcm(reduced_period, denom_vec[j]);
    }
    else
        reduced_period = period;

    // cut numerator into period many pieces and apply standard method
    // we make only reduced_period many components
    quasi_poly = vector<vector<mpz_class> >(reduced_period);
    long nn_size = norm_num.size();
    for (j = 0; j < reduced_period; ++j) {
        quasi_poly[j].reserve(dim);
    }
    for (i = 0; i < nn_size; ++i) {
        if (i % period < reduced_period)
            quasi_poly[i % period].push_back(norm_num[i]);
    }

#pragma omp parallel for
    for (j = 0; j < reduced_period; ++j) {
        INTERRUPT_COMPUTATION_BY_EXCEPTION

        quasi_poly[j] = compute_polynomial(quasi_poly[j], dim);
    }

    // substitute t by t/period:
    // dividing by period^dim and multipling the coeff with powers of period
    mpz_class pp = 1;
    for (i = dim - 2; i >= 0; --i) {
        pp *= period;  // p^i   ok, it is p^(dim-1-i)
        for (j = 0; j < reduced_period; ++j) {
            quasi_poly[j][i] *= pp;
        }
    }  // at the end pp=p^dim-1
    // the common denominator for all coefficients, dim! * pp
    quasi_denom = permutations<mpz_class>(1, dim) * pp;
    // substitute t by t-j
    for (j = 0; j < reduced_period; ++j) {
        // X |--> X - (j + shift)
        linear_substitution<mpz_class>(quasi_poly[j], j + shift);  // replaces quasi_poly[j]
    }
    // divide by gcd //TODO operate directly on vector
    Matrix<mpz_class> QP(quasi_poly);
    mpz_class g = QP.matrix_gcd();
    g = libnormaliz::gcd(g, quasi_denom);
    quasi_denom /= g;
    QP.scalar_division(g);
    // we use a normed shift, so that the cylcic shift % period always yields a non-negative integer
    long normed_shift = -shift;
    while (normed_shift < 0)
        normed_shift += reduced_period;
    for (j = 0; j < reduced_period; ++j) {
        quasi_poly[j] = QP[(j + normed_shift) % reduced_period];  // QP[ (j - shift) % p ]
    }

    long delete_coeff = 0;
    if (nr_coeff_quasipol >= 0)
        delete_coeff = (long)quasi_poly[0].size() - nr_coeff_quasipol;

    for (auto& i : quasi_poly)  // delete coefficients that have not been computed completely
        for (long j = 0; j < delete_coeff; ++j)
            i[j] = 0;

    if (verbose && period > 1) {
        verboseOutput() << " done." << endl;
    }
}

// expands the series to degree to_degree
void HilbertSeries::compute_expansion() const {
    expansion.clear();
    vector<mpz_class> denom_expansion = expand_denom();
    expansion = poly_mult(num, denom_expansion);
    if ((long)expansion.size() > expansion_degree + 1)
        expansion.resize(expansion_degree + 1);
}

vector<mpz_class> HilbertSeries::getExpansion() const {
    compute_expansion();
    return expansion;
}

long HilbertSeries::get_expansion_degree() const {
    return expansion_degree;
}

void HilbertSeries::set_expansion_degree(long degree) {
    expansion_degree = degree;
}

vector<mpz_class> HilbertSeries::expand_denom() const {
    vector<long> denom_vec = to_vector(denom);
    vector<mpz_class> result(1, 1);  // the constant 1
    for (long i : denom_vec) {
        vector<mpz_class> this_factor = expand_inverse(i, expansion_degree);
        result = poly_mult(result, this_factor);
        if ((long)result.size() > expansion_degree + 1)
            result.resize(expansion_degree + 1);
    }
    return result;
}

// computes the series expansion of 1/(1-t^e)
vector<mpz_class> expand_inverse(size_t exponent, long to_degree) {
    vector<mpz_class> expansion(to_degree + 1, 0);
    for (long i = 0; i <= to_degree; i += exponent)
        expansion[i] = 1;
    return expansion;
}

// returns the numerator, repr. as vector of coefficients, the h-vector
const vector<mpz_class>& HilbertSeries::getNum() const {
    simplify();
    return num;
}
// returns the denominator, repr. as a map of the exponents of (1-t^i)^e
const map<long, denom_t>& HilbertSeries::getDenom() const {
    simplify();
    return denom;
}

// returns the numerator, repr. as vector of coefficients
const vector<mpz_class>& HilbertSeries::getCyclotomicNum() const {
    simplify();
    return cyclo_num;
}
// returns the denominator, repr. as a map of the exponents of (1-t^i)^e
const map<long, denom_t>& HilbertSeries::getCyclotomicDenom() const {
    simplify();
    return cyclo_denom;
}

const map<long, denom_t>& HilbertSeries::getHSOPDenom() const {
    simplify();
    return hsop_denom;
}

const vector<mpz_class>& HilbertSeries::getHSOPNum() const {
    simplify();
    assert(v_non_negative(hsop_num));
    return hsop_num;
}

// shift
void HilbertSeries::setShift(long s) {
    if (shift != s) {
        is_simplified = false;
        // remove quasi-poly //TODO could also be adjusted
        quasi_poly.clear();
        quasi_denom = 1;
        shift = s;
    }
}

void HilbertSeries::setHSOPDenom(vector<denom_t> new_denom) {
    hsop_denom = count_in_map<long, denom_t>(new_denom);
}

void HilbertSeries::setHSOPDenom(map<long, denom_t> new_denom) {
    hsop_denom = new_denom;
}

long HilbertSeries::getShift() const {
    return shift;
}

void HilbertSeries::adjustShift() {
    collectData();
    size_t adj = 0;  // adjust shift by
    while (adj < num.size() && num[adj] == 0)
        adj++;
    if (adj > 0) {
        shift += adj;
        num.erase(num.begin(), num.begin() + adj);
        if (cyclo_num.size() != 0) {
            assert(cyclo_num.size() >= adj);
            cyclo_num.erase(cyclo_num.begin(), cyclo_num.begin() + adj);
        }
    }
}

/*
// methods for textual transfer of a Hilbert Series
string HilbertSeries::to_string_rep() const {
    collectData();
    ostringstream s;

    s << num.size() << " ";
    s << num;
    vector<denom_t> denom_vector(to_vector(denom));
    s << denom_vector.size() << " ";
    s << denom_vector;
    return s.str();
}

void HilbertSeries::from_string_rep(const string& input) {
    istringstream s(input);
    long i, size;

    s >> size;
    num.resize(size);
    for (i = 0; i < size; ++i) {
        s >> num[i];
    }

    vector<denom_t> denom_vector;
    s >> size;
    denom_vector.resize(size);
    for (i = 0; i < size; ++i) {
        s >> denom_vector[i];
    }

    denom = count_in_map<long, denom_t>(denom_vector);
    is_simplified = false;
}

// writes in a human readable format
ostream& operator<<(ostream& out, const HilbertSeries& HS) {
    HS.collectData();
    out << "(";
    // i == 0
    if (HS.num.size() > 0)
        out << " " << HS.num[0];
    if (HS.shift != 0)
        out << "*t^" << HS.shift;
    for (size_t i = 1; i < HS.num.size(); ++i) {
        if (HS.num[i] == 1)
            out << " +t^" << i + HS.shift;
        else if (HS.num[i] == -1)
            out << " -t^" << i + HS.shift;
        else if (HS.num[i] > 0)
            out << " +" << HS.num[i] << "*t^" << i + HS.shift;
        else if (HS.num[i] < 0)
            out << " -" << -HS.num[i] << "*t^" << i + HS.shift;
    }
    out << " ) / (";
    if (HS.denom.empty()) {
        out << " 1";
    }
    for (const auto& it : HS.denom) {
        if (it.second != 0)
            out << " (1-t^" << it.first << ")^" << it.second;
    }
    out << " )" << std::endl;
    return out;
}
*/

//---------------------------------------------------------------------------
// polynomial operations, for polynomials repr. as vector of coefficients
//---------------------------------------------------------------------------

// returns the coefficient vector of 1-t^i
template <typename Integer>
vector<Integer> coeff_vector(size_t i) {
    vector<Integer> p(i + 1, 0);
    p[0] = 1;
    p[i] = -1;
    return p;
}

template <typename Integer>
void remove_zeros(vector<Integer>& a) {
    size_t i = a.size();
    while (i > 0 && a[i - 1] == 0)
        --i;

    if (i < a.size()) {
        a.resize(i);
    }
}

// a += b  (also possible to define the += op for vector)
template <typename Integer>
void poly_add_to(vector<Integer>& a, const vector<Integer>& b) {
    size_t b_size = b.size();
    if (a.size() < b_size) {
        a.resize(b_size);
    }
    for (size_t i = 0; i < b_size; ++i) {
        a[i] += b[i];
    }
    remove_zeros(a);
}

// a += b*t^m
template <typename Integer>
void poly_add_to_tm(vector<Integer>& a, const vector<Integer>& b, long m) {
    size_t b_size = b.size();
    size_t b_m = b_size + m;
    if (a.size() < b_m) {
        a.resize(b_m);
    }
    for (size_t i = 0; i < b_size; ++i) {
        a[i + m] += b[i];
    }
    remove_zeros(a);
}

// a -= b  (also possible to define the -= op for vector)
template <typename Integer>
void poly_sub_to(vector<Integer>& a, const vector<Integer>& b) {
    size_t b_size = b.size();
    if (a.size() < b_size) {
        a.resize(b_size);
    }
    for (size_t i = 0; i < b_size; ++i) {
        a[i] -= b[i];
    }
    remove_zeros(a);
}

// a *= t^m
template <typename Integer>
void poly_mult_by_tm(vector<Integer>& a, long m) {
    long a_ori_size = a.size();
    a.resize(a_ori_size + m);
    for (long i = a_ori_size - 1; i >= 0; --i)
        a[i + m] = a[i];
    for (long i = 0; i < m; ++i)
        a[i] = 0;
}

// a * b

/* template<typename Integer>
vector<Integer> old_poly_mult(const vector<Integer>& a, const vector<Integer>& b) {
    size_t a_size = a.size();
    size_t b_size = b.size();


    vector<Integer> p( a_size + b_size - 1 );
    size_t i,j;
    for (i=0; i<a_size; ++i) {
        if (a[i] == 0) continue;
        for (j=0; j<b_size; ++j) {
            if (b[j] == 0) continue;
            p[i+j] += a[i]*b[j];
        }
    }
    return p;
}*/

template <typename Integer>
vector<Integer> karatsubamult(const vector<Integer>& a, const vector<Integer>& b) {
    size_t a_size = a.size();
    size_t b_size = b.size();
    if (a_size * b_size <= 1000 || a_size <= 10 || b_size <= 10) {
        return poly_mult(a, b);
    }

    size_t m = (a_size + 1) / 2;
    if (2 * m < (b_size + 1)) {
        m = (b_size + 1) / 2;
    }

    vector<Integer> f0(m), f1(m), g0(m), g1(m);
    for (size_t i = 0; i < m && i < a_size; ++i)
        f0[i] = a[i];
    for (size_t i = m; i < a_size; ++i)
        f1[i - m] = a[i];
    for (size_t i = 0; i < m && i < b_size; ++i)
        g0[i] = b[i];
    for (size_t i = m; i < b_size; ++i)
        g1[i - m] = b[i];
    remove_zeros(f0);
    remove_zeros(f1);
    remove_zeros(g0);
    remove_zeros(g1);

    vector<Integer> sf = f0;
    vector<Integer> sg = g0;

    vector<Integer> mix;
    vector<Integer> h00;
    vector<Integer> h11;

#pragma omp parallel  // num_threads(3)
    {
#pragma omp single nowait
        {
            h00 = karatsubamult(f0, g0);  // h00 = f0 * g0
        }

#pragma omp single nowait
        {
            h11 = karatsubamult(f1, g1);  // h11 = f1 * g1
        }

#pragma omp single nowait
        {
            poly_add_to(sf, f1);          // f0+f1
            poly_add_to(sg, g1);          // g0 + g1
            mix = karatsubamult(sf, sg);  // (f0 + f1)*(g0 + g1)
        }

    }  // parallel

    f0.clear();
    g0.clear();
    f1.clear();
    g1.clear();

    poly_sub_to(mix, h00);  // mix = mix - f0*g0
    poly_sub_to(mix, h11);  // mix = mix - f1*g1

    poly_add_to_tm(h00, mix, m);
    poly_add_to_tm(h00, h11, 2 * m);

    return h00;
}

// a *= (1-t^d)^e
template <typename Integer>
void poly_mult_to(vector<Integer>& a, long d, long e) {
    assert(d > 0);
    assert(e >= 0);
    long i;
    a.reserve(a.size() + d * e);
    while (e > 0) {
        a.resize(a.size() + d);
        for (i = a.size() - 1; i >= d; --i) {
            a[i] -= a[i - d];
        }
        e--;
    }
}

//---------------------------------------------------------------------------
// computing the Hilbert polynomial from h-vector
//---------------------------------------------------------------------------

// The algorithm follows "Cohen-Macaulay rings", 4.1.5 and 4.1.9.
// The E_vector is the vector of higher multiplicities.
// It is assumed that (d-1)! is used as a common denominator in the calling routine.

template <typename Integer>
vector<Integer> compute_e_vector(vector<Integer> Q, int dim) {
    size_t j;
    int i;
    vector<Integer> E_Vector(dim, 0);
    // cout << "QQQ " << Q;
    // Q.resize(dim+1);
    int bound = Q.size();
    if (bound > dim)
        bound = dim;
    for (i = 0; i < bound; i++) {
        for (j = 0; j < Q.size() - i; j++) {
            E_Vector[i] += Q[j];
        }
        E_Vector[i] /= permutations<Integer>(1, i);
        for (j = 1; j < Q.size() - i; j++) {
            Q[j - 1] = static_cast<unsigned long>(j) * Q[j];
        }
    }
    return E_Vector;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> compute_polynomial(vector<Integer> h_vector, int dim) {
    // handle dimension 0
    if (dim == 0)
        return vector<Integer>(dim);

    vector<Integer> Hilbert_Polynomial = vector<Integer>(dim);
    int i, j;

    Integer mult_factor;
    vector<Integer> E_Vector = compute_e_vector(h_vector, dim);
    vector<Integer> C(dim, 0);
    C[0] = 1;
    for (i = 0; i < dim; i++) {
        mult_factor = permutations<Integer>(i, dim);
        if (((dim - 1 - i) % 2) == 0) {
            for (j = 0; j < dim; j++) {
                Hilbert_Polynomial[j] += mult_factor * E_Vector[dim - 1 - i] * C[j];
            }
        }
        else {
            for (j = 0; j < dim; j++) {
                Hilbert_Polynomial[j] -= mult_factor * E_Vector[dim - 1 - i] * C[j];
            }
        }
        for (j = dim - 1; 0 < j; j--) {
            C[j] = (unsigned long)(i + 1) * C[j] + C[j - 1];
        }
        C[0] = permutations<Integer>(1, i + 1);
    }

    return Hilbert_Polynomial;
}

//---------------------------------------------------------------------------

// substitutes t by (t-a), overwrites the polynomial!
template <typename Integer>
void linear_substitution(vector<Integer>& poly, const Integer& a) {
    long deg = poly.size() - 1;
    // Iterated division by (t+a)
    for (long step = 0; step < deg; ++step) {
        for (long i = deg - 1; i >= step; --i) {
            poly[i] -= a * poly[i + 1];
        }
        // the remainders are the coefficients of the transformed polynomial
    }
}

//---------------------------------------------------------------------------
IntegrationData::IntegrationData() {
}

void IntegrationData::set_nr_coeff_quasipol(long nr_coeff) {
    weighted_Ehrhart_series.first.set_nr_coeff_quasipol(nr_coeff);
}

void IntegrationData::set_expansion_degree(long degree) {
    weighted_Ehrhart_series.first.set_expansion_degree(degree);
}

string IntegrationData::getPolynomial() const {
    return polynomial;
}

long IntegrationData::getDegreeOfPolynomial() const {
    return degree_of_polynomial;
}

void IntegrationData::setDegreeOfPolynomial(const long d) {
    degree_of_polynomial = d;
}

IntegrationData::IntegrationData(const string& poly) {
    polynomial = poly;
    polynomial_is_homogeneous = false;  // to be on the safe side
}

bool IntegrationData::isWeightedEhrhartQuasiPolynomialComputed() const {
    return weighted_Ehrhart_series.first.isHilbertQuasiPolynomialComputed();
}

const vector<vector<mpz_class> >& IntegrationData::getWeightedEhrhartQuasiPolynomial() const {
    return weighted_Ehrhart_series.first.getHilbertQuasiPolynomial();
}

void IntegrationData::resetHilbertQuasiPolynomial() {
    weighted_Ehrhart_series.first.resetHilbertQuasiPolynomial();
}

vector<mpz_class> IntegrationData::getExpansion() const {
    return weighted_Ehrhart_series.first.getExpansion();
}

void IntegrationData::computeWeightedEhrhartQuasiPolynomial() {
    weighted_Ehrhart_series.first.computeHilbertQuasiPolynomial();
}

mpz_class IntegrationData::getWeightedEhrhartQuasiPolynomialDenom() const {
    return weighted_Ehrhart_series.first.getHilbertQuasiPolynomialDenom() * weighted_Ehrhart_series.second;
}

// the following 4 functions are nit used in Normaliz, bur provided for interfaces
const vector<mpz_class>& IntegrationData::getNum_ZZ() const {
    return weighted_Ehrhart_series.first.getNum();
}

const map<long, denom_t>& IntegrationData::getDenom() const {
    return weighted_Ehrhart_series.first.getDenom();
}

const vector<mpz_class>& IntegrationData::getCyclotomicNum_ZZ() const {
    return weighted_Ehrhart_series.first.getCyclotomicNum();
}

const map<long, denom_t>& IntegrationData::getCyclotomicDenom() const {
    return weighted_Ehrhart_series.first.getCyclotomicDenom();
}

const pair<HilbertSeries, mpz_class>& IntegrationData::getWeightedEhrhartSeries() const {
    return weighted_Ehrhart_series;
}

mpq_class IntegrationData::getIntegral() const {
    return integral;
}

nmz_float IntegrationData::getEuclideanIntegral() const {
    return euclidean_integral;
}

mpz_class IntegrationData::getNumeratorCommonDenom() const {
    return weighted_Ehrhart_series.second;
}

mpq_class IntegrationData::getVirtualMultiplicity() const {
    return virtual_multiplicity;
}

void IntegrationData::setIntegral(const mpq_class I) {
    integral = I;
}

void IntegrationData::setEuclideanIntegral(const nmz_float I) {
    euclidean_integral = I;
}

void IntegrationData::setVirtualMultiplicity(const mpq_class I) {
    virtual_multiplicity = I;
}

void IntegrationData::setWeightedEhrhartSeries(const pair<HilbertSeries, mpz_class>& E) {
    weighted_Ehrhart_series = E;
    weighted_Ehrhart_series.first.adjustShift();
}

void IntegrationData::setHomogeneity(const bool hom) {
    polynomial_is_homogeneous = hom;
}

bool IntegrationData::isPolynomialHomogeneous() const {
    return polynomial_is_homogeneous;
}

}  // end namespace libnormaliz

#ifdef NMZ_MIC_OFFLOAD
#pragma offload_attribute(pop)
#endif
