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

/** @file Set.h
    @brief Implementation of pm::Set class
*/


#ifndef POLYMAKE_SET_H
#define POLYMAKE_SET_H

#include "polymake/internal/AVL.h"
#include "polymake/internal/tree_containers.h"
#include "polymake/internal/shared_object.h"
#include "polymake/internal/converters.h"
#include "polymake/IndexedSubset.h"
#include "polymake/SelectedSubset.h"

namespace pm {

template <typename SetRef, cmp_value direction>
class TruncatedSet
   : public GenericSet< TruncatedSet<SetRef,direction>,
                        typename deref<SetRef>::type::element_type, typename deref<SetRef>::type::element_comparator > {
public:
   typedef typename container_traits<SetRef>::value_type value_type;
   typedef typename container_traits<SetRef>::const_reference const_reference;
   typedef const_reference reference;
   typedef bidirectional_iterator_tag container_category;
protected:
   alias<SetRef> set;
   value_type limit;

   typename alias<SetRef>::const_reference get_set() const { return *set; }
public:
   typedef typename alias<SetRef>::arg_type arg_type;

   TruncatedSet(arg_type set_arg, typename function_argument<value_type>::type lim_arg)
      : set(set_arg), limit(lim_arg) {}

   const typename GenericSet<TruncatedSet>::element_comparator& get_comparator() const { return get_set().get_comparator(); }

protected:
   typedef typename container_traits<SetRef>::const_iterator set_iterator;
   typedef typename container_traits<SetRef>::const_reverse_iterator set_reverse_iterator;
   typedef typename std::conditional<direction==cmp_lt, set_iterator, set_reverse_iterator>::type trunc_base;
   typedef typename std::conditional<direction==cmp_gt, set_iterator, set_reverse_iterator>::type range_base;

   class predicate {
      value_type limit;
      typename deref<SetRef>::type::element_comparator cmp;
   public:
      typedef value_type argument_type;
      typedef bool result_type;

      predicate(typename function_argument<value_type>::type lim_arg=value_type()) : limit(lim_arg) {}

      result_type operator() (typename function_argument<value_type>::type i) const
      {
         return cmp(i,limit)==direction;
      }
   };

   typedef input_truncator<trunc_base, predicate> trunc_it;
   typedef typename std::conditional<check_iterator_feature<range_base, end_sensitive>::value, range_base, iterator_range<range_base> >::type range_it;

public:
   typedef typename std::conditional<direction==cmp_lt, trunc_it, range_it>::type iterator;
   typedef iterator const_iterator;
   typedef typename std::conditional<direction==cmp_gt, trunc_it, range_it>::type reverse_iterator;
   typedef reverse_iterator const_reverse_iterator;
protected:
   template <typename TEnd_sensitive>
   iterator begin_impl(int_constant<cmp_lt>, TEnd_sensitive) const
   {
      return iterator(get_set().begin(), predicate(limit));
   }
   iterator end_impl(int_constant<cmp_lt>) const
   {
      return iterator(get_set().find_nearest(limit, polymake::operations::ge()), predicate(limit));
   }
   reverse_iterator rbegin_impl(int_constant<cmp_lt>, std::false_type) const
   {
      return reverse_iterator(get_set().find_nearest(limit, polymake::operations::lt()), get_set().rend());
   }
   reverse_iterator rbegin_impl(int_constant<cmp_lt>, std::true_type) const
   {
      return set_reverse_iterator(get_set().find_nearest(limit, polymake::operations::lt()));
   }
   reverse_iterator rend_impl(int_constant<cmp_lt>) const
   {
      return get_set().rend();
   }
   iterator begin_impl(int_constant<cmp_gt>, std::false_type) const
   {
      return iterator(get_set().find_nearest(limit, polymake::operations::gt()), get_set().end());
   }
   iterator begin_impl(int_constant<cmp_gt>, std::true_type) const
   {
      return get_set().find_nearest(limit, polymake::operations::gt());
   }
   iterator end_impl(int_constant<cmp_gt>) const
   {
      return get_set().end();
   }
   template <typename TEnd_sensitive>
   reverse_iterator rbegin_impl(int_constant<cmp_gt>, TEnd_sensitive) const
   {
      return reverse_iterator(get_set().rbegin(), predicate(limit));
   }
   reverse_iterator rend_impl(int_constant<cmp_gt>) const
   {
      return get_set().find_nearest(limit, polymake::operations::le());
   }
public:
   iterator begin() const { return begin_impl(int_constant<direction>(), bool_constant<check_iterator_feature<range_base, end_sensitive>::value>()); }
   iterator end() const { return end_impl(int_constant<direction>()); }
   reverse_iterator rbegin() const { return rbegin_impl(int_constant<direction>(), bool_constant<check_iterator_feature<range_base, end_sensitive>::value>()); }
   reverse_iterator rend() const { return rend_impl(int_constant<direction>()); }

   reference front() const { return *begin(); }
   reference back() const { return *rbegin(); }

   /// the size of the set
   int size() const { return count_it(begin()); }
   /// true if the set is empty
   bool empty() const { return begin().at_end(); }
};

template <typename SetRef, cmp_value direction>
struct spec_object_traits< TruncatedSet<SetRef,direction> > : spec_object_traits<is_container> {
   static const bool is_temporary=true;
};

// to be used as callback in AVL::Tree::insert()
struct element_seen_op {
   mutable bool seen;
   element_seen_op() : seen(false) {}

