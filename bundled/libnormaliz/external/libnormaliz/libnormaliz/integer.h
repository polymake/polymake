/*
 * Normaliz
 * Copyright (C) 2007-2019  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

#ifndef LIBNORMALIZ_INTEGER_H_
#define LIBNORMALIZ_INTEGER_H_

#include <list>
#include <vector>
#include <string>
#include <limits.h>

#include <libnormaliz/general.h>

// Integer should (may) support:
// Integer abs(Integer); here implemented as Iabs
// Integer min(Integer, Integer); here we use the template min in <algorithm>
// It provides abs, gcd and lcm
//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------
//                     Basic functions
//---------------------------------------------------------------------------

// returns the absolute value of a
template <typename Integer>
inline Integer Iabs(const Integer& a) {
    return (a >= 0) ? (a) : Integer(-a);
}

// returns gcd of a and b,   if one is 0 returns the other integer
template <typename Integer>
Integer gcd(const Integer& a, const Integer& b);
template <>
mpz_class gcd<mpz_class>(const mpz_class& a, const mpz_class& b);

// returns lcm of a and b,   returns 0 if one is 0
template <typename Integer>
Integer lcm(const Integer& a, const Integer& b);
template <>
mpz_class lcm(const mpz_class& a, const mpz_class& b);

// integer division a/b. Returns quot and rem = minimal remainder <= |b|/2
template <typename Integer>
void minimal_remainder(const Integer& a, const Integer& b, Integer& quot, Integer& rem);

// extended Euclidean algorithm: d=ua+vb
template <typename Integer>
Integer ext_gcd(const Integer& a, const Integer& b, Integer& u, Integer& v);

// minimizes u and v and makes d >= 0.
template <typename Integer>
void sign_adjust_and_minimize(const Integer& a, const Integer& b, Integer& d, Integer& u, Integer& v);

// the following functions behave like the C++ floating point functions with the same name
mpz_class floor(const mpq_class& q);
mpz_class ceil(const mpq_class& q);
mpz_class round(const mpq_class& q);

//---------------------------------------------------------------------------
//                     Conversions and checks
//---------------------------------------------------------------------------

// convert val to ret
// does the conversion and returns false if it fails
bool try_convert(long& ret, const long long& val);
inline bool try_convert(long long& ret, const long& val) {
    ret = val;
    return true;
}
bool try_convert(long& ret, const mpz_class& val);
bool try_convert(long long& ret, const mpz_class& val);
inline bool try_convert(mpz_class& ret, const long& val) {
    ret = val;
    return true;
}
bool try_convert(mpz_class& ret, const long long& val);

bool try_convert(nmz_float& ret, const long& val);
bool try_convert(nmz_float& ret, const long long& val);
bool try_convert(nmz_float& ret, const mpz_class& val);

bool try_convert(long& ret, const nmz_float& val);
bool try_convert(long long& ret, const nmz_float& val);
bool try_convert(mpz_class& ret, const nmz_float& val);

nmz_float mpq_to_nmz_float(const mpq_class& val);

#ifdef ENFNORMALIZ
bool try_convert(renf_elem_class& ret, const mpz_class& val);
bool try_convert(mpz_class& ret, const renf_elem_class& val);
bool try_convert(renf_elem_class& ret, const long long& val);
bool try_convert(long long& ret, const renf_elem_class& val);
bool try_convert(renf_elem_class& ret, const long& val);
bool try_convert(long& ret, const renf_elem_class& val);
bool try_convert(mpq_class& ret, const renf_elem_class& val);
bool try_convert(nmz_float& ret, const renf_elem_class& val);
#endif

// template for same type "conversion"
template <typename Type>
inline bool try_convert(Type& ret, const Type& val) {
    ret = val;
    return true;
}

inline bool try_convert(nmz_float& ret, const nmz_float& val) {
    ret = val;
    return true;
}

bool fits_long_range(long long a);

//--------------------------------------------------------------------
template <typename Integer>
inline bool using_GMP() {
    return false;
}

template <>
inline bool using_GMP<mpz_class>() {
    return true;
}

template <>
inline bool using_GMP<mpq_class>() {
    return true;
}

template <typename Integer>
inline bool using_mpq_class() {
    return false;
}

template <>
inline bool using_mpq_class<mpq_class>() {
    return true;
}
//--------------------------------------------------------------------

template <typename Integer>
inline bool using_float() {
    return false;
}

template <>
inline bool using_float<nmz_float>() {
    return true;
}

//--------------------------------------------------------------------

template <typename Number>
inline bool using_renf() {
    return false;
}

#ifdef ENFNORMALIZ
template <>
inline bool using_renf<renf_elem_class>() {
    return true;
}
#endif

//--------------------------------------------------------------------

// for the interpretation of a string as a decimal fraction or floating point number
mpq_class dec_fraction_to_mpq(string s);

//--------------------------------------------------------------------

template <typename Integer>
Integer int_max_value_dual();

template <typename Integer>
Integer int_max_value_primary();

//---------------------------------------------------------------------------

/*template<typename Integer>
inline bool is_scalar_zero(const Integer& m){
    return m==0;
}

template<>
inline bool is_scalar_zero<nmz_float>(const nmz_float& m){
    return (Iabs(m) < 1000000.0*nmz_epsilon);
}*/

