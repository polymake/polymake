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
//---------------------------------------------------------------------------
#ifndef LIBNORMALIZ_VECTOR_OPERATIONS_H
#define LIBNORMALIZ_VECTOR_OPERATIONS_H
//---------------------------------------------------------------------------

#include <vector>
#include <ostream>
#include <list>

#include "libnormaliz/general.h"
#include "libnormaliz/integer.h"
#include "libnormaliz/convert.h"
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
    size_t n = min(a.size(), b.size());
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
        swap(v[i], v[perm[i]]);
        swap(perm[i], perm[inv[i]]);
        swap(inv[i], inv[j]);
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
    vector<mpz_class> numerator = x.get_num_vector();
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
        denom = libnormaliz::lcm(denom, vec[i].get_den());
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

}  // namespace libnormaliz

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
