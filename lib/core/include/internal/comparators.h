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
struct cmp_lex_containers<Matrix1, Matrix2, ComparatorFamily, 2, 2> :
   cmp_extremal_if_ordered< Rows<Matrix1>, Rows<Matrix2> > {

   typedef Matrix1 first_argument_type;
   typedef Matrix2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal_if_ordered< Rows<Matrix1>, Rows<Matrix2> >::operator();

   typedef cmp_lex_containers<Rows<Matrix1>, Rows<Matrix2>, ComparatorFamily> cmp_rows;

   static const bool partially_defined=cmp_rows::partially_defined;

   cmp_value operator()(const Matrix1& a, const Matrix2& b) const
   {
      return cmp_rows()(rows(a), rows(b));
   }

   template <typename Iterator2>
   typename enable_if<typename cons<cmp_value, Iterator2>::head, partially_defined>::type
   operator() (partial_left, const Matrix1& a, const Iterator2& b) const
   {
      return cmp_rows()(partial_left(), rows(a), b);
   }

   template <typename Iterator1>
   typename enable_if<typename cons<cmp_value, Iterator1>::head, partially_defined>::type
   operator() (partial_right, const Iterator1& a, const Matrix2& b) const
   {
      return cmp_rows()(partial_right(), a, rows(b));
   }
};

