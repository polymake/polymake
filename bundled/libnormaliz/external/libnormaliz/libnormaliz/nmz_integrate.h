/*
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef LIBNORMALIZ_NMZ_INTEGRATE_H
#define LIBNORMALIZ_NMZ_INTEGRATE_H

#ifdef NMZ_COCOA

#include "CoCoA/library.H"

#include <fstream>
#include <sstream>
#include <string>
#include <gmpxx.h>

#include "libnormaliz/dynamic_bitset.h"
#include "libnormaliz/general.h"
#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/matrix.h"

namespace libnormaliz {

using namespace CoCoA;

using namespace std;

typedef unsigned int key_type;

extern bool verbose_INT;

struct SIMPLINEXDATA_INT {     // local data of excluded faces
    dynamic_bitset GenInFace;  // indicator for generators of simplex in face
    long mult;                 // multiplicity of this face
    size_t card;               // the cardinality of the face
    bool done;                 // indicates that this face has been done for a given offset
    vector<long> denom;
    vector<long> degrees;
    vector<long> key;
};

class ourFactorization {
   public:
    vector<RingElem> myFactors;
    vector<long> myMultiplicities;
    RingElem myRemainingFactor;

    ourFactorization(const vector<RingElem>& myFactors, const vector<long>& myMultiplicities, const RingElem& myRemainingFactor);
    ourFactorization(const factorization<RingElem>& FF);
    ourFactorization();
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
    CyclRatFunct(const RingElem& c, const vector<long>& d);
};
// class end *****************************************************************

// manipulation of denominators
vector<long> lcmDenom(const vector<long>& df, const vector<long>& dg);
vector<long> prodDenom(const vector<long>& df, const vector<long>& dg);
vector<long> degrees2denom(const vector<long>& d);
vector<long> denom2degrees(const vector<long>& d);
RingElem denom2poly(const SparsePolyRing& P, const vector<long>& d);
vector<long> makeDenom(long k, long n);

//  conversion from CoCoA types to GMP
inline mpz_class mpz(const BigInt& B) {
    return (mpz_class(mpzref(B)));
}

inline mpq_class mpq(const BigRat& B) {
    return (mpq_class(mpqref(B)));
}

inline mpz_class ourFactorial(const long& n) {
    mpz_class fact = 1;
    for (long i = 1; i <= n; ++i)
        fact *= i;
    return (fact);
}

ourFactorization::ourFactorization(const vector<RingElem>& myFactors,
                                   const vector<long>& myMultiplicities,
                                   const RingElem& myRemainingFactor) {
    this->myFactors = myFactors;
    this->myMultiplicities = myMultiplicities;
    this->myRemainingFactor = myRemainingFactor;
}

ourFactorization::ourFactorization() {
}
/*
ourFactorization::ourFactorization(const factorization<RingElem>& FF) {
    ourFactorization(FF.myFactors(), FF.myMultiplicities(), FF.myRemainingFactor());
}

RingElem binomial(const RingElem& f, long k)
// computes binomial coefficient (f choose k)
{
    const SparsePolyRing& P = owner(f);
    RingElem g(P);
    g = 1;
    for (int i = 0; i < k; i++)
        g *= (f - i) / (i + 1);
    return (g);
}
*/

RingElem ascFact(const RingElem& f, long k)
// computes (f+1)*...*(f+k)
{
    const SparsePolyRing& P = owner(f);
    RingElem g(P);
    g = 1;
    for (int i = 0; i < k; i++)
        g *= (f + i + 1);
    return (g);
}

/*
RingElem descFact(const RingElem& f, long k)
// computes f*(f-1)*...*(f-k+1)
{
    const SparsePolyRing& P = owner(f);
    RingElem g(P);
    g = 1;
    for (int i = 0; i < k; i++)
        g *= (f - i);
    return (g);
}
*/

bool compareLength(const RingElem& p, const RingElem& q) {
    return (NumTerms(p) > NumTerms(q));
}

