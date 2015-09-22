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

#include "polymake/ideal/internal/singularConvertTypes.h"

namespace polymake {
namespace ideal {
namespace singular{

static coeffs singular_rational = NULL;


// Convert a Singular number to a GMP rational.
Rational convert_number_to_Rational(number n, ring ring)
{
   if(rField_is_Q(ring)){
      if(SR_HDL(n) & SR_INT){
         long l = SR_TO_INT(n);
         return Rational(l, 1);
      } else {
         switch(n->s){
            case 0:
               return Rational(n->z, n->n);
            case 1:
               return Rational(n->z, n->n);
            case 3:
               return Rational(n->z);
         }
      }
   }
   throw std::runtime_error("I can has number? :P");
}


// Convert a GMP rational to a Singular number.
number convert_Rational_to_number(const Rational& r)
{
   if (singular_rational == NULL)
      singular_rational = nInitChar(n_Q, NULL);
   // cast removes the const as singular just uses it to initialize its own mpz
   number num = n_InitMPZ((mpz_ptr) numerator(r).get_rep(),singular_rational);
   number denom = n_InitMPZ((mpz_ptr) denominator(r).get_rep(),singular_rational);
   number res = n_Div(num,denom,singular_rational);
   n_Delete(&num,singular_rational);
   n_Delete(&denom,singular_rational);
   return res;
}


Polynomial<> convert_poly_to_Polynomial(const poly q, const Ring<>& r){
   int n = r.n_vars();
   poly p = pCopy(q);
   ListMatrix<Vector<int> > exponents(0,n);
   std::vector<Rational> coefficients;
   while(p != NULL){
      number c = pGetCoeff(p);
      coefficients.push_back(convert_number_to_Rational(c, currRing));
      Vector<int> monomial(n);
      for(int i = 1; i<=n; i++){
         monomial[i-1] = pGetExp(p, i);
      }
      exponents /= monomial;
      pIter(p);
   }
   return Polynomial<>(exponents, coefficients, r);
}


poly convert_Polynomial_to_poly(const Polynomial<>& mypoly, ring ring){
   poly p = p_ISet(0,ring);
   for(Entire<Polynomial<>::term_hash>::const_iterator term = entire(mypoly.get_terms()); !term.at_end(); ++term)
   {
      poly monomial = p_NSet(convert_Rational_to_number(term->second),ring);
      for(int k = 0; k<term->first.dim(); k++)
      {
         p_SetExp(monomial,k+1,term->first[k],ring);
      }
      p_Setm(monomial,ring);
      p = p_Add_q(p, monomial,ring);
   }
   return p;
}


} // end namespace singular
} // end namespace ideal
} // end namespace polymake


