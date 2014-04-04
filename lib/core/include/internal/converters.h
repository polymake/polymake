/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_INTERNAL_CONVERTERS_H
#define POLYMAKE_INTERNAL_CONVERTERS_H

#include "polymake/internal/converters_basic_defs.h"
#include "polymake/internal/operations.h"
#include "polymake/TransformedContainer.h"

#include <cstdlib>
#include <cmath>

namespace pm {

// Everything can be converted to @c nothing.
template <typename T>
class conv<T, nothing> {
public:
   typedef T argument_type;
   typedef nothing result_type;
   nothing operator() (typename function_argument<T>::type) const { return nothing(); }
};

// conversion from nothing means creation from scratch, i.e. by default constructor
template <typename T>
class conv<nothing, T> : public operations::clear<T> {
public:
   typedef nothing argument_type;

   const T& operator() (nothing) const
   {
      return operations::clear<T>::operator()();
   }
};

template <typename From1, typename From2, typename To1, typename To2,
          int cc1=(convertible_to<From1, To1>::value * 2 + explicitly_convertible_to<From1, To1>::value),
          int cc2=(convertible_to<From2, To2>::value * 2 + explicitly_convertible_to<From2, To2>::value)>
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


template <typename Result, typename Container> inline
TransformedContainer<const Container&, conv<typename object_traits<typename Container::value_type>::persistent_type, Result> >
attach_converter(const Container& src,
                 typename enable_if<void**, explicitly_convertible_to<typename object_traits<typename Container::value_type>::persistent_type, Result>::value>::type=0)
{
   return src;
}

template <typename Result, typename Container, typename ConverterArg> inline
TransformedContainer<const Container&, conv<typename object_traits<typename Container::value_type>::persistent_type, Result> >
attach_converter(const Container& src, const ConverterArg& conv_arg,
                 typename enable_if<void**, explicitly_convertible_to<typename object_traits<typename Container::value_type>::persistent_type, Result>::value>::type=0)
{
   return TransformedContainer<const Container&, conv<typename object_traits<typename Container::value_type>::persistent_type, Result> >
                              (src, conv_arg);
}

template <typename Result, typename Container> inline
const Container&
attach_converter(const Container& src,
                 typename enable_if<void**, convertible_to<typename object_traits<typename Container::value_type>::persistent_type, Result>::value>::type=0)
{
   return src;
}

template <typename Result, typename Iterator> inline
unary_transform_iterator<Iterator, conv<typename iterator_traits<Iterator>::value_type, Result> >
make_converting_iterator(const Iterator& it,
                         typename enable_if<void**, explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Result>::value>::type=0)
{
   return it;
}

template <typename Result, typename Iterator> inline
const Iterator&
make_converting_iterator(const Iterator& it,
                         typename enable_if<void**, convertible_to<typename iterator_traits<Iterator>::value_type, Result>::value>::type=0)
{
   return it;
}


template <>
class conv<const char*, int> {
public:
   typedef const char* argument_type;
   typedef int result_type;

   result_type operator() (argument_type s) const { return std::atoi(s); }
};

template <>
class conv<const char*, long> {
public:
   typedef const char* argument_type;
   typedef long result_type;

   result_type operator() (argument_type s) const { return std::atol(s); }
};

template <typename From, typename To>
class conv_to_constant {
public:
   typedef From argument_type;
   typedef typename To::constant_type result_type;

   result_type operator() (const argument_type&) const
   {
      return To::value;
   }
};

//! trivial conversion of an object to itself (mutable access)
template <typename Target> inline
Target& convert_to(Target& x)
{
   return x;
}

//! trivial conversion of an object to itself (read-only access)
//! or conversion via allowed (non-explicit) constructor
template <typename Target> inline
const Target& convert_to(const Target& x)
{
   return x;
}

//! conversion via functor
template <typename Target, typename Source> inline
Target convert_to(const Source& x,
                  typename enable_if<void**, explicitly_convertible_to<Source, Target>::value>::type=0)
{
   return conv<Source, Target>()(x);
}


template <typename Iterator, typename Target, bool is_enabled1, typename is_enabled2=True>
struct construct_matching_iterator_helper {};

template <typename Iterator, typename Target>
struct construct_matching_iterator_helper<Iterator, Target, true,
                                          typename enable_if<True, (isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                                    convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   typedef void** enabled;
   typedef Iterator iterator;

   Iterator& operator() (Iterator& src) const { return src; }
};

template <typename Iterator, typename Target>
struct construct_matching_iterator_helper<Iterator, Target, true,
                                          typename enable_if<True, (isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                                    explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   typedef void** enabled;
   typedef unary_transform_iterator<Iterator, conv<typename iterator_traits<Iterator>::value_type, Target> > iterator;

   iterator operator() (const Iterator& src, int) const { return src; }
};

template <typename Iterator, typename Target>
struct construct_matching_iterator :
   construct_matching_iterator_helper<Iterator, Target, !(convertible_to<Iterator, Target>::value || explicitly_convertible_to<Iterator, Target>::value)> {};

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
