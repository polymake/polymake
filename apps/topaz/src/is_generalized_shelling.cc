/* Copyright (c) 1997-2019
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

#include <vector>
#include <list>

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"

/*
Chari, Manoj K.
On discrete Morse functions and combinatorial decompositions.
Formal power series and algebraic combinatorics (Vienna, 1997).
Discrete Math. 217 (2000), no. 1-3, 101-113.
*/

namespace polymake { namespace topaz {

bool is_generalized_shelling(const Array< Set<int> >& FaceList, perl::OptionSet options)
{
  const bool verbose=options["verbose"];
 
  std::vector<int> h; // h-vector
  bool success(true); // let's be optimistic

  for (auto i=entire(FaceList); !i.at_end(); ++i) {
    const Set<int>& thisFace(*i);
    const int thisSize(thisFace.size());
    const int thisDim(thisSize-1);
	 
    // extend the h-vector if necessary
    const int h_size=h.size();
    if (h_size != thisSize) {
      h.resize(thisSize);
      for (int k=h_size; k<thisSize; ++k) h[k]=0;
    }

    // collect maximal intersection with previous facets
    PowerSet<int> maxIntersections;
    for (auto j=entire(FaceList); j!=i; ++j)
      maxIntersections.insertMax(thisFace*(*j));
    
    // examine the intersections
    bool allIntersectionsAreRidges(true);
    Set<int> missingVertices;
    for (auto s=entire(maxIntersections);  allIntersectionsAreRidges && !s.at_end();  ++s) {
      allIntersectionsAreRidges = (s->size() == thisDim);
      missingVertices += thisFace - (*s);
    }

    if (allIntersectionsAreRidges) { // good cases
      const int missing_size=missingVertices.size();
      ++h[missing_size];
      if (verbose) {
	if (missing_size==thisSize)
	  cout << "Critical " << thisDim << "-face " << thisFace << "\n";
	else
	  cout << thisFace << " [" << missingVertices << "]\n";
      }
    } else { // bad cases
      success=false;
      if (verbose) {
	cout << "Bad face " << thisFace << " with intersections " << maxIntersections << "\n";
      }
    }
  }
  
  if (verbose) {
    cout << "h = " << h << endl;
  }
  
  return success;
}


UserFunction4perl("# @category Other\n"
                  "# Check if a given sequence of faces of a simplicial complex is a generalized shelling.\n"
                  "# @param Array<Set> FaceList"
                  "# @option Bool verbose"
                  "# @return Bool",
                  &is_generalized_shelling, "is_generalized_shelling(Array<Set> ; { verbose=>0 })");
} }