   template <typename Key>
   void operator() (const Key&, const nothing&) const { seen=true; }

   operator bool () const { return seen; }
};

/// @ref generic "Generic type" for ordered mutable sets
template <typename TSet, typename E=typename TSet::element_type, typename Comparator=typename TSet::element_comparator>
class GenericMutableSet
   : public GenericSet<TSet, E, Comparator> {
   template <typename, typename, typename> friend class GenericMutableSet;
protected:
   GenericMutableSet() {}
   GenericMutableSet(const GenericMutableSet&) {}
   typedef GenericMutableSet generic_mutable_type;
public:
   typedef typename GenericSet<TSet, E, Comparator>::top_type top_type;
protected:
   template <typename TSet2>
   void plus_seek(const TSet2& s)
   {
      for (auto e2=entire(s); !e2.at_end(); ++e2)
         this->top().insert(*e2);
   }

   template <typename TSet2>
   void plus_seq(const TSet2& s)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto e1=entire(this->top());
      auto e2=entire(s);
      while (!e1.at_end() && !e2.at_end()) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_eq: ++e2;
         case cmp_lt: ++e1; break;
         case cmp_gt: this->top().insert(e1,*e2); ++e2;
         }
      }
      for (; !e2.at_end(); ++e2) this->top().insert(e1,*e2);
   }

   template <typename TSet2, typename E2>
   void plus_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         plus_seek(s.top());
      else
         plus_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void plus_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::false_type)
   {
      plus_seq(s.top());
   }

   template <typename Right>
   void plus_impl(const Right& x, cons<is_set, is_scalar>, std::true_type)
   {
      this->top().insert(x);
   }

   template <typename Right>
   void plus_impl(const Right& x, cons<is_set, is_scalar>, std::false_type)
   {
      plus_seq(scalar2set(x));
   }

   template <typename TSet2>
   void minus_seek(const TSet2& s)
   {
      for (auto e2=entire(s); !e2.at_end(); ++e2)
         this->top().erase(*e2);
   }

   template <typename TSet2>
   void minus_seq(const TSet2& s)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto e1=entire(this->top());
      auto e2=entire(s);
      while (!e1.at_end() && !e2.at_end()) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_lt: ++e1; break;
         case cmp_eq: this->top().erase(e1++);
         case cmp_gt: ++e2;
         }
      }
   }

   template <typename TSet2, typename E2>
   void minus_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         minus_seek(s.top());
      else
         minus_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void minus_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::false_type)
   {
      minus_seq(s.top());
   }

   template <typename TSet2, typename E2, typename TDiscr>
   void minus_impl(const Complement<TSet2, E2, Comparator>& s, cons<is_set, is_set>, TDiscr)
   {
      *this *= s.top();
   }

   template <typename Right>
   void minus_impl(const Right& x, cons<is_set, is_scalar>, std::true_type)
   {
      this->top().erase(x);
   }

   template <typename Right>
   void minus_impl(const Right& x, cons<is_set, is_scalar>, std::false_type)
   {
      minus_seq(scalar2set(x));
   }

   template <typename TSet2>
   void xor_seek(const TSet2& s)
   {
      for (auto e2=entire(s); !e2.at_end(); ++e2)
         this->top().toggle(*e2);
   }

   template <typename TSet2>
   void xor_seq(const TSet2& s)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto e1=entire(this->top());
      auto e2=entire(s);
      while (!e1.at_end() && !e2.at_end()) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_lt:  ++e1;  break;
         case cmp_eq: this->top().erase(e1++);  ++e2;  break;
         case cmp_gt: this->top().insert(e1,*e2);  ++e2;
         }
      }
      for (; !e2.at_end(); ++e2) this->top().insert(e1,*e2);
   }

   template <typename TSet2, typename E2>
   void xor_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         xor_seek(s.top());
      else
         xor_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void xor_impl(const GenericSet<TSet2, E2, Comparator>& s, cons<is_set, is_set>, std::false_type)
   {
      xor_seq(s.top());
   }

   template <typename Right>
   void xor_impl(const Right& x, cons<is_set, is_scalar>, std::true_type)
   {
      this->top().toggle(x);
   }

   template <typename Right>
   void xor_impl(const Right& x, cons<is_set, is_scalar>, std::false_type)
   {
      xor_seq(scalar2set(x));
   }

   template <typename TSet2, typename E2>
   constexpr bool trivial_assignment(const GenericSet<TSet2, E2, Comparator>&) const { return false; }

   constexpr bool trivial_assignment(const GenericMutableSet& s) const { return this==&s; }

   template <typename TSet2, typename E2, typename DiffConsumer>
   void assign(const GenericSet<TSet2, E2, Comparator>& s, DiffConsumer diff)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto dst=entire(this->top());
      auto src=entire(s.top());
      int state=(dst.at_end() ? 0 : zipper_first) + (src.at_end() ? 0 : zipper_second);
      while (state >= zipper_both) {
         switch (cmp_op(*dst, *src)) {
         case cmp_lt:
            if (!is_instance_of<DiffConsumer, black_hole>::value) *diff++=*dst;
            this->top().erase(dst++);
            if (dst.at_end()) state -= zipper_first;
            break;
         case cmp_eq:
            ++dst;
            if (dst.at_end()) state -= zipper_first;
            ++src;
            if (src.at_end()) state -= zipper_second;
            break;
         case cmp_gt:
            if (!is_instance_of<DiffConsumer, black_hole>::value) *diff++=*src;
            this->top().insert(dst, *src);  ++src;
            if (src.at_end()) state -= zipper_second;
         }
      }
      if (state & zipper_first) {
         do {
            if (!is_instance_of<DiffConsumer, black_hole>::value) *diff++=*dst;
            this->top().erase(dst++);
         }
         while (!dst.at_end());
      } else if (state) {
         do {
            if (!is_instance_of<DiffConsumer, black_hole>::value) *diff++=*src;
            this->top().insert(dst, *src);  ++src;
         } while (!src.at_end());
      }
   }

   template <typename TSet2, typename E2>
   void assign(const GenericSet<TSet2, E2, Comparator>& s)
   {
      assign(s, black_hole<E>());
   }

