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
#include "polymake/tropical/canonicalize.h"

namespace polymake { namespace tropical {


FunctionTemplate4perl("canonicalize_to_leading_zero(Vector&)");
FunctionTemplate4perl("canonicalize_to_leading_zero(Matrix&)");

FunctionTemplate4perl("canonicalize_scalar_to_leading_zero(Vector&)");
FunctionTemplate4perl("canonicalize_scalar_to_leading_zero(Matrix&)");

FunctionTemplate4perl("canonicalize_to_leading_zero_and_check_columns(Matrix&)");

FunctionTemplate4perl("canonicalize_vertices_to_leading_zero(Matrix&)");
FunctionTemplate4perl("canonicalize_vertex_to_leading_zero(Vector&)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
