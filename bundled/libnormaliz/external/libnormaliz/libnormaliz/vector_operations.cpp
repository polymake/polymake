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

#include <iostream>
#include <string>
#include <algorithm>
#include <list>

#include "libnormaliz/vector_operations.h"
#include "libnormaliz/matrix.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

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
nmz_float v_scalar_product(const vector<nmz_float>& av, const vector<nmz_float>& bv) {
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
renf_elem_class v_scalar_product(const vector<renf_elem_class>& av, const vector<renf_elem_class>& bv) {
    // loop stretching ; brings some small speed improvement

    assert(av.size() == bv.size());

    renf_elem_class ans = 0;
    size_t n = av.size();

    for (size_t i = 0; i < n; ++i) {
        if (av[i] != 0 && bv[i] != 0)
            ans += av[i] * bv[i];
    }
    return ans;

    /*   typename vector<renf_elem_class>::const_iterator a=av.begin(), b=bv.begin();

       if( n >= 16 )
       {
           for( i = 0; i < ( n >> 4 ); ++i, a += 16, b +=16 ){
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

           n -= i<<4;
       }

       if( n >= 8)
       {
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

       if( n >= 4)
       {
           ans += a[0] * b[0];
           ans += a[1] * b[1];
           ans += a[2] * b[2];
           ans += a[3] * b[3];

           n -= 4;
           a += 4;
           b += 4;
       }

       if( n >= 2)
       {
           ans += a[0] * b[0];
           ans += a[1] * b[1];

           n -= 2;
           a += 2;
           b += 2;
       }

       if(n>0)
           ans += a[0]*b[0];

       return ans;*/
}

#endif

//---------------------------------------------------------------------------

template <>
mpq_class v_scalar_product(const vector<mpq_class>& av, const vector<mpq_class>& bv) {
    // loop stretching ; brings some small speed improvement

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

nmz_float l1norm(vector<nmz_float>& v) {
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

/* for nmz_float is norms the vector to l_1 norm 1.
 *
 * for mpq_class and renf_elem_class it makes the vector coefficents integral
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
nmz_float v_make_prime(vector<nmz_float>& v) {
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
void v_bool_entry_swap(vector<bool>& v, size_t i, size_t j) {
    if (v[i] != v[j]) {
        v[i].flip();
        v[j].flip();
    }
}

//---------------------------------------------------------------

vector<key_t> identity_key(size_t n) {
    vector<key_t> key(n);
    for (size_t k = 0; k < n; ++k)
        key[k] = k;
    return key;
}

vector<key_t> reverse_key(size_t n) {
    vector<key_t> key(n);
    for (size_t k = 0; k < n; ++k)
        key[k] = (n - 1) - k;
    return key;
}

// vector<bool> is special because ordinary swap is not defined for it
void order_by_perm_bool(vector<bool>& v, const vector<key_t>& permfix) {
    vector<key_t> perm = permfix;  // we may want to use permfix a second time
    vector<key_t> inv(perm.size());
    for (key_t i = 0; i < perm.size(); ++i)
        inv[perm[i]] = i;
    for (key_t i = 0; i < perm.size(); ++i) {
        key_t j = perm[i];
        // v.swap(v[i],v[perm[i]]);
        v_bool_entry_swap(v, i, perm[i]);
        swap(perm[i], perm[inv[i]]);
        swap(inv[i], inv[j]);
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
void v_scalar_division(vector<nmz_float>& v, const nmz_float scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    for (i = 0; i < size; i++) {
        v[i] /= scalar;
    }
}

template <>
void v_scalar_division(vector<mpq_class>& v, const mpq_class scalar) {
    size_t i, size = v.size();
    assert(scalar != 0);
    for (i = 0; i < size; i++) {
        v[i] /= scalar;
    }
}

#ifdef ENFNORMALIZ
template <>
void v_scalar_division(vector<renf_elem_class>& v, const renf_elem_class scalar) {
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
nmz_float v_standardize(vector<nmz_float>& v, const vector<nmz_float>& LF) {
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

#ifdef ENFNORMALIZ

template <>
renf_elem_class v_standardize(vector<renf_elem_class>& v, const vector<renf_elem_class>& LF) {
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

template long v_standardize(vector<long>&, const vector<long>&);
template long long v_standardize(vector<long long>&, const vector<long long>&);
template mpz_class v_standardize(vector<mpz_class>&, const vector<mpz_class>&);
#ifdef ENFNORMALIZ
template renf_elem_class v_standardize(vector<renf_elem_class>&, const vector<renf_elem_class>&);
#endif

template long v_standardize(vector<long>&);
template long long v_standardize(vector<long long>&);
template mpz_class v_standardize(vector<mpz_class>&);
#ifdef ENFNORMALIZ
template renf_elem_class v_standardize(vector<renf_elem_class>&);
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

template void v_scalar_division(vector<long>& v, const long scalar);
template void v_scalar_division(vector<long long>& v, const long long scalar);
template void v_scalar_division(vector<mpz_class>& v, const mpz_class scalar);

template long v_make_prime(vector<long>&);
template long long v_make_prime(vector<long long>&);
template mpz_class v_make_prime(vector<mpz_class>&);
#ifdef ENFNORMALIZ
template renf_elem_class v_make_prime(vector<renf_elem_class>&);
#endif

template long v_scalar_product(const vector<long>& a, const vector<long>& b);
template long long v_scalar_product(const vector<long long>& a, const vector<long long>& b);
template mpz_class v_scalar_product(const vector<mpz_class>& a, const vector<mpz_class>& b);

vector<bool> bitset_to_bool(const dynamic_bitset& val) {
    vector<bool> ret(val.size());
    for (size_t i = 0; i < val.size(); ++i)
        ret[i] = val[i];
    return ret;
}

vector<key_t> bitset_to_key(const dynamic_bitset& val) {
    vector<key_t> ret;
    for (size_t i = 0; i < val.size(); ++i)
        ret.push_back(i);
    return ret;
}

}  // end namespace libnormaliz
