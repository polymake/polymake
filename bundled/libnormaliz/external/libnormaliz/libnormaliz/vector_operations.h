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
#ifndef LIBNORMALIZ_VECTOR_OPERATIONS_H
#define LIBNORMALIZ_VECTOR_OPERATIONS_H
//---------------------------------------------------------------------------

#include <vector>
#include <ostream>
#include <list>

#include "libnormaliz/general.h"
#include "libnormaliz/integer.h"
// #include "libnormaliz/convert.h"
#include "libnormaliz/dynamic_bitset.h"

#ifdef NMZ_FLINT
#include "flint/flint.h"
#include "flint/fmpq_poly.h"
#endif

namespace libnormaliz {
using std::vector;

//---------------------------------------------------------------------------
//				Output
//---------------------------------------------------------------------------

template <typename T>
std::ostream& operator<<(std::ostream& out, const vector<T>& vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        out << vec[i] << " ";
    }
    out << std::endl;
    return out;
}

//---------------------------------------------------------------------------
//          Prototypes for vector_operations.cpp
//---------------------------------------------------------------------------

template <typename Integer>
Integer v_scalar_product(const vector<Integer>& a, const vector<Integer>& b);

template <typename Integer>
Integer v_make_prime(vector<Integer>& v);

nmz_float l1norm(vector<nmz_float>& v);

template <typename Integer>
void v_scalar_division(vector<Integer>& v, const Integer scalar);

// special version of order_by_perm (below) since special swap is needed
void order_by_perm_bool(vector<bool>& v, const vector<key_t>& permfix);

template <typename Integer>
vector<Integer> v_select_coordinates(const vector<Integer>& v, const vector<key_t> projection_key);
template <typename Integer>
vector<Integer> v_insert_coordinates(const vector<Integer>& v, const vector<key_t> projection_key, const size_t nr_cols);

//---------------------------------------------------------------------------
//         Templated functions
//---------------------------------------------------------------------------

// returns the scalar product of the truncations of vectors a and b to minimum of lengths
// template<typename Integer>
template <typename Integer>
Integer v_scalar_product_vectors_unequal_lungth(const vector<Integer>& a, const vector<Integer>& b) {
    size_t n = std::min(a.size(), b.size());
    vector<Integer> trunc_a = a;
    vector<Integer> trunc_b = b;
    trunc_a.resize(n);
    trunc_b.resize(n);
    return v_scalar_product(trunc_a, trunc_b);
}

// v = v * scalar
template <typename Integer>
void v_scalar_multiplication(vector<Integer>& v, const Integer scalar) {
    size_t i, size = v.size();
    for (i = 0; i < size; i++) {
        v[i] *= scalar;
    }
}

// make random vector of length n with entries between -m and m
template <typename Integer>
vector<Integer> v_random(size_t n, long m) {
    vector<Integer> result(n);
    for (size_t i = 0; i < n; ++i)
        result[i] = rand() % (2 * m + 1) - m;
    return result;
}

template <typename Integer>
bool compare_last(const vector<Integer>& a, const vector<Integer>& b) {
    return a.back() < b.back();
}

// swaps entry i and j of the vector<bool> v
void v_bool_entry_swap(vector<bool>& v, size_t i, size_t j);

vector<key_t> identity_key(size_t n);
vector<key_t> reverse_key(size_t n);
vector<key_t> random_key(size_t n);

template <typename T>
void order_by_perm(vector<T>& v, const vector<key_t>& permfix) {
    // orders v "in place", v --> w such that
    // w[i]=v[permfix[i]]
    // if v is  the map i --> v[i], then the resulting map is v \circ permfix

    vector<key_t> perm = permfix;  // we may want to use permfix a second time
    vector<key_t> inv(perm.size());
    for (key_t i = 0; i < perm.size(); ++i)
        inv[perm[i]] = i;
    for (key_t i = 0; i < perm.size(); ++i) {
        key_t j = perm[i];
        std::swap(v[i], v[perm[i]]);
        std::swap(perm[i], perm[inv[i]]);
        std::swap(inv[i], inv[j]);
    }
}