vector<RingElem> ourCoeffs(const RingElem& F, const long j) {
    // our version of expanding a poly nomial wrt to indeterminate j
    // The return value is the vector of coefficients of x[j]^i
    vector<RingElem> c;
    const SparsePolyRing& P = owner(F);
    RingElem x = indets(P)[j];
    if (F == 0) {
        c.push_back(zero(P));
        return (c);
    }

    vector<long> v(NumIndets(P));
    long k, cs;

    SparsePolyIter i = BeginIter(F);
    for (; !IsEnded(i); ++i) {
        exponents(v, PP(i));
        k = v[j];
        cs = c.size();
        if (k > cs - 1)
            c.resize(k + 1, zero(P));
        v[j] = 0;
        // c[k]+=monomial(P,coeff(i),v);
        PushBack(c[k], coeff(i), v);
    }
    return (c);
}

RingElem mySubstitution(const RingElem& F, const vector<RingElem>& w) {
    const SparsePolyRing& R = owner(F);
    RingElem G(zero(R));
    RingElem H(one(R));
    vector<long> v(NumIndets(R));
    vector<long> Z(NumIndets(R));

    SparsePolyIter i = BeginIter(F);
    for (; !IsEnded(i); ++i) {
        exponents(v, PP(i));
        H = zero(R);
        PushBack(H, coeff(i), Z);
        for (size_t j = 0; j < v.size(); ++j)
            H *= power(w[j], v[j]);
        G += H;
    }
    return G;
}

template <typename Number>
vector<long> MxV(const vector<vector<Number> >& M, vector<Number> V) {
    // matrix*vector
    vector<Number> P(M.size());
    for (size_t i = 0; i < M.size(); ++i) {
        long s = 0;
        for (size_t j = 0; j < V.size(); ++j)
            s += M[i][j] * V[j];
        P[i] = s;
    }
    return (P);
}

template <typename Number>
vector<RingElem> VxM(const vector<RingElem>& V, const vector<vector<Number> >& M) {
    // vector*matrix
    const SparsePolyRing& R = owner(V[0]);
    RingElem s(zero(R));
    vector<RingElem> P(M[0].size(), zero(R));
    for (size_t j = 0; j < M[0].size(); ++j) {
        s = 0;
        for (size_t i = 0; i < M.size(); ++i)
            s += V[i] * M[i][j];
        P[j] = s;
    }
    return (P);
}

/*
RingElem affineLinearSubstitution(const RingElem& F,const vector<vector<long> >& A,
                     const vector<long>& b, const long& denom){
// NOT IN USE
    size_t i;
    const SparsePolyRing& R=owner(F);
    size_t m=A.size();
    // long n=A[0].size();
    vector<RingElem> v(m,zero(R));
    RingElem g(zero(R));

    for(i=0;i<m;i++)
    {
        g=b[i];
        g=g/denom;
        v[i]=g+indets(R)[i+1];
    }
    vector<RingElem> w=VxM(v,A);
    vector<RingElem> w1(w.size()+1,zero(R));
    w1[0]=indets(R)[0];
    for(i=1;i<w1.size();++i)
        w1[i]=w[i-1];

    RingHom phi=PolyAlgebraHom(R,R,w1);
    RingElem G=phi(F);
    return(G);
}
*/

bool DDD = false;

vector<long> shiftVars(const vector<long>& v, const vector<long>& key) {
    // selects components of v and reorders them according to key
    vector<long> w(v.size(), 0);
    for (size_t i = 0; i < key.size(); ++i) {
        w[i] = v[key[i]];
    }
    return (w);
}

void makeLocalDegreesAndKey(const dynamic_bitset& indicator,
                            const vector<long>& degrees,
                            vector<long>& localDeg,
                            vector<long>& key) {
    localDeg.clear();
    key.clear();
    key.push_back(0);
    for (size_t i = 0; i < indicator.size(); ++i)
        if (indicator.test(i))
            key.push_back(i + 1);
    for (size_t i = 0; i < key.size() - 1; ++i)
        localDeg.push_back(degrees[key[i + 1] - 1]);
}