template <typename Container1, typename Container2, typename ComparatorFamily>
struct cmp_lex_containers<Container1, Container2, ComparatorFamily, 1, 1> :
   cmp_extremal_if_ordered<typename Container1::value_type, typename Container2::value_type> {

   typedef Container1 first_argument_type;
   typedef Container2 second_argument_type;
   typedef cmp_value result_type;

   using cmp_extremal_if_ordered<typename Container1::value_type, typename Container2::value_type>::operator();

   static const bool
      ordered=is_ordered<typename Container1::value_type>::value && is_ordered<typename Container2::value_type>::value,
      partially_defined=build_comparator<typename Container1::value_type, typename Container2::value_type, ComparatorFamily>::partially_defined;

protected:
   template <typename Iterator> static
   bool at_end1(const Iterator& it, end_sensitive*)
   {
      return static_cast<const typename Iterator::first_type&>(it).at_end();
   }
   template <typename Iterator> static
   bool at_end1(const Iterator&, void*)
   {
      return false;
   }
   template <typename Iterator> static
   bool at_end2(const Iterator& it, end_sensitive*)
   {
      return it.second.at_end();
   }
   template <typename Iterator> static
   bool at_end2(const Iterator&, void*)
   {
      return false;
   }

   template <typename Iterator, typename _end1, typename _end2> static
   cmp_value run(Iterator it, _end1 *e1, _end2 *e2, True)
   {
      for (;;) {
         if (at_end1(it, e1))
            return at_end2(it, e2) ? cmp_eq : cmp_lt;
         if (at_end2(it, e2))
            return cmp_gt;
         cmp_value result=*it;
         if (result != cmp_eq)
            return result;
         ++it;
      }
   }

   template <typename Iterator, typename _end1, typename _end2> static
   cmp_value run(Iterator it, _end1*, _end2*, False)
   {
      return first_differ(it, cmp_eq);
   }

   static
   cmp_value compare(const Container1& a, const Container2& b, False, bool2type<ordered>)
   {
      typedef typename if_else<object_classifier::what_is<Container1>::value==object_classifier::is_constant,
                               void, end_sensitive>::type
         feature1;
      typedef typename if_else<object_classifier::what_is<Container2>::value==object_classifier::is_constant,
                               void, end_sensitive>::type
         feature2;

      if (!ordered &&
          object_classifier::what_is<Container1>::value != object_classifier::is_constant &&
          object_classifier::what_is<Container2>::value != object_classifier::is_constant &&
          a.size() != b.size())
         return cmp_ne;

      typedef TransformedContainerPair< masquerade_add_features<const Container1&, feature1>,
                                        masquerade_add_features<const Container2&, feature2>,
                                        ComparatorFamily > TP;
      return run(entire(TP(a, b, ComparatorFamily())), (feature1*)0, (feature2*)0, bool2type<ordered>());
   }

   static
   cmp_value compare(const Container1& a, const Container2& b, True, True)
   {
      const cmp_value result=first_differ(entire(attach_operation(a, b, ComparatorFamily())), cmp_eq);
      return result!=cmp_eq ? result : pm::sign(get_dim(a) - get_dim(b));
   }

   static
   cmp_value compare(const Container1& a, const Container2& b, True, False)
   {
      return get_dim(a)==get_dim(b)
             ? first_differ(entire(attach_operation(a, b, ComparatorFamily())), cmp_eq)
             : cmp_ne;
   }

   static
   cmp_value compare(partial_left, const Container1& c, True)
   {
      ComparatorFamily cmp_el;
      cmp_value ret=cmp_eq;
      for (typename Entire<Container1>::const_iterator it=entire(c); !it.at_end(); ++it)
         if ((ret=cmp_el(partial_left(), *it, it)) != cmp_eq) break;
      return ret;
   }

   static
   cmp_value compare(partial_right, const Container2& c, True)
   {
      ComparatorFamily cmp_el;
      cmp_value ret=cmp_eq;
      for (typename Entire<Container2>::const_iterator it=entire(c); !it.at_end(); ++it)
         if ((ret=cmp_el(partial_right(), it, *it)) != cmp_eq) break;
      return ret;
   }

   static
   cmp_value compare(partial_left, const Container1& a, False)
   {
      return a.empty() ? cmp_eq : cmp_ne;
   }

   static
   cmp_value compare(partial_right, const Container2& a, False)
   {
      return a.empty() ? cmp_eq : cmp_ne;
   }

public:
   cmp_value operator() (const Container1& a, const Container2& b) const
   {
      const bool got_sparse=check_container_feature<Container1,sparse>::value ||
                            check_container_feature<Container2,sparse>::value;
      return compare(a, b, bool2type<got_sparse>(), bool2type<ordered>());
   }

   template <typename Iterator2>
   typename enable_if<typename cons<cmp_value, Iterator2>::head, partially_defined>::type
   operator() (partial_left, const Container1& a, const Iterator2&) const
   {
      return compare(partial_left(), a, bool2type<ordered>());
   }

   template <typename Iterator1>
   typename enable_if<typename cons<cmp_value, Iterator1>::head, partially_defined>::type
   operator() (partial_right, const Iterator1&, const Container2& b) const
   {
      return compare(partial_right(), b, bool2type<ordered>());
   }
};

template <typename T1, typename T2, typename ComparatorFamily, typename Tag>
struct build_comparator<T1, T2, ComparatorFamily, is_container, is_container, cons<Tag,Tag>, true> {
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

   static const bool ordered= list_accumulate_unary<list_and, is_ordered, typename object_traits<Composite1>::elements>::value &&
                              list_accumulate_unary<list_and, is_ordered, typename object_traits<Composite2>::elements>::value;
protected:
   static const int last=l-1;

   // SUGGESTION: make provisions for a list of comparators?

   template <int i> static
   cmp_value compare_step(const Composite1& a, const Composite2& b, int2type<i>)
   {
      cmp_value result=ComparatorFamily()(visit_n_th(a, int2type<i>()), visit_n_th(b, int2type<i>()));
      const int next= i+1<l ? i+1 : i;
      if (i<next && result==cmp_eq) result=compare_step(a, b, int2type<next>());
      return result;
   }

public:
   cmp_value operator() (const Composite1& a, const Composite2& b) const
   {
      return compare_step(a, b, int2type<0>());
   }
};

