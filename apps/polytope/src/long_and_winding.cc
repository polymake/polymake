/* Copyright (c) 1997-2019
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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/PuiseuxFraction.h"
#include "polymake/Polynomial.h"


namespace polymake { namespace polytope {

      namespace {
         typedef Rational coefficient;
         typedef Rational exponent;
         typedef PuiseuxFraction<Max, coefficient, exponent> puiseux_field;
         typedef UniPolynomial<coefficient, exponent> poly_type;
         typedef SparseMatrix<puiseux_field> matrix_type;
         typedef Vector<puiseux_field> vector_type;
         typedef std::pair<matrix_type,vector_type> matrix_vector_pair;

         const poly_type up_t(1,1);
         const puiseux_field t(up_t); // infinitesimally large
 
         perl::Object construct_polytope(const SparseMatrix<puiseux_field>& I, const Vector<puiseux_field>& u, perl::OptionSet options) {
            // in the (exact) Puiseux case: I = FACETS, u = REL_INT_POINT
            // requires more care in the Rational or Float case
            const int d=I.cols();
            const int m=I.rows();
            
            bool eval_ratio_flag = options.exists("eval_ratio");
            bool eval_float_flag = options.exists("eval_float");

            perl::Object p;
   
            bool is_interior_point=true;
            if ( eval_ratio_flag ) {
               Rational value = options["eval_ratio"];
               if (value <= 0) 
                  throw std::runtime_error("long_and_winding: evaluation for positive values only");
               long exp = options.exists("eval_exp") ? options["eval_exp"] : 1;
               SparseMatrix<Rational> Iu_eval = evaluate(I/u, value, exp);
               SparseMatrix<Rational> I_eval = Iu_eval.minor(~scalar2set(m),All);;
               Vector<Rational> u_eval = Iu_eval.row(m);
               p = perl::Object("Polytope", mlist<Rational>());
               p.take("INEQUALITIES") << I_eval;
               for (int i=0; is_interior_point && (i<m); ++i) {
                  if (I_eval[i]*u_eval <= 0) is_interior_point=false;
               }
               if (is_interior_point) {
                  p.take("REL_INT_POINT") << u_eval;
                  p.take("FEASIBLE") << true;
                  p.take("FULL_DIM") << true;
               }
               perl::Object LP("LinearProgram", mlist<Rational>());
               LP.take("LINEAR_OBJECTIVE") << unit_vector<Rational>(d,1);
               p.take("LP") << LP;
            }
            else if ( eval_float_flag ) {
               double value = options["eval_float"];
               if (value <= 0.0) 
                  throw std::runtime_error("long_and_winding: evaluation for positive values only");
               SparseMatrix<double> Iu_eval = evaluate_float(I/u, value);
               SparseMatrix<double> I_eval = Iu_eval.minor(~scalar2set(m),All);;
               Vector<double> u_eval = Iu_eval.row(m);
               p = perl::Object("Polytope<double>");
               p.take("INEQUALITIES") << I_eval;
               for (int i=0; is_interior_point && (i<m); ++i) {
                  if (I_eval[i]*u_eval <= 0) is_interior_point=false;
               }
               if (is_interior_point) {
                  p.take("REL_INT_POINT") << u_eval;
                  p.take("FEASIBLE") << true;
                  p.take("FULL_DIM") << true;
               }
               perl::Object LP("LinearProgram", mlist<double>());
               LP.take("LINEAR_OBJECTIVE") << unit_vector<double>(d,1);
               p.take("LP") << LP;
            } else {
               p = perl::Object("Polytope", mlist<puiseux_field>());
               p.take("FACETS") << I;
               p.take("AFFINE_HULL") << SparseMatrix<puiseux_field>(0,d);
               p.take("REL_INT_POINT") << u;
               perl::Object LP("LinearProgram", mlist<puiseux_field>());
               LP.take("LINEAR_OBJECTIVE") << unit_vector<puiseux_field>(d,1);
               p.take("LP") << LP;
            }

            p.take("BOUNDED") << true;
            p.take("POINTED") << true;
            p.take("CONE_AMBIENT_DIM") << d;

            return p;
         }

         matrix_vector_pair unperturbed_inequalities_and_interior_point(int r) {
            matrix_type I(3*r+1, 2*r+1);
            Rows<matrix_type>::iterator f=rows(I).begin();
             const Rational half(1,2);
            
            (*f)[0] = t*t; (*f)[1] = -1; ++f; // x_1 <= t^2
            (*f)[0] = t;   (*f)[2] = -1; ++f; // x_2 <= t
            
            // exponent vector of interior point bfx (in the Puiseux case), to beconstructed iteratively
            // i.e., the vector x^\lambda for \lambda=2 in Proposition 13
            Vector<Rational> x(2*r+1);
            x[0] = 1;
            x[1] = 2; x[2] = 1;
            
            // interior point bfx with exponents from x, coefficients descending
            vector_type bfx(2*r+1);
            bfx[0] = 1;
            bfx[1] = Rational(1,2)*t*t; bfx[2] = Rational(1,3)*t;
   
            Integer two_to_j(1);
            for (int j=1; j<r; ++j) {
               two_to_j *= 2; // 2^j
               const Rational expo(two_to_j-1, two_to_j);
               poly_type up_s(1, expo);
               puiseux_field s(up_s); // t^{1-1/2^j}
               (*f)[2*j-1] = t; (*f)[2*j+1] = -1; ++f;                // x_{2j+1} <= t*x_{2j-1}
               (*f)[2*j] = t; (*f)[2*j+1] = -1; ++f;                  // x_{2j+1} <= t*x_{2j}
               (*f)[2*j-1] = s; (*f)[2*j] = s; (*f)[2*j+2] = -1; ++f; // x_{2j+2} <= t^{1-1/2^j}*(x_{2j-1} + x_{2j})
               
               x[2*j+1] = 1 + std::min(x[2*j-1], x[2*j]);
               x[2*j+2] = expo + std::max(x[2*j-1], x[2*j]);
               
               poly_type up_bfx_coeff(Rational(1,2*j+2), x[2*j+1]);
               bfx[2*j+1] = puiseux_field(up_bfx_coeff);
               up_bfx_coeff = poly_type(Rational(1,2*j+3), x[2*j+2]);
               bfx[2*j+2] = puiseux_field(up_bfx_coeff);
            }
            
            (*f)[2*r-1] = 1; ++f; // x_{2r-1} >= 0
            (*f)[2*r] = 1;        // x_{2r}   >= 0

            return std::pair<matrix_type,vector_type>(I,bfx);
         }
            
      }


perl::Object long_and_winding(int r, perl::OptionSet options)
{
   if (r < 1)
      throw std::runtime_error("long_and_winding: parameter r >= 1 required");

   matrix_vector_pair I_bfx=unperturbed_inequalities_and_interior_point(r);
   matrix_type I(I_bfx.first);
   vector_type bfx(I_bfx.second);
   perl::Object p = construct_polytope(I,bfx,options);
   p.set_description() << "Unperturbed long and winding path polytope with parameter " << r << "." << endl;
   
   return p;
}

perl::Object perturbed_long_and_winding(int r, perl::OptionSet options)
{
   if (r < 1)
      throw std::runtime_error("long_and_winding: parameter r >= 1 required");

   matrix_vector_pair I_bfx=unperturbed_inequalities_and_interior_point(r);
   matrix_type I(I_bfx.first);
   vector_type bfx(I_bfx.second);

   // perturb last facet
   I(3*r,0) = -1/t; // x_{2r} >= \gamma
   
   perl::Object p = construct_polytope(I,bfx,options);

   p.set_description() << "Perturbed (and thus simple) long and winding path polytope with parameter " << r << "." << endl;
   
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce polytope in dimension 2r with 3r+2 facets such that the total curvature"
                  "# of the central path is at least Omega(2^r); see "
                  "# Allamigeon, Benchimol, Gaubert and Joswig, SIAM J. Appl. Algebra Geom. (2018)."
                  "# See also [[perturbed_long_and_winding]]."
                  "# @param Int r defining parameter"
                  "# @option Rational eval_ratio parameter for evaluating the puiseux rational functions"
                  "# @option Int eval_exp to evaluate at eval_ratio^eval_exp, default: 1"
                  "# @option Float eval_float parameter for evaluating the puiseux rational functions"
                  "# @return Polytope<PuiseuxFraction<Max, Rational, Rational> >"
                  "# @example This yields a 4-polytope over the field of Puiseux fractions."
                  "# > $p = long_and_winding(2);"
                  "# @example This yields a rational 4-polytope with the same combinatorics."
                  "# > $p = long_and_winding(2,eval_ratio=>2);",
                  &long_and_winding, "long_and_winding(Int, {eval_ratio => undef, eval_float => undef, eval_exp => undef} )");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce polytope in dimension 2r with 3r+2 facets such that the total curvature"
                  "# of the central path is at least Omega(2^r)."
                  "# This is a perturbed version of [[long_and_winding]], which yields simple polytopes."
                  "# @param Int r defining parameter"
                  "# @option Rational eval_ratio parameter for evaluating the puiseux rational functions"
                  "# @option Int eval_exp to evaluate at eval_ratio^eval_exp, default: 1"
                  "# @option Float eval_float parameter for evaluating the puiseux rational functions"
                  "# @return Polytope<PuiseuxFraction<Max, Rational, Rational> >"
                  "# @example This yields a simple 4-polytope over the field of Puiseux fractions."
                  "# > $p = perturbed_long_and_winding(2);",
                  &perturbed_long_and_winding, "perturbed_long_and_winding(Int, {eval_ratio => undef, eval_float => undef, eval_exp => undef} )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
