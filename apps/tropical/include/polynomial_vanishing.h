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

#pragma once

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Polynomial.h"

namespace polymake{ namespace tropical {

template <typename Addition, typename Scalar>
TropicalNumber<Addition, Scalar> evaluate_monomial(const Vector<Int>& exp, const Vector<TropicalNumber<Addition, Scalar>>& pt)
{
   Scalar result(0);
   for(auto e=entire<indexed>(exp); !e.at_end(); ++e){
      if(*e != 0){
         // We will not be able to convert tropical zero (which is +-
         // infinity), but this would kill the term anyway.
         if(pt[e.index()] != TropicalNumber<Addition, Scalar>::zero()){
            result += convert_to<Scalar>(pt[e.index()]) * (*e);
         } else {
            return TropicalNumber<Addition, Scalar>::zero();
         }
      }
   }
   return TropicalNumber<Addition, Scalar>(result);
}

/*
 * @brief: Find the indices of the terms that attain their Min/Max at a given point
 */
template <typename Addition, typename Scalar>
Set<Int> polynomial_support(const Polynomial<TropicalNumber<Addition, Scalar>>& poly, const Vector<TropicalNumber<Addition, Scalar>>& pt)
{
   Set<Int> result;
   Int counter = 0;
   TropicalNumber<Addition, Scalar> val(TropicalNumber<Addition, Scalar>::zero());
   for (auto t = entire(poly.get_terms()); !t.at_end(); ++t){
      TropicalNumber<Addition, Scalar> tmp(val);
      TropicalNumber<Addition, Scalar> term = t->second * evaluate_monomial(t->first, pt);
      val += term;
      if(val != tmp){
         result.clear();
      }
      if(term == val){
         result += counter;
      }
      counter++;
   }
   return result;
}

template <typename Addition, typename Scalar>
bool polynomial_vanishes(const Polynomial<TropicalNumber<Addition, Scalar>>& poly, const Vector<TropicalNumber<Addition, Scalar>>& pt)
{
   return polynomial_support(poly, pt).size() >= 2;
}


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