void makeStartEnd(const vector<long>& localDeg, vector<long>& St, vector<long>& End) {
    vector<long> denom = degrees2denom(localDeg);  // first we must find the blocks of equal degree
    if (denom.size() == 0)
        return;
    St.push_back(1);
    for (size_t i = 0; i < denom.size(); ++i)
        if (denom[i] != 0) {
            End.push_back(St[St.size() - 1] + denom[i] - 1);
            if (i < denom.size() - 1)
                St.push_back(End[End.size() - 1] + 1);
        }
    /* if(St.size()!=End.size()){
        for (size_t i=0;i<denom.size(); ++i){
            verboseOutput() << denom.size() << endl;
                    verboseOutput() << denom[i] << " ";
            verboseOutput() << endl;
        }
    }*/
}

vector<long> orderExposInner(vector<long>& vin, const vector<long>& St, vector<long>& End) {
    vector<long> v = vin;
    long p, s, pend, pst;
    bool ordered;
    if (St.size() != End.size()) {
        verboseOutput() << St.size() << " " << End.size() << " " << vin.size() << endl;
        verboseOutput() << St[0] << endl;
        for (size_t i = 0; i < vin.size(); ++i) {
            verboseOutput() << vin[i] << " ";
        }
        verboseOutput() << endl;
        assert(false);
    }
    for (size_t j = 0; j < St.size(); ++j) {  // now we go over the blocks
        pst = St[j];
        pend = End[j];
        while (1) {
            ordered = true;
            for (p = pst; p < pend; ++p) {
                if (v[p] < v[p + 1]) {
                    ordered = false;
                    s = v[p];
                    v[p] = v[p + 1];
                    v[p + 1] = s;
                }
            }
            if (ordered)
                break;
            pend--;
        }
    }
    return (v);
}

RingElem orderExpos(const RingElem& F, const vector<long>& degrees, const dynamic_bitset& indicator, bool compactify) {
    // orders the exponent vectors v of the terms of F
    // the exponents v[i] and v[j], i < j,  are swapped if
    // (1) degrees[i]==degrees[j] and (2) v[i] < v[j]
    // so that the exponents are descending in each degree block
    // the ordered exponent vectors are inserted into a map
    // and their coefficients are added
    // at the end the polynomial is rebuilt from the map
    // If compactify==true, the exponents will be shifted to the left in order to keep the correspondence
    // of variables to degrees
    // compactification not used at present (occurs only in restrictToFaces)

    const SparsePolyRing& P = owner(F);
    vector<long> v(NumIndets(P));
    vector<long> key, localDeg;
    key.reserve(v.size() + 1);
    localDeg.reserve(degrees.size() + 1);

    if (compactify) {
        makeLocalDegreesAndKey(indicator, degrees, localDeg, key);
    }
    else {
        localDeg = degrees;
    }

    vector<long> St, End;
    makeStartEnd(localDeg, St, End);

    // now the main job

    map<vector<long>, RingElem> orderedMons;  // will take the ordered exponent vectors

    SparsePolyIter mon = BeginIter(F);  // go over the given polynomial
    for (; !IsEnded(mon); ++mon) {
        exponents(v, PP(mon));  // this function gives the exponent vector back as v
        if (compactify)
            v = shiftVars(v, key);
        v = orderExposInner(v, St, End);
        auto ord_mon = orderedMons.find(v);  // insert into map or add coefficient
        if (ord_mon != orderedMons.end()) {
            ord_mon->second += coeff(mon);
        }
        else {
            orderedMons.insert(pair<vector<long>, RingElem>(v, coeff(mon)));
        }
    }

    // now we must reconstruct the polynomial
    // we use that the natural order of vectors in C++ STL is inverse
    // to lex. Therefore push_front

    RingElem r(zero(P));
    // JAA   verboseOutput() << "Loop start " << orderedMons.size() <<  endl;
    // JAA   size_t counter=0;
    for (const auto& ord_mon : orderedMons) {
        // JAA       verboseOutput() << counter++ << ord_mon.first << endl;
        // JAA       try {
        PushFront(r, ord_mon.second, ord_mon.first);
        // JAA        }
        // JAA       catch(const std::exception& exc){verboseOutput() << "Caught exception: " << exc.what() << endl;}
    }
    // JAA    verboseOutput() << "Loop end" << endl;
    return (r);
}

