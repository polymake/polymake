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
#include "polymake/Set.h"

namespace polymake { namespace matroid {

void loops_coloops(perl::Object m)
{
  const int n=m.give("N_ELEMENTS");
  Array< Set<int> > bases=m.give("BASES");

  Set<int> loops=sequence(0,n)-accumulate(bases,operations::add());
  Set<int> coloops=accumulate(bases,operations::mul());

  m.take("LOOPS") << loops;
  m.take("COLOOPS") << coloops;
}

Function4perl(&loops_coloops,"loops_coloops(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