template <typename T1, typename T2, typename ComparatorFamily, typename Tag>
struct build_comparator<T1, T2, ComparatorFamily, is_composite, is_composite, cons<Tag,Tag>, true> {
   typedef cmp_lex_composite<T1, T2, ComparatorFamily> type;
   static const bool partially_defined=false;
};

} // end namespace operations

namespace operators {

template <typename Left, typename Right> inline
bool operator!= (const Left& l, const Right& r)
{
   return !(l==r);
}

template <typename Left, typename Right> inline
bool operator> (const Left& l, const Right& r)
{
   return r<l;
}

template <typename Left, typename Right> inline
bool operator<= (const Left& l, const Right& r)
{
   return !(r<l);
}

template <typename Left, typename Right> inline
bool operator>= (const Left& l, const Right& r)
{
   return !(l<r);
}

} // end namespace operators

template <typename T, bool _is_max> inline
bool operator== (const T&, const extremal<T,_is_max>&) { return false; }
template <typename T, bool _is_max> inline
bool operator!= (const T&, const extremal<T,_is_max>&) { return true; }
template <typename T, bool _is_max> inline
bool operator< (const T&, const extremal<T,_is_max>&) { return _is_max; }
template <typename T, bool _is_max> inline
bool operator<= (const T&, const extremal<T,_is_max>&) { return _is_max; }
template <typename T, bool _is_max> inline
bool operator> (const T&, const extremal<T,_is_max>&) { return !_is_max; }
template <typename T, bool _is_max> inline
bool operator>= (const T&, const extremal<T,_is_max>&) { return !_is_max; }
template <typename T, bool _is_max> inline
bool operator== (const extremal<T,_is_max>&, const T&) { return false; }
template <typename T, bool _is_max> inline
bool operator!= (const extremal<T,_is_max>&, const T&) { return true; }
template <typename T, bool _is_max> inline
bool operator< (const extremal<T,_is_max>&, const T&) { return !_is_max; }
template <typename T, bool _is_max> inline
bool operator<= (const extremal<T,_is_max>&, const T&) { return !_is_max; }
template <typename T, bool _is_max> inline
bool operator> (const extremal<T,_is_max>&, const T&) { return _is_max; }
template <typename T, bool _is_max> inline
bool operator>= (const extremal<T,_is_max>&, const T&) { return _is_max; }

template <typename T, bool _is_max1, bool _is_max2> inline
bool operator== (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1==_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> inline
bool operator!= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1!=_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> inline
bool operator< (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1<_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> inline
bool operator<= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1<=_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> inline
bool operator> (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1>_is_max2; }
template <typename T, bool _is_max1, bool _is_max2> inline
bool operator>= (const extremal<T,_is_max1>&, const extremal<T,_is_max2>&) { return _is_max1>=_is_max2; }

// for hash_set and similar
struct is_unordered_set;

template <typename T>
struct hash_func<T, is_container> {
protected:
   hash_func<typename T::value_type> hash_elem;

public:
   size_t operator() (const T& x) const
   {
      size_t h=0;
      int i=1;
      for (typename Entire<T>::const_iterator it=entire(x); !it.at_end(); ++it, ++i) {
         h += hash_elem(*it) * i;
      }
      return h;
   }
};

template <typename T>
struct hash_composite : hash_composite<typename list_tail<T>::type> {
   typedef hash_composite<typename list_tail<T>::type> super;
   static const int magic_number=137;

   typedef typename list_head<T>::type element_type;
   typedef typename deref<element_type>::type value_type;
   super& operator<< (typename attrib<element_type>::plus_const_ref x)
   {
      if (super::magic_number) this->value *= super::magic_number;
      this->value += hash_elem(x);
      return *this;
   }

   hash_func<value_type> hash_elem;
};

template <>
struct hash_composite<void> {
   static const int magic_number=0;
   size_t value;

   hash_composite() : value(0) {}
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
   using std::isinf;
   using pm::isinf;
   using std::isfinite;
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
   }
}

#endif // POLYMAKE_INTERNAL_COMPARATORS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
