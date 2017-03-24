/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>
	*/


#ifndef POLYMAKE_ATINT_SEPARATED_DATA_H
#define POLYMAKE_ATINT_SEPARATED_DATA_H

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

/**
   @brief Check whether a given cone set is compatible with a given set of local restrictions
   @param cone A set of (ray) indices
   @param local_restriction A list of sets of ray indices
   @return true, if and only if cone contains one of the sets of local_restriction 
*/
template <typename TSet, typename TMatrix>
bool is_coneset_compatible(const GenericSet<TSet, int>& cone, const GenericIncidenceMatrix<TMatrix>& local_restriction)
{
  for (auto r=entire(rows(local_restriction)); !r.at_end(); ++r) {
     if (incl(*r, cone)<=0) return true;
  }
  return false;
}

} }

#endif
