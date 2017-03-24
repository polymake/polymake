/* Copyright (c) 1997-2016
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

#ifndef __NAMED_GROUPS_H
#define __NAMED_GROUPS_H

#include "polymake/group/action_datatypes.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace group {

namespace {

inline
int
dn_n_class_reps(int n2)
{
   const int n(n2/2);
   const bool odd(n%2);
   return odd
      ? (n-1)/2 + 2
      : n/2 + 3;
}

}

inline
ConjugacyClassReps dn_reps(int n2)
{
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const int n(n2/2);
   const bool odd(n%2);
   ConjugacyClassReps class_reps(dn_n_class_reps(n2));
   Entire<ConjugacyClassReps>::iterator crit = entire(class_reps);

   /*
      n odd:
      classes (g^j, j=0.. (odd ? (n-1)/2 : n/2)
    */

   // conjugacy classes of type g^j, g = (0 1 2 ... n-1), j = 0..floor(n/2)
   for (int j=0; j <= n/2; ++j) {
      Array<int> c(n);
      int ct(-1);
      for (int i=j; i<n; ++i)
         c[i] = ++ct;
      for (int i=0; i<j; ++i)
         c[i] = ++ct;
      *crit = c; ++crit;
   }

   Array<int> h(n);
   if (odd) {
      for (int i=1; i <= (n-1)/2; ++i) {
         h[i] = n-i;
         h[n-i] = i;
      }
   } else {
      for (int i=0; i <= n/2; ++i) {
         h[i] = n-1-i;
         h[n-1-i] = i;
      }
   }
   *crit = h; ++crit;
   
   if (!odd) {
      Array<int> gh(n);
      for (int i=0; i<n; ++i)
         gh[i] = class_reps[1][h[i]];
      *crit = gh;
   }
   return class_reps;
}

inline
Matrix<CharacterNumberType> dn_character_table(int n2)
{
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const int n(n2/2);
   const bool odd(n%2);
   Array<CharacterNumberType> cyclotomics(n);
   for (int i=0; i<n; ++i) {
      cyclotomics[i] = rounded_if_integer(2 * cos(2 * AccurateFloat::pi() * i / n));
   }
   
   const int n_reps(dn_n_class_reps(n2));
   Matrix<CharacterNumberType> character_table(n_reps, n_reps);
   int row_ct=0, col_ct=0;

   // 1d reps
   // the trivial rep
   for (int j=0; j<n_reps; ++j)
      character_table(0,j) = 1;
   ++row_ct;

   // g^i -> 1, h->-1, [gh->-1]
   for (int j=0; j <= n/2; ++j)
      character_table(row_ct, col_ct++) = 1;
   character_table(row_ct, col_ct++) = -1;
   if (!odd)
      character_table(row_ct, col_ct) = -1;
   ++row_ct;
   
   if (!odd) {
      // two more: 
      // g^k -> (-1)^k, h -> 1, gh -> -1
      col_ct = 0;
      for (int j=0; j <= n/2; ++j)
         character_table(row_ct, col_ct++) = (j%2) ? -1 : 1;
      character_table(row_ct, col_ct++) = 1;
      character_table(row_ct, col_ct) = -1;
      ++row_ct;

      // g^k -> (-1)^k, h -> -1, gh -> 1
      col_ct = 0;
      for (int j=0; j <= n/2; ++j)
         character_table(row_ct, col_ct++) = (j%2) ? -1 : 1;
      character_table(row_ct, col_ct++) = -1;
      character_table(row_ct, col_ct) = 1;
      ++row_ct;
   }

   // 2d reps
   const int n_2d (odd ? (n-1)/2 : (n-2)/2);
   for (int i=1; i<=n_2d; ++i, ++row_ct) {
      col_ct=0;
      character_table(row_ct, col_ct++) = 2; // trace of identity
      for (int j=1; j<=n/2; ++j) {
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
perl::Object dihedral_group_impl(int n2)
{   
   if (n2%2)
      throw std::runtime_error("The order must be even.");

   const int n(n2/2);
   const bool odd(n%2);

   perl::Object a("group::PermutationAction");
   const auto class_reps(dn_reps(n2));
   a.take("GENERATORS") << Array<Array<int>> { class_reps[1], odd ? class_reps.back() : class_reps[class_reps.size()-2] };
   a.take("CONJUGACY_CLASS_REPRESENTATIVES") << class_reps;

   perl::Object Dn("group::Group");
   
   Dn.take("ORDER") << 2*n;
   Dn.take("CHARACTER_TABLE") << dn_character_table(n2);
   Dn.take("PERMUTATION_ACTION") << a;
   Dn.set_description() << "Dihedral group of order " << 2*n << endl;
   return Dn;
}

} }

#endif // __NAMED_GROUPS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


