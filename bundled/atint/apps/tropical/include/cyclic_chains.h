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

	Contains functions to compute the lattice of chains of cyclic flats and the corresponding
   Moebius function.
	*/

#ifndef POLYMAKE_ATINT_CYCLIC_CHAINS_H
#define POLYMAKE_ATINT_CYCLIC_CHAINS_H

#include "polymake/client.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace tropical {

   using graph::Lattice;
   using graph::lattice::Sequential;
   using graph::lattice::BasicDecoration;

   /*
    * @brief Computes the lattice of all chains which include the bottom and top node.
    */
   Lattice<BasicDecoration> cyclic_chains(const Lattice<BasicDecoration, Sequential>& lattice);

    /*
    * @brief Takes a Hasse diagram and computes for each node n the value of the moebius function
    * mu(n,1), where 1 is the maximal element.
    * @return Vector<int> Each entry corresponds to the node of the same index.
    */
   Vector<int> top_moebius_function(const Lattice<BasicDecoration>& HD);

}}

#endif
