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

#ifndef POLYMAKE_INTERNAL_CONVERTERS_H
#define POLYMAKE_INTERNAL_CONVERTERS_H

#include "polymake/internal/converters_basic_defs.h"
#include "polymake/internal/operations.h"
#include "polymake/TransformedContainer.h"

#include <cstdlib>
#include <cmath>

namespace pm {

template <typename From1, typename From2, typename To1, typename To2,
          int cc1=(std::is_convertible<From1, To1>::value * 2 + is_explicitly_convertible_to<From1, To1>::value),
          int cc2=(std::is_convertible<From2, To2>::value * 2 + is_explicitly_convertible_to<From2, To2>::value)>
class pair_conv : public nothing {};

template <typename From1, typename From2, typename To1, typename To2>
class pair_conv<From1, From2, To1, To2, 2, 2>
{
public:
   pair<To1, To2> operator() (const pair<From1, From2>& p) const
   {
      return pair<To1, To2>(p.first, p.second);
   }
};

template <typename From1, typename From2, typename To1, typename To2>
class pair_conv<From1, From2, To1, To2, 2, 1>
{
private:
   conv<From2, To2> c2;
public:
   pair<To1, To2> operator() (const pair<From1, From2>& p) const
   {
      return pair<To1, To2>(p.first, c2(p.second));
   }
};

template <typename From1, typename From2, typename To1, typename To2>
class pair_conv<From1, From2, To1, To2, 1, 2>
{
private:
   conv<From1, To1> c1;
public:
   pair<To1, To2> operator() (const pair<From1, From2>& p) const
   {
      return pair<To1, To2>(c1(p.first), p.second);
   }
};

template <typename From1, typename From2, typename To1, typename To2>
class pair_conv<From1, From2, To1, To2, 1, 1>
{
private:
   conv<From1, To1> c1;
   conv<From2, To2> c2;
public:
   pair<To1, To2> operator() (const pair<From1, From2>& p) const
   {
      return pair<To1, To2>(c1(p.first), c2(p.second));
   }
};

template <typename From1, typename From2, typename To1, typename To2>
class conv< pair<From1, From2>, pair<To1, To2> > :
   public pair_conv<From1, From2, To1, To2>
{
public:
   typedef pair<From1, From2> argument_type;
   typedef pair<To1, To2> result_type;
};

// resolving the ambiguity
template <typename T1, typename T2>
class conv< pair<T1, T2>, pair<T1, T2> > {
public:
   typedef pair<T1, T2> argument_type;
   typedef argument_type result_type;

   const result_type& operator() (const argument_type& p) const { return p; }
};


template <typename Target, typename Container>
auto attach_converter(const Container& src,
                      std::enable_if_t<can_initialize<typename object_traits<typename Container::value_type>::persistent_type, Target>::value &&
                                       !std::is_convertible<typename object_traits<typename Container::value_type>::persistent_type, Target>::value, void**> =nullptr)
{
   typedef conv<typename object_traits<typename Container::value_type>::persistent_type, Target> converter;
   return TransformedContainer<const Container&, converter>(src);
}

template <typename Target, typename Container, typename ConverterArg>
TransformedContainer<const Container&, conv<typename object_traits<typename Container::value_type>::persistent_type, Target> >
attach_converter(const Container& src, const ConverterArg& conv_arg,
                 std::enable_if_t<is_explicitly_convertible_to<typename object_traits<typename Container::value_type>::persistent_type, Target>::value, void**> =nullptr)
{
   return TransformedContainer<const Container&, conv<typename object_traits<typename Container::value_type>::persistent_type, Target> >
                              (src, conv_arg);
}

template <typename Target, typename Container>
const Container&
attach_converter(const Container& src,
                 std::enable_if_t<std::is_convertible<typename object_traits<typename Container::value_type>::persistent_type, Target>::value, void**> =nullptr)
{
   return src;
}

template <typename Target, typename Iterator>
auto make_converting_iterator(Iterator&& it,
                              std::enable_if_t<can_initialize<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                               !std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value, void**> =nullptr)
{
   typedef conv<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type, Target> converter;
   return unary_transform_iterator<pointer2iterator_t<Iterator>, converter>(pointer2iterator(std::forward<Iterator>(it)));
}

template <typename Target, typename Iterator>
decltype(auto) make_converting_iterator(Iterator&& it,
                                        std::enable_if_t<std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value, void**> =nullptr)
{
   return pointer2iterator(std::forward<Iterator>(std::forward<Iterator>(it)));
}

//! trivial conversion of an object to itself (mutable access)
template <typename Target, typename Source>
inherit_reference_t<Target, Source&&>
convert_to(Source&& x,
           std::enable_if_t<is_derived_from<pure_type_t<Source>, Target>::value ||
                            std::is_same<pure_type_t<Source>, Target>::value, void**> =nullptr)
{
   return std::forward<Source>(x);
}

template <typename Target, typename Source>
Target convert_to(Source x,
                  std::enable_if_t<std::is_arithmetic<Source>::value &&
                                   std::is_arithmetic<Target>::value &&
                                   !std::is_same<Source, Target>::value, void**> =nullptr)
{
   return Target(x);
}

//! conversion via functor
template <typename Target, typename Source>
Target convert_to(const Source& x,
                  std::enable_if_t<can_initialize<Source, Target>::value &&
                                   (!std::is_arithmetic<Source>::value || !std::is_arithmetic<Target>::value) &&
                                   !is_derived_from<Source, Target>::value, void**> =nullptr)
{
   return conv<Source, Target>()(x);
}

} // end namespace pm

namespace polymake {
   using pm::conv;
   using pm::attach_converter;
   using pm::convert_to;
}

#endif // POLYMAKE_INTERNAL_CONVERTERS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
