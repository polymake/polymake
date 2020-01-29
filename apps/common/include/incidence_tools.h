/* Copyright (c) 1997-2020
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

#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace common {

/// Collect the indexes of rows containing a 'true' element in any of the given columns
template <typename IMatrix, typename ISet>
Set<Int> incident_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& column_set)
{
   Set<Int> row_set;
   accumulate_in(entire(select(cols(IM), column_set)), operations::add(), row_set);
   return row_set;
}

/// Collect the indexes of rows containing 'false' elements in all given columns
template <typename IMatrix, typename ISet>
Set<Int> not_incident_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& column_set)
{
   Set<Int> row_set(sequence(0, IM.rows()));
   accumulate_in(entire(select(cols(IM), column_set)), operations::sub(), row_set);
   return row_set;
}

/// Collect the indexes of rows containing 'true' elements in all given columns
template <typename IMatrix, typename ISet>
Set<Int> common_rows(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& column_set)
{
   Set<Int> row_set;
   auto c = entire(column_set);
   if (!c.at_end()) {
      row_set = IM.col(*c);
      while (!(++c).at_end()) row_set *= IM.col(*c);
   }
   return row_set;
}

/// Return the index of the first row equal to the given column set, -1 if none found
template <typename IMatrix, typename ISet>
Int find_row(const GenericIncidenceMatrix<IMatrix>& IM, const ISet& column_set)
{
  auto c = entire(column_set);
  if (!c.at_end()) {
    for (const Int r : IM.col(*c)) {
      if (IM.row(r) == column_set) return r;
    }
  } else {
    for (auto r = entire<indexed>(rows(IM)); !r.at_end(); ++r) {
      if (r->empty()) return r->index();
    }
  }
  return -1;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
