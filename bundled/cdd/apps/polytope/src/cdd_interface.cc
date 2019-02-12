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

#define USE_DD_
#include "polymake/polytope/cdd_interface_impl.h"

namespace polymake { namespace polytope { namespace cdd_interface {

#ifdef POLYMAKE_CDD_STANDALONE_GLOBAL_INIT

namespace {

void global_construct_standalone()
{
   dd_set_global_constants();
}

void global_destroy_standalone()
{
   dd_free_global_constants();
}

}

void (* const CddInstance::Initializer::global_construct)() = &global_construct_standalone;
void (* const CddInstance::Initializer::global_destroy)() = &global_destroy_standalone;

#endif

CddInstance::Initializer::Initializer()
{
   global_construct();
}

CddInstance::Initializer::~Initializer()
{
   global_destroy();
}

template <>
Rational cdd_lp_sol<Rational>::optimal_value() const
{
   return Rational(std::move(ptr->optvalue));
}

template class cdd_matrix<Rational>;
template class cdd_polyhedron<Rational>;
template class ConvexHullSolver<Rational>;
template class LP_Solver<Rational>;

dd_ErrorType err;

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
