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
#include "integer.h"
#include "limits.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;



bool fits_long_range(long long a) {
    //TODO make it return true without check when ranges are the same
    return ( a <= LONG_MAX && a >= LONG_MIN);
}

mpz_class to_mpz(long a) {
    return mpz_class(a);
}
mpz_class to_mpz(long long a) {
    if (fits_long_range(a)) {
        return mpz_class(long(a));
    } else {
        static long long mod = LONG_MAX;
        // to ensure the following % and / are done in long long
        return mpz_class(long (a % mod)) + mpz_class(LONG_MAX) * to_mpz(long(a/mod));
    }
}




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
Integer int_max_value_half(){
    Integer k=sizeof(Integer)*8-2;  // number of bytes convetred to number of bits
    Integer test=1;
    test = test << k;  // (maximal positive number)/2
    return test;
}

//---------------------------------------------------------------------------

template<>
mpz_class int_max_value_half<mpz_class>(){
    assert(false);
    return 0;
}

//---------------------------------------------------------------------------

template<typename Integer>
void check_range(CandidateList<Integer>& ll){
    check_range(ll.Candidates);
}

//---------------------------------------------------------------------------

template<typename Integer>
void check_range(const std::list<Candidate<Integer> >& ll){

    if (!test_arithmetic_overflow)
        return;
        
    typename list<Candidate<Integer> >::const_iterator v=ll.begin();
    
    Integer test=int_max_value_half<Integer>();
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

}
