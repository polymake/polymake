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
#include "polymake/Map.h"
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/group/orbit.h"
#include "polymake/permutations.h"

namespace polymake { namespace group {

Array<Set<Array<int> > > conjugacy_classes(const Array<Array<int> >& generators, 
                                           const Array<Array<int> >& conjugacy_classes_representatives)
{
   const int degree = generators[0].size();
   const Set<Array<int> > entire_group = orbit<on_container>(generators, Array<int>(degree, entire(sequence(0, degree))));
   
   Map<Array<int>, Array<int> > inverse_of;
   for (Entire<Set<Array<int> > >::const_iterator git = entire(entire_group); !git.at_end(); ++git) {
      Array<int> inverse;
      inverse_permutation(*git, inverse);
      inverse_of[*git] = inverse;
   }

   Array<Set<Array<int> > > conjugacy_classes(conjugacy_classes_representatives.size());
   for (int i=0; i<conjugacy_classes_representatives.size(); ++i) {
      Set<Array<int> > conjugacy_class;
      for (Entire<Set<Array<int> > >::const_iterator git = entire(entire_group); !git.at_end(); ++git) {
         conjugacy_class += action<on_container>(inverse_of[*git], action<on_container>(conjugacy_classes_representatives[i], *git));
      }
      conjugacy_classes[i] = conjugacy_class;
   }
   return conjugacy_classes;
}


UserFunction4perl("# @category Other"
		  "# Calculate the conjugacy classes of a group"
		  "# @param Array<Array<Int>> the generators of the group"
                  "# @param Array<Array<Int>> the representatives of the conjugacy classes"
                  "# @return Array<Set<Array<Int>>>",
                  &conjugacy_classes, "conjugacy_classes(Array<Array<Int>> Array<Array<Int>>)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

