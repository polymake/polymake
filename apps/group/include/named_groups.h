/* Copyright (c) 1997-2020
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

#ifndef __NAMED_GROUPS_H
#define __NAMED_GROUPS_H

#include "polymake/group/group_tools.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace group {

namespace {

inline
Int dn_n_class_reps(Int n2)
{
   const Int n = n2/2;
   const bool odd = n%2;
   return odd
      ? (n-1)/2+2
      : n/2+3;
}

}

inline
ConjugacyClassReps<Array<Int>>
dn_reps(Int n2)
{
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const Int n = n2/2;
   const bool odd = n%2;
   ConjugacyClassReps<Array<Int>> class_reps(dn_n_class_reps(n2));
   auto crit = entire(class_reps);

   /*
      n odd:
      classes (g^j, j=0.. (odd ? (n-1)/2 : n/2)
    */

   // conjugacy classes of type g^j, g = (0 1 2 ... n-1), j = 0..floor(n/2)
   for (Int j = 0; j <= n/2; ++j) {
      Array<Int> c(n);
      Int ct = -1;
      for (Int i = j; i < n; ++i)
         c[i] = ++ct;
      for (Int i = 0; i < j; ++i)
         c[i] = ++ct;
      *crit = c; ++crit;
   }

   Array<Int> h(n);
   if (odd) {
      for (Int i = 1; i <= (n-1)/2; ++i) {
         h[i] = n-i;
         h[n-i] = i;
      }
   } else {
      for (Int i = 0; i <= n/2; ++i) {
         h[i] = n-1-i;
         h[n-1-i] = i;
      }
   }
   *crit = h; ++crit;
   
   if (!odd) {
      Array<Int> gh(n);
      for (Int i = 0; i < n; ++i)
         gh[i] = class_reps[1][h[i]];
      *crit = gh;
   }
   return class_reps;
}

inline
Matrix<CharacterNumberType> dn_character_table(Int n2)
{
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const Int n = n2/2;
   const bool odd = n%2;
   Array<CharacterNumberType> cyclotomics(n);
   for (Int i = 0; i < n; ++i) {
      bool is_rounded;
      cyclotomics[i] = round_if_integer(2 * cos(2*AccurateFloat::pi()*i/n), is_rounded);
   }
   
   const Int n_reps = dn_n_class_reps(n2);
   Matrix<CharacterNumberType> character_table(n_reps, n_reps);
   Int row_ct = 0, col_ct = 0;

   // 1d reps
   // the trivial rep
   for (Int j = 0; j < n_reps; ++j)
      character_table(0,j) = 1;
   ++row_ct;

   // g^i -> 1, h->-1, [gh->-1]
   for (Int j = 0; j <= n/2; ++j)
      character_table(row_ct, col_ct++) = 1;
   character_table(row_ct, col_ct++) = -1;
   if (!odd)
      character_table(row_ct, col_ct) = -1;
   ++row_ct;
   
   if (!odd) {
      // two more: 
      // g^k -> (-1)^k, h -> 1, gh -> -1
      col_ct = 0;
      for (Int j = 0; j <= n/2; ++j)
         character_table(row_ct, col_ct++) = (j%2) ? -1 : 1;
      character_table(row_ct, col_ct++) = 1;
      character_table(row_ct, col_ct) = -1;
      ++row_ct;

      // g^k -> (-1)^k, h -> -1, gh -> 1
      col_ct = 0;
      for (Int j = 0; j <= n/2; ++j)
         character_table(row_ct, col_ct++) = (j%2) ? -1 : 1;
      character_table(row_ct, col_ct++) = -1;
      character_table(row_ct, col_ct) = 1;
      ++row_ct;
   }

   // 2d reps
   const Int n_2d = odd ? (n-1)/2 : (n-2)/2;
   for (Int i = 1; i <= n_2d; ++i, ++row_ct) {
      col_ct=0;
      character_table(row_ct, col_ct++) = 2; // trace of identity
      for (Int j = 1; j <= n/2; ++j) {
         if (n2 != 10 && n2 != 16 && n2 != 20 && n2 != 24) 
            character_table(row_ct, col_ct++) = cyclotomics[ (i*j) % n ]; // trace of [[ zeta_n^i, 0], [0, zeta_n^{-i}]]

         else switch(n2) {
            case 10:
               if (row_ct == 2 && col_ct == 1 ||
                   row_ct == 3 && col_ct == 2)
                  character_table(row_ct, col_ct++) = { -Rational(1,2),  Rational(1,2), 5 };
               else if (row_ct == 2 && col_ct == 2 ||
                        row_ct == 3 && col_ct == 1)
                  character_table(row_ct, col_ct++) = { -Rational(1,2), -Rational(1,2), 5 };
               else
                  character_table(row_ct, col_ct++) = cyclotomics[ (i*j) % n ];
               break;

            case 16:
               if (row_ct == 4 && col_ct == 1 ||
                   row_ct == 6 && col_ct == 3)
                  character_table(row_ct, col_ct++) = { 0, 1, 2 };
               else if (row_ct == 4 && col_ct == 3 ||
                        row_ct == 6 && col_ct == 1)
                  character_table(row_ct, col_ct++) = { 0, -1, 2 };
               else
                  character_table(row_ct, col_ct++) = cyclotomics[ (i*j) % n ];
               break;

            case 20:
               if (row_ct == 4 && col_ct == 1 ||
                   row_ct == 6 && col_ct == 3)
                  character_table(row_ct, col_ct++) = { Rational(1,2), Rational(1,2), 5 };
               else if (row_ct == 4 && col_ct == 4 ||
                        row_ct == 5 && col_ct == 2 ||
                        row_ct == 5 && col_ct == 3 ||
                        row_ct == 6 && col_ct == 2 ||
                        row_ct == 7 && col_ct == 1 ||
                        row_ct == 7 && col_ct == 4)
                  character_table(row_ct, col_ct++) = { -Rational(1,2), -Rational(1,2), 5 };
               else if (row_ct == 4 && col_ct == 3 ||
                        row_ct == 6 && col_ct == 1)
                  character_table(row_ct, col_ct++) = { Rational(1,2), -Rational(1,2), 5 };
               else if (row_ct == 4 && col_ct == 2 ||
                        row_ct == 5 && col_ct == 1 ||
                        row_ct == 5 && col_ct == 4 ||
                        row_ct == 6 && col_ct == 4 ||
                        row_ct == 7 && col_ct == 2 ||
                        row_ct == 7 && col_ct == 3)
                  character_table(row_ct, col_ct++) = { -Rational(1,2), Rational(1,2), 5 };
               else
                  character_table(row_ct, col_ct++) = cyclotomics[ (i*j) % n ];
               break;

            case 24:
               if (row_ct == 4 && col_ct == 1 ||
                   row_ct == 8 && col_ct == 5)
                  character_table(row_ct, col_ct++) = { 0, 1, 3 };
               else if (row_ct == 4 && col_ct == 5 ||
                        row_ct == 8 && col_ct == 1)
                  character_table(row_ct, col_ct++) = { 0, -1, 3 };
               else
                  character_table(row_ct, col_ct++) = cyclotomics[ (i*j) % n ];
               break;
               
            default:
               break;
         }
      }
      // trace h = trace gh = 0, so we're done
   }
   return character_table;
}

