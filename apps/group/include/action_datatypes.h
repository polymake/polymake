/* Copyright (c) 1997-2016
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

#ifndef __GROUP_ACTION_DATATYPES_H
#define __GROUP_ACTION_DATATYPES_H

#include "polymake/hash_map"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/group/orbit.h"

namespace polymake { namespace group {

typedef Array<Array<int> > ConjugacyClass;
typedef Array<ConjugacyClass> ConjugacyClasses;
typedef Array<Array<int> > ConjugacyClassReps;

template<typename SparseSet>
using SparseSimplexVector = hash_map<SparseSet, Rational>;

template<typename SparseSet>
using SparseIsotypicBasis = Array<SparseSimplexVector<SparseSet>>;

template<typename SetType>
using ActionType = pm::operations::group::action<SetType&, group::on_container, Array<int>>;


} }

#endif // __GROUP_REPRESENTATIONS_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


