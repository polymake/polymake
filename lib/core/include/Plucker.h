/* Copyright (c) 1997-2015
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#ifndef POLYMAKE_PLUCKER_H
#define POLYMAKE_PLUCKER_H

#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/permutations.h"

namespace pm {

namespace {

template <typename T, typename Iterator>   
void make_index (Iterator it, Map<T, int>& index_of)
{
   int index(0);
   while (!it.at_end()) {
      index_of[*it] = index++;
      ++it;
   }
}

Vector<int> squeeze(const Vector<int>& v, const Set<int>& s)
{
   Map<int,int> index_of;
   make_index(entire(s), index_of);
   Vector<int> w(v.size());
   Entire<Vector<int> >::iterator wit = entire(w);
   for (Entire<Vector<int> >::const_iterator vit = entire(v); !vit.at_end(); ++vit)
      *wit++ = index_of[*vit];
   return w;
}

}  // end anonymous namespace

template <typename T> 
class Plucker {         // a class to hold and process Plücker coordinates of a subspace ("flat")
public:
   typedef Map<Set<int>, T> CooType;

protected:
   int _d     // the dimension of the ambient space
      , _k;   // the dimension of the flat
   CooType _coos;  // the binom{_d,_k}-tuple of Plücker coordinates of the flat

public:
   template <typename>
   friend struct spec_object_traits;

   Plucker()
      : _d(0)
      , _k(0)
      , _coos(CooType()) {}

   template <typename E>
   explicit Plucker(const Vector<E>& v)  
      : _d(v.size())
      , _k(1)
      , _coos(CooType()) {
      typename Entire<Vector<E> >::const_iterator vit = entire(v);
      for (int i=0; i<_d; ++i, ++vit)
         _coos[scalar2set(i)] = *vit;
   }

   template <typename E>
   explicit Plucker(int d, int k, const Vector<E>& v)  
      : _d(d)
      , _k(k)
      , _coos(CooType()) {
      if (v.size() != Integer::binom(d, k))
         throw std::runtime_error("The number of coordinates is not the expected one, binom(d,k)");
      typename Entire<Vector<E> >::const_iterator vit = entire(v);
      for (auto fit = entire(all_subsets_of_k(sequence(0,_d), _k)); !fit.at_end(); ++fit, ++vit) 
         _coos[*fit] = *vit;
   }

   explicit Plucker(int d, int k)
      : _d(d)
      , _k(k)
      , _coos(CooType()) {}

   const int d() const { return _d; }
   const int k() const { return _k; }
   const T& operator[] (const Set<int> &s) const { return _coos[s]; }

   const Vector<T> coordinates() const
   {
      Vector<T> v(int(Integer::binom(_d,_k)));
      auto vit = entire(v);
      for (auto cit = entire(_coos); !cit.at_end(); ++cit, ++vit)
         *vit = cit->second;
      return v;
   }

   const Vector<T> point() const
   {
      if (_k!=1) {
         cerr << *this << endl;
         throw std::runtime_error("The dimension is not 1; can't convert this flat to a point");
      }
      return coordinates();
   }

   template <typename Permutation>
   Plucker<T> permuted(const Permutation& perm)const{
      if(perm.size()!=_d)
         throw std::runtime_error("The size of the permutation is not the expected one.");
      Plucker<T> plucker(_d,_k);
      for (typename Entire<CooType>::const_iterator cit = entire(_coos); !cit.at_end(); ++cit)
         plucker._coos[pm::permuted(cit->first,perm)] = cit->second;
      return plucker;
   }

   friend Plucker join (const Plucker& p1, const Plucker& p2) {
      if (p1.d() != p2.d())
         throw std::runtime_error("Ambient dimensions of p1 and p2 are distinct");
      const int d = p1.d(), k = p1.k() + p2.k();
      if (k>d)
         throw std::runtime_error("The sum of the dimensions of the flats is greater than that of the ambient space, so I can't join them");

      // We iterate over all pairs of sets (A,B), A in binom{[d],k1}, B in binom{[d],k2}
      // such that A and B are disjoint.
      Plucker result(d,k);
      for (Entire<Subsets_of_k<const sequence&> >::const_iterator Ait = entire(all_subsets_of_k(sequence(0,p1.d()), p1.k())); !Ait.at_end(); ++Ait) {
         Set<int> rest(sequence(0,d) - *Ait);
         for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator Bit = entire(all_subsets_of_k(rest, p2.k())); !Bit.at_end(); ++Bit) {
            Set<int> U(*Ait); U += *Bit;
            const Vector<int> perm = Vector<int>(p1.k(), entire(*Ait)) | Vector<int>(p2.k(), entire(*Bit));
            result._coos[U] += permutation_sign(squeeze(perm, U)) * p1[*Ait] * p2[*Bit];
         } 
      }
      return result;
   }

   friend Plucker meet (const Plucker& p1, const Plucker& p2)  {
      if (p1.d() != p2.d())
         throw std::runtime_error("Ambient dimensions of p1 and p2 are distinct");
      const int d = p1.d(), k = p1.k() + p2.k() - d;
      if (k<0) {
         cerr << p1 << endl << p2 << endl;
         throw std::runtime_error("The sum of the dimensions of the flats is less than that of the ambient space, so I can't intersect them");
      }

      // We iterate over all pairs of sets (A,B), A in binom{[d],k1}, B in binom{[d],k2},
      // such that |A cap B| = k = k1+k2-d. 
      // For this, we decompose A into A = A1 cup S with S = A cap B, so that B = S cup B1
      // for a (d-k1)-set B1.
      Plucker result(d,k);
      for (Entire<Subsets_of_k<const sequence&> >::const_iterator Ait = entire(all_subsets_of_k(sequence(0,p1.d()), p1.k())); !Ait.at_end(); ++Ait) {
         const Set<int> 
            A(*Ait),
            rest (sequence(0,d) - A);
         for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator Sit = entire(all_subsets_of_k(A, k)); !Sit.at_end(); ++Sit) {
            const Set<int> S(*Sit);
            for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator B1it = entire(all_subsets_of_k(rest, d-p1.k())); !B1it.at_end(); ++B1it) {
               const Set<int> 
                  B1(*B1it),
                  B(S + B1),
                  A1(A - S);
               const Vector<int> perm = Vector<int>(A1.size(), entire(A1)) | Vector<int>(B1.size(), entire(B1));
               result._coos[*Sit] += permutation_sign(squeeze(perm, A1+B1)) * p1[A] * p2[B];
            }
         } 
      }
      return result;
   }

   inline friend Plucker operator+ (const Plucker& p1, const Plucker& p2) { return join(p1,p2); }
   inline friend Plucker operator* (const Plucker& p1, const Plucker& p2) { return meet(p1,p2); }

   /** This function takes a 2-flat F and a vector v that is supposed to be contained in it,
    *  and gives back a vector that spans the orthogonal complement of v in F.
    */
   Vector<T> project_out (const Vector<T>& v) {
      if (_k!=2) throw std::runtime_error("Only projecting from planes is implemented");
      SparseMatrix<T> M(int(Integer::binom(_d,2))+1, _d);
      int row_ct(0);
      for (Entire<Subsets_of_k<const sequence&> >::const_iterator fit = entire(all_subsets_of_k(sequence(0,_d), _k)); !fit.at_end(); ++fit, ++row_ct) {
         M(row_ct, fit->front()) = -v[fit->back()];
         M(row_ct, fit->back())  =  v[fit->front()];
      }
      M.row(row_ct) = v;
      const Vector<T> vs = coordinates() | 1;
      return lin_solve(M, vs).dehomogenize();
   }

   inline SparseVector<T> project_out (const Plucker& p) { return project_out(p.point()); }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& outs, const Plucker& e)
   {
      return outs.top() << "(" << e.d() << " " << e.k() << " [" << e.coordinates() << "])";
   }
   
};

} // end namespace pm

namespace polymake {
   using pm::Plucker;
}

/*
namespace std {
   template <typename T>
   struct numeric_limits<pm::Plucker<T> > {
      //      static const bool is_integer = false;
      //      static const bool is_signed = false;
   };
}
*/

#endif // POLYMAKE_PLUCKER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
