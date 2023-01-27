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

#include "polymake/client.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {
namespace {

bool feasible(const Vector<Rational>& point, const Matrix<Rational>& Inequalities)
{
   for (auto i=entire(rows(Inequalities)); !i.at_end(); ++i)
      if (point*(*i)<0) return false;
   return true;
}

bool feasible_after_update(Vector<Rational>& diff_vec, const Vector<Rational>& ineqs_col)
{
   diff_vec -= ineqs_col;  // update diff vector (to -b + Az_i)

   for (Int i = 0; i < diff_vec.dim(); ++i) {
      if (diff_vec[i] < 0) return false; // infeasible
   }

   return true; // feasible
}

}

//Maximization with utility vector c=(1,...,1)
//Assumption: the LP is feasible and bounded in the direction of c
ListReturn find_transitive_lp_sol(const Matrix<Rational>& Inequalities)
{
   const Int N_cols = Inequalities.cols();
   const Int dim = N_cols-1;
   Rational lower_bound = std::numeric_limits<Rational>::min();
   Rational upper_bound = std::numeric_limits<Rational>::max();
   bool max_bounded = true;
   bool feasible = true;
   Rational a = std::numeric_limits<Rational>::max();
   for (auto i = entire(rows(Inequalities)); !i.at_end(); ++i) {
      Rational sum = 0;
      for (Int j = 1; j < N_cols; ++j) { //add up all coefficients of one ineq (=project to fixed space spanned by (1,...,1))
         sum += (*i)[j];
      }
      if (sum != 0) { // Facets with normalvectors orthogonal to (1,...,1) do not yield bounds
                      // therefore, we can neglect them!
         Rational constraint_val = -(*i)[0]/sum;
         Rational bound = dim*constraint_val;
         if (sum>0 && bound > lower_bound) { //new lower bound
            lower_bound = bound;
         } else if(sum < 0 && bound < upper_bound) { //new upper bound
            a = constraint_val;
            upper_bound = bound;
         }
      }
   }
   if (upper_bound < lower_bound) {
      feasible = false;
   } else if (isinf(upper_bound) != 0) {
      max_bounded = false;
   }
   Vector<Rational> optLPsolution(1 | same_element_vector(a,dim));
   Rational optLPvalue=upper_bound;
   ListReturn result;
   result << optLPsolution
          << optLPvalue
          << feasible
          << max_bounded;
   return result;
}


ListReturn core_point_algo(BigObject p, const Rational optLPvalue, OptionSet options)
{
   const Matrix<Rational> Inequalities = p.give("FACETS|INEQUALITIES");
   const Int n = p.call_method("AMBIENT_DIM");
   const Integer d = floor(optLPvalue / n);
   const Integer r_start = floor(optLPvalue) % n;

   const bool verbose = options["verbose"];

   if (verbose)
      cout << "dimension=" << n << ", "
           << "LP approximation=" << optLPvalue << ", "
           << "d=" << d << endl;

   Int r = Int(r_start);
   bool ILP_not_feasible = true;
   Vector<Rational> optILPsolution;
   Rational optILPvalue;
   while (r>=0 && ILP_not_feasible) {
      if (verbose)
         cout << "trying r=" << r << endl;
      const Vector<Rational> core_point(1|same_element_vector<Rational>(d+1,r)|same_element_vector<Rational>(d,n-r));
      if (feasible(core_point,Inequalities)) {
         optILPsolution = core_point;
         optILPvalue=(d+1)*r+d*(n-r);
         ILP_not_feasible=false;
      } else
         --r;
   }

   ListReturn result;

   if (ILP_not_feasible) {
      if (verbose)
         cout << "ILP infeasible" << endl;
   } else {
      if (verbose)
         cout << "optimal ILP solution=" << optILPsolution << ", "
              << "value=" << optILPvalue << endl;
      result << optILPsolution
             << optILPvalue;
   }

   return result;
}


