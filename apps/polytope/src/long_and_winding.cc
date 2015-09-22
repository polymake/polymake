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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace polytope {


perl::Object long_and_winding(int r, perl::OptionSet options)
{
  typedef Rational coefficient;
  typedef Rational exponent;
  typedef PuiseuxFraction<Max, coefficient, exponent> coeff_field;
    
   if (r < 1)
      throw std::runtime_error("parameter r >= 1 required");

   bool eval_ratio_flag = options.exists("eval_ratio");
   bool eval_float_flag = options.exists("eval_float");

   SparseMatrix<coeff_field > I(3*r+4, 2*r+3);
   Rows< SparseMatrix<coeff_field> >::iterator f=rows(I).begin();
   const UniMonomial<coefficient, exponent> ht(1);
   const coeff_field t(ht);
   const Rational half(1,2);

   (*f)[0] = t; (*f)[1] = -1; ++f;   // u_0 =< t
   (*f)[0] = t*t; (*f)[2] = -1; ++f; // v_0 =< t^2

   Vector<coeff_field > u(r+1);
   Vector<coeff_field > v(r+1);

   u[0] = half * t; v[0] = half * t*t;
     
   for (int i=1; i<=r; ++i) { 
      const Rational expo(Integer::pow(2,i)-1, Integer::pow(2,i));
      UniMonomial<coefficient, exponent> hs(expo);
      coeff_field s(hs);
      (*f)[2*i-1] = t; (*f)[2*i+1] = -1; ++f;           // u_i =< t*u_(i-1)
      (*f)[2*i] = t; (*f)[2*i+1] = -1; ++f;             // u_i =< t*v_(i-1)
      (*f)[2*i+2] = -1; (*f)[2*i-1] = s; (*f)[2*i] = s; // v_i =< s*( u_(i-1)+v_(i-1) )
      ++f; 

      u[i] = half * t * u[i-1]; // recursively defined interior point
      v[i] = half * s * (u[i-1] + v[i-1]);
   }
   (*f)[2*r+1] = 1; ++f; // u_r >= 0
   (*f)[2*r+2] = 1; // v_r >= 0

   u |= v;
   u = 1 | u;

   perl::Object p;
   
   if ( eval_ratio_flag ) {
      Rational value = options["eval_ratio"];
      long exp = options.exists("eval_exp") ? options["eval_exp"] : 1;
      SparseMatrix<Rational> Iu_eval = PuiseuxFraction<Max, Rational, Rational>::evaluate(I/u, value, exp);
      SparseMatrix<Rational> I_eval = Iu_eval.minor(~scalar2set(3*r+4),All);;
      Vector<Rational> u_eval = Iu_eval.row(3*r+4);
      p = perl::Object(perl::ObjectType::construct<Rational>("Polytope"));
      p.take("FACETS") << I_eval;
      p.take("REL_INT_POINT") << u_eval;
   }
   else if ( eval_float_flag) {
      double value = options["eval_float"];
      SparseMatrix<double> Iu_eval = PuiseuxFraction<Max, Rational, Rational>::evaluate_float(I/u, value);
      SparseMatrix<double> I_eval = Iu_eval.minor(~scalar2set(3*r+4),All);;
      Vector<double> u_eval = Iu_eval.row(3*r+4);
      p = perl::Object(perl::ObjectType::construct<double>("Polytope"));
      p.take("FACETS") << I_eval;
      p.take("REL_INT_POINT") << u_eval;
   } else {
      p = perl::Object(perl::ObjectType::construct<coeff_field>("Polytope"));
      p.take("FACETS") << I;
      p.take("REL_INT_POINT") << u;
   }
   p.set_description() << "Long and winding path polytope with parameter " << r << "." << endl;

   p.take("FEASIBLE") << true;
   p.take("BOUNDED") << true;
   p.take("POINTED") << true;
   p.take("FULL_DIM") << true;
   p.take("CONE_AMBIENT_DIM") << 2*r+3;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce polytope in dimension 2r+2 with 3r+4 facets such that the total curvature"
                  "# of the central path is at least Omega(2^r).  This establishes a counter-example to" 
                  "# a continuous analog of the Hirsch conjecture by Deza, Terlaky and Zinchenko,"
                  "# Adv. Mech. Math. 17 (2009).  The construction and its analysis can be found in"
                  "# Allamigeon, Benchimol, Gaubert and Joswig, arXiv: 1405.4161"
                  "# @param Int r defining parameter"
                  "# @option Rational eval_ratio parameter for evaluating the puiseux rational functions"
                  "# @option Int eval_exp to evaluate at eval_ratio^eval_exp, default: 1"
                  "# @option Float eval_float parameter for evaluating the puiseux rational functions"
                  "# @return Polytope<PuiseuxFraction<Max, Rational, Rational> >",
                  &long_and_winding, "long_and_winding(Int, {eval_ratio => undef, eval_float => undef, eval_exp => undef} )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
