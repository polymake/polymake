/* Copyright (c) 1997-2018
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

#include "polymake/client.h"
#include <algorithm>
#include <list>
#include "polymake/list"
#include "polymake/polytope/h_vector.h"

namespace polymake { namespace polytope {

// Compute the (dual) h-vector of a simplicial or simple polytope from the f-vector.
Vector<Integer> h_from_f_vec(const Vector<Integer>& f, const bool simplicial)
{
   const int d=f.size();
   Vector<Integer> h(d+1);
   Vector<Integer>::iterator h_k=h.begin();
   for (int k=0, startsign=1;  k<=d;  ++k, ++h_k, startsign=-startsign) {
      *h_k = startsign * Integer::binom(d,d-k);
      for (int i=1, sign=-startsign;  i<=k;  ++i, sign=-sign)
         *h_k += sign * Integer::binom(d-i,d-k) * (simplicial ? f[i-1] : f[d-i]);
   }

   return h;
}

// Inverse of the above
Vector<Integer> f_from_h_vec(const Vector<Integer>& h, const bool simplicial)
{
   const int d=h.size()-1;
   Vector<Integer> f(d);

   for (int k=0; k<d; ++k) {
      Integer f_k=0;
      for (int i=k; i<=d; ++i)
         f_k+=Integer::binom(i,k)*h[i];
      if (simplicial)
         f[d-1-k]=f_k;
      else
         f[k]=f_k;
   }

   return f;
}

// Compute the h-vector from the g-vector of simplicial polytope
Vector<Integer> h_from_g_vec(const Vector<Integer>& g, const int d)
{
   Vector<Integer> h(d+1);
   Integer s(0);
   for (int k=0; k<=d/2; ++k) {
      s += g[k];
      h[d-k] = h[k] = s;
   }

   return h;
}

// Inverse of the above
Vector<Integer> g_from_h_vec(const Vector<Integer>& h)
{
   const int d=h.dim()-1;
   Vector<Integer> g((d+2)/2);

   g[0]=1;
   for (int k=1; k<(d+2)/2; ++k) {
      g[k] = h[k]-h[k-1];
   }

   return g;
}

Vector<int> binomial_representation(Integer l, int i){
   if(l<1 or i<1)
      throw std::runtime_error("input must be positive");
   std::list<int> rep;
   while(l>0){
      int n(0);
      while(Integer::binom(n,i) <= l)
         ++n;
      rep.push_back(n-1);
      l -= int(Integer::binom(n-1,i));
      --i;
   }
   return Vector<int>(rep.size(), rep.begin());
}

// compute the thing that is commonly denoted l^<i> and sometimes
// referred to as pseudopower.
Integer pseudopower(Integer l, int i){
   if(l==0)
      return 0;
   Integer pp = 0;
   int k = i+1;
   for(auto n : binomial_representation(l,i))
      pp += Integer::binom(n+1,k--);
   return pp;
}

bool m_sequence(Vector<Integer> h){
   if(h[0] != 1)
      return false;
   for(int i = 1; i < h.size()-1; ++i){
      if(pseudopower(h[i],i) < h[i+1])
         return false;
   }
   return true;
}

// Compute the (dual) h-vector of a simplicial or simple polytope from the f-vector.
void h_from_f_vector(perl::Object p, bool simplicial)
{
   Vector<Integer> f=p.give("F_VECTOR");
   Vector<Integer> h=h_from_f_vec(f,simplicial);

   if (simplicial)
      p.take("H_VECTOR") << h;
   else
      p.take("DUAL_H_VECTOR") << h;
}

// Inverse of the above
void f_from_h_vector(perl::Object p, bool simplicial)
{
   Vector<Integer> h;
   if (simplicial) {
      Vector<Integer> h_read=p.give("H_VECTOR");
      h=h_read;
   } else {
      Vector<Integer> h_read=p.give("DUAL_H_VECTOR");
      h=h_read;
   }

   p.take("F_VECTOR") << f_from_h_vec(h,simplicial);
}

// Compute the h-vector from the g-vector of simplicial polytope
void h_from_g_vector(perl::Object p)
{
   const Vector<Integer> g=p.give("G_VECTOR");
   const int d=p.give("COMBINATORIAL_DIM");
   p.take("H_VECTOR") << h_from_g_vec(g,d);
}

// Inverse of the above
void g_from_h_vector(perl::Object p)
{
   const Vector<Integer> h=p.give("H_VECTOR");
   p.take("G_VECTOR") << g_from_h_vec(h);
}


Function4perl(&h_from_f_vector, "h_from_f_vector");
Function4perl(&f_from_h_vector, "f_from_h_vector");
Function4perl(&h_from_g_vector, "h_from_g_vector");
Function4perl(&g_from_h_vector, "g_from_h_vector");
Function4perl(&binomial_representation, "binomial_representation");
UserFunction4perl("# @category Other"
                  "# Compute the i-th pseudopower of l, commonly denoted l^<i>."
                  "# See \"A Proof of the Sufficiency of McMullen’s Conditions of Simplicial Convex Polytopes\""
                  "# by Louis Billera and Carl Lee, DOI: 10.1016/0097-3165(81)90058-3, for the definition."
                  "# @param Integer l"
                  "# @param Int i"
                  "# @return Integer",
                  &pseudopower, "pseudopower");
UserFunction4perl("# @category Other"
                  "# Test if the given vector is an M-sequence."
                  "# @param Vector<Int> h"
                  "# @return Bool"
                  "# @example The h-vector of a simplicial or simple polytope is an M-sequence."
                  "# > print m_sequence(cyclic(7,23)->H_VECTOR);"
                  "# | true",
                  &m_sequence, "m_sequence(Vector<Integer>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
