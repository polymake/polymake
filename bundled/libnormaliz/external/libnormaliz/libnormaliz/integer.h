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

#ifndef INTEGER_H_
#define INTEGER_H_

#include <libnormaliz/general.h>

#include <list>
#include <vector>
#include <iostream>
#include <limits.h>


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
template<typename Integer> inline Integer Iabs(const Integer& a) {
    return (a>=0) ? (a) : Integer(-a);
}

//returns gcd of a and b,   if one is 0 returns the other integer
template<typename Integer> Integer gcd(const Integer& a, const Integer& b);
template<> mpz_class gcd<mpz_class>(const mpz_class& a, const mpz_class& b);

//returns lcm of a and b,   returns 0 if one is 0
template<typename Integer> Integer lcm(const Integer& a, const Integer& b);
template<> mpz_class lcm(const mpz_class& a, const mpz_class& b);

// integer division a/b. Returns quot and rem = minimal remainder <= |b|/2
 template<typename Integer>
void minimal_remainder(const Integer& a, const Integer&b, Integer& quot, Integer& rem);

// extended Euclidean algorithm: d=ua+vb
template <typename Integer>
Integer ext_gcd(const Integer& a, const Integer& b, Integer& u, Integer&v);

// minimizes u and v and makes d >= 0.
template <typename Integer>
void sign_adjust_and_minimize(const Integer& a, const Integer& b, Integer& d, Integer& u, Integer&v);

//---------------------------------------------------------------------------
//                     Conversions and checks
//---------------------------------------------------------------------------

// convert val to ret
// does the conversion and returns false if it fails
bool try_convert(long& ret, const long long& val);
inline bool try_convert(long long& ret, const long& val) {ret = val; return true;}
bool try_convert(long& ret, const mpz_class& val);
bool try_convert(long long& ret, const mpz_class& val);
inline bool try_convert(mpz_class& ret, const long& val) {ret = val; return true;}
bool try_convert(mpz_class& ret, const long long& val);

// template for same typ "conversion"
template<typename Type>
inline bool try_convert(Type& ret, const Type& val) {ret = val; return true;}


bool fits_long_range(long long a);

template<typename Integer>
inline bool using_GMP() {
  return false;
}

template<>
inline bool using_GMP<mpz_class>() {
  return true;
} 

template<typename Integer>
Integer int_max_value_dual();

template<typename Integer>
Integer int_max_value_primary();

//---------------------------------------------------------------------------

template<typename Integer>
inline bool check_range(const Integer& m) {
    const Integer max_primary = int_max_value_primary<Integer>();
    return (Iabs(m) <= max_primary);
}

template<>
inline bool check_range<mpz_class>(const mpz_class& m) {
  return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void check_range_list(const std::list<std::vector<Integer> >& ll);

template<typename Integer> class CandidateList;
template<typename Integer> class Candidate;

template<typename Integer>
void check_range_list(const CandidateList<Integer>& ll);
template<typename Integer>
void check_range_list(const std::list<Candidate<Integer> >& ll);



//---------------------------------------------------------------------------
//                     Special functions
//---------------------------------------------------------------------------

//return the number of decimals, needed to write the Integer a
template<typename Integer> size_t decimal_length(Integer a);

//returns b!/a!
template<typename Integer> Integer permutations(const size_t& a, const size_t& b);
template<typename Integer> Integer permutations_modulo(const size_t& a, const size_t& b, long m);

}

//---------------------------------------------------------------------------
#endif /* INTEGER_H_ */
//---------------------------------------------------------------------------
