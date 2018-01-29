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

/** @file GenericSet.h
    @brief Implementation of various kinds of pm::GenericSet types

    The class definition for GenericSet is contained in Series.h
 */
/** @defgroup genericSets Generic Sets
 *  Functions and operations for GenericSets
 *  @{
 */
#ifndef POLYMAKE_GENERIC_SET_H
#define POLYMAKE_GENERIC_SET_H

#include "polymake/Series.h"
#include "polymake/GenericIO.h"
#include "polymake/internal/comparators.h"

namespace pm {

#if POLYMAKE_DEBUG
template <typename TSet, typename E, typename Comparator>
void GenericSet<TSet, E, Comparator>::dump() const { cerr << this->top() << endl; }
#endif

/* ------------
 *  Complement
 * ------------ */

/** @ingroup genericSets
 *  @brief %Complement as GenericSet.
 */
template <typename SetTop, typename E=typename SetTop::element_type, typename Comparator=typename SetTop::element_comparator>
class Complement : public operators::base {
protected:
   Complement();
   ~Complement();
public:
   typedef E element_type;
   typedef Comparator element_comparator;
   typedef E value_type;
   typedef typename container_traits<SetTop>::const_reference const_reference;
   typedef const_reference reference;
   typedef forward_iterator_tag container_category;
   typedef GenericSet<SetTop, E, Comparator> generic_type;
   typedef SetTop base_type;

