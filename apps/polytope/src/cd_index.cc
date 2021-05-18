/* Copyright (c) 1997-2021
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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/list"
#include "polymake/Fibonacci.h"

/*  Calculate the cd-index from (the non-redundant part of) the flag vector of a polytope.
 *  See
 *
 *      Billera, Ehrenborg, Monotonicity of the cd-index for polytopes
 *      Math.Z. 233 (2000), 421-441, section 7
 *
 *  @author Axel Werner
 */

namespace polymake { namespace polytope {
namespace {

/*************************************************************************/
/*                      k -> cd  transformation                          */
/*************************************************************************/

/* class ivec
 * represents a p-tuple (i_1,...,i_p) such that
 * m_{j-1} <= i_j <= m_j - 2  for a given m-vector (m_0,...,m_p)
 * iterates via ++ over all p-tuples with this property
 */
class ivec
{
private:
   Array<Int> iota;
   Array<Int> mvec;
   bool is_at_end;

public:
   // (i_1,...,i_p), starts with (m_0,...,m_{p-1})
   explicit ivec (const Array<Int>& m = Array<Int>(1))
      : iota(m.size())
      , mvec(m)
   {
      reset();
   }

   void reset()
   {
      iota[0] = 0;
      for (Int i = 1, s = iota.size(); i < s; ++i)
         iota[i] = mvec[i-1];
      is_at_end = false;
   }

   bool at_end() const { return is_at_end; }
      
   ivec& operator++ ()
   {
      if (!at_end()) {
         for (Int i = 1, s = iota.size(); i < s; ++i)
            if (iota[i] == mvec[i]-2) {
               if (i == s-1) {
                  is_at_end = true;
               } else {
                  iota[i] = mvec[i-1];
               }
            } else {
               ++iota[i];
               break;
            }
      }
      return *this;
   }

   int ksign () const
   {
      Int exponent = 0;
      for (Int i = 1; i < iota.size(); ++i)
         exponent += (mvec[i]-iota[i]);
      return (exponent%2==0 ? 1 : -1);

      // better (for later):
      // calculate the starting sign in reset()
      // and update it with every ++
   }

   Int to_idx (const Array<Int>& Fib) const
   {
      Int idx = 0;
      for (Int i=1, s=iota.size(); i<s; ++i)
         idx += Fib[iota[i]+1];
      return idx;
   }
};

// calculates the m-vector for a given monomial
Array<Int> m_vector (Int mon, Int d, const Array<Int>& Fib)
{
   std::list<Int> n;
   Int no_of_c = 0;
   while (d > 1) {
      if (mon >= Fib[d-1]) {
         mon -= Fib[d-1];
         d -= 2;
         n.push_back(no_of_c);
         no_of_c = 0;
      } else {
         --d;
         ++no_of_c;
      }
   }

   Int p = n.size();
   Array<Int> m (p+1); // p+2 ?
   m[0] = 0;
   auto lit = n.begin();
   for (Int i = 1; i <= p; ++i, ++lit)
      m[i] = m[i-1]+(*lit)+2;

   return m;
}

/* calculates the row adjoined to the given monomial of the matrix below */
Vector<Integer> kvec_combination(Int mon, Int d, const Array<Int>& Fib)
{
   Vector<Integer> r(Fib[d]);

   if (mon == 0) {                // mon is ccc...c
      r[0] = 1;
   } else {                     // mon contains at least one d
      Array<Int> m = m_vector(mon, d, Fib);
      for (ivec i(m); !i.at_end(); ++i)
         r[i.to_idx(Fib)] += i.ksign();
   }

   return r;
}

/*************************************************************************/
/*                     flag -> k transformation                          */
/*************************************************************************/

/* class subset_iterator
 * iterates via ++ over all subsets of a sparse set, given by a sum of Fibonacci numbers
 */
class subset_iterator
{
private:
   Int spset;
   Int subset;
   Int coeff;
   Int dim;
   const Array<Int> Fib;

public:
   subset_iterator()
      : spset(0), subset(0), coeff(0), dim(0), Fib(Array<Int>()) { }

   subset_iterator(Int start_set, Int d, const Array<Int>& Fib_arg)
      : spset(start_set), subset(start_set), coeff(1), dim(d), Fib(Fib_arg) { }

   bool at_end() const { return (coeff==0); }

   Int operator* () const { return subset; }

   Int coefficient() const { return coeff; }

   subset_iterator& operator++ ()
   {
      if (!at_end()) {
         Int d = dim;
         Int spsetcopy = spset;
         for ( ; d > 0; --d)
            if (spsetcopy >= Fib[d-1]) {
               spsetcopy -= Fib[d-1];
               if (subset >= Fib[d-1])
                  break;
            }
         if (d > 0) {
            subset -= Fib[d-1];
            coeff *= -2;
            spsetcopy = spset;
            for (Int i = dim; i > d; --i)
               if (spsetcopy >= Fib[i-1])
               {
                  subset += Fib[i-1];
                  coeff /= -2;
                  spsetcopy -= Fib[i-1];
               }
         } else {               // then subset is already 0 (?)
            coeff = 0;
         }
      }
      return *this;
   }
};

Vector<Integer> flag_combination(Int spset, Int d, const Array<Int>& Fib)
{
   Vector<Integer> r(Fib[d]);

   for (subset_iterator sit(spset,d,Fib); !sit.at_end(); ++sit)
      r[*sit] += sit.coefficient();

   return r;
}

/*************************************************************************/
/*                     transformation calculations                       */
/*************************************************************************/

enum transformation_type { flag_to_kvec, kvec_to_cd };

/* calculates the transformation matrices for the given dimension;
 * flag-vector -> k-vector  for t == flag_to_kvec,
 * k-vector -> cd-index     for t == kvec_to_cd
 */
Matrix<Integer> transformation_matrix(Int d, transformation_type t, const Array<Int>& Fib)
{
   Matrix<Integer> T(Fib[d],Fib[d]);

   if (d==0) {
      T(0,0) = 1;
   } else {
      Int objit = 0;
      for (auto rit = entire(rows(T)); !rit.at_end(); ++rit, ++objit)
         switch (t) {
         case kvec_to_cd:
            *rit = kvec_combination(objit, d, Fib);
            break;
         case flag_to_kvec:
            *rit = flag_combination(objit, d, Fib);
            break;
         }
   }

   return T;
}
} // end anonymous namespace

void cd_index(BigObject p)
{
   Int d = p.give("COMBINATORIAL_DIM");
   Vector<Integer> flag = p.give("FLAG_VECTOR");
   Vector<Integer> cd(flag.dim());

   if ( d <= 0 ) { // the trivial case of a point
      p.take("CD_INDEX_COEFFICIENTS") << cd;
      return;
   }

   // precalculate the fibonacci-numbers
   const Array<Int> Fib(d+1, fibonacci_numbers());

   // transform the flag vector into the k-vector
   const Matrix<Integer> T1 = transformation_matrix(d, flag_to_kvec, Fib);
   const Vector<Integer> kvec = T1 * flag;

   // transform the k-vector into the cd-index
   const Matrix<Integer> T2 = transformation_matrix(d, kvec_to_cd, Fib);
   cd = T2 * kvec;

   p.take("CD_INDEX_COEFFICIENTS") << cd;
}

Function4perl(&cd_index, "cd_index");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
