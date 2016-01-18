/*
 * Normaliz
 * Copyright (C) 2007-2014  Winfried Bruns, Bogdan Ichim, Christof Soeger
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

/*
 * HilbertSeries represents a Hilbert series of a (ZZ_+)-graded algebra
 * with generators of different degrees.
 * It is represented as a polynomial divided by a product of (1-t^i).
 * The numerator is represented as vector of coefficients, the h-vector
 * h vector repr.: sum of h[i]*t^i
 * and the denominator is represented as a map d of the exponents of (1-t^i)
 * d vector repr.: product of (1-t^i)^d[i] over i in d
 * Input of the denominator is also possible as a vector of degrees of the 
 * generators.
 *
 * The class offers basic operations on the series, a simplification and
 * different forms of representation of the series.
 *
 * Furthermore this file include operations for the polynomials used.
 */

//---------------------------------------------------------------------------

#ifndef HILBERT_SERIES_H
#define HILBERT_SERIES_H

//---------------------------------------------------------------------------

#include <vector>
#include <map>
#include <ostream>
#include <string>

#include <libnormaliz/general.h>

//---------------------------------------------------------------------------

namespace libnormaliz {
using std::vector;
using std::map;
using std::ostream;
using std::string;

class HilbertSeries;

// write a readable representation to the stream
ostream& operator<< (ostream& out, const HilbertSeries& HS);

typedef long long num_t;    //integer type for numerator
typedef long denom_t;       //integer type for denominator

class HilbertSeries {

public:
    // Constructor, creates 0/1
    HilbertSeries();
    // Constructor, creates num/denom, see class description for format
    HilbertSeries(const vector<num_t>& num, const vector<denom_t>& gen_degrees);
    // Constructor, creates num/denom, see class description for format
    HilbertSeries(const vector<mpz_class>& num, const map<long, denom_t>& denom);
    // Constructor, string as created by to_string_rep
    HilbertSeries(const string& str);

    // resets to 0/1
    void reset();
 
    // add another HilbertSeries to this
    HilbertSeries& operator+=(const HilbertSeries& other);

    // add another HilbertSeries to this
    void add(const vector<num_t>& num, const vector<denom_t>& gen_degrees);

    // simplify, see class description
    // it changes the representation of the series, but not the series itself
    // therefore it is declared const
    void simplify() const;
    // collect data from the denom_classes
    void collectData() const;

    // returns the numerator, repr. as vector of coefficients, the h-vector
    const vector<mpz_class>& getNum() const;
    // returns the denominator, repr. as a map of the exponents of (1-t^i)^e
    const map<long, denom_t>& getDenom() const;

    // returns the numerator, repr. as vector of coefficients
    const vector<mpz_class>& getCyclotomicNum() const;
    // returns the denominator, repr. as a map of the exponents of the cyclotomic polynomials
    const map<long, denom_t>& getCyclotomicDenom() const;

    long getDegreeAsRationalFunction() const;

    long getPeriod() const;

    void computeHilbertQuasiPolynomial() const;
    long isHilbertQuasiPolynomialComputed() const;
    vector< vector<mpz_class> > getHilbertQuasiPolynomial() const;
    mpz_class getHilbertQuasiPolynomialDenom() const;

    // setting the shift will not change the numerator directly, only its interpretation
    // the shift will be considered in the computation of the (quasi) polynomial
    void setShift(long s);
    // adjust the shift so that the series starts in degree 0
    // it does not change the (quasi) polynomial
    void adjustShift();
    // returns the shift of the Hilbert series, that is the lowest degree of an element
    long getShift() const;

    // methods for textual transfer of a Hilbert Series
    string to_string_rep() const;
    void from_string_rep(const string&);

    void setVerbose(bool v) { verbose = v; }

private:
    // collected data in denominator classes
    mutable map< vector<denom_t>, vector<num_t> > denom_classes;
    // add the classes if they get too many
    static const size_t DENOM_CLASSES_BOUND = 50000;

    // the numerator, repr. as vector of coefficients, the h-vector
    mutable vector<mpz_class> num;
    // the denominator, repr. as a map of the exponents of (1-t^i)^e
    mutable map<long, denom_t> denom;

    // the numerator, repr. as vector of coefficients
    mutable vector<mpz_class> cyclo_num;
    // the denominator, repr. as a map of the exponents of the cyclotomic polynomials
    mutable map<long, denom_t> cyclo_denom;

    mutable bool is_simplified;
    mutable long dim;
    mutable long period;
    mutable long degree; // as rational function
    long shift;
    // the quasi polynomial, can have big coefficients
    mutable vector< vector<mpz_class> > quasi_poly;
    mutable mpz_class quasi_denom;

    bool verbose;

    // these are only const when used properly!!
    void performAdd(const vector<num_t>& num, const vector<denom_t>& gen_degrees) const;
    void performAdd(vector<mpz_class>& num, const map<long, denom_t>& denom) const;

    void computeDegreeAsRationalFunction() const;

    friend ostream& operator<< (ostream& out, const HilbertSeries& HS);

};
//class end *****************************************************************


//---------------------------------------------------------------------------
// polynomial operations, for polynomials repr. as vector of coefficients
//---------------------------------------------------------------------------

// a += b
template<typename Integer>
void poly_add_to (vector<Integer>& a, const vector<Integer>& b);

// a -= b
template<typename Integer>
void poly_sub_to (vector<Integer>& a, const vector<Integer>& b);


// a * b
template<typename Integer>
vector<Integer> poly_mult(const vector<Integer>& a, const vector<Integer>& b);

// a *= (1-t^d)^e
template<typename Integer>
void poly_mult_to(vector<Integer>& a, long d, long e = 1);


// division with remainder, a = q * b + r
template<typename Integer>
void poly_div(vector<Integer>& q, vector<Integer>& r, const vector<Integer>& a, const vector<Integer>&b);


// remove leading zero coefficients, 0 polynomial leads to empty list
template<typename Integer>
void remove_zeros(vector<Integer>& a);


// Returns the n-th cyclotomic polynomial, all smaller are computed and stored.
// The n-th cyclotomic polynomial is the product of (X-w) over all 
// n-th primitive roots of unity w.
template<typename Integer>
vector<Integer> cyclotomicPoly(long n);

// returns the coefficient vector of 1-t^i
template<typename Integer>
vector<Integer> coeff_vector(size_t i);

// substitutes t by (t-a), overwrites the polynomial!
template<typename Integer>
void linear_substitution(vector<Integer>& poly, const Integer& a);

//---------------------------------------------------------------------------
// computing the Hilbert polynomial from h-vector
//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> compute_e_vector(vector<Integer> h_vector, int dim);

template<typename Integer>
vector<Integer> compute_polynomial(vector<Integer> h_vector, int dim);

} //end namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

