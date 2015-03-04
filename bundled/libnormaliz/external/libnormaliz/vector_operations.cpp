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

//---------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <algorithm>
#include<list>

#include "integer.h"
#include "vector_operations.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_scalar_product(const vector<Integer>& av,const vector<Integer>& bv){
    //loop stretching ; brings some small speed improvement

    Integer ans = 0;
    size_t i,n=av.size();

    typename vector<Integer>::const_iterator a=av.begin(), b=bv.begin();

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
        
    return ans;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_scalar_product_unequal_vectors_end(const vector<Integer>& a,const vector<Integer>& b){
    Integer ans = 0;
    size_t i,n=a.size(),m=b.size();
    for (i = 1; i <= n; i++) {
        ans+=a[n-i]*b[m-i];
    }
    return ans;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> v_add_overflow_check(const vector<Integer>& a,const vector<Integer>& b){
    size_t i,s=a.size();
    Integer test;
    vector<Integer> d(s);
    for (i = 0; i <s; i++) {
        d[i]=a[i]+b[i];
        test=(a[i]%overflow_test_modulus + b[i]%overflow_test_modulus); // %overflow_test_modulus;
        if((d[i]-test) % overflow_test_modulus !=0){
            errorOutput()<<"Arithmetic failure in vector addition. Moat likely arithmetic overflow.\n";
            throw ArithmeticException();
        }
    }
    return d;
}


//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> v_add(const vector<Integer>& a,const vector<Integer>& b){
   assert(a.size() == b.size());
   /* if (test_arithmetic_overflow) {  // does arithmetic tests
       return(v_add_overflow_check(a,b));
   } */
    size_t i,s=a.size();
    vector<Integer> d(s);
    for (i = 0; i <s; i++) {
        d[i]=a[i]+b[i];
    }
    return d;
}

//---------------------------------------------------------------------------