inline
BigObject dihedral_group_impl(Int n2)
{   
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const Int n = n2/2;
   const bool odd = n%2;

   BigObject a("group::PermutationAction");
   const auto class_reps(dn_reps(n2));
   a.take("GENERATORS") << Array<Array<Int>> { class_reps[1], odd ? class_reps.back() : class_reps[class_reps.size()-2] };
   a.take("CONJUGACY_CLASS_REPRESENTATIVES") << class_reps;

   BigObject Dn("group::Group");
   
   Dn.take("ORDER") << 2*n;
   Dn.take("CHARACTER_TABLE") << dn_character_table(n2);
   Dn.take("PERMUTATION_ACTION") << a;
   Dn.set_description() << "Dihedral group of order " << 2*n << endl;
   return Dn;
}

inline
ConjugacyClassReps<Array<Int>>
sn_reps(Int n) {
   switch(n) {

   case 1:
      return { Array<Int>(1,0) };

   case 2:
      return { {0, 1}, {1, 0} };

   case 3:
      return { {0, 1, 2}, {1, 0, 2}, {1, 2, 0} };

   case 4:
      return { {0, 1, 2, 3}, {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0} };

   case 5:
      return { {0, 1, 2, 3, 4}, {1, 0, 2, 3, 4}, {1, 0, 3, 2, 4}, {1, 2, 0, 3, 4},
               {1, 2, 0, 4, 3}, {1, 2, 3, 0, 4}, {1, 2, 3, 4, 0} };

   case 6:
      return { {0, 1, 2, 3, 4, 5}, {1, 0, 2, 3, 4, 5}, {1, 0, 3, 2, 4, 5}, {1, 0, 3, 2, 5, 4}, {1, 2, 0, 3, 4, 5},
               {1, 2, 0, 4, 3, 5}, {1, 2, 0, 4, 5, 3}, {1, 2, 3, 0, 4, 5}, {1, 2, 3, 0, 5, 4}, {1, 2, 3, 4, 0, 5},
               {1, 2, 3, 4, 5, 0} };

   case 7:
      return { {0, 1, 2, 3, 4, 5, 6}, {1, 0, 2, 3, 4, 5, 6}, {1, 0, 3, 2, 4, 5, 6}, {1, 0, 3, 2, 5, 4, 6},
               {1, 2, 0, 3, 4, 5, 6}, {1, 2, 0, 4, 3, 5, 6}, {1, 2, 0, 4, 3, 6, 5}, {1, 2, 0, 4, 5, 3, 6},
               {1, 2, 3, 0, 4, 5, 6}, {1, 2, 3, 0, 5, 4, 6}, {1, 2, 3, 0, 5, 6, 4}, {1, 2, 3, 4, 0, 5, 6},
               {1, 2, 3, 4, 0, 6, 5}, {1, 2, 3, 4, 5, 0, 6}, {1, 2, 3, 4, 5, 6, 0} };
      
   default:
      throw std::runtime_error("Character tables and conjugacy classes for S_n, n>=8, are not implemented");
   }
}   