void restrictToFaces(const RingElem& G,
                     RingElem& GOrder,
                     vector<RingElem>& GRest,
                     const vector<long> degrees,
                     const vector<SIMPLINEXDATA_INT>& inExSimplData) {
    // Computesd the restrictions of G to the faces in inclusion-exclusion.
    // All terms are simultaneously compactified and exponentwise ordered
    // Polynomials returned in GRest
    // Ordering is also applied to G itself, returned in GOrder
    // Note: degrees are given for the full simplex. Therefore "local" degreees must be made
    // (depend only on face and not on offset, but generation here is cheap)

    const SparsePolyRing& P = owner(G);

    vector<long> v(NumIndets(P));
    vector<long> w(NumIndets(P));
    vector<long> localDeg;
    localDeg.reserve(v.size());
    size_t dim = NumIndets(P) - 1;

    // first we make the facewise data that are needed for the compactification and otrdering
    // of exponent vectors
    vector<vector<long> > St(inExSimplData.size()), End(inExSimplData.size()), key(inExSimplData.size());
    vector<long> active;
    for (size_t i = 0; i < inExSimplData.size(); ++i)
        if (!inExSimplData[i].done) {
            active.push_back(i);
            makeLocalDegreesAndKey(inExSimplData[i].GenInFace, degrees, localDeg, key[i]);
            makeStartEnd(localDeg, St[i], End[i]);
        }

    // now the same for the full simplex (localDeg=degrees)
    dynamic_bitset fullSimpl(dim);
    fullSimpl.set();
    vector<long> StSimpl, EndSimpl;
    makeStartEnd(degrees, StSimpl, EndSimpl);

    vector<map<vector<long>, RingElem> > orderedMons(inExSimplData.size());  // will take the ordered exponent vectors
    map<vector<long>, RingElem> orderedMonsSimpl;

    dynamic_bitset indicator(dim);

    // now we go over the terms of G
    SparsePolyIter term = BeginIter(G);
    // PPMonoid TT = PPM(owner(G));
    for (; !IsEnded(term); ++term) {
        // PPMonoidElem mon(PP(term));
        exponents(v, PP(term));
        w = v;
        indicator.reset();
        for (size_t j = 0; j < dim; ++j)
            if (v[j + 1] != 0)  // we must add 1 since the 0-th indeterminate is irrelevant here
                indicator.set(j);
        for (size_t i = 0; i < active.size(); ++i) {
            int j = active[i];
            if (indicator.is_subset_of(inExSimplData[j].GenInFace)) {
                w = shiftVars(v, key[j]);
                w = orderExposInner(w, St[j], End[j]);
                // w=shiftVars(v,key[j]);
                auto ord_mon = orderedMons[j].find(w);  // insert into map or add coefficient
                if (ord_mon != orderedMons[j].end()) {
                    ord_mon->second += coeff(term);
                }
                else {
                    orderedMons[j].insert(pair<vector<long>, RingElem>(w, coeff(term)));
                }
            }
        }  // for i

        v = orderExposInner(v, StSimpl, EndSimpl);
        auto ord_mon = orderedMonsSimpl.find(v);  // insert into map or add coefficient
        if (ord_mon != orderedMonsSimpl.end()) {
            ord_mon->second += coeff(term);
        }
        else {
            orderedMonsSimpl.insert(pair<vector<long>, RingElem>(v, coeff(term)));
        }
    }  // loop over term

    // now we must make the resulting polynomials from the maps

    for (size_t i = 0; i < active.size(); ++i) {
        int j = active[i];
        for (const auto& ord_mon : orderedMons[j]) {
            PushFront(GRest[j], ord_mon.second, ord_mon.first);
        }
        // verboseOutput() << "GRest[j] " << j << " " << NumTerms(GRest[j]) << endl;
    }

    for (const auto& ord_mon : orderedMonsSimpl) {
        PushFront(GOrder, ord_mon.second, ord_mon.first);
    }
}

