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
#include "polymake/Integer.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"

namespace polymake { namespace common {

template <typename Vector> inline
bool check_int_limit(const GenericVector<Vector,Integer>& V)
{
   for (auto x=entire(V.top()); !x.at_end(); ++x)
      if ( *x > std::numeric_limits<int>::max() || *x < std::numeric_limits<int>::min() ) 
         return false;
   return true;
}

template <typename Matrix> inline
bool check_int_limit(const GenericMatrix<Matrix,Integer>& M)
{
   return check_int_limit(concat_rows(M));
}

FunctionTemplate4perl("check_int_limit(Vector<Integer>)");
FunctionTemplate4perl("check_int_limit(Matrix<Integer>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
