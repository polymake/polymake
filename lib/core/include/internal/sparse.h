/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_INTERNAL_SPARSE_H
#define POLYMAKE_INTERNAL_SPARSE_H

#include "polymake/internal/iterator_filters.h"
#include "polymake/internal/iterator_zipper.h"
#include "polymake/internal/SeriesRaw.h"

namespace pm {

template <typename> class GenericInput;
template <typename> class GenericOutput;
struct SkewSymmetric;

template <typename Container>
class construct_sparse_compatible
   : public redirected_container< construct_sparse_compatible<Container>,
                                  mlist< HiddenTag< Container >,
                                         ExpectedFeaturesTag< mlist<end_sensitive, indexed> > > > {
   typedef redirected_container<construct_sparse_compatible> base_t;
public:
   Int dim() const { return this->size(); }

   void erase(const typename base_t::iterator& where)
   {
      operations::clear<typename base_t::value_type> zero;
      zero(*where);
   }

   // Must be defined, although never called
   void insert(const typename base_t::iterator&, Int, const typename base_t::value_type&) {}
};

template <typename Container>
struct default_enforce_feature<Container, sparse_compatible> {
   typedef construct_sparse_compatible<Container> container;
};

template <typename Container>
struct redirect_object_traits< construct_sparse_compatible<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static constexpr bool is_temporary=false;
};

template <typename Container>
struct check_container_feature<construct_sparse_compatible<Container>, sparse_compatible> : std::true_type {};

struct pure_sparse_constructor {
   template <typename Iterator, typename Predicate, typename ExpectedFeatures>
   struct defs : public unary_predicate_selector_constructor::defs<Iterator,Predicate,ExpectedFeatures> {
      using needed_features = typename mix_features<typename unary_predicate_selector_constructor::template
                                                    defs<Iterator,Predicate,ExpectedFeatures>::needed_features,
                                                    sparse_compatible>::type;
   };
};