   const Complement& top() const { return *this; }
   const SetTop& base() const { return reinterpret_cast<const SetTop&>(*this); }
   bool contains(typename function_argument<E>::type k) const { return !base().contains(k); }
};

template <typename SetTop, typename E, typename Comparator>
struct redirect_object_traits< Complement<SetTop, E, Comparator> >
   : spec_object_traits<is_container> {
   typedef Set<E, Comparator> persistent_type;
   typedef GenericSet<SetTop, E, Comparator> generic_type;
   typedef is_set generic_tag;
   typedef SetTop masquerade_for;
   static const bool is_always_const=true;
};

template <typename SetTop, typename E, typename Comparator>
struct is_suitable_container<Complement<SetTop, E, Comparator>, void, false> : std::true_type {};

/* ------------------
 *  SingleElementSet
 * ------------------ */

/** @ingroup genericSets
 *  @brief A set consisting of exactly one element.
 */
template <typename Eref, typename Comparator>
class SingleElementSetCmp
   : public single_value_container<Eref>
   , public GenericSet<SingleElementSetCmp<Eref, Comparator>, typename deref<Eref>::type, Comparator> {
   typedef single_value_container<Eref> base_t;
public:
   using typename GenericSet<SingleElementSetCmp>::element_type;

   SingleElementSetCmp(typename base_t::arg_type arg)
      : base_t(arg) {}

   bool contains(const element_type& x) const
   {
      Comparator cmp;
      return cmp(x, this->front())==cmp_eq;
   }

   typename base_t::const_iterator find(const element_type& x) const
   {
      return contains(x) ? this->begin() : this->end();
   }

   const Comparator get_comparator() const { return Comparator(); }
};

template <typename E>
using SingleElementSet = SingleElementSetCmp<E, operations::cmp>;

template <typename E, typename Comparator>
struct spec_object_traits< SingleElementSetCmp<E, Comparator> >
   : spec_object_traits< single_value_container<E> > {
   static const bool is_always_const=true;
};

template <typename Comparator, typename E> inline
const SingleElementSetCmp<const E&, Comparator>
scalar2set(const E& x)
{
   return x;
}

/// constructs a one element set
template <typename E> inline
const SingleElementSet<const E&>
scalar2set(const E& x)
{
   return x;
}

/// constructs a one element set, creating a private copy of the element inside
template <typename E> inline
const SingleElementSet<E>
scalarcopy2set(const E& x)
{
   return x;
}

/* ----------
 *  LazySet2
 * ---------- */

template <typename SetRef1, typename SetRef2, typename Controller>
class LazySet2
   : public container_pair_base<SetRef1, SetRef2>
   , public modified_container_pair_impl< LazySet2<SetRef1, SetRef2, Controller>,
                                          mlist< Container1Tag< SetRef1 >,
                                                 Container2Tag< SetRef2 >,
                                                 IteratorCouplerTag< zipping_coupler<typename deref<SetRef1>::type::element_comparator, Controller> >,
                                                 OperationTag< BuildBinaryIt<operations::zipper> >,
                                                 IteratorConstructorTag< binary_transform_constructor<BijectiveTag<std::false_type>> > > >
   , public GenericSet< LazySet2<SetRef1, SetRef2, Controller>,
                        typename deref<SetRef1>::type::element_type,
                        typename deref<SetRef1>::type::element_comparator > {
   typedef container_pair_base<SetRef1, SetRef2> base_t;
public:
   static_assert(std::is_same<typename LazySet2::element_type, typename deref<SetRef2>::type::element_type>::value,
                 "sets with different element types mixed in an expression");
   static_assert(std::is_same<typename LazySet2::element_comparator, typename deref<SetRef2>::type::element_comparator>::value,
                 "sets with different element comparators mixed in an expression");
   typedef indexed cannot_enforce_features;

   LazySet2(typename base_t::first_arg_type src1_arg, typename base_t::second_arg_type src2_arg, Controller=Controller())
      : base_t(src1_arg,src2_arg) {}

   const typename deref<SetRef1>::type::element_comparator& get_comparator() const
   {
      return this->get_container1().get_comparator();
   }

   bool contains(typename function_argument<typename deref<SetRef1>::type::element_type>::type x) const
   {
      return Controller::contains(this->get_container1().contains(x), this->get_container2().contains(x));
   }
};

template <typename SetRef1, typename Set2>
class LazySet2<SetRef1, const Complement<Set2>&, set_difference_zipper>
   : public LazySet2<SetRef1, const Set2&, set_intersection_zipper> {
   typedef LazySet2<SetRef1, const Set2&, set_intersection_zipper> base_t;
public:
   typedef const Complement<Set2>& second_arg_type;
   LazySet2(typename base_t::first_arg_type src1_arg, second_arg_type src2_arg)
      : base_t(src1_arg, src2_arg.base()) {}
};

template <typename SetRef1, typename Set2>
class LazySet2<SetRef1, const Complement<Set2>&, set_intersection_zipper>
   : public LazySet2<SetRef1, const Set2&, set_difference_zipper> {
   typedef LazySet2<SetRef1, const Set2&, set_difference_zipper> base_t;
public:
   typedef const Complement<Set2>& second_arg_type;
   LazySet2(typename base_t::first_arg_type src1_arg, second_arg_type src2_arg)
      : base_t(src1_arg, src2_arg.base()) {}
};

template <typename Set1, typename SetRef2>
class LazySet2<const Complement<Set1>&, SetRef2, set_intersection_zipper>
   : public LazySet2<SetRef2, const Set1&, set_difference_zipper> {
   typedef LazySet2<SetRef2, const Set1&, set_difference_zipper> base_t;
public:
   typedef const Complement<Set1>& first_arg_type;
   LazySet2(first_arg_type src1_arg, typename base_t::second_arg_type src2_arg)
      : base_t(src1_arg.base(), src2_arg) {}
};

template <typename SetRef1, typename SetRef2, typename Controller>
struct spec_object_traits< LazySet2<SetRef1, SetRef2, Controller> >
   : spec_object_traits<is_container> {
   static const bool is_lazy=true, is_temporary=true, is_always_const=true;
};

/* -----------------------------
 *  Set with assigned dimension
 * ----------------------------- */

template <typename SetRef,
          bool is_complement=is_derived_from_instance_of<typename deref<SetRef>::type, Complement>::value>
struct complement_helper : std::false_type {
   typedef typename deref<SetRef>::type container;
   typedef container base_type;
   typedef const container& container_ref;
   static int size(container_ref c, int) { return c.size(); }
};

template <typename SetRef>
struct complement_helper<SetRef, true> : std::true_type {
   typedef typename deref<SetRef>::type::base_type base_type;
   typedef LazySet2<sequence, const base_type&, set_difference_zipper> container;
   typedef container container_ref;
   static int size(SetRef c, int d) { return d-c.base().size(); }
};

/** @ingroup genericSets
 *  @brief %Set_with_dim as GenericSet.
 */
template <typename SetRef>
class Set_with_dim
   : public modified_container_impl< Set_with_dim<SetRef>,
                                     mlist< ContainerTag< typename complement_helper<SetRef>::container >,
                                            OperationTag< pair<nothing, operations::identity<int> > >,
                                            ExpectedFeaturesTag<end_sensitive> > >,
     public GenericSet< Set_with_dim<SetRef>, typename deref<SetRef>::type::element_type,
                        typename deref<SetRef>::type::element_comparator> {
   typedef modified_container_impl<Set_with_dim> base_t;
   typedef typename function_argument<typename deref<SetRef>::type::element_type>::type elem_arg_type;
   typedef complement_helper<SetRef> helper;
protected:
   alias<SetRef> set;
   int _dim;
public:
   typedef typename alias<SetRef>::arg_type arg_type;

   Set_with_dim(arg_type set_arg, int dim_arg)
      : set(set_arg)
      , _dim(dim_arg) {}

   const typename helper::base_type& get_set() const
   {
      return get_set_impl(helper());
   }
   typename helper::container_ref get_container() const
   {
      return get_container_impl(helper());
   }

   /// the size of the set
   int size() const
   {
      return size_impl(helper());
   }
   int max_size() const
   {
      return _dim;
   }
   int dim() const
   {
      return _dim;
   }
   bool contains(elem_arg_type x) const
   {
      return get_container().contains(x);
   }
   const typename deref<SetRef>::type::element_comparator& get_comparator() const
   {
      return get_set().get_comparator();
   }
   typename base_t::const_iterator find(elem_arg_type x) const
   {
      return get_container().find(x);
   }
private:
   const typename helper::base_type& get_set_impl(std::false_type) const
   {
      return *set;
   }
   const typename helper::base_type& get_set_impl(std::true_type) const
   {
      return set->base();
   }
   typename helper::container_ref get_container_impl(std::false_type) const
   {
      return get_set_impl(std::false_type());
   }
   typename helper::container_ref get_container_impl(std::true_type) const
   {
      return typename helper::container(sequence(0,_dim), get_set_impl(std::true_type()));
   }
   int size_impl(std::false_type) const
   {
      return get_set_impl(std::false_type()).size();
   }
   int size_impl(std::true_type) const
   {
      return _dim-get_set_impl(std::true_type()).size();
   }
};

template <typename SetRef>
struct check_container_feature<Set_with_dim<SetRef>, sparse_compatible> : std::true_type {};

template <typename SetRef,
          bool _has_dim=check_container_feature<typename complement_helper<SetRef>::container, sparse_compatible>::value>
struct Set_with_dim_helper : std::true_type {
   typedef SetRef container;
   typedef typename attrib<SetRef>::plus_const_ref container_ref;
   typedef alias<SetRef> alias_type;

