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

#ifndef POLYMAKE_INTERNAL_COMPARATORS_H
#define POLYMAKE_INTERNAL_COMPARATORS_H

#include "polymake/internal/comparators_ops.h"
#include "polymake/internal/matrix_rows_cols.h"
#include "polymake/TransformedContainer.h"

namespace pm {
namespace operations {

template <typename Container1, typename Container2, typename ComparatorFamily,
          int dim1=object_traits<Container1>::dimension,
          int dim2=object_traits<Container2>::dimension>
struct cmp_lex_containers;

template <typename Matrix1, typename Matrix2, typename ComparatorFamily>
struct cmp_lex_containers<Matrix1, Matrix2, ComparatorFamily, 2, 2>
   : cmp_extremal {

   typedef Matrix1 first_argument_type;
   typedef Matrix2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();

   typedef cmp_lex_containers<Rows<Matrix1>, Rows<Matrix2>, ComparatorFamily> cmp_rows;

   static const bool partially_defined=cmp_rows::partially_defined;

   cmp_value operator()(const Matrix1& a, const Matrix2& b) const
   {
      return cmp_rows()(rows(a), rows(b));
   }

   template <typename Iterator2>
   typename std::enable_if<partially_defined, typename mproject1st<cmp_value, Iterator2>::type>::type
   operator() (partial_left, const Matrix1& a, const Iterator2& b) const
   {
      return cmp_rows()(partial_left(), rows(a), b);
   }

   template <typename Iterator1>
   typename std::enable_if<partially_defined, typename mproject1st<cmp_value, Iterator1>::type>::type
   operator() (partial_right, const Iterator1& a, const Matrix2& b) const
   {
      return cmp_rows()(partial_right(), a, rows(b));
   }
};

template <typename Container1, typename Container2, typename ComparatorFamily>
struct cmp_lex_containers<Container1, Container2, ComparatorFamily, 1, 1>
   : cmp_extremal {

   typedef Container1 first_argument_type;
   typedef Container2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal::operator();

   static const bool partially_defined=std::is_same<ComparatorFamily, cmp>::value &&
      define_comparator<typename container_element_type<Container1>::type, typename container_element_type<Container2>::type, ComparatorFamily>::partially_defined;

protected:
   template <typename Iterator> static
   bool at_end1(const Iterator& it, mlist<end_sensitive>)
   {
      return static_cast<const typename Iterator::first_type&>(it).at_end();
   }
   template <typename Iterator> static
   bool at_end1(const Iterator&, mlist<void>)
   {
      return false;
   }
   template <typename Iterator> static
   bool at_end2(const Iterator& it, mlist<end_sensitive>)
   {
      return it.second.at_end();
   }
   template <typename Iterator> static
   bool at_end2(const Iterator&, mlist<void>)
   {
      return false;
   }

   static
   cmp_value compare(const Container1& a, const Container2& b, std::false_type)
   {
      typedef typename std::conditional<object_classifier::what_is<Container1>::value==object_classifier::is_constant,
                                        void, end_sensitive>::type
        feature1;
      typedef typename std::conditional<object_classifier::what_is<Container2>::value==object_classifier::is_constant,
                                        void, end_sensitive>::type
         feature2;

      const TransformedContainerPair< masquerade_add_features<const Container1&, feature1>,
                                      masquerade_add_features<const Container2&, feature2>,
                                      ComparatorFamily > TP(a, b, ComparatorFamily());
      auto it=entire(TP);
      for ( ; !at_end1(it, mlist<feature1>()); ++it) {
         if (at_end2(it, mlist<feature2>()))
            return cmp_gt;
         const cmp_value result=*it;
         if (result != cmp_eq)
            return result;
      }
      return at_end2(it, mlist<feature2>()) ? cmp_eq : std::is_same<ComparatorFamily, cmp_unordered>::value ? cmp_ne : cmp_lt;
   }

   static
   cmp_value compare(const Container1& a, const Container2& b, std::true_type)
   {
      if (std::is_same<ComparatorFamily, cmp_unordered>::value && get_dim(a) != get_dim(b))
         return cmp_ne;
      const cmp_value result=first_differ_in_range(entire(attach_operation(a, b, ComparatorFamily())), cmp_eq);
      if (result != cmp_eq || std::is_same<ComparatorFamily, cmp_unordered>::value)
         return result;
      return cmp_value(sign(get_dim(a) - get_dim(b)));
   }

public:
   cmp_value operator() (const Container1& a, const Container2& b) const
   {
      const bool got_sparse=check_container_feature<Container1, sparse>::value ||
                            check_container_feature<Container2, sparse>::value;
      return compare(a, b, bool_constant<got_sparse>());
   }

   template <typename Iterator2>
   typename std::enable_if<partially_defined, typename mproject1st<cmp_value, Iterator2>::type>::type
   operator() (partial_left, const Container1& a, const Iterator2&) const
   {
      ComparatorFamily cmp_el;
      cmp_value ret=cmp_eq;
      for (auto it=entire(a); !it.at_end(); ++it)
         if ((ret=cmp_el(partial_left(), *it, it)) != cmp_eq) break;
      return ret;
   }

   template <typename Iterator1>
   typename std::enable_if<partially_defined, typename mproject1st<cmp_value, Iterator1>::type>::type
   operator() (partial_right, const Iterator1&, const Container2& b) const
   {
      ComparatorFamily cmp_el;
      cmp_value ret=cmp_eq;
      for (auto it=entire(b); !it.at_end(); ++it)
         if ((ret=cmp_el(partial_right(), it, *it)) != cmp_eq) break;
      return ret;
   }
};

template <typename T, typename ComparatorFamily>
struct is_comparable_container : std::true_type { };

template <typename T>
struct is_comparable_container<T, cmp>
   : is_ordered<typename container_element_type<T>::type> { };

template <typename T>
struct is_comparable_container<T, cmp_unordered>
   : is_unordered_comparable<typename container_element_type<T>::type> { };

template <typename T1, typename T2, typename ComparatorFamily, typename Tag>
struct define_comparator<T1, T2, ComparatorFamily, is_container, is_container, cons<Tag, Tag>,
                         typename std::enable_if<(isomorphic_types<T1, T2>::value &&
                                                  is_comparable_container<T1, ComparatorFamily>::value &&
                                                  is_comparable_container<T2, ComparatorFamily>::value), cmp_value>::type> {
   typedef cmp_lex_containers<T1, T2, ComparatorFamily> type;
   static const bool partially_defined=type::partially_defined;
};

template <typename Composite1, typename Composite2, typename ComparatorFamily,
          int l1=list_length<typename object_traits<Composite1>::elements>::value,
          int l2=list_length<typename object_traits<Composite2>::elements>::value>
struct cmp_lex_composite;

template <typename Composite1, typename Composite2, typename ComparatorFamily, int l>
struct cmp_lex_composite<Composite1, Composite2, ComparatorFamily, l, l> {

