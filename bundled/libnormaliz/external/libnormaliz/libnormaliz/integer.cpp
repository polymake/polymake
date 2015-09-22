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

#include <algorithm>
#include "libnormaliz/integer.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

bool try_convert(long& ret, const long long& val) {
    if (fits_long_range(val)) {
        ret = val;
        return true;
    }
    return false;
}

bool try_convert(long& ret, const mpz_class& val) {
    if (!val.fits_slong_p()) {
        return false;
    }
    ret = val.get_si();
    return true;
}

bool try_convert(long long& ret, const mpz_class& val) {
    if (val.fits_slong_p()) {
        ret = val.get_si();
        return true;
    }
    if (sizeof(long long)==sizeof(long)) {
        return false;
    }
    mpz_class quot;
    ret = mpz_fdiv_q_ui(quot.get_mpz_t(), val.get_mpz_t(), LONG_MAX); //returns remainder
    if (!quot.fits_slong_p()){
        return false;
    }
    ret += ((long long) quot.get_si())*((long long) LONG_MAX);
    return true;
}

bool try_convert(mpz_class& ret, const long long& val) {
    if (fits_long_range(val)) {
        ret = mpz_class(long(val));
    } else {
        ret = mpz_class(long (val % LONG_MAX)) + mpz_class(LONG_MAX) * mpz_class(long(val/LONG_MAX));
    }
    return true;
}

bool fits_long_range(long long a) {
    return sizeof(long long) == sizeof(long) || (a <= LONG_MAX && a >= LONG_MIN);
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer gcd(const Integer& a, const Integer& b){
    if (a==0) {
        return Iabs<Integer>(b);
    }
    if (b==0) {
        return Iabs<Integer>(a);
    }
    Integer q0,q1,r;
    q0=Iabs<Integer>(a);
    r=Iabs<Integer>(b);
    do {
        q1=r;
        r=q0%q1;
        q0=q1;
    } while (r!=0);
    return q1;
}

template<> mpz_class gcd<mpz_class>(const mpz_class& a, const mpz_class& b) {
    mpz_class g;
    mpz_gcd (g.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
    return g;
}

template void sign_adjust_and_minimize<long long>(const long long& a, const long long& b, long long& d, long long& u, long long&v);
template long long ext_gcd<long long>(const long long& a, const long long& b, long long& u, long long&v);


template <typename Integer>
void sign_adjust_and_minimize(const Integer& a, const Integer& b, Integer& d, Integer& u, Integer&v){
    if(d<0){
        d=-d;
        u=-u;
        v=-v;    
    }
    // cout << u << " " << v << endl;
    if(b==0)
        return;
        
    Integer sign=1;
    if(a<0)
        sign=-1;
    Integer u1= (sign*u) % (Iabs(b)/d);
    if(u1==0)
        u1+=Iabs(b)/d;
    u=sign*u1;
    v=(d-a*u)/b;
}


template <typename Integer>
Integer ext_gcd(const Integer& a, const Integer& b, Integer& u, Integer&v){

    u=1;
    v=0;
    Integer d=a;
    if (b==0) {
        sign_adjust_and_minimize(a,b,d,u,v);
        return(d);
    }
    Integer v1=0;
    Integer v3=b;
    Integer q,t1,t3;
    while(v3!=0){
        q=d/v3;
        t3=d-q*v3;
        t1=u-q*v1;
        u=v1;
        d=v3;
        v1=t1;
        v3=t3;
    }
    sign_adjust_and_minimize(a,b,d,u,v);
    return(d);
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer lcm(const Integer& a, const Integer& b){
    if ((a==0)||(b==0)) {
        return 0;
    }
    else
        return Iabs<Integer>(a*b/gcd<Integer>(a,b));
}

template<> mpz_class lcm<mpz_class>(const mpz_class& a, const mpz_class& b) {
    mpz_class g;
    mpz_lcm (g.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
    return g;
}

//---------------------------------------------------------------------------

template <typename Integer>
size_t decimal_length(Integer a){
    size_t l=1;
    if (a<0) {
        a=-a;
        l++;
    }
    while((a/=10)!=0)
        l++;
    return l;
}

//---------------------------------------------------------------------------

template <typename Integer>
Integer permutations(const size_t& a, const size_t& b){
    unsigned long i;
    Integer P=1;
    for (i = a+1; i <= b; i++) {
        P*=i;
    }
    return P;
}

//---------------------------------------------------------------------------

template<typename Integer> 
Integer permutations_modulo(const size_t& a, const size_t& b, long m) {
    unsigned long i;
    Integer P=1;
    for (i = a+1; i <= b; i++) {
        P*=i; P%=m;
    }
    return P;
}
//---------------------------------------------------------------------------

template<typename Integer>
Integer int_max_value_dual(){
    Integer k=sizeof(Integer)*8-2;  // number of bytes convetred to number of bits
    Integer test=1;
    test = test << k;  // (maximal positive number)/2^k
    return test;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer int_max_value_primary(){
    Integer k=sizeof(Integer)*8-12;  // number of bytes convetred to number of bits
    Integer test=1;
    test = test << k;  // (maximal positive number)/2^k
    // test=0; // 10000;
    return test;
}

//---------------------------------------------------------------------------

template<>
mpz_class int_max_value_dual<mpz_class>(){
    assert(false);
    return 0;
}

//---------------------------------------------------------------------------

template<>
mpz_class int_max_value_primary<mpz_class>(){
    assert(false);
    return 0;
}


//---------------------------------------------------------------------------

template<typename Integer>
void check_range_list(const CandidateList<Integer>& ll){
    check_range_list(ll.Candidates);
}

//---------------------------------------------------------------------------

template<typename Integer>
void check_range_list(const std::list<Candidate<Integer> >& ll){

    if (using_GMP<Integer>())
        return;
        
    typename list<Candidate<Integer> >::const_iterator v=ll.begin();
    
    Integer test=int_max_value_dual<Integer>();
    // cout << "test " << test << endl;
    
    for(;v!=ll.end();++v){
        for(size_t i=0;i<v->values.size();++i)
            if(Iabs(v->values[i])>= test){
            // cout << *v;
            // cout << "i " << i << " " << Iabs((*v)[i]) << endl;
                errorOutput()<<"Vector out of range. Imminent danger of arithmetic overflow.\n";
                throw ArithmeticException();
            }
                    
    }
    

}

//---------------------------------------------------------------------------
 template<typename Integer>
void minimal_remainder(const Integer& a, const Integer&b, Integer& quot, Integer& rem) {

    quot=a/b;
    rem=a-quot*b;
    if(rem==0)
        return;
    if(2*Iabs(rem)>Iabs(b)){
        if((rem<0 && b>0) || (rem >0 && b<0)){                
            rem+=b;
            quot--;
        }
        else{
            rem-=b;
            quot++;                
        }
    }
}



}