inline vector<key_t> conjugate_perm(const vector<key_t>& p, const vector<key_t>& k) {
    // p is a permutation of [0,n-1], i --> p[i]
    // k is an injective map [0,m-1] --> [0,n-1]
    // k^{-1} is the partially defined inverse
    // computes   k^{-1} p k
    // works only if Image(k) is stable under p.

    vector<int> inv_k(p.size(), -1);
    for (size_t i = 0; i < k.size(); ++i) {
        inv_k[k[i]] = i;
    }
    vector<key_t> conj(k.size());
    for (size_t i = 0; i < k.size(); ++i) {
        assert(inv_k[k[i]] != -1);
        conj[i] = inv_k[p[k[i]]];
    }
    return conj;
}

template <typename T>
void sort_individual_vectors(vector<vector<T> >& vv) {
    for (size_t i = 0; i < vv.size(); ++i)
        sort(vv[i].begin(), vv[i].end());
}

template <typename Integer>
bool v_scalar_mult_mod_inner(vector<Integer>& w, const vector<Integer>& v, const Integer& scalar, const Integer& modulus) {
    size_t i, size = v.size();
    Integer test;
    for (i = 0; i < size; i++) {
        test = v[i] * scalar;
        if (!check_range(test)) {
            return false;
        }
        w[i] = test % modulus;
        if (w[i] < 0)
            w[i] += modulus;
    }
    return true;
}

template <typename Integer>
vector<Integer> v_scalar_mult_mod(const vector<Integer>& v, const Integer& scalar, const Integer& modulus);

//---------------------------------------------------------------------------

/*
template <typename Integer>
size_t v_nr_negative(const vector<Integer>& v) {
    size_t tmp = 0;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] < 0)
            tmp++;
    }
    return tmp;
}
*/
//---------------------------------------------------------------------------

template <typename Integer>
bool v_non_negative(const vector<Integer>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] < 0)
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------
/*
// returns a key vector containing the positions of non-zero entrys of v
template <typename Integer>
vector<key_t> v_non_zero_pos(const vector<Integer>& v) {
    vector<key_t> key;
    size_t size = v.size();
    key.reserve(size);
    for (key_t i = 0; i < size; i++) {
        if (v[i] != 0) {
            key.push_back(i);
        }
    }
    return key;
}
*/

//---------------------------------------------------------------------------
// returns the vector of absolute values, does not change the argument
template <typename Integer>
vector<Integer> v_abs_value(vector<Integer>& v) {
    size_t i, size = v.size();
    vector<Integer> w = v;
    for (i = 0; i < size; i++) {
        if (v[i] < 0)
            w[i] = Iabs(v[i]);
    }
    return w;
}

//---------------------------------------------------------------------------
// returns gcd of the elements of v
template <typename Integer>
inline Integer v_gcd(const vector<Integer>& v) {
    size_t i, size = v.size();
    Integer g = 0;
    for (i = 0; i < size; i++) {
        g = libnormaliz::gcd(g, v[i]);
        if (g == 1) {
            return 1;
        }
    }
    return g;
}

template <>
inline mpq_class v_gcd(const vector<mpq_class>& v) {
    size_t i, size = v.size();
    mpz_class g = 0;
    for (i = 0; i < size; i++) {
        g = libnormaliz::gcd(g, v[i].get_num());
        if (g == 1) {
            return 1;
        }
    }
    return mpq_class(g);
}

#ifdef ENFNORMALIZ

inline mpz_class get_gcd_num(const renf_elem_class& x) {
    vector<mpz_class> numerator = x.num_vector();
    return v_gcd(numerator);
}

template <>
inline renf_elem_class v_gcd(const vector<renf_elem_class>& v) {
    size_t i, size = v.size();
    mpz_class g = 0;
    mpz_class this_gcd;
    for (i = 0; i < size; i++) {
        // this_gcd=v[i].num_content();
        this_gcd = get_gcd_num(v[i]);
        g = libnormaliz::gcd(g, this_gcd);
        if (g == 1) {
            return 1;
        }
    }
    return renf_elem_class(g);
}
#endif

//---------------------------------------------------------------------------
// returns lcm of the elements of v
template <typename Integer>
Integer v_lcm(const vector<Integer>& v) {
    size_t i, size = v.size();
    Integer g = 1;
    for (i = 0; i < size; i++) {
        g = libnormaliz::lcm(g, v[i]);
        if (g == 0) {
            return 0;
        }
    }
    return g;
}