   static container_ref create(container_ref c, int) { return c; }
   static container_ref get_set(container_ref c) { return c; }
   static typename alias_type::const_reference deref(const alias_type& a) { return *a; }
};

template <typename SetRef>
struct Set_with_dim_helper<SetRef, false> : std::false_type {
   typedef Set_with_dim<SetRef> alias_type;
   typedef alias_type container;

   static alias_type create(typename alias_type::arg_type c, int d)
   {
      return container(c,d);
   }
   static const typename complement_helper<SetRef>::base_type& get_set(const container& c)
   {
      return c.get_set();
   }
   static const container& deref(const alias_type& a) { return a; }
};

/** @ingroup genericSets
 *  @namespace operations
 *  @brief functors for %operations on GenericSet objects
 */

namespace operations {

template <typename OpRef>
struct bitwise_inv_impl<OpRef, is_set> {
   typedef OpRef argument_type;
   typedef const Complement<typename deref<OpRef>::type>& result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return reinterpret_cast<result_type>(x);
   }
};

template <typename Set>
struct bitwise_inv_impl<const Complement<Set>&, is_set> {
   typedef const Complement<Set>& argument_type;
   typedef const Set& result_type;

   result_type operator() (argument_type x) const
   {
      return x.base();
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, typename attrib<RightRef>::plus_const, set_union_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   template <typename Iterator2>
   typename function_argument<LeftRef>::const_type
   operator() (partial_left, typename function_argument<LeftRef>::const_type l, const Iterator2&) const
   {
      return l;
   }

   template <typename Iterator1>
   typename function_argument<RightRef>::const_type
   operator() (partial_right, const Iterator1&, typename function_argument<RightRef>::const_type r) const
   {
      return r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_set, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<RightRef>::plus_const, Comparator> Right;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, Right, set_union_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l+=r;
   }
};

template <typename LeftRef, typename RightRef>
struct add_impl<LeftRef, RightRef, cons<is_scalar, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<LeftRef>::plus_const, Comparator> Left;
   typedef LazySet2<Left, typename attrib<RightRef>::plus_const, set_union_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, typename attrib<RightRef>::plus_const, set_difference_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const {
      return result_type(l,r);
   }

