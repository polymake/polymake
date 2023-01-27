/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#pragma once

#include "polymake/Integer.h"
#include "polymake/Vector.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/permutations.h"

namespace pm {

namespace {

template <typename T, typename Iterator>   
void make_index(Iterator it, Map<T, Int>& index_of)
{
   Int index = 0;
   while (!it.at_end()) {
      index_of[*it] = index++;
      ++it;
   }
}

Vector<Int> squeeze(const Vector<Int>& v, const Set<Int>& s)
{
   Map<Int, Int> index_of;
   make_index(entire(s), index_of);
   Vector<Int> w(v.size());
   auto wit = entire(w);
   for (auto vit = entire(v); !vit.at_end(); ++vit)
      *wit++ = index_of[*vit];
   return w;
}

}  // end anonymous namespace

template <typename T> 
class Plucker {         // a class to hold and process Plücker coordinates of a subspace ("flat")
public:
   typedef Map<Set<Int>, T> CooType;

protected:
   Int d_;   // the dimension of the ambient space
   Int k_;   // the dimension of the flat
   CooType coos_;  // the binom{d_,k_}-tuple of Plücker coordinates of the flat

public:
   template <typename>
   friend struct spec_object_traits;

   Plucker()
      : d_(0)
      , k_(0)
      , coos_(CooType()) {}

   template <typename E>
   explicit Plucker(const Vector<E>& v)  
      : d_(v.size())
      , k_(1)
      , coos_(CooType()) {
      auto vit = entire(v);
      for (Int i = 0; i < d_; ++i, ++vit)
         coos_[scalar2set(i)] = *vit;
   }

   template <typename E>
   Plucker(Int d, Int k, const Vector<E>& v)  
      : d_(d)
      , k_(k)
      , coos_(CooType()) {
      if (v.size() != Integer::binom(d, k))
         throw std::runtime_error("The number of coordinates is not the expected one, binom(d,k)");
      auto vit = entire(v);
      for (auto fit = entire(all_subsets_of_k(sequence(0,d_), k_)); !fit.at_end(); ++fit, ++vit) 
         coos_[*fit] = *vit;
   }

   Plucker(Int d, Int k)
      : d_(d)
      , k_(k)
      , coos_(CooType()) {}

   Int d() const { return d_; }
   Int k() const { return k_; }
   const T& operator[] (const Set<Int>& s) const { return coos_[s]; }

   Vector<T> coordinates() const
   {
      Vector<T> v(static_cast<Int>(Integer::binom(d_,k_)));
      auto vit = entire(v);
      for (auto cit = entire(coos_); !cit.at_end(); ++cit, ++vit)
         *vit = cit->second;
      return v;
   }

   Vector<T> point() const
   {
      if (k_!=1) {
         throw std::runtime_error("The dimension of the flat " + std::to_string(k_) + " > 1, it can't be converted to a point");
      }
      return coordinates();
   }

   template <typename Permutation>
   Plucker<T> permuted(const Permutation& perm) const
   {
      if(perm.size()!=d_)
         throw std::runtime_error("The size of the permutation is not the expected one.");
      Plucker<T> plucker(d_,k_);
      for (auto cit = entire(coos_); !cit.at_end(); ++cit)
         plucker.coos_[pm::permuted(cit->first,perm)] = cit->second;
      return plucker;
   }

   friend Plucker join(const Plucker& p1, const Plucker& p2)
   {
      if (p1.d() != p2.d())
         throw std::runtime_error("Ambient dimensions of p1 and p2 are not equal");
      const Int d = p1.d(), k = p1.k() + p2.k();
      if (k > d)
         throw std::runtime_error("The sum of the dimensions of the flats " + std::to_string(k) +
                                  " is greater than the dimension of the ambient space " + std::to_string(d) +
                                  ", they can't be joined");

      // We iterate over all pairs of sets (A,B), A in binom{[d],k1}, B in binom{[d],k2}
      // such that A and B are disjoint.
      Plucker result(d,k);
      for (auto Ait = entire(all_subsets_of_k(sequence(0,p1.d()), p1.k())); !Ait.at_end(); ++Ait) {
         Set<Int> rest(sequence(0, d) - *Ait);
         for (auto Bit = entire(all_subsets_of_k(rest, p2.k())); !Bit.at_end(); ++Bit) {
            Set<Int> U(*Ait); U += *Bit;
            const Vector<Int> perm = Vector<Int>(p1.k(), entire(*Ait)) | Vector<Int>(p2.k(), entire(*Bit));
            result.coos_[U] += permutation_sign(squeeze(perm, U)) * p1[*Ait] * p2[*Bit];
         } 
      }
      return result;
   }

   friend Plucker meet(const Plucker& p1, const Plucker& p2)
   {
      if (p1.d() != p2.d())
         throw std::runtime_error("Ambient dimensions of p1 and p2 are not equal");
      const Int d = p1.d(), k = p1.k() + p2.k() - d;
      if (k < 0) {
         throw std::runtime_error("The sum of the dimensions of the flats " + std::to_string(p1.k() + p2.k()) +
                                  " is less than the dimension of the ambient space " + std::to_string(d) +
                                  ", they can't be intersected");
      }

      // We iterate over all pairs of sets (A,B), A in binom{[d],k1}, B in binom{[d],k2},
      // such that |A cap B| = k = k1+k2-d. 
      // For this, we decompose A into A = A1 cup S with S = A cap B, so that B = S cup B1
      // for a (d-k1)-set B1.
      Plucker result(d, k);
      for (auto Ait = entire(all_subsets_of_k(sequence(0, p1.d()), p1.k())); !Ait.at_end(); ++Ait) {
         const Set<Int> A{*Ait};
         const Set<Int> rest = sequence(0, d) - A;
         for (auto Sit = entire(all_subsets_of_k(A, k)); !Sit.at_end(); ++Sit) {
            const Set<Int> S{*Sit};
            for (auto B1it = entire(all_subsets_of_k(rest, d-p1.k())); !B1it.at_end(); ++B1it) {
               const Set<Int> B1{*B1it};
               const Set<Int> B = S+B1;
               const Set<Int> A1 = A-S;
               const Vector<Int> perm = Vector<Int>(A1.size(), entire(A1)) | Vector<Int>(B1.size(), entire(B1));
               result.coos_[*Sit] += permutation_sign(squeeze(perm, A1+B1)) * p1[A] * p2[B];
            }
         } 
      }
      return result;
   }

   friend Plucker operator+ (const Plucker& p1, const Plucker& p2) { return join(p1,p2); }
   friend Plucker operator* (const Plucker& p1, const Plucker& p2) { return meet(p1,p2); }

   /** This function takes a 2-flat F and a vector v that is supposed to be contained in it,
    *  and gives back a vector that spans the orthogonal complement of v in F.
    */
   Vector<T> project_out(const Vector<T>& v) const
   {
      if (k_ != 2)
         throw std::runtime_error("Only projecting from planes is implemented");
      SparseMatrix<T> M(static_cast<Int>(Integer::binom(d_, 2)+1), d_);
      Int row_ct = 0;
      for (auto fit = entire(all_subsets_of_k(sequence(0, d_), k_)); !fit.at_end(); ++fit, ++row_ct) {
         M(row_ct, fit->front()) = -v[fit->back()];
         M(row_ct, fit->back())  =  v[fit->front()];
      }
      M.row(row_ct) = v;
      const Vector<T> vs = coordinates() | 1;
      return lin_solve(M, vs).dehomogenize();
   }

   SparseVector<T> project_out(const Plucker& p) const
   {
      return project_out(p.point());
   }

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


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