// returns lcm of the elements of v from index k up to index j
template <typename Integer>
Integer v_lcm_to(const vector<Integer>& v, const size_t k, const size_t j) {
    assert(k <= j);
    size_t i;
    Integer g = 1;
    for (i = k; i <= j; i++) {
        g = libnormaliz::lcm(g, v[i]);
        if (g == 0) {
            return 0;
        }
    }
    return g;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer>& v_abs(vector<Integer>& v) {
    size_t i, size = v.size();
    for (i = 0; i < size; i++) {
        if (v[i] < 0)
            v[i] = Iabs(v[i]);
    }
    return v;
}

//---------------------------------------------------------------------------
// returns a new vector with the content of a extended by b
template <typename T>
vector<T> v_merge(const vector<T>& a, const T& b) {
    size_t s = a.size();
    vector<T> c(s + 1);
    for (size_t i = 0; i < s; i++) {
        c[i] = a[i];
    }
    c[s] = b;
    return c;
}

//---------------------------------------------------------------------------

template <typename T>
vector<T> v_merge(const vector<T>& a, const vector<T>& b) {
    size_t s1 = a.size(), s2 = b.size(), i;
    vector<T> c(s1 + s2);
    for (i = 0; i < s1; i++) {
        c[i] = a[i];
    }
    for (i = 0; i < s2; i++) {
        c[s1 + i] = b[i];
    }
    return c;
}

//---------------------------------------------------------------------------

template <typename Integer>
void v_reduction_modulo(vector<Integer>& v, const Integer& modulo) {
    size_t i, size = v.size();
    for (i = 0; i < size; i++) {
        v[i] = v[i] % modulo;
        if (v[i] < 0) {
            v[i] = v[i] + modulo;
        }
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer>& v_add_to_mod(vector<Integer>& a, const vector<Integer>& b, const Integer& m) {
    //  assert(a.size() == b.size());
    size_t i, s = a.size();
    for (i = 0; i < s; i++) {
        //      a[i] = (a[i]+b[i])%m;
        if ((a[i] += b[i]) >= m) {
            a[i] -= m;
        }
    }
    return a;
}

//---------------------------------------------------------------------------

template <typename Integer>
bool v_is_zero(const vector<Integer>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] != 0)
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> v_add(const vector<Integer>& a, const vector<Integer>& b) {
    assert(a.size() == b.size());
    size_t i, s = a.size();
    vector<Integer> d(s);
    for (i = 0; i < s; i++) {
        d[i] = a[i] + b[i];
    }
    return d;
}

//---------------------------------------------------------------------------

template <typename Integer>
void v_add_result(vector<Integer>& result, const size_t s, const vector<Integer>& a, const vector<Integer>& b) {
    assert(a.size() == b.size() && a.size() == result.size());
    size_t i;
    // vector<Integer> d(s);
    for (i = 0; i < s; i++) {
        result[i] = a[i] + b[i];
    }
    // return d;
}

//---------------------------------------------------------------------------
/*
// returns a new vector with the last size entries of v
template <typename T>
vector<T> v_cut_front(const vector<T>& v, size_t size) {
    size_t s, k;
    vector<T> tmp(size);
    s = v.size() - size;
    for (k = 0; k < size; k++) {
        tmp[k] = v[s + k];
    }
    return tmp;
}
*/

//---------------------------------------------------------------------------

template <typename Integer>
bool v_is_symmetric(const vector<Integer>& v) {
    for (size_t i = 0; i < v.size() / 2; ++i) {
        if (v[i] != v[v.size() - 1 - i])
            return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template <typename Integer>
void v_el_trans(const vector<Integer>& av, vector<Integer>& bv, const Integer& F, const size_t start) {
    size_t i, n = av.size();

    auto a = av.begin();
    auto b = bv.begin();

    a += start;
    b += start;
    n -= start;

    if (n >= 8) {
        for (i = 0; i < (n >> 3); ++i, a += 8, b += 8) {
            b[0] += F * a[0];
            b[1] += F * a[1];
            b[2] += F * a[2];
            b[3] += F * a[3];
            b[4] += F * a[4];
            b[5] += F * a[5];
            b[6] += F * a[6];
            b[7] += F * a[7];
        }
        n -= i << 3;
    }

    if (n >= 4) {
        b[0] += F * a[0];
        b[1] += F * a[1];
        b[2] += F * a[2];
        b[3] += F * a[3];

        n -= 4;
        a += 4;
        b += 4;
    }

    if (n >= 2) {
        b[0] += F * a[0];
        b[1] += F * a[1];

        n -= 2;
        a += 2;
        b += 2;
    }

    if (n > 0)
        b[0] += F * a[0];

    for (size_t i = 0; i < bv.size(); ++i)
        if (!check_range(bv[i]))
            throw ArithmeticException("Vector entry out of range. Imminent danger of arithmetic overflow.");
}

/*
template <typename Integer>
Integer v_max_abs(const vector<Integer>& v) {
    Integer tmp = 0;
    for (size_t i = 0; i < v.size(); i++) {
        if (Iabs(v[i]) > tmp)
            tmp = Iabs(v[i]);
    }
    return tmp;
}
*/

template <typename Integer>
Integer v_standardize(vector<Integer>& v, const vector<Integer>& LF);

template <typename Integer>
Integer v_standardize(vector<Integer>& v);

vector<bool> bitset_to_bool(const dynamic_bitset& BS);
vector<key_t> bitset_to_key(const dynamic_bitset& BS);
dynamic_bitset bool_to_bitset(const vector<bool>& val);
dynamic_bitset key_to_bitset(const vector<key_t>& key, long size);

template <typename Integer>
inline void make_integral(vector<Integer>& vec) {
}

// from the old renfxx.h

#ifdef ENFNORMALIZ

inline void vector2fmpq_poly(fmpq_poly_t flp, const std::vector<mpq_class>& poly_vector) {
    slong n = (slong)poly_vector.size();

    fmpq_poly_fit_length(flp, n);
    for (size_t i = 0; i < poly_vector.size(); ++i) {
        fmpq_poly_set_coeff_mpq(flp, (slong)i, poly_vector[i].get_mpq_t());
    }
}

inline void fmpq_poly2vector(std::vector<mpq_class>& poly_vector, const fmpq_poly_t flp) {
    slong length = fmpq_poly_length(flp);
    if (length == 0) {
        poly_vector.push_back(mpz_class(0));
        return;
    }
    poly_vector.resize(length);
    for (slong i = 0; i < length; i++) {
        mpq_t current_coeff;
        mpq_init(current_coeff);
        fmpq_poly_get_coeff_mpq(current_coeff, flp, (slong)i);
        poly_vector[i] = mpq_class(current_coeff);
    }
}

template <>
inline void make_integral(vector<renf_elem_class>& vec) {
    mpz_class denom = 1;
    for (size_t i = 0; i < vec.size(); ++i) {
        denom = libnormaliz::lcm(denom, vec[i].den());
    }
    renf_elem_class fact(denom);
    if (fact != 1)
        v_scalar_multiplication(vec, fact);
}
#endif

template <>
inline void make_integral(vector<mpq_class>& vec) {
    mpz_class denom = 1;
    for (size_t i = 0; i < vec.size(); ++i) {
        denom = libnormaliz::lcm(denom, vec[i].get_den());
    }
    mpq_class fact(denom);
    if (fact != 1)
        v_scalar_multiplication(vec, fact);
}

//=============================================================

// old vector_operations.cpp

template <typename Integer>
Integer v_scalar_product(const vector<Integer>& av, const vector<Integer>& bv) {
    // loop stretching ; brings some small speed improvement

    Integer ans = 0;
    size_t i, n = av.size();

#if 0  // #ifdef __MIC__   // not for newer compiler versions
    // this version seems to be better vectorizable on the mic
    for (i=0; i<n; ++i)
        ans += av[i]*bv[i];

#else   // __MIC__
    auto a = av.begin(), b = bv.begin();

    if (n >= 16) {
        for (i = 0; i < (n >> 4); ++i, a += 16, b += 16) {
            ans += a[0] * b[0];
            ans += a[1] * b[1];
            ans += a[2] * b[2];
            ans += a[3] * b[3];
            ans += a[4] * b[4];
            ans += a[5] * b[5];
            ans += a[6] * b[6];
            ans += a[7] * b[7];
            ans += a[8] * b[8];
            ans += a[9] * b[9];
            ans += a[10] * b[10];
            ans += a[11] * b[11];
            ans += a[12] * b[12];
            ans += a[13] * b[13];
            ans += a[14] * b[14];
            ans += a[15] * b[15];
        }

        n -= i << 4;
    }

    if (n >= 8) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];
        ans += a[4] * b[4];
        ans += a[5] * b[5];
        ans += a[6] * b[6];
        ans += a[7] * b[7];

        n -= 8;
        a += 8;
        b += 8;
    }

    if (n >= 4) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];

        n -= 4;
        a += 4;
        b += 4;
    }

    if (n >= 2) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];

        n -= 2;
        a += 2;
        b += 2;
    }

    if (n > 0)
        ans += a[0] * b[0];