   template <typename Iterator2>
   typename function_argument<LeftRef>::const_type
   operator() (partial_left, typename function_argument<LeftRef>::const_type l, const Iterator2&) const
   {
      return l;
   }

   template <typename Iterator1>
   typename result_type::persistent_type
   operator() (partial_right, const Iterator1&, typename function_argument<RightRef>::const_type) const
   {
      return typename result_type::persistent_type();
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, typename RightRef>
struct sub_impl<LeftRef, RightRef, cons<is_set, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<RightRef>::plus_const, Comparator> Right;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, Right, set_difference_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l-=r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, typename attrib<RightRef>::plus_const, set_intersection_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l*=r;
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_set, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<RightRef>::plus_const, Comparator> Right;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, Right, set_intersection_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }
};

template <typename LeftRef, typename RightRef>
struct mul_impl<LeftRef, RightRef, cons<is_scalar, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<LeftRef>::plus_const, Comparator> Left;
   typedef LazySet2<Left, typename attrib<RightRef>::plus_const, set_intersection_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_xor_impl<LeftRef, RightRef, cons<is_set, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, typename attrib<RightRef>::plus_const, set_symdifference_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   template <typename Iterator2>
   typename function_argument<LeftRef>::const_type
   operator() (partial_left, typename function_argument<LeftRef>::const_type l, const Iterator2&) const
   {
      return l;
   }

   template <typename Iterator1>
   typename function_argument<RightRef>::const_type
   operator() (partial_right, const Iterator1&, typename function_argument<RightRef>::const_type r) const
   {
      return r;
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l^=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_xor_impl<LeftRef, RightRef, cons<is_set, is_scalar> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<LeftRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<RightRef>::plus_const, Comparator> Right;
   typedef LazySet2<typename attrib<LeftRef>::plus_const, Right, set_symdifference_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }

   void assign(typename lvalue_arg<LeftRef>::type l, typename function_argument<RightRef>::const_type r) const
   {
      l^=r;
   }
};

template <typename LeftRef, typename RightRef>
struct bitwise_xor_impl<LeftRef, RightRef, cons<is_scalar, is_set> > {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef typename deref<RightRef>::type::element_comparator Comparator;
   typedef SingleElementSetCmp<typename attrib<LeftRef>::plus_const,Comparator> Left;
   typedef LazySet2<Left, typename attrib<RightRef>::plus_const, set_symdifference_zipper> result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type l,
                           typename function_argument<RightRef>::const_type r) const
   {
      return result_type(l,r);
   }
};

template <typename LeftRef, typename RightRef>
struct includes {
   typedef LeftRef first_argument_type;
   typedef RightRef second_argument_type;
   typedef bool result_type;

   result_type operator() (typename function_argument<LeftRef>::const_type s1,
                           typename function_argument<RightRef>::const_type s2) const
   {
      return incl(s2,s1)<1;
   }
};

template <typename SetRef>
class element_of {
   alias<SetRef> set;
public:
   typedef typename deref<SetRef>::type set_type;
   typedef typename set_type::element_type argument_type;
   typedef bool result_type;

   element_of(typename alias<SetRef>::arg_type set_arg) : set(set_arg) {}