template <typename Integer>
inline bool check_range(const Integer& m) {
    const Integer max_primary = int_max_value_primary<Integer>();
    return (Iabs(m) <= max_primary);
}

template <>
inline bool check_range<mpz_class>(const mpz_class& m) {
    return true;
}

template <>
inline bool check_range<nmz_float>(const nmz_float& m) {
    return true;
}

template <>
inline bool check_range<mpq_class>(const mpq_class& m) {
    return true;
}

#ifdef ENFNORMALIZ
template <>
inline bool check_range<renf_elem_class>(const renf_elem_class& m) {
    return true;
}
#endif
//---------------------------------------------------------------------------

template <typename Integer>
void check_range_list(const std::list<std::vector<Integer> >& ll);

template <typename Integer>
class CandidateList;
template <typename Integer>
class Candidate;

template <typename Integer>
void check_range_list(const CandidateList<Integer>& ll);
template <typename Integer>
void check_range_list(const std::list<Candidate<Integer> >& ll);

//---------------------------------------------------------------------------
//                     String conversion functions
//---------------------------------------------------------------------------

// forward declaration to silence clang error:
// 'operator<<' should be declared prior to the call site or in the global namespace
template <typename T>
std::ostream& operator<<(std::ostream& out, const vector<T>& vec);

template <typename Integer>
string toString(Integer a) {
    ostringstream ostream;
    ostream << a;
    return ostream.str();
}
template <>
inline string toString(mpz_class a) {
    return a.get_str();
}
template <>
inline string toString(mpq_class a) {
    return a.get_str();
}

//----------------------------------------------------------------------
// the next functions produce an integer quotient of absolute values and determine whether
// there is a remainder

bool int_quotient(long long& Quot, const mpz_class& Num, const mpz_class& Den);
bool int_quotient(long& Quot, const long& Num, const long& Den);
bool int_quotient(long long& Quot, const long long& Num, const long long& Den);
bool int_quotient(mpz_class& Quot, const mpz_class& Num, const mpz_class& Den);
template <typename IntegerRet>
bool int_quotient(IntegerRet& Quot, const nmz_float& Num, const nmz_float& Den);

// find the floor and ceol of Num/Den
template <typename IntegerRet, typename IntegerVal>
IntegerRet floor_quot(const IntegerVal Num, IntegerVal Den);

template <typename IntegerRet, typename IntegerVal>
IntegerRet ceil_quot(const IntegerVal Num, IntegerVal Den);

//---------------------------------------------------------------------------
template <typename Integer>
void minimal_remainder(const Integer& a, const Integer& b, Integer& quot, Integer& rem) {
    quot = a / b;
    rem = a - quot * b;
    if (rem == 0)
        return;
    Integer test = 2 * Iabs(rem) - Iabs(b);
    if (test > 0) {
        if ((rem < 0 && b > 0) || (rem > 0 && b < 0)) {
            rem += b;
            quot--;
        }
        else {
            rem -= b;
            quot++;
        }
    }
    if (test == 0 && rem < 0) {
        rem = -rem;
        if (b > 0)
            quot--;
        else
            quot++;
    }
}

