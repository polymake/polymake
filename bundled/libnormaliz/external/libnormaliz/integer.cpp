/*
 * Normaliz
 * Copyright (C) 2007-2013  Winfried Bruns, Bogdan Ichim, Christof Soeger
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

}
