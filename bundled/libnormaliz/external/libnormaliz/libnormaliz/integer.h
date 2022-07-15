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

#ifndef LIBNORMALIZ_INTEGER_H_
#define LIBNORMALIZ_INTEGER_H_

#include <list>
#include <vector>
#include <string>
#include <climits>
#include <cmath>
#include <iosfwd>

#include <libnormaliz/general.h>

// Integer should (may) support:
// Integer abs(Integer); here implemented as Iabs
// Integer min(Integer, Integer); here we use the template min in <algorithm>
// It provides abs, gcd and lcm
//---------------------------------------------------------------------------

namespace libnormaliz {

using std::cerr;
using std::endl;
using std::istream;
using std::ostream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::vector;
using std::ws;

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
mpq_class dec_fraction_to_mpq(std::string s);

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
inline bool check_range<mpz_class>(const mpz_class&) {
    return true;
}

template <>
inline bool check_range<nmz_float>(const nmz_float&) {
    return true;
}

template <>
inline bool check_range<mpq_class>(const mpq_class&) {
    return true;
}

#ifdef ENFNORMALIZ
template <>
inline bool check_range<renf_elem_class>(const renf_elem_class&) {
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

// formerly convert.h
// conversion for integers, throws ArithmeticException if conversion fails
template <typename ToType, typename FromType>
inline void convert(ToType& ret, const FromType& val) {
    if (!try_convert(ret, val)) {
        throw ArithmeticException(val);
    }
}

// conversion of vectors
template <typename ToType, typename FromType>
inline void convert(vector<ToType>& ret_vect, const vector<FromType>& from_vect) {
    size_t s = from_vect.size();
    ret_vect.resize(s);
    for (size_t i = 0; i < s; ++i)
        convert(ret_vect[i], from_vect[i]);
}

// general conversion with return, throws ArithmeticException if conversion fails
template <typename ToType, typename FromType>
ToType convertTo(const FromType& val) {
    ToType copy;
    convert(copy, val);
    return copy;
}

inline bool try_convert(mpz_class& ret, const mpq_class&) {
    assert(false);  // must never be used
    return false;
}

inline bool try_convert(mpq_class& ret, const mpz_class&) {
    assert(false);  // must never be used
    return false;
}

#ifdef ENFNORMALIZ
inline bool try_convert(renf_elem_class& ret, const mpz_class& val) {
    ret = val;
    return true;
}

inline bool try_convert(mpz_class& ret, const renf_elem_class& val) {
    renf_elem_class help = val;
    if (!help.is_integer())
        throw ArithmeticException("field element cannot be converted to integer");
    ret = help.num();
    return true;
}

inline bool try_convert(renf_elem_class& ret, const long long& val) {
    ret = convertTo<long>(val);
    return true;
}

inline bool try_convert(long long& ret, const renf_elem_class& val) {
    mpz_class bridge;
    try_convert(bridge, val);
    return try_convert(ret, bridge);
}

inline bool try_convert(renf_elem_class& ret, const long& val) {
    ret = val;
    return true;
}

inline bool try_convert(long& ret, const renf_elem_class& val) {
    mpz_class bridge;
    try_convert(bridge, val);
    return try_convert(ret, bridge);
}

inline bool try_convert(mpq_class& ret, const renf_elem_class& val) {
    nmz_float ret_double = static_cast<double>(val);
    ret = mpq_class(ret_double);
    return true;
}

inline bool try_convert(nmz_float& ret, const renf_elem_class& val) {
    ret = static_cast<double>(val);
    return true;
}
#endif

inline bool try_convert(long& ret, const long long& val) {
    if (fits_long_range(val)) {
        ret = val;
        return true;
    }
    return false;
}

inline bool try_convert(long& ret, const mpz_class& val) {
    if (!val.fits_slong_p()) {
        return false;
    }
    ret = val.get_si();
    return true;
}

inline bool try_convert(long long& ret, const mpz_class& val) {
    if (val.fits_slong_p()) {
        ret = val.get_si();
        return true;
    }
    if (sizeof(long long) == sizeof(long)) {
        return false;
    }
    mpz_class quot;
    ret = mpz_fdiv_q_ui(quot.get_mpz_t(), val.get_mpz_t(), LONG_MAX);  // returns remainder
    if (!quot.fits_slong_p()) {
        return false;
    }
    ret += ((long long)quot.get_si()) * ((long long)LONG_MAX);
    return true;
}

inline bool try_convert(mpz_class& ret, const long long& val) {
    if (fits_long_range(val)) {
        ret = mpz_class(long(val));
    }
    else {
        ret = mpz_class(long(val % LONG_MAX)) + mpz_class(LONG_MAX) * mpz_class(long(val / LONG_MAX));
    }
    return true;
}

inline bool try_convert(float& ret, const mpz_class& val) {
    if (!val.fits_slong_p())
        return false;
    long dummy = convertTo<long>(val);
    ret = (float)dummy;
    return true;
}

inline bool fits_long_range(long long a) {
    return sizeof(long long) == sizeof(long) || (a <= LONG_MAX && a >= LONG_MIN);
}

inline bool try_convert(nmz_float& ret, const long& val) {
    ret = (nmz_float)val;
    return true;
}

inline bool try_convert(nmz_float& ret, const mpz_class& val) {
    ret = val.get_d();
    return true;
}

inline bool try_convert(mpz_class& ret, const nmz_float& val) {
    ret = mpz_class(val);
    return true;
}

inline bool try_convert(nmz_float& ret, const long long& val) {
    ret = (nmz_float)val;
    return true;
}

inline bool try_convert(long& ret, const nmz_float& val) {
    mpz_class bridge;
    if (!try_convert(bridge, val))
        return false;
    return try_convert(ret, bridge);
}

inline bool try_convert(long long& ret, const nmz_float& val) {
    mpz_class bridge;
    if (!try_convert(bridge, val))
        return false;
    return try_convert(ret, bridge);
}
//---------------------------------------------------------------------------

template <typename Integer>
Integer gcd(const Integer& a, const Integer& b) {
    if (a == 0) {
        return Iabs<Integer>(b);
    }
    if (b == 0) {
        return Iabs<Integer>(a);
    }
    Integer q0, q1, r;
    q0 = Iabs<Integer>(a);
    r = Iabs<Integer>(b);
    do {
        q1 = r;
        r = q0 % q1;
        q0 = q1;
    } while (r != 0);
    return q1;
}

template <>
inline nmz_float gcd(const nmz_float& a, const nmz_float& b) {
    if (a == 0 && b == 0)
        return 0;
    return 1.0;
}

template <>
inline mpz_class gcd(const mpz_class& a, const mpz_class& b) {
    mpz_class g;
    mpz_gcd(g.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
    return g;
}

#ifdef ENFNORMALIZ
template <>
inline renf_elem_class gcd(const renf_elem_class& a, const renf_elem_class& b) {
    if (a == 0 && b == 0)
        return 0;
    return 1;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
Integer lcm(const Integer& a, const Integer& b) {
    if ((a == 0) || (b == 0)) {
        return 0;
    }
    else
        return Iabs<Integer>(a * b / gcd<Integer>(a, b));
}

template <>
inline mpz_class lcm<mpz_class>(const mpz_class& a, const mpz_class& b) {
    mpz_class g;
    mpz_lcm(g.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
    return g;
}

#ifdef ENFNORMALIZ
template <>
inline renf_elem_class lcm<renf_elem_class>(const renf_elem_class& a, const renf_elem_class& b) {
    return 1;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
Integer int_max_value_dual() {
    Integer k = sizeof(Integer) * 8 - 2;  // number of bytes convetred to number of bits
    Integer test = 1;
    test = test << k;  // 2^k
    return test;
}

// bool int_max_value_dual_long_computed = false;

template <>
inline long int_max_value_dual() {
    static long max_value;

    if (int_max_value_dual_long_computed)
        return max_value;

    long k = sizeof(long) * 8 - 2;  // number of bytes convetred to number of bits
    long test = 1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    max_value = test;
    int_max_value_dual_long_computed = true;
    return test;
}

// bool int_max_value_dual_long_long_computed = false;

template <>
inline long long int_max_value_dual() {
    static long long max_value;

    if (int_max_value_dual_long_long_computed)
        return max_value;

    long long k = sizeof(long long) * 8 - 2;  // number of bytes convetred to number of bits
    long long test = 1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    max_value = test;
    int_max_value_dual_long_long_computed = true;
    return test;
}

//---------------------------------------------------------------------------

template <>
inline mpz_class int_max_value_dual<mpz_class>() {
    assert(false);
    return 0;
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer int_max_value_primary() {
    Integer k = sizeof(Integer) * 8 - 12;  // number of bytes convetred to number of bits
    Integer test = 1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    return test;
}

// bool int_max_value_primary_long_computed = false;

template <>
inline long int_max_value_primary() {
    static long max_value;

    if (int_max_value_primary_long_computed)
        return max_value;

    long k = sizeof(long) * 8 - 12;  // number of bytes convetred to number of bits
    long test = 1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    int_max_value_primary_long_computed = true;
    max_value = test;
    return test;
}

// bool int_max_value_primary_long_long_computed = false;

template <>
inline long long int_max_value_primary() {
    static long long max_value;

    if (int_max_value_primary_long_long_computed)
        return max_value;

    long long k = sizeof(long long) * 8 - 12;  // number of bytes convetred to number of bits
    long long test = 1;
    test = test << k;  // 2^k
#ifdef NMZ_EXTENDED_TESTS
    if (test_linear_algebra_GMP)
        test = 0;
#endif
    max_value = test;
    int_max_value_primary_long_long_computed = true;
    return test;
}

//---------------------------------------------------------------------------

template <>
inline mpz_class int_max_value_primary<mpz_class>() {
    assert(false);
    return 0;
}

#ifdef ENFNORMALIZ
template <>
inline renf_elem_class int_max_value_primary<renf_elem_class>() {
    assert(false);
    return 0;
}
#endif

//---------------------------------------------------------------------------

template <typename Integer>
void check_range_list(const CandidateList<Integer>& ll) {
    check_range_list(ll.Candidates);
}

//---------------------------------------------------------------------------

template <typename Integer>
void check_range_list(const std::list<Candidate<Integer> >& ll) {
    if (using_GMP<Integer>())
        return;

    Integer test = int_max_value_dual<Integer>();
    // cout << "test " << test << endl;

    for (const auto& v : ll) {
        for (size_t i = 0; i < v.values.size(); ++i)
            if (Iabs(v.values[i]) >= test) {
                // cout << v;
                // cout << "i " << i << " " << Iabs(v[i]) << endl;
                throw ArithmeticException("Vector entry out of range. Imminent danger of arithmetic overflow.");
            }
    }
}

//---------------------------------------------------------------------------

inline mpq_class dec_fraction_to_mpq(string s) {
    size_t skip = 0;  // skip leading spaces
    for (; skip < s.size(); ++skip) {
        if (!isspace(s[skip]))
            break;
    }
    s = s.substr(skip);

    mpz_class sign = 1;
    if (s[0] == '+')
        s = s.substr(1);
    else if (s[0] == '-') {
        s = s.substr(1);
        sign = -1;
    }

    if (s[0] == '+' || s[0] == '-')
        throw BadInputException("Error in decimal fraction " + s);

    string int_string, frac_string, exp_string;
    size_t frac_part_length = 0;
    size_t pos_point = s.find(".");
    size_t pos_E = s.find("e");
    if (pos_point != string::npos) {
        int_string = s.substr(0, pos_point);
        if (pos_E != string::npos) {
            frac_part_length = pos_E - (pos_point + 1);
        }
        else
            frac_part_length = s.size() - (pos_point + 1);
        frac_string = s.substr(pos_point + 1, frac_part_length);
        if (frac_string[0] == '+' || frac_string[0] == '-')
            throw BadInputException("Error in decimal fraction " + s);
    }
    else
        int_string = s.substr(0, pos_E);
    if (pos_E != string::npos)
        exp_string = s.substr(pos_E + 1, s.size() - (pos_E + 1));

    /* cout << "int  " << int_string << endl;
    cout << "frac " << frac_string << endl;
    cout << "exp  " << exp_string << endl; */

    // remove leading 0 and +
    if (int_string.size() > 0 && int_string[0] == '+')
        int_string = int_string.substr(1);
    while (int_string.size() > 0 && int_string[0] == '0')
        int_string = int_string.substr(1);
    while (frac_string.size() > 0 && frac_string[0] == '0')
        frac_string = frac_string.substr(1);
    if (exp_string.size() > 0 && exp_string[0] == '+')
        exp_string = exp_string.substr(1);
    bool exponent_could_be_zero = false;
    while (exp_string.size() > 0 && exp_string[0] == '0') {
        exponent_could_be_zero = true;
        exp_string = exp_string.substr(1);
    }

    if (pos_E != string::npos && exp_string == "" && !exponent_could_be_zero)
        throw BadInputException("No exponent following character e in floating point number");

    mpq_class int_part, frac_part, exp_part;
    if (!int_string.empty())
        int_part = mpz_class(int_string);

    if (pos_E == 0)
        int_part = 1;

    // cout << "int_part " << int_part << endl;

    mpz_class den = 1;
    if (!frac_string.empty()) {
        frac_part = mpz_class(frac_string);
        for (size_t i = 0; i < frac_part_length; ++i)
            den *= 10;
    }
    // cout << "frac_part " << frac_part << endl;
    mpq_class result = int_part;
    if (frac_part != 0)
        result += frac_part / den;
    if (!exp_string.empty()) {
        mpz_class expo(exp_string);  // we take mpz_class because it has better error checking
        // long expo=stol(exp_string);
        mpz_class abs_expo = Iabs(expo);
        mpz_class factor = 1;
        for (long i = 0; i < abs_expo; ++i)
            factor *= 10;
        if (expo >= 0)
            result *= factor;
        else
            result /= factor;
    }
    /* cout <<" result " << sign*result << endl;
    cout << "==========" << endl; */
    return sign * result;
}

//----------------------------------------------------------------------
// the next function produce an integer quotient and determine whether
// there is a remainder

inline bool int_quotient(long& Quot, const long& Num, const long& Den) {
    Quot = Iabs(Num) / Iabs(Den);
    return Quot * Iabs(Den) != Iabs(Num);
}

inline bool int_quotient(long long& Quot, const long long& Num, const long long& Den) {
    Quot = Iabs(Num) / Iabs(Den);
    return Quot * Iabs(Den) != Iabs(Num);
}

inline bool int_quotient(mpz_class& Quot, const mpz_class& Num, const mpz_class& Den) {
    Quot = Iabs(Num) / Iabs(Den);
    return Quot * Iabs(Den) != Iabs(Num);
}

inline bool int_quotient(long long& Quot, const mpz_class& Num, const mpz_class& Den) {
    mpz_class mpz_Quot = (Iabs(Num) / Iabs(Den));
    convert(Quot, mpz_Quot);
    return mpz_Quot * Iabs(Den) != Iabs(Num);
}

template <typename IntegerRet>
inline bool int_quotient(IntegerRet& Quot, const nmz_float& Num, const nmz_float& Den) {
    nmz_float FloatQuot = Iabs(Num) / Iabs(Den);         // cout << "FF " << FloatQuot << endl;
    nmz_float IntQuot = trunc(FloatQuot + nmz_epsilon);  // cout << "II " << IntQuot << endl;
    Quot = convertTo<IntegerRet>(IntQuot);               // cout << "QQ " <<  Quot << endl;
    return FloatQuot - IntQuot > nmz_epsilon;
}

template <typename IntegerRet, typename IntegerVal>
IntegerRet floor_quot(const IntegerVal Num, IntegerVal Den) {
    IntegerRet Quot;
    bool frac = int_quotient(Quot, Num, Den);
    if ((Num >= 0 && Den >= 0) || (Num < 0 && Den < 0)) {
        return Quot;
    }
    else {
        if (frac) {
            return -Quot - 1;
        }
        return -Quot;
    }
}

template <typename IntegerRet, typename IntegerVal>
IntegerRet ceil_quot(const IntegerVal Num, IntegerVal Den) {
    IntegerRet Quot;
    bool frac = int_quotient(Quot, Num, Den);
    if ((Num >= 0 && Den >= 0) || (Num < 0 && Den < 0)) {
        if (frac)
            return Quot + 1;
        return Quot;
    }
    else {
        return -Quot;
    }
}

#ifdef ENFNORMALIZ
template <>
inline mpz_class floor_quot(const renf_elem_class Num, renf_elem_class Den) {
    return floor(Num / Den);
}

template <>
inline mpz_class ceil_quot(const renf_elem_class Num, renf_elem_class Den) {
    return ceil(Num / Den);
}
#endif

//----------------------------------------------------------------------

inline mpz_class floor(const mpq_class& q) {
    mpz_class num = q.get_num();
    mpz_class den = q.get_den();
    mpz_class ent = num / den;
    if (num < 0 && den * ent != num)
        ent--;
    return ent;
}

inline mpz_class ceil(const mpq_class& q) {
    mpz_class num = q.get_num();
    mpz_class den = q.get_den();
    mpz_class ent = num / den;
    if (num > 0 && den * ent != num)
        ent++;
    return ent;
}

inline mpz_class round(const mpq_class& q) {
    mpq_class work;
    if (q >= 0) {
        work = q - mpq_class(1, 2);
        return ceil(work);
    }
    work = q + mpq_class(1, 2);
    return floor(work);
}

template <typename Integer>
mpz_class nmz_factorial(Integer n) {
    assert(n >= 0);
    mpz_class f = 1;
    long nlong = convertTo<long>(n);
    for (long i = 1; i <= nlong; ++i)
        f *= i;
    return f;
}

template <typename Integer>
mpz_class nmz_binomial(Integer n, Integer k) {
    if (k > n)
        return 0;
    return nmz_factorial(n) / nmz_factorial(k);
}

inline nmz_float mpq_to_nmz_float(const mpq_class& val) {
    mpz_class bound = 1;
    for (size_t i = 0; i < 60; ++i)
        bound *= 10;
    mpz_class gmp_num = val.get_num(), gmp_den = val.get_den();
    while (Iabs(gmp_num) > bound && Iabs(gmp_den) > bound) {
        gmp_num /= 10;
        gmp_den /= 10;
    }
    nmz_float num, den;
    convert(num, gmp_num);
    convert(den, gmp_den);
    return num / den;
}

template <typename Integer>
long convertToLong(const Integer& val) {
    long ret;
    try {
        ret = convertTo<long>(val);
    } catch (const ArithmeticException& e) {
        throw LongException(val);
    }

    return ret;
}

template <typename Integer>
long convertToLongLong(const Integer& val) {
    long ret;
    try {
        ret = convertTo<long long>(val);
    } catch (const ArithmeticException& e) {
        throw LongLongException(val);
    }

    return ret;
}
}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif /* INTEGER_H_ */
//---------------------------------------------------------------------------
