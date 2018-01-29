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

#ifndef POLYMAKE_INTERNAL_SPARSE_H
#define POLYMAKE_INTERNAL_SPARSE_H

#include "polymake/Series.h"
#include "polymake/internal/comparators_ops.h"
#include "polymake/SelectedSubset.h"
#include "polymake/internal/iterator_zipper.h"

namespace pm {

template <typename> class GenericInput;
template <typename> class GenericOutput;
struct SkewSymmetric;

template <typename Container>
class construct_sparse_compatible
   : public redirected_container< construct_sparse_compatible<Container>,
                                  mlist< HiddenTag< Container >,
                                         ExpectedFeaturesTag< cons<end_sensitive, indexed> > > > {
   typedef redirected_container<construct_sparse_compatible> base_t;
public:
   int dim() const { return this->size(); }

   void erase(const typename base_t::iterator& where)
   {
      operations::clear<typename base_t::value_type> zero;
      zero(*where);
   }

   // Must be defined, although never called
   void insert(const typename base_t::iterator&, int, const typename base_t::value_type&) {}
};

template <typename Container>
struct default_enforce_feature<Container, sparse_compatible> {
   typedef construct_sparse_compatible<Container> container;
};

template <typename Container>
struct redirect_object_traits< construct_sparse_compatible<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct check_container_feature<construct_sparse_compatible<Container>, sparse_compatible> : std::true_type {};

struct pure_sparse_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs : public unary_predicate_selector_constructor::defs<Iterator,Predicate,ExpectedFeatures> {
      typedef typename mix_features<typename unary_predicate_selector_constructor::template
                                       defs<Iterator,Predicate,ExpectedFeatures>::needed_features,
                                    sparse_compatible>::type
         needed_features;
   };
};

template <typename Container, int TKind=object_classifier::what_is<Container>::value>
class construct_pure_sparse
   : public modified_container_impl< construct_pure_sparse<Container>,
                                     mlist< HiddenTag< Container >,
                                            OperationTag< BuildUnary<operations::non_zero> >,
                                            IteratorConstructorTag< pure_sparse_constructor > > > {
public:
   int dim() const { return this->hidden().size(); }
};

template <typename Container>
class construct_pure_sparse<Container, object_classifier::is_constant>
   : public construct_sparse_compatible<Container> {
   typedef construct_sparse_compatible<Container> _super;
public:
   typename _super::iterator begin() const
   {
      return this->hidden().front() ? _super::begin() : _super::end();
   }
   int size() const
   {
      return this->hidden().front() ? this->dim() : 0;
   }
   bool empty() const
   {
      return !this->hidden().front();
   }
};

template <typename Container>
struct default_enforce_feature<Container, pure_sparse> {
   typedef construct_pure_sparse<Container> container;
};

template <typename Container>
struct redirect_object_traits< construct_pure_sparse<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container, int kind>
struct check_container_feature<construct_pure_sparse<Container,kind>, pure_sparse> : std::true_type {};

