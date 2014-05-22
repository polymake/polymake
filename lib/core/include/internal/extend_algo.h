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

#ifndef POLYMAKE_INTERNAL_EXTEND_ALGO_H
#define POLYMAKE_INTERNAL_EXTEND_ALGO_H

namespace pm {

template <typename Iterator1, typename Iterator2> inline
typename enable_if<Iterator2, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                               !check_iterator_feature<Iterator2,end_sensitive>::value)>::type
copy(Iterator1 src, Iterator2 dst)
{
   for (; !src.at_end(); ++src, ++dst) *dst=*src;
   return dst;
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<Iterator2, (!check_iterator_feature<Iterator1,end_sensitive>::value &&
                               check_iterator_feature<Iterator2,end_sensitive>::value)>::type
copy(Iterator1 src, Iterator2 dst)
{
   for (; !dst.at_end(); ++src, ++dst) *dst=*src;
   return dst;
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<Iterator2, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                               check_iterator_feature<Iterator2,end_sensitive>::value)>::type
copy(Iterator1 src, Iterator2 dst)
{
   for (; !src.at_end() && !dst.at_end(); ++src, ++dst) *dst=*src;
   return dst;
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<bool, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                          !check_iterator_feature<Iterator2,end_sensitive>::value)>::type
equal(Iterator1 it1, Iterator2 it2)
{
   for (; !it1.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return true;
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<bool, (!check_iterator_feature<Iterator1,end_sensitive>::value &&
                          check_iterator_feature<Iterator2,end_sensitive>::value)>::type
equal(Iterator1 it1, Iterator2 it2)
{
   for (; !it2.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return true;
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<bool, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                          check_iterator_feature<Iterator2,end_sensitive>::value)>::type
equal(Iterator1 it1, Iterator2 it2)
{
   for (; !it1.at_end() && !it2.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return it1.at_end() && it2.at_end();
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<void, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                          !check_iterator_feature<Iterator2,end_sensitive>::value)>::type
swap_ranges(Iterator1 it1, Iterator2 it2)
{
   for (; !it1.at_end(); ++it1, ++it2) std::swap(*it1,*it2);
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<void, (!check_iterator_feature<Iterator1,end_sensitive>::value &&
                          check_iterator_feature<Iterator2,end_sensitive>::value)>::type
swap_ranges(Iterator1 it1, Iterator2 it2)
{
   for (; !it2.at_end(); ++it1, ++it2) std::swap(*it1,*it2);
}

template <typename Iterator1, typename Iterator2> inline
typename enable_if<void, (check_iterator_feature<Iterator1,end_sensitive>::value &&
                          check_iterator_feature<Iterator2,end_sensitive>::value)>::type
swap_ranges(Iterator1 it1, Iterator2 it2)
{
   for (; !it1.at_end() && !it2.at_end(); ++it1, ++it2) std::swap(*it1,*it2);
}

template <typename Iterator> inline
typename iterator_traits<Iterator>::value_type
first_differ(Iterator src, typename function_argument<typename iterator_traits<Iterator>::value_type>::type from)
{
   for (; !src.at_end(); ++src) {
      const typename iterator_traits<Iterator>::value_type v=*src;
      if (v != from) return v;
   }
   return from;
}

template <typename Iterator, typename T> inline
void fill(Iterator dst, const T& value)
{
   for (; !dst.at_end(); ++dst) *dst=value;
}

template <typename Iterator, typename Value> inline
Iterator find(Iterator src, const Value x)
{
   while (!src.at_end() && *src != x) ++src;
   return src;
}

template <typename Iterator, typename Predicate> inline
Iterator find_if(Iterator src, const Predicate& pred_arg)
{
   typedef pm::unary_helper<Iterator, Predicate> helper;
   const typename helper::operation& pred=helper::create(pred_arg);
   while (!src.at_end() && !pred(*helper::get(src))) ++src;
   return src;
}

} // end namespace pm

#endif // POLYMAKE_INTERNAL_EXTEND_ALGO_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
