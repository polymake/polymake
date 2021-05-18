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

#pragma once

#include "polymake/Vector.h"
#include "polymake/Integer.h"

namespace polymake { namespace polytope {

Vector<Integer> h_from_f_vec(const Vector<Integer>& f, bool simplicial);
Vector<Integer> f_from_h_vec(const Vector<Integer>& h, const bool simplicial);
Vector<Integer> h_from_g_vec(const Vector<Integer>& g, const Int d);
Vector<Integer> g_from_h_vec(const Vector<Integer>& h);
Vector<Int> binomial_representation(Integer l, Int i);
Integer pseudopower(Integer l, Int i);
bool m_sequence(Vector<Integer> h);


} } // namespaces


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
