/*
 * nmzIntegrate
 * Copyright (C) 2012-2014  Winfried Bruns, Christof Soeger
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

#ifndef NMZ_INTEGRATE_H
#define NMZ_INTEGRATE_H

#ifdef NMZ_COCOA

#include "CoCoA/library.H"
using namespace CoCoA;

#include <fstream>
#include <sstream>
#include<string>
#include <gmpxx.h>

#include <boost/dynamic_bitset.hpp>

#include "libnormaliz/libnormaliz.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/matrix.h"

#include "libnormaliz/my_omp.h"

using namespace std;

namespace libnormaliz {
    
typedef unsigned int key_type;

bool verbose_INT;

struct SIMPLINEXDATA_INT{                        // local data of excluded faces
        boost::dynamic_bitset<> GenInFace;   // indicator for generators of simplex in face 
        long mult;                           // multiplicity of this face
        size_t card;                                // the cardinality of the face
        bool done;                           // indicates that this face has been done for a given offset
        vector<long> denom;
        vector<long> degrees;
        vector<long> key;
};

class ourFactorization{
public:

    vector<RingElem> myFactors;
    vector<long> myMultiplicities;
    RingElem myRemainingFactor;
    
    ourFactorization(const vector<RingElem>& myFactors, 
             const  vector<long>& myMultiplicities, const RingElem& myRemainingFactor);
    ourFactorization(const factorization<RingElem>& FF);

};
// end class

class CyclRatFunct {
// class for rational functions whose denominator is a product
// of cyclotomic polynomials
// We work with denominators that are products of factors 1-t^i
// which is of course equivalent
// the numerator is a polynomial in its ring
// the denominator is an integer vector that at index i
// gives the multiplicity of 1-t^i in the denominator
// (the entry at index 0 is not used and must always be equal to 0)
public:

    RingElem num;
    vector<long> denom;

    void extendDenom(const vector<long>& target);
    void addCRF(const CyclRatFunct& r);
    void multCRF(const CyclRatFunct& r);
    void simplifyCRF();
    void set2(const RingElem& f, const vector<long>& d);
    void set2(const RingElem& f);
    void showCRF();
    void showCoprimeCRF();
    CyclRatFunct(const RingElem& c);
    CyclRatFunct(const RingElem& c,const vector<long>& d);

};
//class end *****************************************************************

// manipulation of denominators 
vector<long> lcmDenom(const vector<long>& df, const vector<long>& dg);
vector<long> prodDenom(const vector<long>& df, const vector<long>& dg);
vector<long> degrees2denom(const vector<long>& d);
vector<long> denom2degrees(const vector<long>& d);
RingElem denom2poly(const SparsePolyRing& P, const vector<long>& d);
vector<long> makeDenom(long k,long n);


RingElem processInputPolynomial(const string& poly_as_string, const SparsePolyRing& R, const SparsePolyRing& RZZ,
                vector<RingElem>& resPrimeFactors, vector<RingElem>& resPrimeFactorsNonhom, vector<long>& resMultiplicities,
                RingElem& remainingFactor, bool& homogeneous,const bool& do_leadCoeff);

//  conversion from CoCoA types to GMP
mpz_class mpz(const BigInt& B) {
    return(mpz_class(mpzref(B))); 
}

mpq_class mpq(const BigRat& B) {
    return(mpq_class(mpqref(B)));  
}

mpz_class ourFactorial(const long& n){
    mpz_class fact=1;
    for(long i=1;i<=n;++i)
        fact*=i;
    return(fact);
}

}  //end namespace libnormaliz

#endif //NMZ_COCOA

#endif // NMZ_INTEGRATE_H

