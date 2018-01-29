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

#ifndef POLYMAKE_TOPAZ_CONNECTED_SUM_H
#define POLYMAKE_TOPAZ_CONNECTED_SUM_H

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/hash_map"
#include <string>

namespace polymake { namespace topaz {

typedef hash_map<int,int> map;

/// Computes the connected sum of two complexes and adjusts the labels.
/// The client writes the new lables into L1.
/// If the labels are empty, the client just ignores them.
/// The automorphism P determines to which of the vertices of f1 the
/// vertices of f2 get glued.
/// If the map is empty, the trivial map is used.
template <typename Complex_1, typename Complex_2>
std::list< Set<int> > connected_sum(const Complex_1& C1,
                                    const Complex_2& C2,
                                    const int f1, const int f2,
                                    Array<std::string>& L1,
                                    const Array<std::string>& L2,
                                    map& P);
   
/// Computes the connected sum of two complexes,
/// using the first facet of each complex and the
/// trivial map.
template <typename Complex_1, typename Complex_2>
std::list< Set<int> > connected_sum(const Complex_1& C1,
                                    const Complex_2& C2)
{
   map P;
   Array<std::string> L;
      
   return connected_sum(C1,C2,0,0,L,L,P);
}

} }

#include "polymake/topaz/connected_sum.tcc"

#endif // POLYMAKE_TOPAZ_CONNECTED_SUM_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