ListReturn core_point_algo_Rote(BigObject p, const Rational& optLPvalue, OptionSet options)
{
   const Matrix<Rational> Inequalities=p.give("FACETS|INEQUALITIES");
   const Int n = p.call_method("AMBIENT_DIM");
   const Integer d = floor(optLPvalue / n);
   const Integer r_start = floor(optLPvalue) % n;

   const bool verbose = options["verbose"];

   if (verbose)
      cout << "dimension=" << n << ", "
           << "LP approximation=" << optLPvalue << ", "
           << "d=" << d << endl;

   Int r = Int(r_start);
   bool ILP_not_feasible = true;
   Vector<Rational> optILPsolution;
   Rational optILPvalue;

   // initializing the difference vector b-A*current_core_point
   Vector<Rational> init_core_point(1|same_element_vector<Rational>(d+1,r)|same_element_vector<Rational>(d,n-r));
   Vector<Rational> diff_vec(Inequalities.rows());
   bool init_is_feasible = true;

   for (Int i = 0; i < Inequalities.rows(); ++i) {
      diff_vec[i] = init_core_point*Inequalities[i];
      if ( diff_vec[i] < 0 ) init_is_feasible = false;
   }
   // finished initialization

   if (init_is_feasible) {
      optILPsolution = init_core_point;
      optILPvalue=(d+1)*r+d*(n-r);
      ILP_not_feasible=false;
   } else { // initial core point was not feasible; test others via update of diff_vec
      --r;

      while (r>=0 && ILP_not_feasible) {
         if (verbose)
            cout << "trying r=" << r << endl;

         if ( feasible_after_update(diff_vec, Inequalities.col(r+1)) ) { // do comparison via update of vector -b+Az, A=ineqs,z=core point
            const Vector<Rational> core_point(1|same_element_vector<Rational>(d+1,r)|same_element_vector<Rational>(d,n-r));
            optILPsolution = core_point;
            optILPvalue=(d+1)*r+d*(n-r);
            ILP_not_feasible=false;
         } else
            --r;
      }
   }

   ListReturn result;

   if (ILP_not_feasible) {
      if (verbose)
         cout << "ILP infeasible" << endl;
   } else {
      if (verbose)
         cout << "optimal ILP solution=" << optILPsolution << ", "
              << "value=" << optILPvalue << endl;
      result << optILPsolution
             << optILPvalue;
   }

   return result;
}


UserFunction4perl("# @category Optimization"
                  "# Algorithm to solve highly symmetric integer linear programs (ILP)."
                  "# It is required that the group of the ILP induces the alternating or symmetric group"
                  "# on the set of coordinate directions."
                  "# The linear objective function is the vector (0,1,1,..,1)."
                  "# "
                  "# @param Polytope p"
                  "# @param Rational optLPvalue optimal value of LP approximation"
                  "# @option Bool verbose"
                  "# @return List (Vector<Rational> optimal solution, Rational optimal value) may be empty",
                  &core_point_algo, "core_point_algo(Polytope, $; {verbose => undef})");

UserFunction4perl("# @category Optimization"
                  "# Version of core_point_algo with improved running time"
                  "# (according to a suggestion by G. Rote)."
                  "# The core_point_algo is an algorithm to solve highly symmetric integer linear programs (ILP)."
                  "# It is required that the group of the ILP induces the alternating or symmetric group"
                  "# on the set of coordinate directions."
                  "# The linear objective function is the vector (0,1,1,..,1)."
                  "# "
                  "# @param Polytope p"
                  "# @param Rational optLPvalue optimal value of LP approximation"
                  "# @option Bool verbose"
                  "# @return List (Vector<Rational> optimal solution, Rational optimal value) may be empty",
                  &core_point_algo_Rote, "core_point_algo_Rote(Polytope, $; {verbose => undef})");

UserFunction4perl("# @category Optimization"
                  "# Algorithm to solve symmetric linear programs (LP) of the form"
                  "# max c<sup>t</sup>x , c=(0,1,1,..,1)"
                  "# subject to the inequality system given by //Inequalities//."
                  "# It is required that the symmetry group of the LP acts transitively"
                  "# on the coordinate directions."
                  "# "
                  "# @param Matrix Inequalities the inequalities describing the feasible region"
                  "# @return List (Vector<Rational> optimal solution, Rational optimal value, Bool feasible, Bool max_bounded)"
                  "# @example Consider the LP described by the facets of the 3-cube:"
                  "# > @sol=find_transitive_lp_sol(cube(3)->FACETS);"
                  "# > print $_, \"\\n\" for @sol;"
                  "# | 1 1 1 1"
                  "# | 3"
                  "# | true"
                  "# | true"
                  "# The optimal solution is [1,1,1,1], its value under c is 3, and the LP is feasible and bounded in direction of c.",
                  &find_transitive_lp_sol, "find_transitive_lp_sol(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