#endif  // __MIC__

    if (!check_range(ans)) {
#pragma omp atomic
        GMP_scal_prod++;

        // cout << "av " << av;
        // cout << "bv " << bv;
        vector<mpz_class> mpz_a(av.size()), mpz_b(bv.size());
        convert(mpz_a, av);
        convert(mpz_b, bv);
        convert(ans, v_scalar_product(mpz_a, mpz_b));
    }

    return ans;
}

template <>
inline nmz_float v_scalar_product(const vector<nmz_float>& av, const vector<nmz_float>& bv) {
    // loop stretching ; brings some small speed improvement

    nmz_float ans = 0;
    size_t i, n = av.size();

    auto a = av.begin(), b = bv.begin();

    if (n >= 16) {
        for (i = 0; i < (n >> 4); ++i, a += 16, b += 16) {
            ans += a[0] * b[0];
            ans += a[1] * b[1];
            ans += a[2] * b[2];
            ans += a[3] * b[3];
            ans += a[4] * b[4];
            ans += a[5] * b[5];
            ans += a[6] * b[6];
            ans += a[7] * b[7];
            ans += a[8] * b[8];
            ans += a[9] * b[9];
            ans += a[10] * b[10];
            ans += a[11] * b[11];
            ans += a[12] * b[12];
            ans += a[13] * b[13];
            ans += a[14] * b[14];
            ans += a[15] * b[15];
        }

        n -= i << 4;
    }

    if (n >= 8) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];
        ans += a[4] * b[4];
        ans += a[5] * b[5];
        ans += a[6] * b[6];
        ans += a[7] * b[7];

        n -= 8;
        a += 8;
        b += 8;
    }

    if (n >= 4) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];

        n -= 4;
        a += 4;
        b += 4;
    }

    if (n >= 2) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];

        n -= 2;
        a += 2;
        b += 2;
    }

    if (n > 0)
        ans += a[0] * b[0];

    return ans;
}