long nrActiveFaces = 0;
long nrActiveFacesOld = 0;

void all_contained_faces(const RingElem& G,
                         RingElem& GOrder,
                         const vector<long>& degrees,
                         dynamic_bitset& indicator,
                         long Deg,
                         vector<SIMPLINEXDATA_INT>& inExSimplData,
                         deque<pair<vector<long>, RingElem> >& facePolysThread) {
    const SparsePolyRing& R = owner(G);
    vector<RingElem> GRest;
    // size_t dim=indicator.size();
    for (size_t i = 0; i < inExSimplData.size(); ++i) {
        GRest.push_back(zero(R));

        if (!indicator.is_subset_of(inExSimplData[i].GenInFace))
            inExSimplData[i].done = true;  // done if face cannot contribute to result for this offset
        else
            inExSimplData[i].done = false;  // not done otherwise
    }
    restrictToFaces(G, GOrder, GRest, degrees, inExSimplData);

    for (size_t j = 0; j < inExSimplData.size(); ++j) {
        if (inExSimplData[j].done)
            continue;
#pragma omp atomic
        nrActiveFaces++;
        // verboseOutput() << "Push back " << NumTerms(GRest[j]);
        GRest[j] = power(indets(R)[0], Deg) * inExSimplData[j].mult *
                   GRest[j];  // shift by degree of offset amd multiply by mult of face
        facePolysThread.push_back(pair<vector<long>, RingElem>(inExSimplData[j].degrees, GRest[j]));
        // verboseOutput() << " Now " << facePolysThread.size() << endl;
    }
}

template <typename Number>
RingElem affineLinearSubstitutionFL(const ourFactorization& FF,
                                    const vector<vector<Number> >& A,
                                    const vector<Number>& b,
                                    const Number& denom,
                                    const SparsePolyRing& R,
                                    const vector<Number>& degrees,
                                    const BigInt& lcmDets,
                                    vector<SIMPLINEXDATA_INT>& inExSimplData,
                                    deque<pair<vector<Number>, RingElem> >& facePolysThread) {
    // applies linar substitution y --> lcmDets*A(y+b/denom) to all factors in FF
    // and returns the product of the modified factorsafter ordering the exponent vectors

    size_t i;
    size_t m = A.size();
    size_t dim = m;  // TO DO: eliminate this duplication
    vector<RingElem> v(m, zero(R));
    RingElem g(zero(R));

    for (i = 0; i < m; i++) {
        g = b[i] * (lcmDets / denom);
        v[i] = g + lcmDets * indets(R)[i + 1];
    }
    vector<RingElem> w = VxM(v, A);
    vector<RingElem> w1(w.size() + 1, zero(R));
    w1[0] = RingElem(R, lcmDets);
    for (i = 1; i < w1.size(); ++i)
        w1[i] = w[i - 1];

    // RingHom phi=PolyAlgebraHom(R,R,w1);

    RingElem G1(zero(R));
    list<RingElem> sortedFactors;
    for (i = 0; i < FF.myFactors.size(); ++i) {
        // G1=phi(FF.myFactors[i]);
        G1 = mySubstitution(FF.myFactors[i], w1);
        for (int nn = 0; nn < FF.myMultiplicities[i]; ++nn)
            sortedFactors.push_back(G1);
    }

    sortedFactors.sort(compareLength);

    RingElem G(one(R));

    for (const auto& sf : sortedFactors)
        G *= sf;

    if (inExSimplData.size() == 0) {  // not really necesary, but a slight shortcut
        dynamic_bitset dummyInd;
        return (orderExpos(G, degrees, dummyInd, false));
    }

    // if(inExSimplData.size()!=0){
    long Deg = 0;
    dynamic_bitset indicator(dim);  // indicates the non-zero components of b
    indicator.reset();
    for (size_t i = 0; i < dim; ++i)
        if (b[i] != 0) {
            indicator.set(i);
            Deg += degrees[i] * b[i];
        }
    Deg /= denom;
    RingElem Gorder(zero(R));
    all_contained_faces(G, Gorder, degrees, indicator, Deg, inExSimplData, facePolysThread);
    return (Gorder);
    // }
}

