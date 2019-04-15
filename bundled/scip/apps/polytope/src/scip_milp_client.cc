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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "scip/pub_misc.h"
#include "scip/scip.h"
#include "scip/scipdefplugins.h"
#include "scip/misc.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/generic_milp_client.h"

#include <vector>

namespace polymake { 
namespace polytope { 
namespace scip_interface {

bool check_solution(const Vector<Rational>& solution, const Matrix<Rational>& ineq, const Matrix<Rational>& eq){
   for(const auto& h:rows(ineq)){
      if(h*solution < 0){
         return false;
      }
   }
   for(const auto& e:rows(eq)){
      if(e*solution != 0){
         return false;
      }
   }
   return true;
}

SCIP_RETCODE insert_inequality(
      SCIP* scip,
      const Vector<Rational>& coeffs,
      SCIP_VAR** variables,
      std::vector<SCIP_CONS*>& constraints,
      bool isEquation) {
   SCIP_CONS* cons;
   SCIP_Real vals[coeffs.dim()];
   for(int i=0; i<coeffs.dim(); i++){
      vals[i] = (double)coeffs[i];
   }
   SCIP_CALL( SCIPcreateConsBasicLinear(scip, &cons, "is this important?", coeffs.dim(), variables, vals, 0, isEquation ? 0 : SCIPinfinity(scip)) );
   SCIP_CALL( SCIPaddCons(scip, cons) );
   constraints.push_back(cons);
   return SCIP_OKAY;
}


class InnerSolver {
   // All SCIP methods return a SCIP_RETCODE. Thus we do some wrapping so we
   // are able to return void or other things.
   private:
#if POLYMAKE_DEBUG
   const bool debug_print;
#endif
      const Set<int>& integerVariables;
      int dim;
      SCIP_VAR** variables;
      std::vector<SCIP_CONS*> constraints;
      Vector<Rational> solution;
      LP_status status;
      SCIP* scip;

      SCIP_RETCODE init(){
         SCIP_CALL( SCIPcreate(& scip) );

         // load default plugins linke separators, heuristics, etc.
         SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

         // disable scip output to stdout
         SCIPmessagehdlrSetQuiet(SCIPgetMessagehdlr(scip), TRUE);
         SCIP_CALL( SCIPcreateProbBasic(scip, "milp") );
         return SCIP_OKAY;
      }

      // Make SCIP variables, set them to integer or continuous.
      // The 0th variable is set to be 1.
      SCIP_RETCODE populate_variables() {
         variables = new SCIP_VAR*[dim];
         
         SCIP_VAR* x0;
         std::string varstring = "x0";
         // Arguments of SCIPcreateVarBasic are:
         // global_scip_object, SCIP_VAR*, variable_name, lower_bound, upper_bound, i_dont_know, SCIP_VARTYPE
         SCIP_CALL( SCIPcreateVarBasic(scip, &x0, varstring.c_str(), 1.0, 1.0, 0.0, SCIP_VARTYPE_CONTINUOUS) );
         SCIP_CALL( SCIPaddVar(scip, x0) );
         variables[0] = x0;
         
         for(int i=1; i<dim; i++){
            // For integerVariables put SCIP_VARTYPE_INTEGER
            // Otherwise SCIP_VARTYPE_CONTINUOUS
            // Currently we set everything to integer
            SCIP_VAR* x;
            varstring = "x" + std::to_string(i);
#if POLYMAKE_DEBUG
            if (debug_print) {
               cout << "Variable " << i << " set to:" << (integerVariables.contains(i) ? "INTEGER" : "CONTINUOUS") << endl;
            }
#endif
            auto type = integerVariables.contains(i) ? SCIP_VARTYPE_INTEGER : SCIP_VARTYPE_CONTINUOUS;
            SCIP_CALL( SCIPcreateVarBasic(scip, &x, varstring.c_str(), -SCIPinfinity(scip), SCIPinfinity(scip), 0.0, type) );
            SCIP_CALL( SCIPaddVar(scip, x) );
            variables[i] = x;
         }
         return SCIP_OKAY;
      }

      // Calls SCIPs solve method and retrieves result. Handles infeasibility
      // and unboundedness.
      SCIP_RETCODE solve_inner(){
         SCIP_CALL(SCIPsolve(scip));
         SCIP_SOL* sol = SCIPgetBestSol(scip);
#if POLYMAKE_DEBUG
         if (debug_print) {
            print_scip_solution_inner(sol);
         }
#endif
         if( SCIPgetStatus(scip) == SCIP_STATUS_INFEASIBLE ){
            status = LP_status::infeasible;
         } else {
            if( SCIPgetStatus(scip) == SCIP_STATUS_UNBOUNDED ){
               status = LP_status::unbounded;
            } else {
               status = LP_status::valid;
               solution = Vector<Rational>(dim);
               for(int i=0; i<dim; i++){
                  if(integerVariables.contains(i)){
                     // Here we call SCIPround, eliminate when SCIP works with GMP rationals.
                     solution[i] = convert_to<Integer>(SCIPround(scip, SCIPgetSolVal(scip, sol, variables[i])));
                  } else {
                     solution[i] = convert_to<Rational>(SCIPgetSolVal(scip, sol, variables[i]));
                  }
               }
            }
         }
         return SCIP_OKAY;
      }