   result_type operator() (typename function_argument<argument_type>::type x) const
   {
      return set->contains(x);
   }
};

} // end namespace operations


/** @namespace operators
    @brief functors for %operators on GenericSet objects
 */

namespace operators {

/// union of two GenericSet objects
template <typename Set1, typename Set2> inline
typename operations::add_impl<const Set1&, const Set2&>::result_type
operator+ (const GenericSet<Set1>& l, const GenericSet<Set2>& r)
{
   operations::add_impl<const Set1&, const Set2&> op;
   return op(l.top(), r.top());
}

/// union of a GenericSet and a singleton
template <typename Set1, typename E> inline
typename operations::add_impl<const Set1&, const E&>::result_type
operator+ (const GenericSet<Set1,E>& l, const E& r)
{
   operations::add_impl<const Set1&, const E&> op;
   return op(l.top(), r);
}

/// union of singleton and a GenericSet
template <typename E, typename Set2> inline
typename operations::add_impl<const E&, const Set2&>::result_type
operator+ (const E& l, const GenericSet<Set2,E>& r)
{
   operations::add_impl<const E&, const Set2&> op;
   return op(l, r.top());
}

/// difference of two GenericSet objects
template <typename Set1, typename Set2> inline
typename operations::sub_impl<const Set1&, const Set2&>::result_type
operator- (const GenericSet<Set1>& l, const GenericSet<Set2>& r)
{
   operations::sub_impl<const Set1&, const Set2&> op;
   return op(l.top(), r.top());
}

/// difference of GenericSet and a singleton
template <typename Set1, typename E> inline
typename operations::sub_impl<const Set1&, const E&>::result_type
operator- (const GenericSet<Set1,E>& l, const E& r)
{
   operations::sub_impl<const Set1&, const E&> op;
   return op(l.top(), r);
}

template <typename Set1, typename Set2> inline
typename operations::sub_impl<const Set1&, const Set2&>::result_type
operator- (const GenericSet<Set1>& l, const Complement<Set2>& r)
{
   operations::sub_impl<const Set1&, const Complement<Set2>&> op;
   return op(l.top(), r);
}

/// equality of GenericSet objects
template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator== (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   return equal_ranges(entire(l.top()), entire(r.top()));
}

template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator!= (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   return !(l==r);
}

/// lexicographical comparison of GenericSet objects
template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator< (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   operations::lt<const Set1&, const Set2&> cmp_op;
   return cmp_op(l.top(), r.top());
}

template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator> (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   return r < l;
}

template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator<= (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   return !(r < l);
}

template <typename Set1, typename Set2, typename E, typename Comparator> inline
bool operator>= (const GenericSet<Set1, E, Comparator>& l, const GenericSet<Set2, E, Comparator>& r)
{
   return !(l < r);
}

} // end namespace operators

template <typename Set1, typename Set2, bool _both_have_size=
          (iterator_traits<typename Set1::iterator>::is_bidirectional &&
           iterator_traits<typename Set2::iterator>::is_bidirectional)>
struct size_estimator {
   /** Estimates how the insertion or removal of a sequence of elements could be made faster.
       Returns true if \\n2*log(n1) < n1+n2\\, which means that seeking for each element of set2
       in set1 would be faster than sequentially comparing element pairs from set1 and set2.
   */
   static bool seek_cheaper_than_sequential(const Set1& set1, const Set2& set2)
   {
      const int n1=set1.size(), n2=set2.size();
      return n2==0 || set1.tree_form() && ( n1/n2>=31 || (1<<(n1/n2))>n1 );
   }

   static int compare(const Set1& set1, const Set2& set2)
   {
      return sign(set1.size()-set2.size());
   }
};

template <typename Set1, typename Set2>
struct size_estimator<Set1, Set2, false> {
   static bool seek_cheaper_than_sequential(const Set1&, const Set2&) { return true; }
   static int compare(const Set1&, const Set2&) { return 0; }
};

