/* Copyright (c) 1997-2022
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

#include "polymake/group/group_tools.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace group {

ConjugacyClassReps<Array<Int>> dn_reps(Int n2);

Matrix<CharacterNumberType> dn_character_table(Int n2);

ConjugacyClassReps<Array<Int>> sn_reps(Int n);

Matrix<CharacterNumberType> sn_character_table(Int n);

Array<Array<Int>> symmetric_group_gens(Int n);

BigObject symmetric_group(Int n);

BigObject alternating_group(Int n);

BigObject cyclic_group(Int n);

BigObject dihedral_group(Int n2);

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
