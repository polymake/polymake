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

#ifndef POLYMAKE_POLYTOPE_MINKOWSKI_SUM_FUKUDA_H
#define POLYMAKE_POLYTOPE_MINKOWSKI_SUM_FUKUDA_H

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/polytope/to_interface.h"

namespace polymake { namespace polytope {

template <typename E>
struct choose_solver {
   typedef typename to_interface::solver<E> solver;
};

template <typename E>
Matrix<E> minkowski_sum_vertices_fukuda(const Array<perl::Object>& summands);

}  }

#endif
