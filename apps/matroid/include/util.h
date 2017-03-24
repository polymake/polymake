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

#ifndef __POLYMAKE_MATROID_UTIL
#define __POLYMAKE_MATROID_UTIL

namespace polymake { namespace matroid {

namespace operations {

using namespace polymake::operations;

template <typename SetRef>
struct contains {
   typename pm::deref<SetRef>::type::element_type elem;
public:
   typedef typename pm::deref<SetRef>::type set_type;
   typedef set_type argument_type;
   typedef bool result_type;

   contains(typename set_type::element_type e) : elem(e) {}

   result_type operator() (typename pm::function_argument<SetRef>::const_type set) const
   {
      return set.contains(elem);
   }
};

template <typename Scalar>
struct dropshift {
   typedef typename pm::deref<Scalar>::type scalar_type;
   scalar_type elem;
public:
   typedef scalar_type argument_type;
   typedef scalar_type result_type;

   dropshift(scalar_type e) : elem(e) {}

   result_type operator() (argument_type value) const
   {
      return value < elem ? value : value-1;
   }
};

}

template <typename Container>
typename std::enable_if<pm::isomorphic_to_container_of<Container, Set<int>>::value,
                        pm::SelectedSubset<const Container&, operations::contains<Set<int>>> >::type
select_k(const Container& sets, const int k)
{
   return pm::SelectedSubset<const Container&, operations::contains<Set<int>>>(sets,operations::contains<Set<int>>(k));
}

template <typename Container>
typename std::enable_if<pm::isomorphic_to_container_of<Container, Set<int>>::value,
                        pm::SelectedSubset<const Container&, operations::composed11<operations::contains<Set<int>>, std::logical_not<bool>>> >::type
select_not_k(const Container& sets, const int k)
{
   return pm::SelectedSubset<const Container&, operations::composed11<operations::contains<Set<int>>, std::logical_not<bool>>>(sets, operations::composed11<operations::contains<Set<int>>, std::logical_not<bool>>(operations::contains<Set<int>>(k), std::logical_not<bool>()));
}

template <typename Container>
auto drop_shift(const Container& sets, const int k)
{
   return attach_operation(
            attach_operation(sets, pm::operations::construct_unary2_with_arg< pm::SelectedSubset, operations::fix2<int,operations::ne> >(operations::fix2<int,operations::ne>(k))),
              pm::operations::construct_unary2_with_arg<pm::TransformedContainer, operations::dropshift<int> >(operations::dropshift<int>(k)));
}

template <typename Container>
Array<Set<int>>
shift_elements(const Container& sets, const int n)
{
   return Array<Set<int>>{ attach_operation(sets, 
                                            pm::operations::construct_unary2_with_arg<pm::TransformedContainer, 
                                            operations::fix2<int,operations::add> >(operations::fix2<int,operations::add>(n))) };
}

}}

#endif // __POLYMAKE_MATROID_UTIL

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