      // Hand objective to SCIP
      SCIP_RETCODE read_objective_inner(const Vector<Rational>& objective, bool maximize){
         // Objective
         for(int i=0; i<objective.dim(); i++){
            SCIP_CALL( SCIPchgVarObj(scip, variables[i], (double)objective[i]) );
         }
         if(maximize) SCIP_CALL( SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE) );
         return SCIP_OKAY;
      }

      // Destructor for Solver
      SCIP_RETCODE destroy_scip(){
         for(int i=0; i<dim; i++){
            SCIP_CALL( SCIPreleaseVar(scip, &variables[i]) );
         }
         delete[] variables;
         for(auto c: constraints){
            SCIP_CALL( SCIPreleaseCons(scip, &c) );
         }
         constraints.clear();
         SCIP_CALL( SCIPfree(&scip) );
         return SCIP_OKAY;
      }
      
      SCIP_RETCODE print_scip_solution_inner(SCIP_SOL* sol){
         cout << "SCIP solution: ";
         for(int i=0; i<dim; i++){
            cout << SCIPgetSolVal(scip, sol, variables[i]) << " ";
         }
         cout << endl;
         return SCIP_OKAY;
      }

   public:
      InnerSolver(int d, const Set<int>& iv):
#if POLYMAKE_DEBUG
         debug_print(perl::get_debug_level() > 1),
#endif
         integerVariables(iv), dim(d), constraints(0) {
         if( init() != SCIP_OKAY ){
            throw std::runtime_error("Error when initializing SCIP object.");
         }
         populate_variables();
      }
      
      SCIP_RETCODE print_scip_solution(){
         SCIP_SOL* sol = SCIPgetBestSol(scip);
         print_scip_solution_inner(sol);
         return SCIP_OKAY;
      }
      
      void read_inequalities(const Matrix<Rational>& ineq){
         for(const auto& h:rows(ineq)){
            if( insert_inequality(scip, h, variables, constraints, false) != SCIP_OKAY ){
               throw std::runtime_error("Error when inserting inequality.");
            }
         }
      }

      void read_equations(const Matrix<Rational>& eq){
         for(const auto& h:rows(eq)){
            if( insert_inequality(scip, h, variables, constraints, true) != SCIP_OKAY ){
               throw std::runtime_error("Error when inserting equation.");
            }
         }
      }

      void read_objective(const Vector<Rational>& objective, bool maximize){
         if( read_objective_inner(objective, maximize) != SCIP_OKAY ){
            throw std::runtime_error("Error when setting objective.");
         }
      }

      LP_status solve(){
         if( solve_inner() != SCIP_OKAY ){
            throw std::runtime_error("Error when solving MILP");
         }
         return status;
      }

      const Vector<Rational>& get_solution(){
         return solution;
      }

      ~ InnerSolver(){
         if(destroy_scip() != SCIP_OKAY){
            cerr << "Could not destroy SCIP object" << endl;
         }
      }
      

};

class Solver : public MILP_Solver<Rational> {
   public:
      Solver() {}

      MILP_Solution<Rational> solve(const Matrix<Rational>& H,
         const Matrix<Rational>& E,
         const Vector<Rational>& Obj,
         const Set<int>& integerVariables,
         bool maximize) const {
         scip_interface::InnerSolver S(Obj.dim(), integerVariables);
         S.read_inequalities(H);
         S.read_equations(E);
         S.read_objective(Obj, maximize);
         MILP_Solution<Rational> result;
         result.status = S.solve();
         if(result.status == LP_status::valid){
            result.solution = S.get_solution();
            if(!check_solution(result.solution, H, E)){
               S.print_scip_solution();
               throw std::runtime_error("Solution is not inside polytope.");
            }
            // There should be another way to get this value directly from SCIP.
            result.objective_value = Obj * result.solution;
         }
         return result;
      }
};

template <typename Scalar=Rational>
auto create_MILP_solver()
{
   return cached_MILP_solver<Rational>(new Solver(), true);
}


} // end namespace scip_interface


void scip_milp_client(perl::Object p, perl::Object milp, bool maximize, perl::OptionSet options)
{

   scip_interface::Solver S;
   generic_milp_client<Rational, scip_interface::Solver>(p, milp, maximize, S);
}

Function4perl(&scip_milp_client, "scip_milp_client(Polytope<Rational>, MixedIntegerLinearProgram<Rational>, $; {initial_basis => undef})");

InsertEmbeddedRule("function scip.milp: create_MILP_solver<Scalar> [Scalar==Rational] () : c++ (name => 'scip_interface::create_MILP_solver') : returns(cached);\n");

} // end namespace polytope
} // end namespace polymake

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
