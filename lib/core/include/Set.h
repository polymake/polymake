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
   using value_type = typename container_traits<SetRef>::value_type;
   using const_reference = typename container_traits<SetRef>::const_reference;
   using reference = const_reference;
   using container_category = bidirectional_iterator_tag;
protected:
   using alias_t = alias<SetRef>;
   alias_t set;
   value_type limit;

   decltype(auto) get_set() const { return *set; }
public:
   template <typename Arg, typename = std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   TruncatedSet(Arg&& set_arg, const value_type& lim_arg)
      : set(std::forward<Arg>(set_arg))
      , limit(lim_arg) {}

   decltype(auto) get_comparator() const { return get_set().get_comparator(); }

protected:
   using set_iterator = typename container_traits<SetRef>::const_iterator;
   using set_reverse_iterator = typename container_traits<SetRef>::const_reverse_iterator;
   using trunc_base = std::conditional_t<direction==cmp_lt, set_iterator, set_reverse_iterator>;
   using range_base = std::conditional_t<direction==cmp_gt, set_iterator, set_reverse_iterator>;

   class predicate {
      value_type limit;
      typename deref<SetRef>::type::element_comparator cmp;
   public:
      typedef value_type argument_type;
      typedef bool result_type;

      predicate(const value_type& lim_arg=value_type()) : limit(lim_arg) {}

      result_type operator() (const value_type& i) const
      {
         return cmp(i, limit)==direction;
      }
   };

   using trunc_it = input_truncator<trunc_base, predicate>;
   using range_it = std::conditional_t<check_iterator_feature<range_base, end_sensitive>::value, range_base, iterator_range<range_base>>;

public:
   using iterator = std::conditional_t<direction==cmp_lt, trunc_it, range_it>;
   using const_iterator = iterator ;
   using reverse_iterator = std::conditional_t<direction==cmp_gt, trunc_it, range_it>;
   using const_reverse_iterator = reverse_iterator;
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
   Int size() const { return count_it(begin()); }
   /// true if the set is empty
   bool empty() const { return begin().at_end(); }
};

