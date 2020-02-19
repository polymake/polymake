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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace matroid {

/*
 * @brief This computes the coloops of the deletion of a matroid 
 * @param Array<Set<Int>> bases The list of bases
 * @param Set<Int> deleted_set The elements that are deleted
 * @return Set<Int> The coloops of the deletion.
    */
Set<Int> find_coloops_of_deletion(const Array<Set<Int>>&bases, const Set<Int>& deleted_set)
{
  // Find all bases that have minimal intersection with deleted_set (these give the bases of the deletion)
  Set<Int> minimal_basis_indices;
  Int minimal_intersection_size = deleted_set.size();
  for (Int i = 0; i < bases.size(); ++i) {
    Int bsize = (bases[i]*deleted_set).size();
    if (bsize == minimal_intersection_size)
      minimal_basis_indices += i;
    if (bsize < minimal_intersection_size) {
      minimal_basis_indices = scalar2set(i);
      minimal_intersection_size = bsize;
    }
  }
  // Take the intersection and remove the deleted set 
  return accumulate( select(bases, minimal_basis_indices), operations::mul()); 
}

/*
 * @brief Constructs the unique maximal presentation of a transversal matroid
 * @param Int n The size of the ground set (0,..,n-1)
 * @param Array<Set<Int>> bases The bases of the matroid 
 * @param Array<Set<Int>> transversal_presentation One possible transversal presentation
 * @param Set<Int> A set of indices in transversal_presentation of size rank(matroid), which corresponds
 * to one maximal transversal. The maximal presentation will be constructed only from these sets 
 * @return IncidenceMatrix<> The maximal presentation (rows = sets, columns = ground set)
 */
IncidenceMatrix<> maximal_transversal_presentation(Int n,
                                                   const Array<Set<Int>>& bases, 
                                                   const Array<Set<Int>>& transversal_presentation,
                                                   const Set<Int> &one_matching)
{
  IncidenceMatrix<> result(one_matching.size(), n);
  Int i = 0;
  for (auto om = entire(one_matching); !om.at_end(); om++,i++) {
    result.row(i) = transversal_presentation[*om] + 
      find_coloops_of_deletion(bases, transversal_presentation[*om] );
  }

  return result;
}

Function4perl(&maximal_transversal_presentation, "maximal_transversal_presentation($,Array<Set<Int> >, Array<Set<Int> >, Set<Int>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
