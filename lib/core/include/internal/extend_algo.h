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

#ifndef POLYMAKE_INTERNAL_EXTEND_ALGO_H
#define POLYMAKE_INTERNAL_EXTEND_ALGO_H

#if !defined(__clang__) && __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#endif

namespace pm {

template <typename Iterator1, typename Iterator2>
void copy_range_impl(Iterator1&& src, Iterator2&& dst, std::true_type, std::false_type)
{
   for (; !src.at_end(); ++src, ++dst) *dst=*src;
}

template <typename Iterator1, typename Iterator2>
void copy_range_impl(Iterator1&& src, Iterator2&& dst, std::false_type, std::true_type)
{
   for (; !dst.at_end(); ++src, ++dst) *dst=*src;
}

template <typename Iterator1, typename Iterator2>
void copy_range_impl(Iterator1&& src, Iterator2&& dst, std::true_type, std::true_type)
{
   for (; !src.at_end() && !dst.at_end(); ++src, ++dst) *dst=*src;
}

template <typename Iterator1, typename Iterator2,
          typename=std::enable_if_t<check_iterator_feature<Iterator1, end_sensitive>::value ||
                                    check_iterator_feature<Iterator2, end_sensitive>::value>>
private_mutable_t<Iterator2> copy_range(Iterator1&& src, Iterator2&& dst)
{
   private_mutable_t<Iterator2> dst_it=ensure_private_mutable(std::forward<Iterator2>(dst));
   copy_range_impl(ensure_private_mutable(std::forward<Iterator1>(src)), dst_it,
                   bool_constant<check_iterator_feature<Iterator1, end_sensitive>::value>(),
                   bool_constant<check_iterator_feature<Iterator2, end_sensitive>::value>());
   return std::move(dst_it);
}


template <typename Iterator1, typename Iterator2>
bool equal_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::true_type, std::false_type)
{
   for (; !it1.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return true;
}

template <typename Iterator1, typename Iterator2>
bool equal_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::false_type, std::true_type)
{
   for (; !it2.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return true;
}

template <typename Iterator1, typename Iterator2>
bool equal_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::true_type, std::true_type)
{
   for (; !it1.at_end() && !it2.at_end(); ++it1, ++it2)
      if (*it1 != *it2) return false;
   return it1.at_end() && it2.at_end();
}

template <typename Iterator1, typename Iterator2,
          typename=std::enable_if_t<check_iterator_feature<Iterator1, end_sensitive>::value ||
                                    check_iterator_feature<Iterator2, end_sensitive>::value>>
bool equal_ranges(Iterator1&& it1, Iterator2&& it2)
{
   return equal_ranges_impl(ensure_private_mutable(std::forward<Iterator1>(it1)),
                            ensure_private_mutable(std::forward<Iterator2>(it2)),
                            bool_constant<check_iterator_feature<Iterator1, end_sensitive>::value>(),
                            bool_constant<check_iterator_feature<Iterator2, end_sensitive>::value>());
}


template <typename Iterator1, typename Iterator2>
void swap_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::true_type, std::false_type)
{
   for (; !it1.at_end(); ++it1, ++it2) std::swap(*it1, *it2);
}

template <typename Iterator1, typename Iterator2>
void swap_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::false_type, std::true_type)
{
   for (; !it2.at_end(); ++it1, ++it2) std::swap(*it1, *it2);
}

template <typename Iterator1, typename Iterator2>
void swap_ranges_impl(Iterator1&& it1, Iterator2&& it2, std::true_type, std::true_type)
{
   for (; !it1.at_end() && !it2.at_end(); ++it1, ++it2) std::swap(*it1, *it2);
}

template <typename Iterator1, typename Iterator2,
          typename=std::enable_if_t<check_iterator_feature<Iterator1, end_sensitive>::value ||
                                    check_iterator_feature<Iterator2, end_sensitive>::value>>
void swap_ranges(Iterator1&& it1, Iterator2&& it2)
{
   swap_ranges_impl(ensure_private_mutable(std::forward<Iterator1>(it1)),
                    ensure_private_mutable(std::forward<Iterator2>(it2)),
                    bool_constant<check_iterator_feature<Iterator1, end_sensitive>::value>(),
                    bool_constant<check_iterator_feature<Iterator2, end_sensitive>::value>());
}


template <typename Iterator,
          typename=std::enable_if_t<check_iterator_feature<Iterator, end_sensitive>::value>>
typename iterator_traits<Iterator>::value_type
first_differ_in_range(Iterator&& src, const typename iterator_traits<Iterator>::value_type& from)
{
   auto&& it=ensure_private_mutable(std::forward<Iterator>(src));
   for (; !it.at_end(); ++it) {
      const typename iterator_traits<Iterator>::value_type v=*it;
      if (v != from) return v;
   }
   return from;
}

template <typename Iterator, typename Value,
          typename=std::enable_if_t<check_iterator_feature<Iterator, end_sensitive>::value>>
void fill_range(Iterator&& dst, const Value& x)
{
   auto&& it=ensure_private_mutable(std::forward<Iterator>(dst));
   for (; !it.at_end(); ++it) *it=x;
}

template <typename Iterator, typename Value,
          typename=std::enable_if_t<check_iterator_feature<Iterator, end_sensitive>::value>>
private_mutable_t<Iterator>
find_in_range(Iterator&& src, const Value& x)
{
   private_mutable_t<Iterator> it=ensure_private_mutable(std::forward<Iterator>(src));
   while (!it.at_end() && *it != x) ++it;
   return std::move(it);
}

template <typename Iterator, typename Predicate,
          typename=std::enable_if_t<check_iterator_feature<Iterator, end_sensitive>::value>>
private_mutable_t<Iterator>
find_in_range_if(Iterator&& src, const Predicate& pred_arg)
{
   typedef pm::unary_helper<pure_type_t<Iterator>, Predicate> helper;
   const typename helper::operation& pred=helper::create(pred_arg);
   private_mutable_t<Iterator> it=ensure_private_mutable(std::forward<Iterator>(src));
   while (!it.at_end() && !pred(*helper::get(it))) ++it;
   return std::move(it);
}

} // end namespace pm

#if !defined(__clang__) && __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif

#endif // POLYMAKE_INTERNAL_EXTEND_ALGO_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
