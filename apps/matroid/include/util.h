/* Copyright (c) 1997-2023
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

#pragma once

namespace polymake { namespace matroid {

namespace operations {

using namespace polymake::operations;

template <typename SetRef>
struct contains {
   typedef typename pm::deref<SetRef>::type set_type;
   typedef set_type argument_type;
   typedef bool result_type;
   typedef typename pm::deref<SetRef>::type::element_type element_type;

   element_type elem;

   contains(element_type e = element_type()) : elem(e) {}

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

   dropshift(scalar_type e = scalar_type()) : elem(e) {}

   result_type operator() (argument_type value) const
   {
      return value < elem ? value : value-1;
   }
};

}

template <typename Container>
auto
select_k(const Container& sets, const Int k,
         std::enable_if_t<pm::isomorphic_to_container_of<Container, Set<Int>>::value, std::nullptr_t> = nullptr)
{
   return pm::SelectedSubset<const Container&, operations::contains<Set<Int>>>(sets, operations::contains<Set<Int>>(k));
}

template <typename Container>
auto
select_not_k(const Container& sets, const Int k,
             std::enable_if_t<pm::isomorphic_to_container_of<Container, Set<Int>>::value, std::nullptr_t> = nullptr)
{
   return pm::SelectedSubset<const Container&, operations::composed11<operations::contains<Set<Int>>, std::logical_not<bool>>>(sets, operations::composed11<operations::contains<Set<Int>>, std::logical_not<bool>>(operations::contains<Set<Int>>(k), std::logical_not<bool>()));
}

template <typename Container>
auto drop_shift(const Container& sets, const Int k)
{
   return attach_operation(
            attach_operation(sets, pm::operations::construct_unary2_with_arg< pm::SelectedSubset, operations::fix2<Int, operations::ne> >(operations::fix2<Int, operations::ne>(k))),
              pm::operations::construct_unary2_with_arg<pm::TransformedContainer, operations::dropshift<Int>>(operations::dropshift<Int>(k)));
}

template <typename Container>
Array<Set<Int>>
shift_elements(const Container& sets, const Int n)
{
   return Array<Set<Int>>{ attach_operation(sets, 
                                            pm::operations::construct_unary2_with_arg<pm::TransformedContainer, 
                                            operations::fix2<Int, operations::add> >(operations::fix2<Int, operations::add>(n))) };
}

}}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