template <typename Iterator, typename Target, typename is_enabled=void>
struct construct_sparse_iterator {};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, indexed>::value &&
                                                          check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef Iterator iterator;

   Iterator&& operator() (Iterator&& src, int) const { return std::forward<Iterator>(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, pair<int, Target> >::value &&
                                                          std::is_convertible<typename iterator_traits<Iterator>::value_type::second_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef Iterator iterator;

   Iterator&& operator() (Iterator&& src, int) const { return std::forward<Iterator>(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, indexed>::value &&
                                                          check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          !std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef conv<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type, Target> converter;
   typedef unary_transform_iterator<src_iterator, converter> iterator;

   iterator operator() (const Iterator& src, int) const { return iterator(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, pair<int, Target> >::value &&
                                                          !std::is_convertible<typename iterator_traits<Iterator>::value_type::second_type, Target>::value &&
                                                          explicitly_convertible_to<typename iterator_traits<Iterator>::value_type::second_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef unary_transform_iterator<src_iterator, conv<typename iterator_traits<Iterator>::value_type, pair<int, Target> > > iterator;

   iterator operator() (const Iterator& src, int) const { return iterator(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(!check_iterator_feature<Iterator, indexed>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef ensure_features<sequence, sparse_compatible>::const_iterator indexer;
   typedef iterator_pair<src_iterator, indexer, mlist<FeaturesViaSecondTag<indexed>>> it_pair;
   typedef unary_predicate_selector<it_pair, BuildUnary<operations::non_zero>> iterator;

   iterator operator() (const Iterator& src, int dim) const
   {
      return it_pair(src, ensure(sequence(0, dim), (sparse_compatible*)0).begin());
   }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(!check_iterator_feature<Iterator, indexed>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          !std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef ensure_features<sequence, sparse_compatible>::const_iterator indexer;
   typedef iterator_pair<src_iterator, indexer, mlist<FeaturesViaSecondTag<indexed>>> it_pair;
   typedef unary_predicate_selector<it_pair, BuildUnary<operations::non_zero>> filter;
   typedef conv<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type, Target> converter;
   typedef unary_transform_iterator<filter, converter> iterator;

   iterator operator() (const Iterator& src, int dim) const
   {
      return filter(it_pair(src, ensure(sequence(0,dim), (sparse_compatible*)0).begin()));
   }
};

template <typename LeftRef, typename RightRef>
struct implicit_zero {
   typedef LeftRef first_argument_type;
   typedef int second_argument_type;
   typedef LeftRef result_type;

   result_type operator() (LeftRef l, second_argument_type) const { return l; }
   template <typename Iterator2>
   result_type operator() (operations::partial_left, LeftRef l, const Iterator2&) const { return l; }
   template <typename Iterator1>
   result_type operator() (operations::partial_right, const Iterator1&, second_argument_type) const
   {
      return zero_value<typename deref<LeftRef>::type>();
   }
};

template <typename Iterator> inline
typename attrib<typename iterator_traits<Iterator>::reference>::plus_const
deref_sparse_iterator(const Iterator& it)
{
   return it.at_end() ? zero_value<typename iterator_traits<Iterator>::value_type>() : *it;
}

template <typename Container>
struct dense_helper {
   typedef mlist< Container1Tag< Container >,
                  Container2Tag< sequence >,
                  IteratorCouplerTag< zipping_coupler< operations::cmp, set_union_zipper, true, false> >,
                  HiddenTag< Container > > params;
};

template <typename Container>
class construct_dense_pair
   : public container_pair_impl< construct_dense_pair<Container>, typename dense_helper<Container>::params> {
public:
   const Container& get_container1() const { return this->hidden(); }
   sequence get_container2() const { return sequence(0, this->size()); }
   int size() const { return this->hidden().dim(); }
};

template <typename Container>
class construct_dense
   : public modified_container_pair_impl< construct_dense<Container>,
                                          typename mlist_concat< OperationTag< pair< BuildBinary<implicit_zero>,
                                                                                     operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                                                 typename dense_helper<Container>::params >::type > {
public:
   const Container& get_container1() const { return this->hidden(); }
   sequence get_container2() const { return sequence(0, this->size()); }
   int size() const { return this->hidden().dim(); }
};

template <typename Container>
struct default_enforce_feature<Container, dense> {
   typedef construct_dense<Container> container;
};

template <typename Container>
struct redirect_object_traits< construct_dense<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct redirect_object_traits< construct_dense_pair<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static const bool is_temporary=false;
};

template <typename Container>
struct check_container_feature<construct_dense<Container>, dense> : std::true_type {};

template <typename Controller>
struct sparse_coupler {
   typedef operations::cmp Comparator;
   template <typename Iterator1, typename Iterator2, typename ExpectedFeatures>
   struct defs {
      typedef iterator_zipper<Iterator1, Iterator2, Comparator, Controller, true, true> iterator;
      typedef typename mix_features<ExpectedFeatures, sparse_compatible>::type needed_features1;
      typedef needed_features1 needed_features2;
   };
};

template <typename Controller>
struct reverse_coupler< sparse_coupler<Controller> > {
   typedef sparse_coupler< reverse_zipper<Controller> > type;
};

template <typename Base, typename E=typename Base::value_type, typename Params=void>
class sparse_elem_proxy : public Base {
protected:
   static const bool is_skew=list_contains<Params,SkewSymmetric>::value;

   typename std::conditional<is_skew, operations::neg<const E&>, nothing>::type op;

   bool inversed_impl(std::false_type) const { return false; }
   bool inversed_impl(std::true_type) const { return this->i > this->vec->get_line_index(); }
   bool inversed() const { return inversed_impl(bool_constant<is_skew>()); }

   void store(const E& x, bool, std::false_type) { this->insert(x); }
   void store(const E& x, bool do_inverse, std::true_type) { if (do_inverse) this->insert(op(x)); else this->insert(x); }

   const E& get_impl(std::false_type) const { return Base::get(); }
   E get_impl(std::true_type) const { return inversed(std::true_type()) ? op(Base::get()) : Base::get(); }

public:
   typedef Params parameters;

   sparse_elem_proxy(const Base& base_arg) : Base(base_arg) {}
   typedef typename std::conditional<is_skew, const E, const E&>::type const_reference;

   const_reference get() const { return get_impl(bool_constant<is_skew>()); }

   operator const_reference () const { return get(); }

   template <typename T,
             typename=typename std::enable_if<std::is_constructible<T, E>::value && !is_derived_from<E, T>::value>::type>
   explicit operator T () const
   {
      return static_cast<T>(get());
   }

   sparse_elem_proxy& operator= (const sparse_elem_proxy& p2)
   {
      if (p2.exists())
         store(p2.get_impl(std::false_type()), this->inversed() != p2.inversed(), bool_constant<is_skew>());
      else
         this->erase();
      return *this;
   }

   template <typename E2>
   typename std::enable_if<std::is_convertible<E2, E>::value, sparse_elem_proxy>::type&
   operator= (const E2& x)
   {
      if (!is_zero(x))
         store(x, this->inversed(), bool_constant<is_skew>());
      else
         this->erase();
      return *this;
   }

   template <typename E2>
   typename std::enable_if<explicitly_convertible_to<E2, E>::value && !std::is_convertible<E2, E>::value, sparse_elem_proxy>::type&
   operator= (const E2& x)
   {
      if (!is_zero(x))
         store(conv<E2, E>()(x), this->inversed(), bool_constant<is_skew>());
      else
         this->erase();
      return *this;
   }

   sparse_elem_proxy& negate()
   {
      if (this->exists()) pm::negate(*this->find());
      return *this;
   }

   sparse_elem_proxy& operator++ ()
   {
      typename Base::iterator_type where=this->find();
      if (this->inversed()) {
         if (is_zero(--(*where))) this->erase(where);
      } else {
         if (is_zero(++(*where))) this->erase(where);
      }
      return *this;
   }

   sparse_elem_proxy& operator-- ()
   {
      typename Base::iterator_type where=this->find();
      if (this->inversed()) {
         if (is_zero(++(*where))) this->erase(where);
      } else {
         if (is_zero(--(*where))) this->erase(where);
      }
      return *this;
   }

   const E operator++ (int)
   {
      typename Base::iterator_type where=this->find();
      E v=*where;
      if (this->inversed()) {
         if (is_zero(--(*where))) this->erase(where);
      } else {
         if (is_zero(++(*where))) this->erase(where);
      }
      return v;
   }

   const E operator-- (int)
   {
      typename Base::iterator_type where=this->find();
      E v=*where;
      if (this->inversed()) {
         if (is_zero(++(*where))) this->erase(where);
      } else {
         if (is_zero(--(*where))) this->erase(where);
      }
      return v;
   }

   template <typename E2>
   sparse_elem_proxy& operator+= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (this->inversed()) {
         if (is_zero(*where -= x)) this->erase(where);
      } else {
         if (is_zero(*where += x)) this->erase(where);
      }
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator-= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (this->inversed()) {
         if (is_zero(*where += x)) this->erase(where);
      } else {
         if (is_zero(*where -= x)) this->erase(where);
      }
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator*= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (is_zero(*where *= x)) this->erase(where);
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator/= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (is_zero(*where /= x)) this->erase(where);
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator%= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (is_zero(*where %= x)) this->erase(where);
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator<<= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (is_zero(*where <<= x)) this->erase(where);
      return *this;
   }

   template <typename E2>
   sparse_elem_proxy& operator>>= (const E2& x)
   {
      typename Base::iterator_type where=this->find();
      if (is_zero(*where >>= x)) this->erase(where);
      return *this;
   }

   template <typename Traits> friend
   std::basic_ostream<char, Traits>& operator<< (std::basic_ostream<char, Traits>& os, const sparse_elem_proxy& me)
   {
      return os << me.get();
   }

   template <typename Traits> friend
   std::basic_istream<char, Traits>& operator>> (std::basic_istream<char, Traits>& is, sparse_elem_proxy& me)
   {
      E x;
      is >> x;
      me=x;
      return is;
   }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& is, sparse_elem_proxy& me)
   {
      E x;
      is.top() >> x;
      me=x;
      return is.top();
   }
};

template <typename Base>
class sparse_elem_proxy<Base, bool, void> : public Base {
public:
   typedef void parameters;

   sparse_elem_proxy(const Base& base_arg) : Base(base_arg) {}

   using Base::get;

   operator bool () const { return this->get(); }

   sparse_elem_proxy& operator= (const sparse_elem_proxy& p2)
   {
      return *this=static_cast<bool>(p2);
   }

   sparse_elem_proxy& operator= (bool x)
   {
      if (x)
         this->insert();
      else
         this->erase();
      return *this;
   }

   sparse_elem_proxy& operator&= (bool x)
   {
      if (!x) this->erase();
      return *this;
   }

   sparse_elem_proxy& operator|= (bool x)
   {
      if (x) this->insert();
      return *this;
   }

   sparse_elem_proxy& operator^= (bool x)
   {
      if (x) this->toggle();
      return *this;
   }

   template <typename Traits> friend
   std::basic_ostream<char, Traits>& operator<< (std::basic_ostream<char, Traits>& os, const sparse_elem_proxy& me)
   {
      return os << me.get();
   }

   template <typename Traits> friend
   std::basic_istream<char, Traits>& operator>> (std::basic_istream<char, Traits>& is, sparse_elem_proxy& me)
   {
      bool x;
      is >> x;
      me=x;
      return is;
   }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& is, sparse_elem_proxy& me)
   {
      bool x;
      is.top() >> x;
      me=x;
      return is.top();
   }
};

template <typename Vector, typename Iterator=typename Vector::iterator>
class sparse_proxy_base {
protected:
   typedef Vector container_type;
   typedef Iterator iterator_type;
public:
   typedef typename iterator_traits<iterator_type>::value_type value_type;
protected:
   Vector* vec;
   int i;

   const value_type& get() const
   {
      return deref_sparse_iterator(const_cast<const Vector*>(vec)->find(i));
   }

   iterator_type find() { return vec->insert(i); }

   bool exists() const { return vec->exists(i); }

   void insert(const value_type& x) { vec->insert(i,x); }

   void erase() { vec->erase(i); }

   void erase(const iterator_type& it) { vec->erase(it); }
public:
   sparse_proxy_base(Vector& vec_arg, int i_arg)
      : vec(&vec_arg), i(i_arg) {}
};

template <typename Vector, typename Iterator>
class sparse_proxy_it_base : public sparse_proxy_base<Vector,Iterator> {
   typedef sparse_proxy_base<Vector,Iterator> super;
protected:
   typedef Iterator iterator_type;
   mutable iterator_type where;

   iterator_type find()
   {
      return where=super::find();
   }
public:
   bool exists() const
   {
      return !where.at_end() && where.index()==this->i;
   }
protected:
   const typename super::value_type& get() const
   {
      if (exists()) return *where;
      return zero_value<typename super::value_type>();
   }

   void insert(const typename super::value_type& x)
   {
      if (exists())
         *where=x;
      else
         where=this->vec->insert(where,this->i,x);
   }

   void erase()
   {
      if (exists()) super::erase(where++);
   }

   void erase(const iterator_type& it)
   {
      where=it; ++where;
      super::erase(it);
   }
public:
   sparse_proxy_it_base(Vector& vec_arg, const iterator_type& it_arg, int i_arg)
      : super(vec_arg,i_arg), where(it_arg) {}
};

template <typename Base, typename E, typename Params>
struct object_traits< sparse_elem_proxy<Base, E, Params> >
   : object_traits<E> {
   typedef E proxy_for;
   static const bool is_temporary=true, is_persistent=false;
};

template <typename Base, typename E, typename Params> inline
bool is_zero(const sparse_elem_proxy<Base, E, Params>& x)
{
   return !x.exists();
}

template <typename Base, typename E, typename Params> inline
bool is_one(const sparse_elem_proxy<Base, E, Params>& x)
{
   return x.exists() && is_one(x.get());
}

namespace operations {
   template <typename ContainerRef>
   struct front_index {
      typedef ContainerRef argument_type;
      typedef int result_type;
      result_type operator() (typename function_argument<ContainerRef>::const_type l) const { return l.begin().index(); }
   };

   template <typename ContainerRef>
   struct back_index {
      typedef ContainerRef argument_type;
      typedef int result_type;
      result_type operator() (typename function_argument<ContainerRef>::const_type l) const { return l.rbegin().index(); }
   };
}

template <typename Container, typename Iterator>
Iterator assign_sparse(Container& c, Iterator src)
{
   typename Container::iterator dst=c.begin();
   int state=(dst.at_end() ? 0 : zipper_first) + (src.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const int idiff=dst.index()-src.index();
      if (idiff<0) {
         c.erase(dst++);
         if (dst.at_end()) state -= zipper_first;
      } else if (idiff>0) {
         c.insert(dst, src.index(), *src);
         ++src;
         if (src.at_end()) state -= zipper_second;
      } else {
         *dst=*src;
         ++dst;
         if (dst.at_end()) state -= zipper_first;
         ++src;
         if (src.at_end()) state -= zipper_second;
      }
   }
   if (state & zipper_first) {
      do {
         c.erase(dst++);
      } while (!dst.at_end());
   } else if (state) {
      do {
         c.insert(dst, src.index(), *src);  ++src;
      } while (!src.at_end());
   }
   return src;
}

template <typename Container, typename Iterator>
void fill_sparse(Container& c, Iterator src)
{
   typename Container::iterator dst=c.begin();
   const int d=c.dim();
   int i;
   if (!dst.at_end()) {
      for (; (i=src.index())<d; ++src)
         if (i<dst.index()) {
            c.insert(dst,i,*src);
         } else {
            *dst=*src;  ++dst;
            if (dst.at_end()) { ++src; break; }
         }
   }
   for (; (i=src.index())<d; ++src)
      c.insert(dst,i,*src);
}

template <typename Container, typename Iterator2, typename Operation>
void perform_assign_sparse(Container& c, Iterator2 src2, const Operation& op_arg)
{
   typedef binary_op_builder<Operation, typename Container::const_iterator, Iterator2> opb;
   const typename opb::operation& op=opb::create(op_arg);
   typename Container::iterator dst=c.begin();
   int state=(dst.at_end() ? 0 : zipper_first) + (src2.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const int idiff=dst.index()-src2.index();
      if (idiff<0) {
         ++dst;
         if (dst.at_end()) state -= zipper_first;
      } else if (idiff>0) {
         c.insert(dst, src2.index(), op(operations::partial_right(), dst, *src2));
         ++src2;
         if (src2.at_end()) state -= zipper_second;
      } else {
         op.assign(*dst, *src2);
         if (!is_zero(*dst))
            ++dst;
         else
            c.erase(dst++);

         if (dst.at_end()) state -= zipper_first;
         ++src2;
         if (src2.at_end()) state -= zipper_second;
      }
   }
   if (state & zipper_second) {
      do {
         c.insert(dst, src2.index(), op(operations::partial_right(), dst, *src2));  ++src2;
      } while (!src2.at_end());
   }
}

template <typename Container1, typename Container2>
void swap_sparse(Container1& c1, Container2& c2)
{
   typename Container1::iterator e1=c1.begin();
   typename Container2::iterator e2=c2.begin();
   int state=(e1.at_end() ? 0 : zipper_first) + (e2.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const int idiff=e1.index()-e2.index();
      if (idiff<0) {
         c2.insert(e2, e1.index(), *e1);
         c1.erase(e1++);
         if (e1.at_end()) state -= zipper_first;
      } else if (idiff>0) {
         c1.insert(e1, e2.index(), *e2);
         c2.erase(e2++);
         if (e2.at_end()) state -= zipper_second;
      } else {
         std::swap(*e1,*e2);
         ++e1;
         if (e1.at_end()) state -= zipper_first;
         ++e2;
         if (e2.at_end()) state -= zipper_second;
      }
   }
   if (state & zipper_first) {
      do {
         c2.insert(e2, e1.index(), *e1);
         c1.erase(e1++);
      } while (!e1.at_end());
   } else if (state) {
      do { 
         c1.insert(e1, e2.index(), *e2);
         c2.erase(e2++);
      } while (!e2.at_end());
   }
}

} // end namespace pm

#endif // POLYMAKE_INTERNAL_SPARSE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