template <typename Integer>
void sign_adjust_and_minimize(const Integer& a, const Integer& b, Integer& d, Integer& u, Integer& v) {
    if (d < 0) {
        d = -d;
        u = -u;
        v = -v;
    }
    // cout << u << " " << v << endl;
    if (b == 0)
        return;

    Integer sign = 1;
    if (a < 0)
        sign = -1;
    Integer u1 = (sign * u) % (Iabs(b) / d);
    if (u1 == 0)
        u1 += Iabs(b) / d;
    u = sign * u1;
    v = (d - a * u) / b;
}

template <typename Integer>
Integer ext_gcd(const Integer& a, const Integer& b, Integer& u, Integer& v) {
    u = 1;
    v = 0;
    Integer d = a;
    if (b == 0) {
        sign_adjust_and_minimize(a, b, d, u, v);
        return (d);
    }
    Integer v1 = 0;
    Integer v3 = b;
    Integer q, t1, t3;
    while (v3 != 0) {
        q = d / v3;
        t3 = d - q * v3;
        t1 = u - q * v1;
        u = v1;
        d = v3;
        v1 = t1;
        v3 = t3;
    }
    sign_adjust_and_minimize(a, b, d, u, v);
    return (d);
}

template <typename Integer>
size_t decimal_length(Integer a) {
    ostringstream test;
    test << a;
    return test.str().size();
}

//---------------------------------------------------------------------------
//                     Special functions
//---------------------------------------------------------------------------

// return the number of decimals, needed to write the Integer a
template <typename Integer>
size_t decimal_length(Integer a);

template <typename Integer>
mpz_class nmz_factorial(Integer n);

//---------------------------------------------------------------------------
//                     Input
//---------------------------------------------------------------------------

inline mpq_class mpq_read(istream& in) {
    const string numeric = "+-0123456789/.e";
    in >> std::ws;
    string s;
    char c;
    bool is_float = false;
    while (in.good()) {
        c = in.peek();
        size_t pos = numeric.find(c);
        if (pos == string::npos)
            break;
        if (pos > 12)
            is_float = true;
        in >> c;
        s += c;
    }

    if (s == "") {
        string t;
        t += c;
        throw BadInputException("Empty number string preceding character " + t +
                                ". Most likely mismatch of amb_space and matrix format or forgotten keyword.");
    }

    // cout << "t " << s << " f " << is_float << endl;

    if (s[0] == '+')
        s = s.substr(1);  // must suppress + sign for mpq_class

    try {
        if (!is_float) {
            return mpq_class(s);
        }
        else
            return dec_fraction_to_mpq(s);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        throw BadInputException("Illegal number string " + s + " in input, Exiting.");
    }
}

inline void string2coeff(mpq_class& coeff, istream& in, const string& s) {  // in here superfluous parameter
    
    stringstream sin(s);
    coeff = mpq_read(sin);
    // coeff=mpq_class(s);
}

inline void read_number(istream& in, mpq_class& number) {
    number = mpq_read(in);
}

inline void read_number(istream& in, long& number) {
    in >> number;
}

inline void read_number(istream& in, long long& number) {
    in >> number;
}

inline void read_number(istream& in, nmz_float& number) {
    in >> number;
}

inline void read_number(istream& in, mpz_class& number) {
    in >> number;
}

#ifdef ENFNORMALIZ

inline void string2coeff(renf_elem_class& coeff, istream& in, const string& s) {  // we need in to access the renf

    try {
        renf_class* K = (renf_class*)in.pword(renf_class::xalloc());
        coeff = renf_elem_class(*K, s);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
        throw BadInputException("Illegal number string " + s + " in input, Exiting.");
    }
}

inline void read_number(istream& in, renf_elem_class& number) {
    // in >> number;

    char c;

    in >> ws;
    c = in.peek();
    if (c != '(') {  // rational number
        mpq_class rat = mpq_read(in);
        number = renf_elem_class(rat);
        return;
    }

    // now we have a proper field contains

    in >> c;  // read (

    string num_string;
    bool skip = false;
    while (in.good()) {
        c = in.peek();
        if (c == ')') {
            in >> c;
            break;
        }
        if (c == '~' || c == '=' || c == '[')  // skip the approximation
            skip = true;
        in.get(c);
        if (in.fail())
            throw BadInputException("Error in reading number: field element not terminated by )");
        if (!skip)
            num_string += c;
    }
    string2coeff(number, in, num_string);
}
#endif

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif /* INTEGER_H_ */
//---------------------------------------------------------------------------
