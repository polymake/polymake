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
#include <sstream>
#include <math.h>
#include "libnormaliz/integer.h"
#include "libnormaliz/convert.h"
#include "libnormaliz/vector_operations.h"
#include "libnormaliz/reduction.h"

//---------------------------------------------------------------------------

namespace libnormaliz {
using namespace std;

bool try_convert(mpz_class& ret, const mpq_class& val) {
    assert(false); // must never be used
    return false;
}

bool try_convert(mpq_class& ret, const mpz_class& val) {
    assert(false); // must never be used
    return false;
}

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

bool try_convert(nmz_float& ret, const long& val){
    ret= (nmz_float) val;
    return true;
}

bool try_convert(nmz_float& ret, const mpz_class& val){    
    ret=val.get_d();
    return true;
}

bool try_convert(mpz_class& ret, const nmz_float& val){    
    ret=mpz_class(val);
    return true;
}

bool try_convert(nmz_float& ret, const long long& val){
    ret= (nmz_float) val;
    return true;
}

bool try_convert(long& ret, const nmz_float& val){
    mpz_class bridge;
    if(!try_convert(bridge,val))
        return false;
    return try_convert(ret,bridge);
}

bool try_convert(long long& ret, const nmz_float& val){
    mpz_class bridge;
    if(!try_convert(bridge,val))
        return false;
    return try_convert(ret,bridge);
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

template <>
nmz_float gcd(const nmz_float& a, const nmz_float& b){
    if (a==0 && b==0)
        return 0;
    return 1.0;
}

template<> mpz_class gcd<mpz_class>(const mpz_class& a, const mpz_class& b) {
    mpz_class g;
    mpz_gcd (g.get_mpz_t(), a.get_mpz_t(), b.get_mpz_t());
    return g;
}

template long gcd<long>(const long& a, const long& b);
template nmz_float gcd<nmz_float>(const nmz_float& a, const nmz_float& b);
template long long gcd<long long>(const long long& a, const long long& b);

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

template long lcm<long>(const long& a, const long& b);
template long long lcm<long long>(const long long& a, const long long& b);


//---------------------------------------------------------------------------

template<typename Integer>
Integer int_max_value_dual(){
    Integer k=sizeof(Integer)*8-2;  // number of bytes convetred to number of bits
    Integer test=1;
    test = test << k;  // 2^k
    return test;
}



bool int_max_value_dual_long_computed=false;

template<>
long int_max_value_dual(){
    static long max_value;
    
    if(int_max_value_dual_long_computed)
        return max_value;
    
    long k=sizeof(long)*8-2;  // number of bytes convetred to number of bits
    long test=1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    max_value=test;
    int_max_value_dual_long_computed=true;
    return test;
}

bool int_max_value_dual_long_long_computed=false;

template<>
long long int_max_value_dual(){    
    static long long max_value;
    
    if(int_max_value_dual_long_long_computed)
        return max_value;
    
    long long k=sizeof(long long)*8-2;  // number of bytes convetred to number of bits
    long long test=1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    max_value=test;
    int_max_value_dual_long_long_computed=true;
    return test;
}

//---------------------------------------------------------------------------

template<>
mpz_class int_max_value_dual<mpz_class>(){
    assert(false);
    return 0;
}

//---------------------------------------------------------------------------

template<typename Integer>
Integer int_max_value_primary(){
    Integer k=sizeof(Integer)*8-12;  // number of bytes convetred to number of bits
    Integer test=1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    return test;
}

bool int_max_value_primary_long_computed=false;

template<>
long int_max_value_primary(){
    static long max_value;
    
    if(int_max_value_primary_long_computed)
        return max_value;
    
    long k=sizeof(long)*8-12;  // number of bytes convetred to number of bits
    long test=1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    int_max_value_primary_long_computed=true;
    max_value=test;
    return test;
}

bool int_max_value_primary_long_long_computed=false;

template<>
long long int_max_value_primary(){    
    static long long max_value;
    
    if(int_max_value_primary_long_long_computed)
        return max_value;
    
    long long k=sizeof(long long)*8-12;  // number of bytes convetred to number of bits
    long long test=1;
    test = test << k;  // 2^k
    // test=0; // 10000;
    max_value=test;
    int_max_value_primary_long_long_computed=true;
    return test;
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

template void check_range_list(const CandidateList<long>&);
template void check_range_list(const CandidateList<long long>&);
template void check_range_list(const CandidateList<mpz_class>&);

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
                throw ArithmeticException("Vector entry out of range. Imminent danger of arithmetic overflow.");
            }
                    
    }
    

}

//---------------------------------------------------------------------------