/** Analyze the inclusion relation of two sets.
    @returnval  0 $s1$ and $s2$ are equal
    @returnval -1 $s1$ is included in $s2$
    @returnval  1 $s2$ is included in $s1$
    @returnval  2 $s1$ and $s2$ are independent
*/
template <typename Set1, typename Set2, typename E1, typename E2, class Comparator>
int incl(const GenericSet<Set1, E1, Comparator>& s1, const GenericSet<Set2, E2, Comparator>& s2)
{
   typename Entire<Set1>::const_iterator e1=entire(s1.top());
   typename Entire<Set2>::const_iterator e2=entire(s2.top());
   int result = size_estimator<Set1, Set2>::compare(s1.top(),s2.top());
   while (!e1.at_end() && !e2.at_end()) {
      switch (s1.top().get_comparator()(*e2,*e1)) {
      case cmp_eq: ++e1; ++e2; break;
      case cmp_lt:
         if (result>0) return 2;
         result=-1;
         ++e2;
         break;
      case cmp_gt:
         if (result<0) return 2;
         result=1;
         ++e1;
         break;
      }
   }
   if ((!e1.at_end() && result<0) || (!e2.at_end() && result>0)) return 2;
   return result;
}

template <typename Container, typename Comparator=operations::cmp, typename ProvidedFeatures=void>
class OrderedContainer
   : public redirected_container< OrderedContainer<Container, Comparator, ProvidedFeatures>,
                                  mlist< HiddenTag< Container >,
                                         ExpectedFeaturesTag< ProvidedFeatures > > >
   , public GenericSet< OrderedContainer<Container, Comparator, ProvidedFeatures>,
                        typename object_traits<typename Container::value_type>::persistent_type,
                        Comparator > {};

template <typename Container, typename Comparator, typename ProvidedFeatures, typename Feature>
struct enforce_features<OrderedContainer<Container, Comparator, ProvidedFeatures>, Feature> {
   typedef OrderedContainer<Container, Comparator, typename mix_features<ProvidedFeatures, Feature>::type> container;
};

template <typename Container> inline
const OrderedContainer<Container>&
assure_ordered(const Container& c)
{
   return reinterpret_cast<const OrderedContainer<Container>&>(c);
}

template <typename Comparator, typename Container> inline
const OrderedContainer<Container, Comparator>&
assure_ordered(const Container& c)
{
   return reinterpret_cast<const OrderedContainer<Container, Comparator>&>(c);
}

template <typename ContainerRef>
class Indices
   : public modified_container_impl< Indices<ContainerRef>,
                                     mlist< ContainerTag< ContainerRef >,
                                            OperationTag< BuildUnaryIt<operations::index2element> >,
                                            ExpectedFeaturesTag< indexed > > >,
     public GenericSet< Indices<ContainerRef>, int, operations::cmp> {
   typedef modified_container_impl<Indices> base_t;
protected:
   alias<ContainerRef> c;
public:
   typedef typename least_derived_class<bidirectional_iterator_tag, typename container_traits<ContainerRef>::category>::type container_category;

   Indices(typename alias<ContainerRef>::arg_type c_arg) : c(c_arg) {}

   const typename base_t::container& get_container() const { return *c; }

   bool contains(int i) const { return !get_container().find(i).at_end(); }

   typename base_t::const_iterator find(int i) const { return get_container().find(i); }
};

template <typename ContainerRef>
struct spec_object_traits< Indices<ContainerRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename Container> inline
const Indices<const Container&>
indices(const Container& c)
{
   return c;
}

template <typename Set>
struct hash_func<Set, is_set> {
   size_t operator() (const Set& s) const
   {
      hash_func<typename Set::element_type> element_hasher;
      size_t a=1, b=0;
      for (typename pm::Entire<Set>::const_iterator e=entire(s); !e.at_end(); ++e, ++b)
         a=a*element_hasher(*e)+b;
      return a;
   }
};

} // end namespace pm

namespace polymake {
   using pm::GenericSet;
   using pm::scalar2set;
   using pm::assure_ordered;
   using pm::Indices;
   using pm::indices;

   namespace operations {
      typedef BuildBinary<pm::operations::includes> includes;

      template <typename Set> inline
      pm::operations::element_of<const Set&> element_of(const Set& s) { return s; }
   }
}

#endif // POLYMAKE_GENERIC_SET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