vector<RingElem> homogComps(const RingElem& F) {
    // returns the vector of homogeneous components of F
    // w.r.t. standard grading

    const SparsePolyRing& P = owner(F);
    long dim = NumIndets(P);
    vector<long> v(dim);
    vector<RingElem> c(deg(F) + 1, zero(P));
    long j, k;

    // TODO there is a leading_term() function coming in cocoalib
    // TODO maybe there will be even a "splice_leading_term"
    SparsePolyIter i = BeginIter(F);
    for (; !IsEnded(i); ++i) {
        exponents(v, PP(i));
        k = 0;
        for (j = 0; j < dim; j++)
            k += v[j];
        PushBack(c[k], coeff(i), v);
    }
    return (c);
}

RingElem homogenize(const RingElem& F) {
    // homogenizes F wrt the zeroth variable and returns the
    // homogenized polynomial

    SparsePolyRing P = owner(F);
    int d = deg(F);
    vector<RingElem> c(d + 1, zero(P));
    c = homogComps(F);
    RingElem h(zero(P));
    for (int i = 0; i <= d; ++i)
        h += c[i] * power(indets(P)[0], d - i);
    return (h);
}

RingElem makeZZCoeff(const RingElem& F, const SparsePolyRing& RZZ) {
    // F is a polynomial over RingQQ with integral coefficients
    // This function converts it into a polynomial over RingZZ

    SparsePolyIter mon = BeginIter(F);  // go over the given polynomial
    RingElem G(zero(RZZ));
    for (; !IsEnded(mon); ++mon) {
        PushBack(G, num(coeff(mon)), PP(mon));
    }
    return (G);
}

RingElem makeQQCoeff(const RingElem& F, const SparsePolyRing& R) {
    // F is a polynomial over RingZZ
    // This function converts it into a polynomial over RingQQ
    SparsePolyIter mon = BeginIter(F);  // go over the given polynomial
    RingElem G(zero(R));
    for (; !IsEnded(mon); ++mon) {
        PushBack(G, RingElem(RingQQ(), coeff(mon)), PP(mon));
    }
    return (G);
}

CyclRatFunct genFunct(const vector<vector<CyclRatFunct> >& GFP, const RingElem& F, const vector<long>& degrees)
// writes \sum_{x\in\ZZ_+^n} f(x,t) T^x
// under the specialization T_i --> t^g_i
// as a rational function in t
{
    const SparsePolyRing& P = owner(F);
    RingElem t = indets(P)[0];

    CyclRatFunct s(F);  // F/1

    CyclRatFunct g(zero(P)), h(zero(P));

    long nd = degrees.size();
    long i, k, mg;
    vector<RingElem> c;

    for (k = 1; k <= nd; k++) {
        c = ourCoeffs(s.num, k);  // we split the numerator according
                                  // to powers of var k
        mg = c.size();            // max degree+1 in  var k

        h.set2(zero(P));
        for (i = 0; i < mg; i++)  // now we replace the powers of var k
        {                         // by the corrseponding rational function,
                                  // multiply, and sum the products

            h.num = (1 - power(t, degrees[k - 1])) * h.num + GFP[degrees[k - 1]][i].num * c[i];
            h.denom = GFP[degrees[k - 1]][i].denom;
        }
        s.num = h.num;
        s.denom = prodDenom(s.denom, h.denom);
    }
    return (s);
}