public:
   top_type& operator= (const GenericMutableSet& s)
   {
      if (!this->trivial_assignment(s)) this->top().assign(s);
      return this->top();
   }

   template <typename TSet2, typename E2,
             typename=typename std::enable_if<can_initialize<E2, E>::value>::type>
   top_type& operator= (const GenericSet<TSet2, E2, Comparator>& other)
   {
      this->top().assign(other);
      return this->top();
   }

   template <typename E2,
             typename=typename std::enable_if<can_initialize<E2, E>::value>::type>
   top_type& operator= (std::initializer_list<E2> l)
   {
      this->top().clear();
      this->top().insert_from(entire(l));
      return this->top();
   }

   template <typename TSet2>
   void swap(GenericMutableSet<TSet2, E, Comparator>& s)
   {
      if (trivial_assignment(s)) return;
      const Comparator& cmp_op=this->top().get_comparator();
      auto e1=entire(this->top());
      auto e2=entire(s.top());
      int state=(e1.at_end() ? 0 : zipper_first) + (e2.at_end() ? 0 : zipper_second);
      while (state >= zipper_both) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_lt:
            s.top().insert(e2, e1.index());
            this->top().erase(e1++);
            if (e1.at_end()) state -= zipper_first;
            break;
         case cmp_gt:
            this->top().insert(e1, e2.index());
            s.top().erase(e2++);
            if (e2.at_end()) state -= zipper_second;
            break;
         case cmp_eq:
            ++e1;
            if (e1.at_end()) state -= zipper_first;
            ++e2;
            if (e2.at_end()) state -= zipper_second;
         }
      }
      if (state & zipper_first) {
         do {
            s.top().insert(e2, e1.index());
            this->top().erase(e1++);
         } while (!e1.at_end());
      } else if (state) {
         do {
            this->top().insert(e1, e2.index());
            s.top().erase(e2++);
         } while (!e2.at_end());
      }
   }

   /// %Set union
   template <typename Right>
   top_type& operator+= (const Right& x)
   {
      plus_impl(x, typename isomorphic_types<top_type, Right>::discriminant(),
                   is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// Add to the set, report true if existed formerly.
   template <typename Right>
   bool collect(const Right& x)
   {
      element_seen_op seen;
      this->top().insert(x,nothing(),seen);
      return seen;
   }

   /// %Set difference
   template <typename Right>
   top_type& operator-= (const Right& x)
   {
      minus_impl(x, typename isomorphic_types<top_type, Right>::discriminant(),
                    is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// %Set intersection
   template <typename TSet2, typename E2>
   top_type& operator*= (const GenericSet<TSet2, E2, Comparator>& s)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto e1=entire(this->top());
      auto e2=entire(s.top());
      while (!e1.at_end() && !e2.at_end()) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_lt: this->top().erase(e1++); break;
         case cmp_eq: ++e1;
         case cmp_gt: ++e2;
         }
      }
      while (!e1.at_end()) this->top().erase(e1++);
      return this->top();
   }

   template <typename TSet2, typename E2>
   top_type& operator*= (const Complement<TSet2, E2, Comparator>& s)
   {
      return *this -= s.top();
   }

   /// Symmetrical difference
   template <typename Right>
   top_type& operator^= (const Right& x)
   {
      xor_impl(x, typename isomorphic_types<top_type, Right>::discriminant(),
                  is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// Compute the symmetrical difference and make *this equal to s
   template <typename TSet2, typename E2>
   Set<E, Comparator> extract_symdif(const GenericSet<TSet2, E2, Comparator>& s)
   {
      Set<E, Comparator> result;
      assign(s, std::back_inserter(result));
      return result;
   }

   auto operator<< (typename function_argument<E>::type upper_limit) const
   {
      return TruncatedSet<const top_type&, cmp_lt>(this->top(), upper_limit);
   }

   auto operator>> (typename function_argument<E>::type lower_limit) const
   {
      return TruncatedSet<const top_type&, cmp_gt>(this->top(), lower_limit);
   }

   top_type& operator<<= (typename function_argument<E>::type upper_limit)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto it=rentire(this->top());
      while (!it.at_end() && cmp_op(*it,upper_limit)>=cmp_eq)
         this->top().erase(it++);
      return this->top();
   }

   top_type& operator>>= (typename function_argument<E>::type lower_limit)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto it=entire(this->top());
      while (!it.at_end() && cmp_op(*it,lower_limit)<=cmp_eq)
         this->top().erase(it++);
      return this->top();
   }

   template <typename TSet2>
   top_type& select(const GenericSet<TSet2>& selector)
   {
      auto e1=entire(this->top());
      typename unwary_t<TSet2>::element_type cur(0);
      for (auto s=entire(selector.top()); !s.at_end(); ++s) {
         for (; cur < *s; ++cur) this->top().erase(e1++);
         ++e1; ++cur;
      }
      while (!e1.at_end()) this->top().erase(e1++);
      return this->top();
   }

   template <typename TSet2>
   top_type& select(const Complement<TSet2>& selector)
   {
      auto e1=entire(this->top());
      typename TSet2::element_type cur(0);
      for (auto s=entire(selector.base()); !s.at_end(); ++s) {
         for (; cur < *s; ++cur) ++e1;
         this->top().erase(e1++); ++cur;
      }
      return this->top();
   }

   template <typename Result>
   struct rebind_generic {
      typedef GenericMutableSet<Result, E, Comparator> type;
   };

}; // end class GenericMutableSet