#ifdef ENFNORMALIZ

template <>
inline renf_elem_class v_scalar_product(const vector<renf_elem_class>& av, const vector<renf_elem_class>& bv) {
    // loop stretching ; brings some small speed improvement

    assert(av.size() == bv.size());

    renf_elem_class ans = 0;
    size_t n = av.size();
    renf_elem_class help;

    for (size_t i = 0; i < n; ++i) {
        if (av[i] != 0 && bv[i] != 0) {
            ans += av[i] * bv[i];
            /* help = av[i];
            help *= bv[i]; // does not seem to help
            ans += help;*/
        }
    }
    return ans;
}

#endif

//---------------------------------------------------------------------------

template <>
inline mpq_class v_scalar_product(const vector<mpq_class>& av, const vector<mpq_class>& bv) {
    // loop stretching ; brings some small speed improvement

    assert(false);
    return 0;
}

/* body removed for the time being
    mpq_class ans = 0;
    size_t i, n = av.size();

#if 0  // #ifdef __MIC__   // not for newer compiler versions
    // this version seems to be better vectorizable on the mic
    for (i=0; i<n; ++i)
        ans += av[i]*bv[i];

#else   // __MIC__
    auto a = av.begin(), b = bv.begin();

    if (n >= 16) {
        for (i = 0; i < (n >> 4); ++i, a += 16, b += 16) {
            ans += a[0] * b[0];
            ans += a[1] * b[1];
            ans += a[2] * b[2];
            ans += a[3] * b[3];
            ans += a[4] * b[4];
            ans += a[5] * b[5];
            ans += a[6] * b[6];
            ans += a[7] * b[7];
            ans += a[8] * b[8];
            ans += a[9] * b[9];
            ans += a[10] * b[10];
            ans += a[11] * b[11];
            ans += a[12] * b[12];
            ans += a[13] * b[13];
            ans += a[14] * b[14];
            ans += a[15] * b[15];
        }

        n -= i << 4;
    }

    if (n >= 8) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];
        ans += a[4] * b[4];
        ans += a[5] * b[5];
        ans += a[6] * b[6];
        ans += a[7] * b[7];

        n -= 8;
        a += 8;
        b += 8;
    }

    if (n >= 4) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];

        n -= 4;
        a += 4;
        b += 4;
    }

    if (n >= 2) {
        ans += a[0] * b[0];
        ans += a[1] * b[1];

        n -= 2;
        a += 2;
        b += 2;
    }

    if (n > 0)
        ans += a[0] * b[0];
#endif  // __MIC__

    return ans;
}

*/

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> v_select_coordinates(const vector<Integer>& v, const vector<key_t> projection_key) {
    vector<Integer> w(projection_key.size());
    for (size_t i = 0; i < w.size(); ++i)
        w[i] = v[projection_key[i]];
    return w;
}