vector<RingElem> power2ascFact(const SparsePolyRing& P, const long& k)
// computes the representation of the power x^n as the linear combination
// of (x+1)_n,...,(x+1)_0
// return value is the vector of coefficients (they belong to ZZ)
{
    RingElem t = indets(P)[0];
    const vector<long> ONE(NumIndets(P));
    RingElem f(P), g(P), h(P);
    f = power(t, k);
    long m;
    vector<RingElem> c(k + 1, zero(P));
    while (f != 0) {
        m = deg(f);
        h = monomial(P, LC(f), ONE);
        c[m] = h;
        f -= h * ascFact(t, m);
    }
    return (c);
}

CyclRatFunct genFunctPower1(const SparsePolyRing& P, long k, long n)
// computes the generating function for
//  \sum_j j^n (t^k)^j
{
    vector<RingElem> a = power2ascFact(P, n);
    RingElem b(P);
    vector<long> u;
    CyclRatFunct g(zero(P)), h(zero(P));
    long i, s = a.size();
    for (i = 0; i < s; ++i) {
        u = makeDenom(k, i + 1);
        b = a[i] * factorial(i);
        g.set2(b, u);
        h.addCRF(g);
    }
    return (h);
}

void CyclRatFunct::extendDenom(const vector<long>& target)
// extends the denominator to target
// by multiplying the numrerator with the remaining factor
{
    RingElem t = indets(owner(num))[0];
    long i, ns = target.size(), nf = denom.size();
    for (i = 1; i < ns; ++i) {
        if (i > nf - 1)
            num *= power(1 - power(t, i), (target[i]));
        else if (target[i] > denom[i])
            num *= power(1 - power(t, i), (target[i] - denom[i]));
    }
    denom = target;
}

vector<long> lcmDenom(const vector<long>& df, const vector<long>& dg) {
    // computes the lcm of ztwo denominators as used in CyclRatFunct
    // (1-t^i and 1-t^j, i != j, are considered as coprime)
    size_t nf = df.size(), ng = dg.size(), i;
    size_t n = max(nf, ng);
    vector<long> dh = df;
    dh.resize(n);
    for (i = 1; i < n; ++i)
        if (i < ng && dh[i] < dg[i])
            dh[i] = dg[i];
    return (dh);
}

vector<long> prodDenom(const vector<long>& df, const vector<long>& dg) {
    // as above, but computes the profduct
    size_t nf = df.size(), ng = dg.size(), i;
    size_t n = max(nf, ng);
    vector<long> dh = df;
    dh.resize(n);
    for (i = 1; i < n; ++i)
        if (i < ng)
            dh[i] += dg[i];
    return (dh);
}

vector<long> degrees2denom(const vector<long>& d) {
    // converts a vector of degrees to a "denominator"
    // listing at position i the multiplicity of i in d
    long m = 0;
    size_t i;
    if (d.size() == 0)
        return vector<long>(0);
    for (i = 0; i < d.size(); ++i)
        m = max(m, d[i]);
    vector<long> e(m + 1);
    for (i = 0; i < d.size(); ++i)
        e[d[i]]++;
    return (e);
}

vector<long> denom2degrees(const vector<long>& d) {
    // the converse operation
    vector<long> denomDeg;
    for (size_t i = 0; i < d.size(); ++i)
        for (long j = 0; j < d[i]; ++j)
            denomDeg.push_back(i);
    return (denomDeg);
}

RingElem denom2poly(const SparsePolyRing& P, const vector<long>& d) {
    // converts a denominator into a real polynomial
    // the variable for the denominator is x[0]
    RingElem t = indets(P)[0];
    RingElem f(one(P));
    for (size_t i = 1; i < d.size(); ++i)
        f *= power(1 - power(t, i), d[i]);
    return (f);
}

