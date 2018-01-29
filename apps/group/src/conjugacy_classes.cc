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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/group/orbit.h"
#include "polymake/group/group_tools.h"
#include "polymake/permutations.h"
#include "polymake/linalg.h"

namespace polymake { namespace group {

template<typename Element>
auto
all_group_elements_impl(const Array<Element>& generators)
{
   return orbit_impl<LeftAction<Element>, Element, Element>(generators, identity(degree(generators[0]), Element()));
}

template<typename Scalar>
auto
all_group_elements(perl::Object MatrixAction)
{
   const Array<Matrix<Scalar>> generators = MatrixAction.give("GENERATORS");
   return Set<Matrix<Scalar>>(entire(all_group_elements_impl(generators)));
}

template<typename Element>
auto conjugacy_classes(const Array<Element>& generators, 
                       const Array<Element>& conjugacy_classes_representatives)
{
   Array<Set<Element>> conjugacy_classes(conjugacy_classes_representatives.size());
   for (int i=0; i<conjugacy_classes_representatives.size(); ++i)
      conjugacy_classes[i] = Set<Element>(entire(orbit_impl<ConjugationAction<Element>, Element, Element>(generators, conjugacy_classes_representatives[i])));

   return conjugacy_classes;
}

template<typename Element>
auto conjugacy_class(perl::Object A, const Element& element)
{
   const Array<Element> generators = A.give("GENERATORS");
   return Set<Element>(entire(orbit_impl<ConjugationAction<Element>, Element, Element>(generators, element)));
}

template<typename Scalar>
auto conjugacy_class(perl::Object A, const Matrix<Scalar>& element)
{
   const Array<Matrix<Scalar>> generators = A.give("GENERATORS");
   return Set<Matrix<Scalar>>(entire(orbit_impl<ConjugationAction<Matrix<Scalar>>, Matrix<Scalar>, Matrix<Scalar>>(generators, element)));
}
      
      
template<typename Element>
auto conjugacy_classes_and_reps(const Array<Element>& generators)
{
   Set<Element> sorted_group(entire(all_group_elements_impl(generators)));
   
   std::vector<Set<Element>> classes;
   std::vector<Element> reps;
   while (sorted_group.size()) {
      const Set<Element> new_class(entire(orbit_impl<ConjugationAction<Element>, Element, Element>(generators, sorted_group.front())));
      classes.push_back(new_class);
      reps.push_back(classes.back().front());
      sorted_group -= new_class;
   }
   return std::make_pair(Array<Set<Element>>(classes.size(), entire(classes)),
                         Array<Element>(reps.size(), entire(reps)));
}
      

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Compute all elements of a given group, expressed as a matrix group action."
                          "# @param MatrixActionOnVectors<Scalar> action the action of a permutation group"
                          "# @tparam Scalar S the underlying number type"
                          "# @return Set<Matrix<Scalar>> G a set containing all group elements"
                          "# @example To generate all elements of the regular representation of S_3, type"
                          "# > print all_group_elements(symmetric_group(3)->REGULAR_REPRESENTATION);"
                          "# | <0 0 1"
                          "# | 0 1 0"
                          "# | 1 0 0"
                          "# | >"
                          "# | <0 0 1"
                          "# | 1 0 0"
                          "# | 0 1 0"
                          "# | >"
                          "# | <0 1 0"
                          "# | 0 0 1"
                          "# | 1 0 0"
                          "# | >"
                          "# | <0 1 0"
                          "# | 1 0 0"
                          "# | 0 0 1"
                          "# | >"
                          "# | <1 0 0"
                          "# | 0 0 1"
                          "# | 0 1 0"
                          "# | >"
                          "# | <1 0 0"
                          "# | 0 1 0"
                          "# | 0 0 1"
                          "# | >",
                          "all_group_elements<Scalar>(MatrixActionOnVectors<Scalar>)");

FunctionTemplate4perl("conjugacy_classes<Element>(Array<Element> Array<Element>)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Compute the conjugacy class of a group element under a given action"
                          "# @param Action action the action of the group"
                          "# @param Element e the element to be acted upon"
                          "# @tparam Element E the underlying element type"
                          "# @return Set<Element> S a set containing the conjugacy class of the element",
                          "conjugacy_class<Element>(Action Element)");

UserFunctionTemplate4perl("# @category Symmetry"
                          "# Compute the conjugacy class of a group element under a given action"
                          "# @param MatrixActionOnVectors<Scalar> action the action of the group"
                          "# @param Matrix<Scalar> e the element to be acted upon"
                          "# @tparam Scalar E the underlying number type"
                          "# @return Set<Matrix<Element>> S a set containing the conjugacy class of the element",
                          "conjugacy_class<Scalar>(MatrixActionOnVectors<Scalar> Matrix<Scalar>)");
      
FunctionTemplate4perl("conjugacy_classes_and_reps<Element>(Array<Element>)");

}
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