   typedef Composite1 first_argument_type;
   typedef Composite2 second_argument_type;
   typedef cmp_value result_type;

protected:
   static const int last=l-1;

   // SUGGESTION: make provisions for a list of comparators?

   template <int i> static
   cmp_value compare_step(const Composite1& a, const Composite2& b, int_constant<i>)
   {
      cmp_value result=ComparatorFamily()(visit_n_th(a, int_constant<i>()), visit_n_th(b, int_constant<i>()));
      const int next= i+1<l ? i+1 : i;
      if (i<next && result==cmp_eq) result=compare_step(a, b, int_constant<next>());
      return result;
   }

public:
   cmp_value operator() (const Composite1& a, const Composite2& b) const
   {
      return compare_step(a, b, int_constant<0>());
   }
};

template <typename T, typename ComparatorFamily>
struct is_comparable_composite : std::true_type { };

template <typename T>
struct is_comparable_composite<T, cmp>
   : list_accumulate_unary<list_and, is_ordered, typename object_traits<T>::elements> { };

template <typename T>
struct is_comparable_composite<T, cmp_unordered>
   : list_accumulate_unary<list_and, is_unordered_comparable, typename object_traits<T>::elements> { };

template <typename T1, typename T2, typename ComparatorFamily, typename Tag>
struct define_comparator<T1, T2, ComparatorFamily, is_composite, is_composite, cons<Tag, Tag>,
                         typename std::enable_if<(isomorphic_types<T1, T2>::value &&
                                                  is_comparable_composite<T1, ComparatorFamily>::value &&
                                                  is_comparable_composite<T2, ComparatorFamily>::value), cmp_value>::type> {
   typedef cmp_lex_composite<T1, T2, ComparatorFamily> type;
   static const bool partially_defined=false;
};

} // end namespace operations

template <typename T, bool _is_max> constexpr
bool operator== (const T&, const extremal<T,_is_max>&) { return false; }
template <typename T, bool _is_max> constexpr
bool operator!= (const T&, const extremal<T,_is_max>&) { return true; }
template <typename T, bool _is_max> constexpr
bool operator< (const T&, const extremal<T,_is_max>&) { return _is_max; }
template <typename T, bool _is_max> constexpr
bool operator<= (const T&, const extremal<T,_is_max>&) { return _is_max; }
template <typename T, bool _is_max> constexpr
bool operator> (const T&, const extremal<T,_is_max>&) { return !_is_max; }
template <typename T, bool _is_max> constexpr
bool operator>= (const T&, const extremal<T,_is_max>&) { return !_is_max; }
template <typename T, bool _is_max> constexpr
bool operator== (const extremal<T,_is_max>&, const T&) { return false; }
template <typename T, bool _is_max> constexpr
bool operator!= (const extremal<T,_is_max>&, const T&) { return true; }
template <typename T, bool _is_max> constexpr
bool operator< (const extremal<T,_is_max>&, const T&) { return !_is_max; }
template <typename T, bool _is_max> constexpr
bool operator<= (const extremal<T,_is_max>&, const T&) { return !_is_max; }
template <typename T, bool _is_max> constexpr
bool operator> (const extremal<T,_is_max>&, const T&) { return _is_max; }
template <typename T, bool _is_max> constexpr
bool operator>= (const extremal<T,_is_max>&, const T&) { return _is_max; }

template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator== (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1==_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator!= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1!=_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator< (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1<_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator<= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1<=_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator> (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1>_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> constexpr
bool operator>= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1>=_is_max2; }

// for hash_set and similar
struct is_unordered_set;

// copied from boost/functional/hash.hpp
template <typename SizeT>
inline
typename std::enable_if<sizeof(SizeT)==sizeof(uint64_t)>::type hash_combine(SizeT& h, SizeT k)
{
   const uint64_t m = 0xc6a4a7935bd1e995ul;
   const int r = 47;

   k *= m;
   k ^= k >> r;
   k *= m;

   h ^= k;
   h *= m;
}

template <typename SizeT>
inline
typename std::enable_if<sizeof(SizeT)==sizeof(uint32_t)>::type hash_combine(SizeT& h, SizeT k)
{
   const size_t c1 = 0xcc9e2d51;
   const size_t c2 = 0x1b873593;

   k *= c1;
   k = (k << 15) | (k >> (32 - 15));
   k *= c2;

   h ^= k;
   h = (h << 13) | (h >> (32 - 13));
   h = h*5+0xe6546b64;
}

template <typename T>
struct hash_func<T, is_container> {
protected:
   hash_func<typename T::value_type> hash_elem;

public:
   size_t operator() (const T& x) const
   {
      size_t h=0;
      for (auto it=entire(x); !it.at_end(); ++it) {
         hash_combine(h, hash_elem(*it));
      }
      return h;
   }
};

template <typename T>
struct hash_composite : hash_composite<typename list_tail<T>::type> {
   typedef hash_composite<typename list_tail<T>::type> super;

   typedef typename list_head<T>::type element_type;
   typedef typename deref<element_type>::type value_type;
   super& operator<< (typename attrib<element_type>::plus_const_ref x)
   {
      hash_combine(this->value, hash_elem(x));
      return *this;
   }

   hash_func<value_type> hash_elem;
};

template <>
struct hash_composite<void> {
   size_t value = 0;
};

template <typename T>
struct hash_func<T, is_composite> {
public:
   size_t operator() (const T& x) const
   {
      hash_composite<typename object_traits<T>::elements> h;
      object_traits<T>::visit_elements(x, h);
      return h.value;
   }
};

} // end namespace pm

namespace polymake {
using pm::sign;
using pm::assign_max;
using pm::assign_min;
using pm::assign_min_max;
using pm::maximal;
using pm::minimal;
using std::abs;
using pm::abs;
using pm::isinf;
using pm::isfinite;
using pm::local_epsilon;

namespace operations {
typedef BuildUnary<pm::operations::positive> positive;
typedef BuildUnary<pm::operations::negative> negative;
typedef BuildUnary<pm::operations::non_zero> non_zero;
typedef BuildUnary<pm::operations::equals_to_zero> is_zero;

using pm::operations::cmp;
using pm::operations::cmp_with_leeway;
typedef BuildBinary<pm::operations::eq> eq;
typedef BuildBinary<pm::operations::ne> ne;
typedef BuildBinary<pm::operations::lt> lt;
typedef BuildBinary<pm::operations::le> le;
typedef BuildBinary<pm::operations::gt> gt;
typedef BuildBinary<pm::operations::ge> ge;
typedef BuildBinary<pm::operations::max> max;
typedef BuildBinary<pm::operations::min> min;
typedef BuildUnary<pm::operations::abs_value> abs_value;

// replacement for std::less performing lexicographic comparison
// to be used with STL algorithms and associative containers
struct lex_less {
   typedef bool result_type;

   template <typename Left, typename Right>
   bool operator() (const Left& l, const Right& r) const
   {
      return lex_compare(l, r)==pm::cmp_lt;
   }
};

} }

#endif // POLYMAKE_INTERNAL_COMPARATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