vector<long> makeDenom(long k, long n)
// makes the denominator (1-t^k)^n
{
    vector<long> d(k + 1);
    d[k] = n;
    return (d);
}

void CyclRatFunct::addCRF(const CyclRatFunct& r) {
    // adds r to *this, r is preserved in its given form
    CyclRatFunct s(zero(owner(num)));
    const vector<long> lcmden(lcmDenom(denom, r.denom));
    s = r;
    s.extendDenom(lcmden);
    extendDenom(lcmden);
    num += s.num;
}

/*
void CyclRatFunct::multCRF(const CyclRatFunct& r) {
    // nmultiplies *this by r
    num *= r.num;
    denom = prodDenom(denom, r.denom);
}
*/

void CyclRatFunct::showCRF() {
    if (!verbose_INT)
        return;

    verboseOutput() << num << endl;
    for (size_t i = 1; i < denom.size(); ++i)
        verboseOutput() << denom[i] << " ";
    verboseOutput() << endl;
}

void CyclRatFunct::showCoprimeCRF() {
    // shows *this also with coprime numerator and denominator
    // makes only sense if only x[0] appears in the numerator (not checked)

    if (!verbose_INT)
        return;

    verboseOutput() << "--------------------------------------------" << endl << endl;
    verboseOutput() << "Given form" << endl << endl;
    showCRF();
    verboseOutput() << endl;
    const SparsePolyRing& R = owner(num);
    SparsePolyRing P = NewPolyRing_DMPI(RingQQ(), symbols("t"));
    vector<RingElem> Im(NumIndets(R), zero(P));
    Im[0] = indets(P)[0];
    RingHom phi = PolyAlgebraHom(R, P, Im);
    RingElem f(phi(num));
    RingElem g(denom2poly(P, denom));
    RingElem h = CoCoA::gcd(f, g);
    f /= h;
    g /= h;
    verboseOutput() << "Coprime numerator (for denom with remaining factor 1)" << endl << endl;
    factorization<RingElem> gf = factor(g);
    verboseOutput() << f / gf.myRemainingFactor() << endl << endl << "Factorization of denominator" << endl << endl;
    size_t nf = gf.myFactors().size();
    for (size_t i = 0; i < nf; ++i)
        verboseOutput() << gf.myFactors()[i] << "  mult " << gf.myMultiplicities()[i] << endl;
    verboseOutput() << "--------------------------------------------" << endl;
}

void CyclRatFunct::simplifyCRF() {
    // cancels factors 1-t^i from the denominator that appear there explicitly
    // (and not just as factors of 1-t^j for some j)

    const SparsePolyRing& R = owner(num);
    long nd = denom.size();
    for (long i = 1; i < nd; i++) {
        while (denom[i] > 0) {
            if (!IsDivisible(num, 1 - power(indets(R)[0], i)))
                break;
            num /= 1 - power(indets(R)[0], i);
            denom[i]--;
        }
    }
}

void CyclRatFunct::set2(const RingElem& f, const vector<long>& d) {
    num = f;
    denom = d;
}

void CyclRatFunct::set2(const RingElem& f) {
    num = f;
    denom.resize(1, 0);
}

CyclRatFunct::CyclRatFunct(const RingElem& c)
    : num(c)
// constructor starting from a RingElem
// initialization necessary because RingElem has no default
// constructor
{
    denom.resize(1, 0);
}

CyclRatFunct::CyclRatFunct(const RingElem& c, const vector<long>& d) : num(c), denom(d) {
}
//--------------------------------------

struct PolynomialData {
    ourFactorization FF;
    bool homogeneous;
    long degree;
    vector<BigInt> Factorial;
    vector<BigInt> FactQuot;
    long dimension;

    RingElem F;
};

}  // end namespace libnormaliz

#endif  // NMZ_COCOA

#endif  // NMZ_INTEGRATE_H