template<typename Integer>
void v_add_result(vector<Integer>& result, const vector<Integer>& a,const vector<Integer>& b){
   assert(a.size() == b.size() && a.size() == result.size());
   /* if (test_arithmetic_overflow) {  // does arithmetic tests
       return(v_add_overflow_check(a,b));
   } */
    size_t i,s=a.size();
    // vector<Integer> d(s);
    for (i = 0; i <s; i++) {
        result[i]=a[i]+b[i];
    }
    // return d;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer>& v_add_to_mod(vector<Integer>& a, const vector<Integer>& b, const Integer& m) {
//  assert(a.size() == b.size());
    size_t i, s=a.size();
    for (i = 0; i <s; i++) {
//      a[i] = (a[i]+b[i])%m;
        if ((a[i] += b[i]) >= m) {
            a[i] -= m;
        }
    }
    return a;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer>& v_abs(vector<Integer>& v){
    size_t i, size=v.size();
    for (i = 0; i < size; i++) {
        if (v[i]<0) v[i] = Iabs(v[i]);
    }
    return v;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_gcd(const vector<Integer>& v){
    size_t i, size=v.size();
    Integer g=0;
    for (i = 0; i < size; i++) {
        g=gcd(g,v[i]);
        if (g==1) {
            return 1;
        }
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_lcm(const vector<Integer>& v){
    size_t i,size=v.size();
    Integer g=1;
    for (i = 0; i < size; i++) {
        g=lcm(g,v[i]);
        if (g==0) {
            return 0;
        }
    }
    return g;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_make_prime(vector<Integer>& v){
    size_t i, size=v.size();
    Integer g=v_gcd(v);
    if (g!=0) {
        for (i = 0; i < size; i++) {
            v[i] /= g;
        }
    }
    return g;
}


//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> v_scalar_multiplication_two(const vector<Integer>& v, const Integer& scalar){
    size_t i,size=v.size();
    vector<Integer> w(size);
    for (i = 0; i <size; i++) {
        w[i]=v[i]*scalar;
    }
    return w;
}

//---------------------------------------------------------------------------

template<typename Integer>
void v_scalar_division(vector<Integer>& v, const Integer& scalar){
    size_t i,size=v.size();
    for (i = 0; i <size; i++) {
        assert(v[i]%scalar == 0);
        v[i] /= scalar;
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
void v_reduction_modulo(vector<Integer>& v, const Integer& modulo){
    size_t i,size=v.size();
    for (i = 0; i <size; i++) {
        v[i]=v[i]%modulo;
        if (v[i]<0) {
            v[i]=v[i]+modulo;
        }
    }
}

//---------------------------------------------------------------------------

template<typename Integer>
bool v_test_scalar_product(const vector<Integer>& av,const vector<Integer>& bv, const Integer& result, const long& m){
    Integer ans = 0;
    size_t i,n=av.size();
    typename vector<Integer>::const_iterator a=av.begin(),b=bv.begin();

    if( n >= 16 )
    {
        for( i = 0; i < ( n >> 4 ); ++i, a += 16, b += 16 ){
            ans += a[0] * b[0];
            ans += a[1] * b[1];
            ans += a[2] * b[2];
            ans += a[3] * b[3];
            ans %= m;
            ans += a[4] * b[4];
            ans += a[5] * b[5];
            ans += a[6] * b[6];
            ans += a[7] * b[7];
            ans %= m;
            ans += a[8] * b[8];
            ans += a[9] * b[9];
            ans += a[10] * b[10];
            ans += a[11] * b[11];
            ans %= m;
            ans += a[12] * b[12];
            ans += a[13] * b[13];
            ans += a[14] * b[14];
            ans += a[15] * b[15];
            ans %= m;
        }
        n -= i << 4;
    }

    if( n >= 8)
    {
        ans += a[0] * b[0];
        ans += a[1] * b[1];
        ans += a[2] * b[2];
        ans += a[3] * b[3];
        ans %= m;
        ans += a[4] * b[4];
        ans += a[5] * b[5];
        ans += a[6] * b[6];
        ans += a[7] * b[7];
        ans %= m;

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
        ans %= m;

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
        
    ans %= m;

    if (((result-ans) % m)!=0) {
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename T>
vector<T> v_merge(const vector<T>& a, const T& b) {
    size_t s=a.size();
    vector<T> c(s+1);
    for (size_t i = 0; i < s; i++) {
        c[i]=a[i];
    }
    c[s] = b;
    return c;
}

//---------------------------------------------------------------------------

template<typename T>
vector<T> v_merge(const vector<T>& a,const vector<T>& b){
    size_t s1=a.size(), s2=b.size(), i;
    vector<T> c(s1+s2);
    for (i = 0; i < s1; i++) {
        c[i]=a[i];
    }
    for (i = 0; i < s2; i++) {
        c[s1+i]=b[i];
    }
    return c;
}
//---------------------------------------------------------------------------

template<typename T>
vector<T> v_cut_front(const vector<T>& v, size_t size){
    size_t s,k;
    vector<T> tmp(size);
    s=v.size()-size;
    for (k = 0; k < size; k++) {
        tmp[k]=v[s+k];
    }
    return tmp;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<key_t> v_non_zero_pos(const vector<Integer>& v){
    vector<key_t> key;
    size_t size=v.size();
    key.reserve(size);
    for (key_t i = 0; i <size; i++) {
        if (v[i]!=0) {
            key.push_back(i);
        }
    }
    return key;
}

//---------------------------------------------------------------------------

template<typename Integer>
bool v_is_zero(const vector<Integer>& v) {
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] != 0) return false;
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
void v_el_trans(const vector<Integer>& av,vector<Integer>& bv, const Integer& F, const size_t& start){

    size_t i,n=av.size();

    typename vector<Integer>::const_iterator a=av.begin();
    typename vector<Integer>::iterator b=bv.begin();

    a += start;
    b += start;
    n -= start;


    if( n >= 8 )
    {
        for( i = 0; i < ( n >> 3 ); ++i, a += 8, b += 8 ){
            b[0] += F*a[0];
            b[1] += F*a[1];
            b[2] += F*a[2];
            b[3] += F*a[3];
            b[4] += F*a[4];
            b[5] += F*a[5];
            b[6] += F*a[6];
            b[7] += F*a[7];
        }
        n -= i << 3;
    }

    if( n >= 4)
    {
        b[0] += F*a[0];
        b[1] += F*a[1];
        b[2] += F*a[2];
        b[3] += F*a[3];

        n -=4;
        a +=4;
        b +=4;
    }

    if( n >= 2)
    {
        b[0] += F*a[0];
        b[1] += F*a[1];

        n -=2;
        a +=2;
        b +=2;
    }

    if(n>0)
        b[0] += F*a[0];
}

//---------------------------------------------------------------

vector<bool> v_bool_andnot(const vector<bool>& a, const vector<bool>& b) {
    assert(a.size() == b.size());
    vector<bool> result(a);
    for (size_t i=0; i<b.size(); ++i) {
        if (b[i])
            result[i]=false;
    }
    return result;
}


//---------------------------------------------------------------

// computes approximating lattice simplex using the A_n dissection of the unit cube
// q is a rational vector with the denominator in the FIRST component q[0]

template<typename Integer>
void approx_simplex(const vector<Integer>& q, std::list<vector<Integer> >& approx){

    long dim=q.size();
    vector<Integer> quot(dim);
    vector<pair<Integer,size_t> > remain(dim);
    for(long i=0;i<dim;++i){
        quot[i]=q[i]/q[0];          // write q[i]=quot*q[0]+remain
        remain[i].first=q[i]%q[0];  // with 0 <= remain < q[0]
        if(remain[i].first<0){
            remain[i].first+=q[0];
            quot[i]--;
        }
        remain[i].second=i;  // after sorting we must know where elements come from
    }
    

    remain[0].first=q[0];  // helps to avoid special treatment of i=0
    sort(remain.begin(),remain.end()); 
    reverse(remain.begin(),remain.end()); // we sort remain into descending order
    
    /*for(long i=0;i<dim;++i){
        cout << remain[i].first << " " << remain[i].second << endl;
    } */
    
    for(long i=1;i<dim;++i){
        if(remain[i].first<remain[i-1].first)
        {
            approx.push_back(quot);
            // cout << i << " + " << remain[i].first << " + " << quot << endl;
        }
        quot[remain[i].second]++;    
    }
    if(remain[dim-1].first > 0){
        // cout << "E " << quot << endl;
        approx.push_back(quot);
    }

}

} // end namespace libnormaliz
