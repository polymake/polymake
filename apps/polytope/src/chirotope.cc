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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/PowerSet.h"
#include "polymake/GenericMatrix.h"

namespace polymake { 
namespace polytope {

template<typename TMatrix>
std::string chirotope(const GenericMatrix<TMatrix>& V)
{
   const Int r = rank(V);
   if (r != V.cols())
      throw std::runtime_error("chirotope: Input matrix must have full column rank");
   const Int n = V.rows();
   std::ostringstream os;
   os << n << "," << r << ":\n";
   for (const auto set : all_subsets_of_k(sequence(0,n), r)){
      const auto d = sign(det(V.minor(set, All)));
      if(d == 1) os << '+';
      else if (d == 0) os << '0';
      else os << '-';
   }
   os << "\n";
   return os.str();
}

UserFunctionTemplate4perl(
   "# @category Triangulations, subdivisions and volume"
   "# Compute the chirotope of a point configuration given as the rows of a matrix."
   "# @param Matrix M The rows are the points"
   "# @return String",
   "chirotope(Matrix)");
      
} // namespace polytope 
} // namespace polymake

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