inline
Matrix<CharacterNumberType> sn_character_table(Int n) {
   switch (n) {
   case 1:
      return {
         { 1 }
      };

   case 2:
      return {
         { 1, -1 },
         { 1,  1 }
      };

   case 3:
      return {
        {1, -1,  1},
        {2,  0, -1},
        {1,  1,  1}
      };

   case 4:
      return {
         {1, -1,  1,  1, -1},
         {3, -1, -1,  0,  1},
         {2,  0,  2, -1,  0},
         {3,  1, -1,  0, -1},
         {1,  1,  1,  1,  1}
      };

   case 5:
      return {
         { 1, -1,  1,  1, -1, -1,  1},
         { 4, -2,  0,  1,  1,  0, -1},
         { 5, -1,  1, -1, -1,  1,  0},
         { 6,  0, -2,  0,  0,  0,  1},
         { 5,  1,  1, -1,  1, -1,  0},
         { 4,  2,  0,  1, -1,  0, -1},
         { 1,  1,  1,  1,  1,  1,  1}
      };

   case 6:
      return {
         { 1, -1,  1, -1,  1, -1,  1, -1,  1,  1, -1},
         { 5, -3,  1,  1,  2,  0, -1, -1, -1,  0,  1},
         { 9, -3,  1, -3,  0,  0,  0,  1,  1, -1,  0},
         { 5, -1,  1,  3, -1, -1,  2,  1, -1,  0,  0},
         {10, -2, -2,  2,  1,  1,  1,  0,  0,  0, -1},
         {16,  0,  0,  0, -2,  0, -2,  0,  0,  1,  0},
         { 5,  1,  1, -3, -1,  1,  2, -1, -1,  0,  0},
         {10,  2, -2, -2,  1, -1,  1,  0,  0,  0,  1},
         { 9,  3,  1,  3,  0,  0,  0, -1,  1, -1,  0},
         { 5,  3,  1, -1,  2,  0, -1,  1, -1,  0, -1},
         { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1}
      };

   case 7:
      return {
         { 1, -1,  1, -1,  1, -1,  1,  1, -1,  1, -1,  1, -1, -1,  1},
         { 6, -4,  2,  0,  3, -1, -1,  0, -2,  0,  1,  1,  1,  0, -1},
         {14, -6,  2, -2,  2,  0,  2, -1,  0,  0,  0, -1, -1,  1,  0},
         {14, -4,  2,  0, -1, -1, -1,  2,  2,  0, -1, -1,  1,  0,  0},
         {15, -5, -1,  3,  3,  1, -1,  0, -1, -1, -1,  0,  0,  0,  1},
         {35, -5, -1, -1, -1,  1, -1, -1,  1,  1,  1,  0,  0, -1,  0},
         {21, -1,  1,  3, -3, -1,  1,  0,  1, -1,  1,  1, -1,  0,  0},
         {21,  1,  1, -3, -3,  1,  1,  0, -1, -1, -1,  1,  1,  0,  0},
         {20,  0, -4,  0,  2,  0,  2,  2,  0,  0,  0,  0,  0,  0, -1},
         {35,  5, -1,  1, -1, -1, -1, -1, -1,  1, -1,  0,  0,  1,  0},
         {14,  4,  2,  0, -1,  1, -1,  2, -2,  0,  1, -1, -1,  0,  0},
         {15,  5, -1, -3,  3, -1, -1,  0,  1, -1,  1,  0,  0,  0,  1},
         {14,  6,  2,  2,  2,  0,  2, -1,  0,  0,  0, -1,  1, -1,  0},
         { 6,  4,  2,  0,  3,  1, -1,  0,  2,  0, -1,  1, -1,  0, -1},
         { 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1}
      };
      
   default:
      throw std::runtime_error("Character tables and conjugacy classes for S_n, n>=8, are not implemented");
   }
}

inline
Array<Array<Int>>
symmetric_group_gens(Int n)
{
   Array<Array<Int>> sgs(n-1);
   for (Int i = 0; i < n-1; ++i) {
      Array<Int> gen(n);
      for (Int j = 0; j < n; ++j)
         gen[j] = j;
      std::swap(gen[i], gen[i+1]);
      sgs[i] = gen;
   }
   return sgs;
}

inline
BigObject symmetric_group_impl(Int n)
{
   if (n < 1) 
      throw std::runtime_error("symmetric_group: the degree must be greater or equal than 1");
   
   BigObject pa("group::PermutationAction");
   pa.take("GENERATORS") << symmetric_group_gens(n);
   if (n <= 7) {
      pa.take("CONJUGACY_CLASS_REPRESENTATIVES") << sn_reps(n);
   }      
   
   BigObject g("group::Group");
   g.take("PERMUTATION_ACTION") << pa;
   g.set_description() << "Symmetric group of degree " << n << endl;

   if (n <= 7) {
      g.take("CHARACTER_TABLE") << sn_character_table(n);
   }

   return g;
}

      
} }

#endif // __NAMED_GROUPS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


