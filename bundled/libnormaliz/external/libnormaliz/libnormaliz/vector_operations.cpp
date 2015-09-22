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
#include <list>

#include "libnormaliz/integer.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/matrix.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

//---------------------------------------------------------------------------

template<typename Integer>
Integer v_scalar_product(const vector<Integer>& av,const vector<Integer>& bv){
    //loop stretching ; brings some small speed improvement

    Integer ans = 0;
    size_t i,n=av.size();


#ifdef __MIC__
    // this version seems to be better vectorizable on the mic
    for (i=0; i<n; ++i)
        ans += av[i]*bv[i];

#else // __MIC__
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
#endif // __MIC__
        
    if(!check_range(ans)){
		#pragma omp atomic
		GMP_scal_prod++;
    
        // cout << "av " << av;
        // cout << "bv " << bv;   
        vector<mpz_class> mpz_a(av.size()), mpz_b(bv.size());
        convert(mpz_a, av);
        convert(mpz_b, bv);
        convert(ans, v_scalar_product(mpz_a,mpz_b));
    }
        
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

/* 
 * template<typename Integer>
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
*/

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> v_add(const vector<Integer>& a,const vector<Integer>& b){
   assert(a.size() == b.size());
    size_t i,s=a.size();
    vector<Integer> d(s);
    for (i = 0; i <s; i++) {
        d[i]=a[i]+b[i];
    }
    return d;
}

//---------------------------------------------------------------------------

template<typename Integer>
void v_add_result(vector<Integer>& result, const size_t s, const vector<Integer>& a,const vector<Integer>& b){
   assert(a.size() == b.size() && a.size() == result.size());
    size_t i;
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
vector<Integer> v_abs_value(vector<Integer>& v){
    size_t i, size=v.size();
    vector<Integer> w=v;
    for (i = 0; i < size; i++) {
        if (v[i]<0) w[i] = Iabs(v[i]);
    }
    return w;
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
bool v_scalar_mult_mod_inner(vector<Integer>& w, const vector<Integer>& v, const Integer& scalar, const Integer& modulus){
    size_t i,size=v.size();
    Integer test;
    for (i = 0; i <size; i++) {
        test=v[i]*scalar;
        if(!check_range(test)){
            return false;
        }
        w[i]=test % modulus;
        if(w[i]<0)
            w[i]+=modulus;
    }
    return true;
}

//---------------------------------------------------------------------------

template<typename Integer>
vector<Integer> v_scalar_mult_mod(const vector<Integer>& v, const Integer& scalar, const Integer& modulus){
    
    vector<Integer> w(v.size());
    if(v_scalar_mult_mod_inner(w,v,scalar,modulus))
        return w;
    
    #pragma omp atomic
    GMP_scal_prod++;
    vector<mpz_class> x,y(v.size());
    convert(x,v);
    v_scalar_mult_mod_inner(y,x,convertTo<mpz_class>(scalar),convertTo<mpz_class>(modulus));
    return convertTo<vector<Integer>>(y);       
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

// swaps entry i and j of the vector<bool> v
void v_bool_entry_swap(vector<bool>& v, size_t i, size_t j) {
    if (v[i] != v[j]) {
        v[i].flip();
        v[j].flip();
    }
}


//---------------------------------------------------------------

// computes approximating lattice simplex using the A_n dissection of the unit cube
// q is a rational vector with the denominator in the FIRST component q[0]

template<typename Integer>
void approx_simplex(const vector<Integer>& q, std::list<vector<Integer> >& approx, const long k){
	
	//cout << "approximate the point " << q;
    long dim=q.size();
    long l=k;
    //if (k>q[0]) l=q[0]; // approximating on level q[0](=grading) is the best we can do
    // TODO in this case, skip the rest and just approximate on q[0]
    Matrix<Integer> quot =  Matrix<Integer>(l,dim);
    Matrix<Integer> remain=Matrix<Integer>(l,dim);
    for(long j=0;j<l;j++){
	    for(long i=0;i<dim;++i){
	        quot[j][i]=(q[i]*(j+1))/q[0];          // write q[i]=quot*q[0]+remain
	        //quot[j][0] = 1;
	        remain[j][i]=(q[i]*(j+1))%q[0];  // with 0 <= remain < q[0]
	        if(remain[j][i]<0){
	            remain[j][i]+=q[0];
	            quot[j][i]--;
	        }
	          
	    }
	    v_make_prime(quot[j]);
	    remain[j][0]=q[0];  // helps to avoid special treatment of i=0
	}
	// choose best level
	//cout << "this is the qout matrix" << endl;
	//quot.pretty_print(cout);
	//cout << "this is the remain matrix" << endl;
	//remain.pretty_print(cout);
	long best_level=l-1;
	vector<long> nr_zeros(l);
	for(long j=l-1;j>=0;j--){
		for(long i=0;i<dim;++i){
			if(remain[j][i]==0) nr_zeros[j]++;
		}
		if (nr_zeros[j]>nr_zeros[best_level]) best_level=j;
	}
	//cout << "the best level is " << (best_level+1) << endl;
	//now we proceed as before
	vector<pair<Integer,size_t>> best_remain(dim);
	for(long i=0;i<dim;i++){
		best_remain[i].first = remain[best_level][i];
		best_remain[i].second = i; // after sorting we must lnow where elements come from
	}
	
    sort(best_remain.begin(),best_remain.end()); 
    reverse(best_remain.begin(),best_remain.end()); // we sort remain into descending order
    
    /*for(long i=0;i<dim;++i){
        cout << remain[i].first << " " << remain[i].second << endl;
    } */
    
    for(long i=1;i<dim;++i){
        if(best_remain[i].first<best_remain[i-1].first)
        {
            approx.push_back(quot[best_level]);
            //cout << "add the point " << quot[best_level];
            // cout << i << " + " << remain[i].first << " + " << quot << endl;
        }
        quot[best_level][best_remain[i].second]++;    
    }
    if(best_remain[dim-1].first > 0){
        // cout << "E " << quot << endl;
        approx.push_back(quot[best_level]);
        //cout << "add the point " << quot[best_level];
    }

}

vector<key_t> identity_key(size_t n){
    vector<key_t> key(n);
    for(size_t k=0;k<n;++k)
        key[k]=k;
    return key;
}

//---------------------------------------------------------------
// Sorting

template <typename T>
void order_by_perm(vector<T>& v, const vector<key_t>& permfix){
    
    vector<key_t> perm=permfix; // we may want to use permfix a second time
    vector<key_t> inv(perm.size());
    for(key_t i=0;i<perm.size();++i)
        inv[perm[i]]=i;
    for(key_t i=0;i<perm.size();++i){
        key_t j=perm[i];
        swap(v[i],v[perm[i]]);        
        swap(perm[i],perm[inv[i]]);        
        swap(inv[i],inv[j]);                
    }
}

// vector<bool> is special
template <>
void order_by_perm(vector<bool>& v, const vector<key_t>& permfix){
    
    vector<key_t> perm=permfix; // we may want to use permfix a second time
    vector<key_t> inv(perm.size());
    for(key_t i=0;i<perm.size();++i)
        inv[perm[i]]=i;
    for(key_t i=0;i<perm.size();++i){
        key_t j=perm[i];
        // v.swap(v[i],v[perm[i]]);
        v_bool_entry_swap(v,i,perm[i]);
        swap(perm[i],perm[inv[i]]);        
        swap(inv[i],inv[j]);                
    }
}



template long      v_make_prime(vector<long     >&);
template long long v_make_prime(vector<long long>&);
template mpz_class v_make_prime(vector<mpz_class>&);

template void v_add_result<long     >(vector<long     >&, size_t, const vector<long     >&, const vector<long     >&);
template void v_add_result<long long>(vector<long long>&, size_t, const vector<long long>&, const vector<long long>&);
template void v_add_result<mpz_class>(vector<mpz_class>&, size_t, const vector<mpz_class>&, const vector<mpz_class>&);

} // end namespace libnormaliz