/** @class Set 
    @brief An associative container based on a balanced binary search (%AVL) tree.
    @c Comparator is a functor defining a total ordering on the element value domain.
    In most cases, the default choice (lexicographical order) will suffice for your needs.

    The data tree is attached to the Set object via a @ref refcounting "smart pointer".
    Arithmetic operations for sets are listed at @ref genericSets "operations".
    <br>The following standard functions for sets are also implemented:<br>
    <code> 
       contains(); empty(); size();
    </code>
*/

template <typename E, typename Comparator>
class Set
   : public modified_tree< Set<E,Comparator>,
                           mlist< ContainerTag< AVL::tree< AVL::traits<E, nothing, Comparator> > >,
                                  OperationTag< BuildUnary<AVL::node_accessor> > > >
   , public GenericMutableSet< Set<E, Comparator>, E, Comparator > {
protected:
   typedef AVL::tree< AVL::traits<E, nothing, Comparator> > tree_type;
   shared_object<tree_type, AliasHandlerTag<shared_alias_handler>> tree;

   friend Set& make_mutable_alias(Set& alias, Set& owner)
   {
      alias.tree.make_mutable_alias(owner.tree);
      return alias;
   }
public:
   tree_type& get_container() { return *tree; }
   const tree_type& get_container() const { return *tree; }

   /// Create an empty set.
   Set() {}

   /// Create an empty set with a non-default Comparator
   explicit Set(const Comparator& cmp_arg)
      : tree(cmp_arg) {}

   /// Create a Set from an iterator
   template <typename Iterator>
   Set(Iterator&& src, Iterator&& end,
       typename std::enable_if<assess_iterator_value<Iterator, can_initialize, E>::value, void**>::type=nullptr)
   {
      insert_from(make_iterator_range(std::forward<Iterator>(src), std::forward<Iterator>(end)));
   }

   template <typename Iterator>
   explicit Set(Iterator&& src,
                typename std::enable_if<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                                        assess_iterator_value<Iterator, can_initialize, E>::value,
                                        void**>::type=nullptr)
   {
      insert_from(ensure_private_mutable(std::forward<Iterator>(src)));
   }

   /// Copy of a disguised Set object.
   Set(const GenericSet<Set, E, Comparator>& s)
      : tree(s.top().tree) {}

   /// Copy of an abstract set of the same element type.
   template <typename Set2>
   Set(const GenericSet<Set2, E, Comparator>& s)
      : tree(entire(s.top())) {}

   /// Copy of an abstract set with element conversion.
   template <typename Set2, typename E2, typename Comparator2>
   explicit Set(const GenericSet<Set2, E2, Comparator2>& s)
      : tree(entire(attach_converter<E>(s.top()))) {}

   /// One-element set
   explicit Set(typename function_argument<E>::type x)
      : tree(entire(scalar2set(x))) {}

   template <typename E2, typename=typename std::enable_if<can_initialize<E2, E>::value>::type>
   Set(std::initializer_list<E2> l)
   {
      insert_from(entire(l));
   }

   template <typename Container>
   explicit Set(const Container& src,
                typename std::enable_if<isomorphic_to_container_of<Container, E, is_set>::value, void**>::type=nullptr)
   {
      insert_from(entire(src));
   }

   Set& operator= (const Set& other) { assign(other); return *this; }
   using Set::generic_mutable_type::operator=;

   /// Make the set empty.
   void clear() { tree.apply(shared_clear()); }

   /// For compatibility with common::boost_dynamic_bitset, add a trivial method
   void resize(int) {}

   /// For compatibility with common::boost_dynamic_bitset, add a trivial method
   void reset(void) {}

   /** @brief Swap the content with another Set.
       @param s the other Set
   */
   void swap(Set& s) { tree.swap(s.tree); }

   friend void relocate(Set* from, Set* to)
   {
      relocate(&from->tree, &to->tree);
   }

#if POLYMAKE_DEBUG
   void check(const char* label) const { tree->check(label); }
   void tree_dump() const { tree->dump(); }
#endif

   Set& operator<<= (typename function_argument<E>::type upper_limit)
   {
      if (tree.is_shared())
         *this=*this << upper_limit;
      else
         Set::generic_mutable_type::operator<<=(upper_limit);
      return *this;
   }

   Set& operator>>= (typename function_argument<E>::type lower_limit)
   {
      if (tree.is_shared())
         *this=*this >> lower_limit;
      else
         Set::generic_mutable_type::operator>>=(lower_limit);
      return *this;
   }

   template <typename Set2>
   Set& select(const GenericSet<Set2>& selector)
   {
      if (tree.is_shared())
         *this=Set(entire(pm::select(const_cast<const Set&>(*this), selector.top())));
      else
         Set::generic_mutable_type::select(selector);
      return *this;
   }

   template <typename Set2>
   Set& select(const Complement<Set2>& selector)
   {
      if (tree.is_shared())
         *this=Set(entire(pm::select(const_cast<const Set&>(*this), selector)));
      else
         Set::generic_mutable_type::select(selector);
      return *this;
   }

   /// Return the (pointwise) image of this under a permutation.
   template <typename Permutation>
   Set copy_permuted(const Permutation& perm) const
   {
      Set result;
      for (typename ensure_features<Permutation, cons<end_sensitive,indexed> >::const_iterator p=ensure(perm, (cons<end_sensitive,indexed>*)0).begin();  !p.at_end();  ++p)
         if (this->exists(*p)) result.tree->push_back(p.index());
      return result;
   }

   /// Return the (pointwise) image of this under the inverse of a given permutation.
   template <typename Permutation>
   Set copy_permuted_inv(const Permutation& perm) const
   {
      const Set result(pm::select(perm, *this));
      return result;
   }

protected:
   template <typename, typename, typename> friend class GenericMutableSet;

   void assign(const GenericSet<Set>& s) { tree=s.top().tree; }

   template <typename Set2, typename E2>
   void assign(const GenericSet<Set2, E2, Comparator>& s)
   {
      if (tree.is_shared()) {
         assign(Set(s));
      } else {
         tree->assign(entire(s.top()));
      }
   }

   /// Insert elements from a sequence, coming in any order.
   template <typename Iterator>
   void insert_from(Iterator&& src)
   {
      tree_type* t=tree.get();
      for (; !src.at_end(); ++src)
         t->insert(*src);
   }
};

template <typename E, typename Comparator, typename Permutation> inline
Set<E,Comparator> permuted(const Set<E,Comparator>& s, const Permutation& perm)
{
   return s.copy_permuted(perm);
}

template <typename E, typename Comparator, typename Permutation> inline
Set<E,Comparator> permuted_inv(const Set<E,Comparator>& s, const Permutation& perm)
{
   return s.copy_permuted_inv(perm);
}

} // end namespace pm

namespace polymake {
   using pm::Set;
}

namespace std {
   template <typename Set1, typename Set2, typename E, typename Comparator> inline
   void swap(pm::GenericMutableSet<Set1,E,Comparator>& s1, pm::GenericMutableSet<Set2,E,Comparator>& s2)
   {
      s1.top().swap(s2.top());
   }

   template <typename E, typename Comparator> inline
   void swap(pm::Set<E,Comparator>& s1, pm::Set<E,Comparator>& s2) { s1.swap(s2); }
}

#endif // POLYMAKE_SET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