//---------------------------------------------------------------------------

template <typename Integer>
vector<Integer> v_insert_coordinates(const vector<Integer>& v, const vector<key_t> projection_key, const size_t nr_cols) {
    vector<Integer> w(nr_cols);
    for (size_t i = 0; i < projection_key.size(); ++i) {
        assert(projection_key[i] < nr_cols);
        w[projection_key[i]] = v[i];
    }
    return w;
}
//---------------------------------------------------------------------------

inline nmz_float l1norm(vector<nmz_float>& v) {
    size_t i, size = v.size();
    nmz_float g = 0;
    for (i = 0; i < size; i++) {
        if (Iabs(v[i]) > nmz_epsilon)
            g += Iabs(v[i]);
        else
            v[i] = 0;
    }
    return g;
}

/*
mpq_class l1norm(vector<mpq_class>& v) {
    size_t i, size = v.size();
    mpq_class g = 0;
    for (i = 0; i < size; i++) {
        if (Iabs(v[i]) > 0)
            g += Iabs(v[i]);
        else
            v[i] = 0;
    }
    return g;
}
*/

/* for nmz_float is norms the vector to l_1 norm 1.
 *
 * for mpq_class and renf_elem_class it makes the vector coefficients integral
 *
 * then it extracts the gcd of the coefficients
 */

template <typename Integer>
Integer v_make_prime(vector<Integer>& v) {
    size_t i, size = v.size();

#ifdef ENFNORMALIZ
    if (using_renf<Integer>()) {
        v_standardize(v);
        make_integral(v);
        return (1);
    }
#endif

    if (using_mpq_class<Integer>())
        make_integral(v);
    Integer g = v_gcd(v);
    if (g != 0 && g != 1) {
        for (i = 0; i < size; i++) {
            v[i] /= g;
        }
    }
    return g;
}

template <>
inline nmz_float v_make_prime(vector<nmz_float>& v) {
    size_t i, size = v.size();
    nmz_float g = l1norm(v);
    if (g != 0) {
        for (i = 0; i < size; i++) {
            v[i] /= g;
        }
    }
    return g;
}

//---------------------------------------------------------------

// swaps entry i and j of the vector<bool> v
inline void v_bool_entry_swap(vector<bool>& v, size_t i, size_t j) {
    if (v[i] != v[j]) {
        v[i].flip();
        v[j].flip();
    }
}

//---------------------------------------------------------------

inline vector<key_t> identity_key(size_t n) {
    vector<key_t> key(n);
    for (size_t k = 0; k < n; ++k)
        key[k] = k;
    return key;
}

inline vector<key_t> reverse_key(size_t n) {
    vector<key_t> key(n);
    for (size_t k = 0; k < n; ++k)
        key[k] = (n - 1) - k;
    return key;
}

inline vector<key_t> random_key(size_t n) {
    vector<key_t> key = identity_key(n);
    for (size_t k = 0; k < 3 * n; ++k)
        std::swap(key[rand() % n], key[rand() % n]);
    return key;
}

// vector<bool> is special because ordinary swap is not defined for it
inline void order_by_perm_bool(vector<bool>& v, const vector<key_t>& permfix) {
    vector<key_t> perm = permfix;  // we may want to use permfix a second time
    vector<key_t> inv(perm.size());
    for (key_t i = 0; i < perm.size(); ++i)
        inv[perm[i]] = i;
    for (key_t i = 0; i < perm.size(); ++i) {
        key_t j = perm[i];
        // v.swap(v[i],v[perm[i]]);
        v_bool_entry_swap(v, i, perm[i]);
        std::swap(perm[i], perm[inv[i]]);
        std::swap(inv[i], inv[j]);
    }
}