template <typename SetRef, cmp_value direction>
struct spec_object_traits< TruncatedSet<SetRef, direction> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = is_effectively_const<SetRef>::value;
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
template <typename TSet, typename E = typename TSet::element_type, typename Comparator = typename TSet::element_comparator>
class GenericMutableSet
   : public GenericSet<TSet, E, Comparator> {
   template <typename, typename, typename> friend class GenericMutableSet;
protected:
   GenericMutableSet() = default;
   GenericMutableSet(const GenericMutableSet&) = default;
   using generic_mutable_type = GenericMutableSet;
public:
   using typename GenericSet<TSet, E, Comparator>::top_type;

   template <typename Right>
   using is_compatible_element = typename mlist_and<is_lossless_convertible<Right, E>, are_comparable_via<E, Right, Comparator>>::type;

   template <typename Right, bool is_set = is_generic_set<Right>::value>
   struct is_compatible_set : std::false_type {};

   template <typename Right>
   struct is_compatible_set<Right, true> : mlist_and<is_compatible_element<typename Right::element_type>, std::is_same<Comparator, typename Right::element_comparator>>::type {};

protected:
   template <typename TSet2>
   void plus_seek(const TSet2& s)
   {
      for (auto e2 = entire(s); !e2.at_end(); ++e2)
         this->top().insert(*e2);
   }

   template <typename TSet2>
   void plus_seq(const TSet2& s)
   {
      const Comparator& cmp_op = this->top().get_comparator();
      auto e1 = entire(this->top());
      auto e2 = entire(s);
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
   void plus_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         plus_seek(s.top());
      else
         plus_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void plus_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::false_type)
   {
      plus_seq(s.top());
   }

   template <typename Right>
   void plus_element_impl(const Right& x, std::true_type)
   {
      this->top().insert(x);
   }

   template <typename Right>
   void plus_element_impl(const Right& x, std::false_type)
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
      auto e1 = entire(this->top());
      auto e2 = entire(s);
      while (!e1.at_end() && !e2.at_end()) {
         switch (cmp_op(*e1,*e2)) {
         case cmp_lt: ++e1; break;
         case cmp_eq: this->top().erase(e1++);
         case cmp_gt: ++e2;
         }
      }
   }

   template <typename TSet2, typename E2>
   void minus_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         minus_seek(s.top());
      else
         minus_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void minus_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::false_type)
   {
      minus_seq(s.top());
   }

   template <typename Right>
   void minus_element_impl(const Right& x, std::true_type)
   {
      this->top().erase(x);
   }

   template <typename Right>
   void minus_element_impl(const Right& x, std::false_type)
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
      const Comparator& cmp_op = this->top().get_comparator();
      auto e1 = entire(this->top());
      auto e2 = entire(s);
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
   void xor_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::true_type)
   {
      if (size_estimator<top_type, unwary_t<TSet2>>::seek_cheaper_than_sequential(this->top(), s.top()))
         xor_seek(s.top());
      else
         xor_seq(s.top());
   }

   template <typename TSet2, typename E2>
   void xor_set_impl(const GenericSet<TSet2, E2, Comparator>& s, std::false_type)
   {
      xor_seq(s.top());
   }

   template <typename Right>
   void xor_element_impl(const Right& x, std::true_type)
   {
      this->top().toggle(x);
   }

   template <typename Right>
   void xor_element_impl(const Right& x, std::false_type)
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
      auto dst = entire(this->top());
      auto src = entire(s.top());
      int state = (dst.at_end() ? 0 : zipper_first) + (src.at_end() ? 0 : zipper_second);
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
             typename = std::enable_if_t<can_initialize<E2, E>::value>>
   top_type& operator= (const GenericSet<TSet2, E2, Comparator>& other)
   {
      this->top().assign(other);
      return this->top();
   }

   template <typename E2,
             typename = std::enable_if_t<can_initialize<E2, E>::value>>
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
      const Comparator& cmp_op = this->top().get_comparator();
      auto e1 = entire(this->top());
      auto e2 = entire(s.top());
      int state = (e1.at_end() ? 0 : zipper_first) + (e2.at_end() ? 0 : zipper_second);
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
   std::enable_if_t<is_compatible_set<Right>::value, top_type&>
   operator+= (const Right& x)
   {
      plus_set_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   template <typename Right>
   std::enable_if_t<is_compatible_element<Right>::value, top_type&>
   operator+= (const Right& x)
   {
      plus_element_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// Add to the set, report true if existed formerly.
   template <typename Right>
   std::enable_if_t<is_compatible_element<Right>::value, bool> collect(const Right& x)
   {
      element_seen_op seen;
      this->top().insert(x, nothing(), seen);
      return seen;
   }

   /// %Set difference
   template <typename Right>
   std::enable_if_t<is_compatible_set<Right>::value, top_type&>
   operator-= (const Right& x)
   {
      minus_set_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   template <typename Right>
   std::enable_if_t<is_compatible_element<Right>::value, top_type&>
   operator-= (const Right& x)
   {
      minus_element_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// %Set intersection
   template <typename Right>
   std::enable_if_t<is_compatible_set<Right>::value, top_type&>
   operator*= (const Right& x)
   {
      const Comparator& cmp_op = this->top().get_comparator();
      auto e1 = entire(this->top());
      auto e2 = entire(x.top());
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

   /// Symmetrical difference
   template <typename Right>
   std::enable_if_t<is_compatible_set<Right>::value, top_type&>
   operator^= (const Right& x)
   {
      xor_set_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   template <typename Right>
   std::enable_if_t<is_compatible_element<Right>::value, top_type&>
   operator^= (const Right& x)
   {
      xor_element_impl(x, is_derived_from_instance_of<top_type, modified_tree>());
      return this->top();
   }

   /// Compute the symmetrical difference and make *this equal to s
   template <typename Right>
   std::enable_if_t<is_compatible_set<Right>::value, Set<E, Comparator>>
   extract_symdif(const Right& x)
   {
      Set<E, Comparator> result;
      assign(x, std::back_inserter(result));
      return result;
   }

   auto operator<< (const E& upper_limit) const
   {
      return TruncatedSet<const top_type&, cmp_lt>(this->top(), upper_limit);
   }

   auto operator>> (const E& lower_limit) const
   {
      return TruncatedSet<const top_type&, cmp_gt>(this->top(), lower_limit);
   }

   top_type& operator<<= (const E& upper_limit)
   {
      const Comparator& cmp_op=this->top().get_comparator();
      auto it=entire<reversed>(this->top());
      while (!it.at_end() && cmp_op(*it,upper_limit)>=cmp_eq)
         this->top().erase(it++);
      return this->top();
   }

   top_type& operator>>= (const E& lower_limit)
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
   : public modified_tree< Set<E, Comparator>,
                           mlist< ContainerTag< AVL::tree<typename AVL::single_key_traits<E, nothing, Comparator>::type> >,
                                  OperationTag< BuildUnary<AVL::node_accessor> > > >
   , public GenericMutableSet< Set<E, Comparator>, E, Comparator > {
protected:
   using tree_type = AVL::tree<typename AVL::single_key_traits<E, nothing, Comparator>::type>;
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
       std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value, std::nullptr_t> =nullptr)
   {
      insert_from(make_iterator_range(std::forward<Iterator>(src), std::forward<Iterator>(end)));
   }

   template <typename Iterator>
   explicit Set(Iterator&& src,
                std::enable_if_t<assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value &&
                                 assess_iterator_value<Iterator, can_initialize, E>::value,
                                 std::nullptr_t> = nullptr)
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
   template <typename Set2, typename E2, typename Comparator2, typename = std::enable_if_t<can_initialize<E2, E>::value>>
   explicit Set(const GenericSet<Set2, E2, Comparator2>& s)
      : tree(entire(attach_converter<E>(s.top()))) {}

   template <typename E2, typename = std::enable_if_t<can_initialize<E2, E>::value>>
   Set(std::initializer_list<E2> l)
   {
      insert_from(entire(l));
   }

   template <typename Container>
   explicit Set(const Container& src,
                std::enable_if_t<isomorphic_to_container_of<Container, E, is_set>::value, std::nullptr_t> = nullptr)
   {
      insert_from(entire(src));
   }

   Set& operator= (const Set& other) { assign(other); return *this; }
   using Set::generic_mutable_type::operator=;

   /// Make the set empty.
   void clear() { tree.apply(shared_clear()); }

   /// for compatibility with Bitset
   void resize(Int) {}

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

   Set& operator<<= (const E& upper_limit)
   {
      if (tree.is_shared())
         *this=*this << upper_limit;
      else
         Set::generic_mutable_type::operator<<=(upper_limit);
      return *this;
   }

   Set& operator>>= (const E& lower_limit)
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
      for (auto p = entire<indexed>(perm);  !p.at_end();  ++p)
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

template <typename E, typename Comparator, typename Permutation>
Set<E,Comparator> permuted(const Set<E,Comparator>& s, const Permutation& perm)
{
   return s.copy_permuted(perm);
}

template <typename E, typename Comparator, typename Permutation>
Set<E,Comparator> permuted_inv(const Set<E,Comparator>& s, const Permutation& perm)
{
   return s.copy_permuted_inv(perm);
}

// FIXME: temporary hack until all Vector<Set> disappear from atint
template <typename E, typename Comparator>
struct spec_object_traits<Set<E, Comparator>> : spec_object_traits<is_container> {
   static const Set<E, Comparator>& zero()
   {
      static const Set<E, Comparator> z{};
      return z;
   }
};

} // end namespace pm

namespace polymake {
   using pm::Set;
}

namespace std {
   template <typename Set1, typename Set2, typename E, typename Comparator>
   void swap(pm::GenericMutableSet<Set1,E,Comparator>& s1, pm::GenericMutableSet<Set2,E,Comparator>& s2)
   {
      s1.top().swap(s2.top());
   }

   template <typename E, typename Comparator>
   void swap(pm::Set<E,Comparator>& s1, pm::Set<E,Comparator>& s2) { s1.swap(s2); }
}

#endif // POLYMAKE_SET_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
