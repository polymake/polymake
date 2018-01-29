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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/client.h"

namespace polymake { namespace common {

template <typename IMatrix, typename ISet>
Set<int> incident_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& S)
{
   Set<int> result;
   accumulate_in(entire(select(cols(IM),S)),operations::add(),result);
   return result;
}

template <typename IMatrix, typename ISet>
Set<int> not_incident_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& S)
{
   Set<int> result(sequence(0,IM.rows()));
   accumulate_in(entire(select(cols(IM),S)),operations::sub(),result);
   return result;
}

template <typename IMatrix, typename ISet>
Set<int> common_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& S)
{
   Set<int> result;
   typename Entire<ISet>::const_iterator s=entire(S);
   if (!s.at_end()) {
      result=IM.col(*s);
      while (!(++s).at_end()) result *= IM.col(*s);
   }
   return result;
}

FunctionTemplate4perl("incident_rows(IncidenceMatrix, *)");
FunctionTemplate4perl("not_incident_rows(IncidenceMatrix, *)");
FunctionTemplate4perl("common_rows(IncidenceMatrix, *)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