//---------------------------------------------------------------------------

template <typename Integer>
void v_scalar_division(vector<Integer>& v, const Integer scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    for (i = 0; i < size; i++) {
        assert(v[i] % scalar == 0);
        v[i] /= scalar;
    }
}

template <>
inline void v_scalar_division(vector<nmz_float>& v, const nmz_float scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    for (i = 0; i < size; i++) {
        v[i] /= scalar;
    }
}

template <>
inline void v_scalar_division(vector<mpq_class>& v, const mpq_class scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    for (i = 0; i < size; i++) {
        v[i] /= scalar;
    }
}

#ifdef ENFNORMALIZ
template <>
inline void v_scalar_division(vector<renf_elem_class>& v, const renf_elem_class scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    renf_elem_class fact = 1 / scalar;
    for (i = 0; i < size; i++) {
        v[i] *= fact;
    }
}
#endif

/* v_standardize
 *
 * defined only for mpq_class, nmz_float and renf_elem_class
 *
 * makes the value under LF equal to 1 (checks for positivity of value)
 *
 * or the last component equal to +-1
 */

template <typename Integer>
Integer v_standardize(vector<Integer>& v, const vector<Integer>& LF) {
    assert(false);
    return 0;
}

template <typename Integer>
Integer v_standardize(vector<Integer>& v) {
    vector<Integer> LF;
    return v_standardize(v, LF);
}

template <>
inline nmz_float v_standardize(vector<nmz_float>& v, const vector<nmz_float>& LF) {
    nmz_float denom = 0;
    if (LF.size() == v.size()) {
        denom = v_scalar_product(v, LF);
    }

    if (denom == 0) {
        for (long i = (long)v.size() - 1; i >= 0; --i) {
            if (v[i] != 0) {
                denom = v[i];
                break;
            }
        }
    }
    denom = Iabs(denom);

    if (denom == 0)
        return denom;
    if (denom != 1)
        v_scalar_division(v, denom);

    return denom;
}

/*
template <>
mpq_class v_standardize(vector<mpq_class>& v, const vector<mpq_class>& LF) {
    mpq_class denom = 0;
    if (LF.size() == v.size()) {
        denom = v_scalar_product(v, LF);
    };

    if (denom == 0) {
        for (long i = (long)v.size() - 1; i >= 0; --i) {
            if (v[i] != 0) {
                denom = v[i];
                break;
            }
        }
    }
    denom = Iabs(denom);

    if (denom == 0)
        return denom;
    if (denom != 1)
        v_scalar_division(v, denom);

    return denom;
}
*/

#ifdef ENFNORMALIZ

template <>
inline renf_elem_class v_standardize(vector<renf_elem_class>& v, const vector<renf_elem_class>& LF) {
    renf_elem_class denom = 0;
    if (LF.size() == v.size()) {
        denom = v_scalar_product(v, LF);
    }

    if (denom == 0) {
        for (long i = (long)v.size() - 1; i >= 0; --i) {
            if (v[i] != 0) {
                denom = v[i];
                break;
            }
        }
    }
    denom = Iabs(denom);

    if (denom == 0)
        return denom;
    if (denom != 1)
        v_scalar_division(v, denom);

    return denom;
}
#endif

/* Not used presently
// the following function removes the denominators and then extracts the Gcd of the numerators
mpq_class v_standardize(vector<mpq_class>& v, const vector<mpq_class>& LF){
    size_t size=v.size();
    mpz_class d=1;
    for (size_t i = 0; i < size; i++)
        //d=lcm(d,v[i].get_den());  // GMP C++ function only available in GMP >= 6.1
        mpz_lcm(d.get_mpz_t(), d.get_mpz_t(), v[i].get_den().get_mpz_t());
    for (size_t i = 0; i < size; i++)
        v[i]*=d;
    mpz_class g=0;
    for (size_t i = 0; i < size; i++)
        //g=gcd(g,v[i].get_num());  //  GMP C++ function only available in GMP >= 6.1
        mpz_gcd(g.get_mpz_t(), g.get_mpz_t(), v[i].get_num().get_mpz_t());
    if (g==0)
        return 0;
    for (size_t i = 0; i < size; i++)
        v[i]/=g;
    return 1;
}
*/

