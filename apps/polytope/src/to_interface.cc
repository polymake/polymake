/* Copyright (c) 1997-2015
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

#include "polymake/polytope/to_interface_impl.h"

#include <polymake/Rational.h>
#include <polymake/QuadraticExtension.h>
#include <polymake/PuiseuxFraction.h>

namespace polymake { namespace polytope { namespace to_interface {

template class solver<Rational>;
template class solver<double>;
template class solver<QuadraticExtension<Rational> >;
template class solver< PuiseuxFraction<Min,Rational,Rational> >;
template class solver< PuiseuxFraction<Max,Rational,Rational> >;
template class solver< PuiseuxFraction<Min,Rational,int> >;
template class solver< PuiseuxFraction<Max,Rational,int> >;
template class solver< PuiseuxFraction<Min,Rational,Integer> >;
template class solver< PuiseuxFraction<Max,Rational,Integer> >;
template class solver< PuiseuxFraction<Min,PuiseuxFraction<Min,Rational,Rational>,Rational> >;
template class solver< PuiseuxFraction<Min,PuiseuxFraction<Max,Rational,Rational>,Rational> >;
template class solver< PuiseuxFraction<Max,PuiseuxFraction<Min,Rational,Rational>,Rational> >;
template class solver< PuiseuxFraction<Max,PuiseuxFraction<Max,Rational,Rational>,Rational> >;


} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
