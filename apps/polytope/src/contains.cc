/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   But WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/
#include "polymake/client.h"
#include "polymake/polytope/contains.h"

namespace polymake { namespace polytope {
template <typename Scalar>
bool cone_contains(BigObject p_in, BigObject p_out){
   return contains<Scalar>(p_in,p_out);
}
FunctionTemplate4perl("cone_contains<Scalar> (Cone<Scalar>, Cone<Scalar>)");

}}