mpq_class dec_fraction_to_mpq(string s){

    size_t skip=0;               // skip leading spaces
    for(;skip<s.size();++skip){
            if(!isspace(s[skip]))
                break;
    }
    s=s.substr(skip);

    mpz_class sign=1;
    if(s[0]=='+')
        s=s.substr(1);
    else
        if(s[0]=='-'){
            s=s.substr(1);
            sign=-1;
    }
    
    if(s[0]=='+' || s[0]=='-')
            throw BadInputException("Error in decimal fraction "+s);
    
    string int_string,frac_string,exp_string;
    size_t frac_part_length=0;
    size_t pos_point=s.find(".");
    size_t pos_E=s.find("e");
    if(pos_point!=string::npos){
        int_string=s.substr(0,pos_point);
        if(pos_E!=string::npos){
            frac_part_length=pos_E-(pos_point+1);
        }
        else
            frac_part_length=s.size()-(pos_point+1);
        frac_string=s.substr(pos_point+1,frac_part_length);
        if(frac_string[0]=='+' || frac_string[0]=='-')
            throw BadInputException("Error in decimal fraction "+s);
    }
    else
        int_string=s.substr(0,pos_E);
    if(pos_E!=string::npos)
        exp_string=s.substr(pos_E+1,s.size()-(pos_E+1));
    
    /* cout << "int  " << int_string << endl;
    cout << "frac " << frac_string << endl;
    cout << "exp  " << exp_string << endl; */
    
    // remove leading 0
    while(int_string[0]=='0')
        int_string=int_string.substr(1);
    while(frac_string[0]=='0')
        frac_string=frac_string.substr(1);
    
    mpq_class int_part, frac_part, exp_part;
    if(!int_string.empty())
        int_part=mpz_class(int_string);

    if(pos_E==0)
        int_part=1;
    
    // cout << "int_part " << int_part << endl;
    
    mpz_class den=1;
    if(!frac_string.empty()){
        frac_part=mpz_class(frac_string);
        for(size_t i=0;i<frac_part_length;++i)
            den*=10;        
    }
    // cout << "frac_part " << frac_part << endl;
    mpq_class result=int_part;
    if(frac_part!=0)
        result+=frac_part/den;
    if(!exp_string.empty()){
        mpz_class expo(exp_string); // we take mpz_class because it has better error checking
        // long expo=stol(exp_string);        
        mpz_class abs_expo=Iabs(expo);
        // cout << "expo " << expo << endl;
        mpz_class factor=1;
        for(long i=0;i< abs_expo;++i)
            factor*=10;
        if(expo>=0)
            result*=factor;
        else
            result/=factor;
    }
    /* cout <<" result " << sign*result << endl;
    cout << "==========" << endl; */
    return sign*result;
}

//----------------------------------------------------------------------
// the next function produce an integer quotient and determine whether
// there is a remainder

bool int_quotient(long& Quot, const long & Num, const long& Den){
    
    Quot=Iabs(Num)/Iabs(Den);
    return Quot*Iabs(Den)!=Iabs(Num);    
}

bool int_quotient(long long& Quot, const long long& Num, const long long& Den){
    
    Quot=Iabs(Num)/Iabs(Den);
    return Quot*Iabs(Den)!=Iabs(Num);    
}


bool int_quotient(mpz_class& Quot, const mpz_class& Num, const mpz_class& Den){
    
    Quot=Iabs(Num)/Iabs(Den);
    return Quot*Iabs(Den)!=Iabs(Num);    
}

bool int_quotient(long long& Quot, const mpz_class& Num, const mpz_class& Den){
    
    mpz_class mpz_Quot=(Iabs(Num)/Iabs(Den));
    convert(Quot,mpz_Quot);
    return mpz_Quot*Iabs(Den)!=Iabs(Num);    
}

template<typename IntegerRet>
bool int_quotient(IntegerRet& Quot, const nmz_float& Num, const nmz_float& Den){
   
    nmz_float FloatQuot=Iabs(Num)/Iabs(Den); // cout << "FF " << FloatQuot << endl;
    nmz_float IntQuot=trunc(FloatQuot+nmz_epsilon);      // cout << "II " << IntQuot << endl;
    Quot=convertTo<IntegerRet>(IntQuot);     // cout << "QQ " <<  Quot << endl;
    return FloatQuot-IntQuot > nmz_epsilon;    
}

template bool int_quotient<long>(long& , const nmz_float&, const nmz_float&);
template bool int_quotient<long long>(long long& , const nmz_float&, const nmz_float&);
template bool int_quotient<mpz_class>(mpz_class& , const nmz_float&, const nmz_float&);
template bool int_quotient<nmz_float>(nmz_float& , const nmz_float&, const nmz_float&);

//----------------------------------------------------------------------

mpz_class floor(const mpq_class& q){
        mpz_class num=q.get_num();
        mpz_class den=q.get_den();
        mpz_class ent=num/den;
        if(num<0 && den*ent!=num)
            ent--;
        return ent;
}

mpz_class ceil(const mpq_class& q){
        mpz_class num=q.get_num();
        mpz_class den=q.get_den();
        mpz_class ent=num/den;
        if(num>0 && den*ent!=num)
            ent++;
        return ent;
}


mpz_class round(const mpq_class& q){
    mpq_class work;
    if(q>=0){
        work=q-mpq_class(1,2);
        return ceil(work);
    }
    work=q+mpq_class(1,2);
    return floor(work);
}

template<typename Integer> 
mpz_class nmz_factorial(Integer n){
    assert(n >=0);
    mpz_class f=1;
    long nlong=convertTo<long>(n);
    for(long i=1;i<=nlong;++i)
        f*=i;
    return f;
}

template mpz_class nmz_factorial<long>(long);
template mpz_class nmz_factorial<long long>(long long);
template mpz_class nmz_factorial<mpz_class>(mpz_class);

template<typename Integer> 
mpz_class nmz_binomial (Integer n, Integer k){
    if(k>n)
        return 0;
    return nmz_factorial(n)/nmz_factorial(k);
}

template<typename Integer> 
mpz_class nmz_binomial (Integer n, Integer k);

template mpz_class nmz_binomial<long>(long, long);
template mpz_class nmz_binomial<long long>(long long, long long);
template mpz_class nmz_binomial<mpz_class>(mpz_class, mpz_class);

nmz_float mpq_to_nmz_float(const mpq_class& val){
    mpz_class bound=1;
    for(size_t i=0;i<60; ++i)
        bound*=10;
    mpz_class gmp_num=val.get_num(),gmp_den=val.get_den();
    while(Iabs(gmp_num) > bound && Iabs(gmp_den) > bound){
            gmp_num/=10;
            gmp_den/=10;
    }
    nmz_float num,den;
    convert(num,gmp_num);
    convert(den,gmp_den);
    return num/den;
}


} //end namespace libnormaliz
