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
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

perl::ListReturn triang_boundary(const Array< Set<int> >& triang, const IncidenceMatrix<>& vif)
{
  IncidenceMatrix<> VIF=vif;
  const int dim=triang[0].size()-1;             // #vertices(simplex) == dim+1
  std::vector<Set<int> > triang_boundary;
  Array<Set<int> > facet_triag(VIF.rows());
  // simplicial facets need not be triangulated
  int n_simplices=0;
  for (Entire< Rows< IncidenceMatrix<> > >::iterator f=entire(rows(VIF)); !f.at_end(); ++f)
    if (f->size() == dim) {
      triang_boundary.push_back(*f);
      facet_triag[f->index()].push_back(n_simplices++);
      f->clear();
    }
     
  for (Entire< Array< Set<int> > >::const_iterator t=entire(triang); !t.at_end(); ++t) {
    typedef Subsets_less_1<const Set<int>&> faces;
    for (Entire<faces>::const_iterator face=entire(all_subsets_less_1(*t)); !face.at_end(); ++face) {
      faces::value_type::const_iterator v=entire(*face);
      Set<int> common_facets=VIF.col(*v);
      for (++v; !v.at_end() && !common_facets.empty(); ++v)
        common_facets *= VIF.col(*v);
      if (!common_facets.empty()) {
        triang_boundary.push_back(*face);
        facet_triag[common_facets.front()].push_back(n_simplices++);
      }
    }
  }

  perl::ListReturn result;
  result << Array< Set<int> >(n_simplices,entire(triang_boundary))
         << facet_triag;
  return result;
}

Function4perl(&triang_boundary,"triang_boundary");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
