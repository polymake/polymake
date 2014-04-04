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
#include "polymake/Array.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace polytope {

template <typename SetType>
SetType lex_min_representative(perl::Object G, const SetType& S)
{
   const group::PermlibGroup group = group::group_from_perlgroup(G);
   const SetType R = group.lex_min_representative(S);
   return R;
}

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Computes the lexicographically smallest representative of a given set with respect to a group"
                          "# @param Group G a symmetry group"
                          "# @param Set S a set" 
                          "# @return Set the lex-min representative of S",
                          "lex_min_representative(group::Group Set)");


  } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