template <typename Container, int TKind=object_classifier::what_is<Container>::value>
class construct_pure_sparse
   : public modified_container_impl< construct_pure_sparse<Container>,
                                     mlist< HiddenTag< Container >,
                                            OperationTag< BuildUnary<operations::non_zero> >,
                                            IteratorConstructorTag< pure_sparse_constructor > > > {
public:
   Int dim() const { return this->hidden().size(); }
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
   Int size() const
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
   static constexpr bool is_temporary=false;
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

   Iterator&& operator() (Iterator&& src, Int) const { return std::forward<Iterator>(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, pair<Int, Target> >::value &&
                                                          std::is_convertible<typename iterator_traits<Iterator>::value_type::second_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef Iterator iterator;

   Iterator&& operator() (Iterator&& src, Int) const { return std::forward<Iterator>(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, indexed>::value &&
                                                          check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          is_only_explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef conv<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type, Target> converter;
   typedef unary_transform_iterator<src_iterator, converter> iterator;

   iterator operator() (const Iterator& src, Int) const { return iterator(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(check_iterator_feature<Iterator, end_sensitive>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, pair<Int, Target> >::value &&
                                                          is_only_explicitly_convertible_to<typename iterator_traits<Iterator>::value_type::second_type, Target>::value)>::type>
{
   static const bool enabled=true;
   typedef pure_type_t<Iterator> src_iterator;
   typedef unary_transform_iterator<src_iterator, conv<typename iterator_traits<Iterator>::value_type, pair<Int, Target> > > iterator;

   iterator operator() (const Iterator& src, Int) const { return iterator(src); }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(!check_iterator_feature<Iterator, indexed>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          std::is_convertible<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static constexpr bool enabled=true;
   using src_iterator = pure_type_t<Iterator>;
   using indexer = ensure_features<sequence_raw, sparse_compatible>::const_iterator;
   using it_pair = iterator_pair<src_iterator, indexer, mlist<FeaturesViaSecondTag<indexed>>>;
   using iterator = unary_predicate_selector<it_pair, BuildUnary<operations::non_zero>>;

   iterator operator() (const Iterator& src, Int dim) const
   {
      return it_pair(src, ensure(sequence_raw(0, dim), sparse_compatible()).begin());
   }
};

template <typename Iterator, typename Target>
struct construct_sparse_iterator<Iterator, Target,
                                 typename std::enable_if<(!check_iterator_feature<Iterator, indexed>::value &&
                                                          isomorphic_types<typename iterator_traits<Iterator>::value_type, Target>::value &&
                                                          is_only_explicitly_convertible_to<typename iterator_traits<Iterator>::value_type, Target>::value)>::type>
{
   static constexpr bool enabled=true;
   using src_iterator = pure_type_t<Iterator>;
   using indexer = ensure_features<sequence_raw, sparse_compatible>::const_iterator;
   using it_pair = iterator_pair<src_iterator, indexer, mlist<FeaturesViaSecondTag<indexed>>>;
   using filter = unary_predicate_selector<it_pair, BuildUnary<operations::non_zero>>;
   using converter = conv<typename object_traits<typename iterator_traits<Iterator>::value_type>::persistent_type, Target>;
   using iterator = unary_transform_iterator<filter, converter>;

   iterator operator() (const Iterator& src, Int dim) const
   {
      return filter(it_pair(src, ensure(sequence_raw(0,dim), sparse_compatible()).begin()));
   }
};

template <typename LeftRef, typename RightRef>
struct implicit_zero {
   typedef LeftRef first_argument_type;
   typedef Int second_argument_type;
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

template <typename Iterator>
typename attrib<typename iterator_traits<Iterator>::reference>::plus_const
deref_sparse_iterator(const Iterator& it)
{
   return it.at_end() ? zero_value<typename iterator_traits<Iterator>::value_type>() : *it;
}

template <typename Container>
struct dense_helper {
   typedef mlist< Container1Tag< Container >,
                  Container2Tag< sequence_raw >,
                  IteratorCouplerTag< zipping_coupler< operations::cmp, set_union_zipper, true, false> >,
                  HiddenTag< Container > > params;
};

template <typename Container>
class construct_dense_pair
   : public container_pair_impl< construct_dense_pair<Container>, typename dense_helper<Container>::params> {
public:
   const Container& get_container1() const { return this->hidden(); }
   sequence_raw get_container2() const { return sequence_raw(0, this->size()); }
   Int size() const { return this->hidden().dim(); }
};

template <typename Container>
class construct_dense
   : public modified_container_pair_impl< construct_dense<Container>,
                                          typename mlist_concat< OperationTag< pair< BuildBinary<implicit_zero>,
                                                                                     operations::apply2< BuildUnaryIt<operations::dereference> > > >,
                                                                 typename dense_helper<Container>::params >::type > {
public:
   const Container& get_container1() const { return this->hidden(); }
   sequence_raw get_container2() const { return sequence_raw(0, this->size()); }
   Int size() const { return this->hidden().dim(); }
};

template <typename Container>
struct default_enforce_feature<Container, dense> {
   typedef construct_dense<Container> container;
};

template <typename Container>
struct redirect_object_traits< construct_dense<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static constexpr bool is_temporary=false;
};

template <typename Container>
struct redirect_object_traits< construct_dense_pair<Container> >
   : object_traits<Container> {
   typedef Container masquerade_for;
   static constexpr bool is_temporary=false;
};

template <typename Container>
struct check_container_feature<construct_dense<Container>, dense> : std::true_type {};

template <typename Controller>
struct sparse_coupler {
   typedef operations::cmp Comparator;
   template <typename Iterator1, typename Iterator2, typename... ExpectedFeatures>
   struct defs {
      using expected_features = typename mlist_wrap<ExpectedFeatures...>::type;
      using iterator = iterator_zipper<Iterator1, Iterator2, Comparator, Controller, true, true>;
      using needed_features1 = typename mix_features<expected_features, sparse_compatible>::type;
      using needed_features2 = needed_features1;
   };
};

template <typename Controller>
struct reverse_coupler< sparse_coupler<Controller> > {
   typedef sparse_coupler< reverse_zipper<Controller> > type;
};

template <typename Base, typename E=typename Base::value_type, typename... Params>
class sparse_elem_proxy : public Base {
protected:
   void store(const E& x, bool, std::false_type) { this->insert(x); }

   template <typename T>
   static
   std::enable_if_t<!is_instance_of<T, pm::sparse_elem_proxy>::value, bool>
   op_is_non_zero(const T& x)
   {
      return !is_zero(x);
   }

   template <typename Base2, typename E2, typename... Params2>
   static
   bool op_is_non_zero(const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      return x.exists();
   }

   template <typename T, bool allow_explicit_conversion>
   static
   std::enable_if_t<std::is_convertible<pure_type_t<T>, E>::value &&
                    !is_instance_of<pure_type_t<T>, pm::sparse_elem_proxy>::value, T&&>
   op_value(T&& x, bool_constant<allow_explicit_conversion>)
   {
      return std::forward<T>(x);
   }

   template <typename T>
   static
   std::enable_if_t<is_only_explicitly_convertible_to<pure_type_t<T>, E>::value &&
                    !is_instance_of<pure_type_t<T>, pm::sparse_elem_proxy>::value, E>
   op_value(T&& x, std::true_type)
   {
      return conv<pure_type_t<T>, E>()(std::forward<T>(x));
   }

   template <typename Base2, typename E2, typename... Params2, bool allow_explicit_conversion>
   static
   std::enable_if_t<std::is_convertible<E2, E>::value, const E2&>
   op_value(const sparse_elem_proxy<Base2, E2, Params2...>& x, bool_constant<allow_explicit_conversion>)
   {
      return x.get();
   }

   template <typename Base2, typename E2, typename... Params2>
   static
   std::enable_if_t<is_only_explicitly_convertible_to<E2, E>::value, E>
   op_value(const sparse_elem_proxy<Base2, E2, Params2...>& x, std::true_type)
   {
      return conv<E2, E>()(x.get());
   }

   template <typename T, typename=void>
   struct can_assign : std::false_type {};

   template <typename T>
   struct can_assign<T, accept_valid_type<decltype(op_value(std::declval<const pure_type_t<T>&>(), std::true_type()))>>
      : std::true_type {};

   template <typename T>
   void assign(T&& x)
   {
      if (op_is_non_zero(x))
         this->insert(op_value(std::forward<T>(x), std::true_type()));
      else
         this->erase();
   }

   template <typename Operation, typename T>
   struct defined_operation
      : cleanOperations::can<Operation, E, T> {};

   template <typename Operation, typename Base2, typename E2, typename... Params2>
   struct defined_operation<Operation, sparse_elem_proxy<Base2, E2, Params2...>>
      : cleanOperations::can<Operation, E, E2> {};

public:
   sparse_elem_proxy(Base&& base_arg) : Base(std::move(base_arg)) {}

   template <typename OtherBase>
   static sparse_elem_proxy<OtherBase, E, Params...> construct(OtherBase&& base_arg)
   {
      return sparse_elem_proxy<OtherBase, E, Params...>(std::forward<OtherBase>(base_arg));
   }

   using const_reference = const E&;

   operator const_reference () const { return this->get(); }

   template <typename T,
             std::enable_if_t<std::is_constructible<T, E>::value && !is_derived_from<E, T>::value>>
   explicit operator T () const
   {
      return static_cast<T>(this->get());
   }

   sparse_elem_proxy& operator= (const sparse_elem_proxy& x)
   {
      if (x.exists())
         this->insert(x.get());
      else
         this->erase();
      return *this;
   }

   template <typename T>
   std::enable_if_t<can_assign<T>::value, sparse_elem_proxy&>
   operator= (T&& x)
   {
      assign(std::forward<T>(x));
      return *this;
   }

   sparse_elem_proxy& negate()
   {
      if (this->exists())
         cleanOperations::assign<cleanOperations::neg>()(*this->find());
      return *this;
   }

   sparse_elem_proxy& operator++ ()
   {
      auto where=this->find();
      if (is_zero(++(*where))) this->erase(where);
      return *this;
   }

   sparse_elem_proxy& operator-- ()
   {
      auto where=this->find();
      if (is_zero(--(*where))) this->erase(where);
      return *this;
   }

   E operator++ (int)
   {
      if (this->exists()) {
         auto where=this->find();
         const E v=*where;
         if (is_zero(++(*where))) this->erase(where);
         return v;
      }
      this->insert(one_value<E>());
      return zero_value<E>();
   }

   E operator-- (int)
   {
      if (this->exists()) {
         auto where=this->find();
         const E v=*where;
         if (is_zero(--(*where))) this->erase(where);
         return v;
      }
      this->insert(-one_value<E>());
      return zero_value<E>();
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::add, T>::value, sparse_elem_proxy&>
   operator+= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where += op_value(x, std::false_type()))) this->erase(where);
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::sub, T>::value, sparse_elem_proxy&>
   operator-= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where -= op_value(x, std::false_type()))) this->erase(where);
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::mul, T>::value, sparse_elem_proxy&>
   operator*= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where *= op_value(x, std::false_type()))) this->erase(where);
      } else {
         this->erase();
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::div, T>::value, sparse_elem_proxy&>
   operator/= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where /= op_value(x, std::false_type()))) this->erase(where);
      } else {
         throw std::domain_error("zero division");
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::mod, T>::value, sparse_elem_proxy&>
   operator%= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where %= op_value(x, std::false_type()))) this->erase(where);
      } else {
         throw std::domain_error("zero division");
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::lshift, T>::value, sparse_elem_proxy&>
   operator<<= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where <<= op_value(x, std::false_type()))) this->erase(where);
      }
      return *this;
   }

   template <typename T>
   std::enable_if_t<defined_operation<cleanOperations::rshift, T>::value, sparse_elem_proxy&>
   operator>>= (const T& x)
   {
      if (op_is_non_zero(x)) {
         auto where=this->find();
         if (is_zero(*where >>= op_value(x, std::false_type()))) this->erase(where);
      }
      return *this;
   }

