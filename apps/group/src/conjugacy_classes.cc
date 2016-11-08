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

Array<Array<Array<int>>> 
conjugacy_classes(const Array<Array<int>>& generators, 
                  const Array<Array<int>>& conjugacy_classes_representatives)
{
   const int degree = generators[0].size();
   const auto entire_group = orbit<on_container>(generators, Array<int>(degree, entire(sequence(0, degree))));
   
   Map<Array<int>, Array<int>> inverse_of;
   for (const auto& g : entire_group) {
      Array<int> inverse;
      inverse_permutation(g, inverse);
      inverse_of[g] = inverse;
   }

   Array<Array<Array<int>>> conjugacy_classes(conjugacy_classes_representatives.size());
   for (int i=0; i<conjugacy_classes_representatives.size(); ++i) {
      hash_set<Array<int>> conjugacy_class;
      for (const auto& g : entire_group) {
         conjugacy_class += action<on_container>(inverse_of[g], action<on_container>(conjugacy_classes_representatives[i], g));
      }
      conjugacy_classes[i] = Array<Array<int>>(conjugacy_class.size(), entire(conjugacy_class));
   }
   return conjugacy_classes;
}


UserFunction4perl("# @category Other"
		  "# Calculate the conjugacy classes of a group"
		  "# @param Array<Array<Int>> the generators of the group"
                  "# @param Array<Array<Int>> the representatives of the conjugacy classes"
                  "# @return Array<Array<Array<Int>>>",
                  &conjugacy_classes, "conjugacy_classes(Array<Array<Int>> Array<Array<Int>>)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

