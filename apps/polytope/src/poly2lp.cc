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
#include "polymake/polytope/poly2lp.h"

namespace polymake { namespace polytope {

template<typename Scalar>
Int poly2lp(BigObject p, BigObject lp, const bool maximize, const std::string& file)
{
   if(!(lp.isa("LinearProgram") || lp.isa("MixedIntegerLinearProgram"))){
      throw std::runtime_error("Second argument must be a (MixedInteger)LinearProgram");
   }

   bool is_lp = lp.isa("LinearProgram");
   
   if (file.empty() || file=="-") {
      if(is_lp){
         print_lp<Scalar, true>(p, lp, maximize, perl::cout);
      } else {
         print_lp<Scalar, false>(p, lp, maximize, perl::cout);
      }
      return 1;
   } else {
      std::ofstream os(file.c_str());
      os.exceptions(std::ofstream::failbit | std::ofstream::badbit);
      if(is_lp){
         print_lp<Scalar, true>(p, lp, maximize, os);
      } else {
         print_lp<Scalar, false>(p, lp, maximize, os);
      }
      return 1;
   }
}

UserFunctionTemplate4perl("# @category Optimization"
                          "# Convert a polymake description of a polyhedron to LP format (as used by CPLEX and"
                          "# other linear problem solvers) and write it to standard output or to a //file//."
                          "# If //LP// comes with an attachment 'INTEGER_VARIABLES' (of type Array<Bool>),"
                          "# the output will contain an additional section 'GENERAL',"
                          "# allowing for IP computations in CPLEX."
                          "# If the polytope is not FEASIBLE, the function will throw a runtime error."
                          "# Alternatively one can also provide a //MILP//, instead of a //LP// with 'INTEGER_VARIABLES' attachment."
                          "# @param Polytope P"
                          "# @param LinearProgram LP default value: //P//->LP"
                          "# @param Bool maximize produces a maximization problem; default value: 0 (minimize)"
                          "# @param String file default value: standard output",
                          "poly2lp<Scalar>(Polytope<Scalar>; $=$_[0]->LP, $=0, $='')");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
