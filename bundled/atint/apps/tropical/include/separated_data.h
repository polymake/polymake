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
	  @param Set<int> cone A set of (ray) indices
	  @param IncidenceMatrix<> local_restriction A list of sets of ray indices
	  @return true, if and only if cone contains one of the sets of local_restriction 
	  */
	bool is_coneset_compatible(const Set<int> &cone, const IncidenceMatrix<> &local_restriction);

}}

#endif
