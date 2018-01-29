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
#include "polymake/polytope/poly2lp.h"

namespace polymake { namespace polytope {

template<typename Scalar>
int poly2lp(perl::Object p, perl::Object lp, const bool maximize, const std::string& file)
{
   if (file.empty() || file=="-") {
      print_lp<Scalar>(p, lp, maximize, perl::cout);
   } else {
      std::ofstream os(file.c_str());
      print_lp<Scalar>(p, lp, maximize, os);
   }
   return 1;
}

UserFunctionTemplate4perl("# @category Optimization"
                          "# Convert a polymake description of a polyhedron to LP format (as used by CPLEX and"
                          "# other linear problem solvers) and write it to standard output or to a //file//."
                          "# If //LP// comes with an attachment 'INTEGER_VARIABLES' (of type Array<Bool>),"
                          "# the output will contain an additional section 'GENERAL',"
                          "# allowing for IP computations in CPLEX."
                          "# If the polytope is not FEASIBLE, the function will throw a runtime error."
                          "# @param Polytope P"
                          "# @param LinearProgram LP default value: //P//->LP"
                          "# @param Bool maximize produces a maximization problem; default value: 0 (minimize)"
                          "# @param String file default value: standard output",
                          "poly2lp<Scalar>(Polytope<Scalar>; LinearProgram=$_[0]->LP, $=0, $='')");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
