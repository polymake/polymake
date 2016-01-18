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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace matroid {

   /*
    * @brief This computes the coloops of the deletion of a matroid 
    * @param Array<Set<int> > bases The list of bases
    * @param Set<int> deleted_set The elements that are deleted
    * @return Set<int> The coloops of the deletion.
    */
   Set<int> find_coloops_of_deletion(const Array<Set<int> >&bases, const Set<int> deleted_set) {
      //Find all bases that have minimal intersection with deleted_set (these give the bases of the deletion)
      Set<int> minimal_basis_indices;
      int minimal_intersection_size = deleted_set.size();
      for(int i =0; i < bases.size(); i++) {
         int bsize = (bases[i]*deleted_set).size();
         if(bsize == minimal_intersection_size) minimal_basis_indices += i;
         if(bsize < minimal_intersection_size) {
            minimal_basis_indices = scalar2set(i);
            minimal_intersection_size = bsize;
         }
      }
      //Take the intersection and remove the deleted set 
      return accumulate( select(bases, minimal_basis_indices), operations::mul()); 
   }

   /*
    * @brief Constructs the unique maximal presentation of a transversal matroid
    * @param int n The size of the ground set (0,..,n-1)
    * @param Array<Set<int> > bases The bases of the matroid 
    * @param Array<Set<int> > transversal_presentation One possible transversal presentation
    * @param Set<int> A set of indices in transversal_presentation of size rank(matroid), which corresponds
    * to one maximal transversal. The maximal presentation will be constructed only from these sets 
    * @return IncidenceMatrix<> The maximal presentation (rows = sets, columns = ground set)
    */
   IncidenceMatrix<> maximal_transversal_presentation(int n,
                                                      const Array<Set<int> > &bases, 
                                                      const Array<Set<int> > &transversal_presentation,
                                                      const Set<int> &one_matching) {
      IncidenceMatrix<> result(one_matching.size(), n);
      int i = 0;
      for(Entire<Set<int> >::const_iterator om = entire(one_matching); !om.at_end(); om++,i++) {
         result.row(i) = transversal_presentation[*om] + 
            find_coloops_of_deletion(bases, transversal_presentation[*om] );

      }

      return result;
   }

   Function4perl(&maximal_transversal_presentation, "maximal_transversal_presentation($,Array<Set<Int> >, Array<Set<Int> >, Set<Int>)");

}}