private:
   template <typename E2, template <typename, typename> class Check,
             bool=is_instance_of<E2, pm::sparse_elem_proxy>::value>
   struct is_suitable_for : std::false_type {};

   template <typename E2, template <typename, typename> class Check>
   struct is_suitable_for<E2, Check, false> : Check<E, E2> {};

   template <typename E2, template <typename, typename> class Check>
   using cmp_result = std::enable_if_t<is_suitable_for<E2, Check>::value, bool>;

public:
   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator== (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get()==r : is_zero(r);
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator== (const E2& l, const sparse_elem_proxy& r)
   {
      return r==l;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator== (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return this->exists() ? (r.exists() && this->get()==r.get()) : !r.exists();
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator!= (const sparse_elem_proxy& l, const E2& r)
   {
      return !(l==r);
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator!= (const E2& l, const sparse_elem_proxy& r)
   {
      return !(r==l);
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator!= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(*this==r);
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator< (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() < r : zero_value<E>() < r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator> (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() > r : zero_value<E>() > r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator<= (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() <= r : zero_value<E>() <= r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator>= (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() >= r : zero_value<E>() >= r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator< (const E2& l, const sparse_elem_proxy& r)
   {
      return r > l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator> (const E2& l, const sparse_elem_proxy& r)
   {
      return r < l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator<= (const E2& l, const sparse_elem_proxy& r)
   {
      return r >= l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator>= (const E2& l, const sparse_elem_proxy& r)
   {
      return r <= l;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator< (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      if (this->exists()) {
         return r.exists() ? this->get() < r.get() : this->get() < zero_value<E>();
      } else {
         return r.exists() && zero_value<E2>() < r.get();
      }
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator> (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return r < *this;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator<= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(r < *this);
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator>= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(*this < r);
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

template <typename Base, typename E>
class sparse_elem_proxy<Base, E, SkewSymmetric> : public sparse_elem_proxy<Base, E> {
#if 0
// TODO: resurrect when needed
protected:
   using params = typename mlist_wrap<Params...>::type;
   static const bool is_skew=mlist_contains<params, >::value;

   typename std::conditional<is_skew, operations::neg<const E&>, nothing>::type op;

   bool inversed_impl(std::false_type) const { return false; }
   bool inversed_impl(std::true_type) const { return this->i > this->vec->get_line_index(); }
   bool inversed() const { return inversed_impl(bool_constant<is_skew>()); }

   void store(const E& x, bool, std::false_type) { this->insert(x); }
   void store(const E& x, bool do_inverse, std::true_type) { if (do_inverse) this->insert(op(x)); else this->insert(x); }

   const E& get_impl(std::false_type) const { return Base::get(); }
   E get_impl(std::true_type) const { return inversed(std::true_type()) ? op(Base::get()) : Base::get(); }

public:
   sparse_elem_proxy(Base&& base_arg) : Base(std::move(base_arg)) {}

   template <typename OtherBase>
   static sparse_elem_proxy<OtherBase, E, Params...> construct(OtherBase&& base_arg)
   {
      return sparse_elem_proxy<OtherBase, E, Params...>(std::forward<OtherBase>(base_arg));
   }

   using const_reference = std::conditional_t<is_skew, const E, const E&>;

   const_reference get() const { return get_impl(bool_constant<is_skew>()); }

   operator const_reference () const { return get(); }

   template <typename T,
             std::enable_if_t<std::is_constructible<T, E>::value && !is_derived_from<E, T>::value>>
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
   std::enable_if_t<std::is_convertible<E2, E>::value, sparse_elem_proxy>&
   operator= (const E2& x)
   {
      if (!is_zero(x))
         store(x, this->inversed(), bool_constant<is_skew>());
      else
         this->erase();
      return *this;
   }

   template <typename E2>
   std::enable_if_t<explicitly_convertible_to<E2, E>::value && !std::is_convertible<E2, E>::value, sparse_elem_proxy>&
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
      auto where=this->find();
      if (this->inversed()) {
         if (is_zero(--(*where))) this->erase(where);
      } else {
         if (is_zero(++(*where))) this->erase(where);
      }
      return *this;
   }

   sparse_elem_proxy& operator-- ()
   {
      auto where=this->find();
      if (this->inversed()) {
         if (is_zero(++(*where))) this->erase(where);
      } else {
         if (is_zero(--(*where))) this->erase(where);
      }
      return *this;
   }

   E operator++ (int)
   {
      auto where=this->find();
      E v=*where;
      if (this->inversed()) {
         if (is_zero(--(*where))) this->erase(where);
      } else {
         if (is_zero(++(*where))) this->erase(where);
      }
      return v;
   }

   E operator-- (int)
   {
      auto where=this->find();
      E v=*where;
      if (this->inversed()) {
         if (is_zero(++(*where))) this->erase(where);
      } else {
         if (is_zero(--(*where))) this->erase(where);
      }
      return v;
   }

   template <typename E2, typename=std::enable_if_t<!is_instance_of<E2, pm::sparse_elem_proxy>::value,
                                                    accept_valid_type<decltype(std::declval<E&>() += std::declval<E2>())>>>
   sparse_elem_proxy& operator+= (const E2& x)
   {
      auto where=this->find();
      if (this->inversed()) {
         if (is_zero(*where -= x)) this->erase(where);
      } else {
         if (is_zero(*where += x)) this->erase(where);
      }
      return *this;
   }

   template <typename Base2, typename E2, typename... Params2,
             typename=accept_valid_type<decltype(std::declval<E&>() += std::declval<E2>())>>
   sparse_elem_proxy& operator+= (const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      if (x.exists()) *this += x.get();
      return *this;
   }

   template <typename E2, typename=std::enable_if_t<!is_instance_of<E2, pm::sparse_elem_proxy>::value,
                                                    accept_valid_type<decltype(std::declval<E&>() -= std::declval<E2>())>>>
   sparse_elem_proxy& operator-= (const E2& x)
   {
      auto where=this->find();
      if (this->inversed()) {
         if (is_zero(*where += x)) this->erase(where);
      } else {
         if (is_zero(*where -= x)) this->erase(where);
      }
      return *this;
   }

   template <typename Base2, typename E2, typename... Params2,
             typename=accept_valid_type<decltype(std::declval<E&>() -= std::declval<E2>())>>
   sparse_elem_proxy& operator-= (const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      if (x.exists()) *this -= x.get();
      return *this;
   }

   template <typename E2, typename=std::enable_if_t<!is_instance_of<E2, pm::sparse_elem_proxy>::value,
                                                    accept_valid_type<decltype(std::declval<E&>() *= std::declval<E2>())>>>
   sparse_elem_proxy& operator*= (const E2& x)
   {
      auto where=this->find();
      if (is_zero(*where *= x)) this->erase(where);
      return *this;
   }

   template <typename Base2, typename E2, typename... Params2,
             typename=accept_valid_type<decltype(std::declval<E&>() *= std::declval<E2>())>>
   sparse_elem_proxy& operator*= (const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      if (x.exists())
         *this *= x.get();
      else
         this->erase();
      return *this;
   }

   template <typename E2, typename=std::enable_if_t<!is_instance_of<E2, pm::sparse_elem_proxy>::value,
                                                    accept_valid_type<decltype(std::declval<E&>() /= std::declval<E2>())>>>
   sparse_elem_proxy& operator/= (const E2& x)
   {
      auto where=this->find();
      if (is_zero(*where /= x)) this->erase(where);
      return *this;
   }

   template <typename Base2, typename E2, typename... Params2,
             typename=accept_valid_type<decltype(std::declval<E&>() /= std::declval<E2>())>>
   sparse_elem_proxy& operator/= (const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      if (x.exists())
         *this /= x.get();
      else
         throw std::domain_error("zero division");
   }

   template <typename E2, typename=std::enable_if_t<!is_instance_of<E2, pm::sparse_elem_proxy>::value,
                                                    accept_valid_type<decltype(std::declval<E&>() %= std::declval<E2>())>>>
   sparse_elem_proxy& operator%= (const E2& x)
   {
      auto where=this->find();
      if (is_zero(*where %= x)) this->erase(where);
      return *this;
   }

   template <typename Base2, typename E2, typename... Params2,
             typename=accept_valid_type<decltype(std::declval<E&>() %= std::declval<E2>())>>
   sparse_elem_proxy& operator%= (const sparse_elem_proxy<Base2, E2, Params2...>& x)
   {
      if (x.exists())
         *this %= x.get();
      else
         throw std::domain_error("zero division");
   }

   template <typename E2, typename=accept_valid_type<decltype(std::declval<E&>() <<= std::declval<E2>())>>
   sparse_elem_proxy& operator<<= (const E2& x)
   {
      auto where=this->find();
      if (is_zero(*where <<= x)) this->erase(where);
      return *this;
   }

   template <typename E2, typename=accept_valid_type<decltype(std::declval<E&>() >>= std::declval<E2>())>>
   sparse_elem_proxy& operator>>= (const E2& x)
   {
      auto where=this->find();
      if (is_zero(*where >>= x)) this->erase(where);
      return *this;
   }

private:
   template <typename E2, template <typename, typename> class Check,
             bool=is_instance_of<E2, pm::sparse_elem_proxy>::value>
   struct is_suitable_for : std::false_type {};

   template <typename E2, template <typename, typename> class Check>
   struct is_suitable_for<E2, Check, false> : Check<E, E2> {};

   template <typename E2, template <typename, typename> class Check>
   using cmp_result = std::enable_if_t<is_suitable_for<E2, Check>::value, bool>;

public:
   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator== (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get()==r : is_zero(r);
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator== (const E2& l, const sparse_elem_proxy& r)
   {
      return r==l;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator== (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return this->exists() ? (r.exists() && this->get()==r.get()) : !r.exists();
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator!= (const sparse_elem_proxy& l, const E2& r)
   {
      return !(l==r);
   }

   template <typename E2> friend
   cmp_result<E2, are_comparable>
   operator!= (const E2& l, const sparse_elem_proxy& r)
   {
      return !(r==l);
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_comparable<E, E2>::value, bool>
   operator!= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(*this==r);
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator< (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() < r : zero_value<E>() < r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator> (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() > r : zero_value<E>() > r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator<= (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() <= r : zero_value<E>() <= r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator>= (const sparse_elem_proxy& l, const E2& r)
   {
      return l.exists() ? l.get() >= r : zero_value<E>() >= r;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator< (const E2& l, const sparse_elem_proxy& r)
   {
      return r > l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator> (const E2& l, const sparse_elem_proxy& r)
   {
      return r < l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator<= (const E2& l, const sparse_elem_proxy& r)
   {
      return r >= l;
   }

   template <typename E2> friend
   cmp_result<E2, are_less_greater_comparable>
   operator>= (const E2& l, const sparse_elem_proxy& r)
   {
      return r <= l;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator< (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      if (this->exists()) {
         return r.exists() ? this->get() < r.get() : this->get() < zero_value<E>();
      } else {
         return r.exists() && zero_value<E2>() < r.get();
      }
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator> (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return r < *this;
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator<= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(r < *this);
   }

   template <typename Base2, typename E2, typename... Params2>
   std::enable_if_t<are_less_greater_comparable<E, E2>::value, bool>
   operator>= (const sparse_elem_proxy<Base2, E2, Params2...>& r) const
   {
      return !(*this < r);
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
#endif
};

template <typename Base>
class sparse_elem_proxy<Base, bool> : public Base {
public:
   sparse_elem_proxy(Base&& base_arg) : Base(std::move(base_arg)) {}

   template <typename OtherBase>
   static sparse_elem_proxy<OtherBase, bool> construct(OtherBase&& base_arg)
   {
      return sparse_elem_proxy<OtherBase, bool>(std::forward<OtherBase>(base_arg));
   }

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
   using value_type = typename iterator_traits<iterator_type>::value_type;
protected:
   Vector* vec;
   Int i;

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
   sparse_proxy_base(Vector& vec_arg, Int i_arg)
      : vec(&vec_arg), i(i_arg) {}
};

template <typename Vector, typename Iterator>
class sparse_proxy_it_base : public sparse_proxy_base<Vector, Iterator> {
   using base_t = sparse_proxy_base<Vector, Iterator>;
protected:
   using iterator_type = Iterator;
   mutable iterator_type where;

   iterator_type find()
   {
      return where=base_t::find();
   }
public:
   bool exists() const
   {
      return !where.at_end() && where.index()==this->i;
   }
protected:
   const typename base_t::value_type& get() const
   {
      if (exists()) return *where;
      return zero_value<typename base_t::value_type>();
   }

   void insert(const typename base_t::value_type& x)
   {
      if (exists())
         *where=x;
      else
         where=this->vec->insert(where,this->i,x);
   }

   void erase()
   {
      if (exists()) base_t::erase(where++);
   }

   void erase(const iterator_type& it)
   {
      where=it;  ++where;
      base_t::erase(it);
   }
public:
   sparse_proxy_it_base(Vector& vec_arg, const iterator_type& it_arg, Int i_arg)
      : base_t(vec_arg,i_arg)
      , where(it_arg) {}
};

template <typename Base, typename E, typename... Params>
struct object_traits< sparse_elem_proxy<Base, E, Params...> >
   : object_traits<E> {
   typedef E proxy_for;
   static constexpr bool is_temporary = true, is_persistent = false;
};

template <typename Base, typename E, typename... Params>
bool is_zero(const sparse_elem_proxy<Base, E, Params...>& x)
{
   return !x.exists();
}

template <typename Base, typename E, typename... Params>
bool is_one(const sparse_elem_proxy<Base, E, Params...>& x)
{
   return x.exists() && is_one(x.get());
}

namespace operations {
   template <typename ContainerRef>
   struct front_index {
      typedef ContainerRef argument_type;
      typedef Int result_type;
      result_type operator() (typename function_argument<ContainerRef>::const_type l) const { return l.begin().index(); }
   };

   template <typename ContainerRef>
   struct back_index {
      typedef ContainerRef argument_type;
      typedef Int result_type;
      result_type operator() (typename function_argument<ContainerRef>::const_type l) const { return l.rbegin().index(); }
   };
}

template <typename Container, typename Iterator>
Iterator assign_sparse(Container& c, Iterator src)
{
   auto dst = c.begin();
   int state = (dst.at_end() ? 0 : zipper_first) + (src.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const Int idiff = dst.index() - src.index();
      if (idiff < 0) {
         c.erase(dst++);
         if (dst.at_end())
            state -= zipper_first;
      } else if (idiff > 0) {
         c.insert(dst, src.index(), *src);
         ++src;
         if (src.at_end())
            state -= zipper_second;
      } else {
         *dst = *src;
         ++dst;
         if (dst.at_end())
            state -= zipper_first;
         ++src;
         if (src.at_end())
            state -= zipper_second;
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
   auto dst = c.begin();
   const Int d = c.dim();
   Int i;
   if (!dst.at_end()) {
      for (; (i = src.index()) < d; ++src)
         if (i < dst.index()) {
            c.insert(dst, i, *src);
         } else {
            *dst = *src;  ++dst;
            if (dst.at_end()) { ++src; break; }
         }
   }
   for (; (i = src.index()) < d; ++src)
      c.insert(dst, i, *src);
}

template <typename Container, typename Iterator2, typename Operation>
void perform_assign_sparse(Container& c, Iterator2 src2, const Operation& op_arg)
{
   typedef binary_op_builder<Operation, typename Container::const_iterator, Iterator2> opb;
   const typename opb::operation& op=opb::create(op_arg);
   auto dst = c.begin();
   int state = (dst.at_end() ? 0 : zipper_first) + (src2.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const Int idiff = dst.index() - src2.index();
      if (idiff < 0) {
         ++dst;
         if (dst.at_end())
            state -= zipper_first;
      } else if (idiff > 0) {
         c.insert(dst, src2.index(), op(operations::partial_right(), dst, *src2));
         ++src2;
         if (src2.at_end())
            state -= zipper_second;
      } else {
         op.assign(*dst, *src2);
         if (!is_zero(*dst))
            ++dst;
         else
            c.erase(dst++);

         if (dst.at_end())
            state -= zipper_first;
         ++src2;
         if (src2.at_end())
            state -= zipper_second;
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
   auto e1 = c1.begin();
   auto e2 = c2.begin();
   int state = (e1.at_end() ? 0 : zipper_first) + (e2.at_end() ? 0 : zipper_second);
   while (state >= zipper_both) {
      const Int idiff = e1.index() - e2.index();
      if (idiff < 0) {
         c2.insert(e2, e1.index(), *e1);
         c1.erase(e1++);
         if (e1.at_end())
            state -= zipper_first;
      } else if (idiff > 0) {
         c1.insert(e1, e2.index(), *e2);
         c2.erase(e2++);
         if (e2.at_end())
            state -= zipper_second;
      } else {
         std::swap(*e1,*e2);
         ++e1;
         if (e1.at_end())
            state -= zipper_first;
         ++e2;
         if (e2.at_end())
            state -= zipper_second;
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
