/* Copyright (c) 1997-2014
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
#include "polymake/Vector.h"

namespace polymake { namespace tropical {

template <typename Scalar>
Scalar norm(const Vector<Scalar>& vec)
{
   const Scalar min(accumulate(vec,operations::min()));
   const Scalar max(accumulate(vec,operations::max()));
   return max-min;
}
  
UserFunctionTemplate4perl("# @category Other"
                          "# Calculates the tropical norm of a vector in the tropical torus"
                          "# which is the difference between the maximal and minimal coordinate "
                          "# in any coordinate representation of the vector."
                          "# @param Vector<Scalar> vector"
                          "# @return Scalar" ,
                          "norm<Scalar>(Vector<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