template <typename Integer>
vector<Integer> v_scalar_mult_mod(const vector<Integer>& v, const Integer& scalar, const Integer& modulus) {
    vector<Integer> w(v.size());
    if (v_scalar_mult_mod_inner(w, v, scalar, modulus))
        return w;

#pragma omp atomic
    GMP_scal_prod++;
    vector<mpz_class> x, y(v.size());
    convert(x, v);
    v_scalar_mult_mod_inner(y, x, convertTo<mpz_class>(scalar), convertTo<mpz_class>(modulus));
    return convertTo<vector<Integer> >(y);
}

inline vector<bool> bitset_to_bool(const dynamic_bitset& val) {
    vector<bool> ret(val.size());
    for (size_t i = 0; i < val.size(); ++i)
        ret[i] = val[i];
    return ret;
}

inline dynamic_bitset bool_to_bitset(const vector<bool>& val) {
    dynamic_bitset ret(val.size());
    for (size_t i = 0; i < val.size(); ++i)
        ret[i] = val[i];
    return ret;
}

inline vector<key_t> bitset_to_key(const dynamic_bitset& val) {
    vector<key_t> ret;
    for (size_t i = 0; i < val.size(); ++i)
        if (val[i])
            ret.push_back(i);
    return ret;
}

inline dynamic_bitset key_to_bitset(const vector<key_t>& key, long size) {
    dynamic_bitset bs(size);
    for (size_t i = 0; i < key.size(); ++i) {
        assert(key[i] < size);
        bs[key[i]] = 1;
    }
    return bs;
}

template <typename T>
vector<bool> binary_expansion(T n) {
    vector<bool> bin;
    while (n != 0) {
        bin.push_back(n & 1);
        n = n >> 1;
    }
    return bin;
}

template <typename Integer>
Integer vector_sum_cascade(vector<Integer>& summands) {
    size_t step = 2;
    bool added = true;
    while (added) {
        added = false;
#pragma omp parallel for
        for (size_t k = 0; k < summands.size(); k += step) {
            if (summands.size() > k + step / 2) {
                summands[k] += summands[k + step / 2];
                added = true;
            }
        }
        step *= 2;
    }
    return summands[0];
}

//--------------------------------------------------------------

template <typename Integer>
class AdditionPyramid {
   public:
    vector<Integer> accumulator;
    vector<size_t> counter;
    size_t capacity;
    void add_inner(const Integer summand, const size_t level);

    AdditionPyramid();
    AdditionPyramid(const size_t& given_capacity);
    void add(const Integer& summand);
    Integer sum();
    void reset();
    void set_capacity(const size_t& given_capacity);
};

template <typename Integer>
void AdditionPyramid<Integer>::add_inner(const Integer summand, const size_t level) {
    // cout << "***** " << summand << " -- " << level << endl;

    assert(level <= counter.size());

    if (level == counter.size()) {
        counter.resize(level + 1);
        accumulator.resize(level + 1);
        // cout << "$$$$$ " << accumulator[level] << " -- " << summand << endl;
        accumulator[level] = summand;
        // cout << "+++ " << accumulator[level] << endl;
        return;
    }

    counter[level]++;

    if (counter[level] < capacity) {
        accumulator[level] += summand;
        return;
    }

    add_inner(accumulator[level], level + 1);
    counter[level] = 0;
    accumulator[level] = summand;
}

template <typename Integer>
AdditionPyramid<Integer>::AdditionPyramid() {
}

template <typename Integer>
void AdditionPyramid<Integer>::reset() {
    counter.clear();
    accumulator.clear();
}

template <typename Integer>
AdditionPyramid<Integer>::AdditionPyramid(const size_t& given_capacity) {
    capacity = given_capacity;
    reset();
}

template <typename Integer>
void AdditionPyramid<Integer>::set_capacity(const size_t& given_capacity) {
    capacity = given_capacity;
}

template <typename Integer>
Integer AdditionPyramid<Integer>::sum() {
    Integer our_sum;  // this version works also for CoCoALib::Bigrat
    our_sum = 0;
    for (size_t i = 0; i < accumulator.size(); ++i)
        our_sum += accumulator[i];
    return our_sum;
}

template <typename Integer>
void AdditionPyramid<Integer>::add(const Integer& summand) {
    if (counter.size() > 0) {
        if (counter[0] < capacity - 1) {
            counter[0]++;
            accumulator[0] += summand;
            return;
        }
    }
    add_inner(summand, 0);
}

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
