/* Copyright (c) 1997-2021
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

#pragma once

#include "polymake/internal/iterators.h"
#include "polymake/internal/operations.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <limits>
#include <cassert>

namespace pm {

template <typename> class CopyOnWriteTag;
template <typename> class PrefixDataTag;
template <typename> class AliasHandlerTag;
template <typename> class DivorceHandlerTag;

class nop_shared_alias_handler {
protected:
   template <typename Master> static
   void CoW(Master *me, Int)
   {
      me->divorce();
   }

   static bool preCoW(Int) { return false; }
   static bool need_postCoW() { return false; }

   template <typename Master> static
   void postCoW(Master*,bool) {}

   static bool is_owner() { return true; }
public:
   void swap(nop_shared_alias_handler&) {}
   friend void relocate(nop_shared_alias_handler*, nop_shared_alias_handler*) {}
};

struct nop_divorce_handler {
   template <typename Rep, bool copied>
   Rep* operator() (Rep *body, bool_constant<copied>) const { return body; }
   void swap(nop_divorce_handler&) {}
   friend void relocate(nop_divorce_handler*, nop_divorce_handler*) {}
};

class shared_alias_handler {
protected:
   struct AliasSet {
      struct alias_array {
         Int n_alloc;
         AliasSet* aliases[1];
      };
      union {
         alias_array* set;
         AliasSet* owner;
      };
      Int n_aliases;

      static alias_array* allocate(size_t n)
      {
         allocator alloc;
         alias_array* a = (alias_array*)alloc.allocate(sizeof(alias_array) + (n-1)*sizeof(AliasSet*));
         a->n_alloc = n;
         return a;
      }
      static alias_array* reallocate(alias_array* a)
      {
         alias_array* n = allocate(a->n_alloc+3);
         std::memcpy(n->aliases, a->aliases, a->n_alloc * sizeof(AliasSet*));
         deallocate(a);
         return n;
      }
      static void deallocate(alias_array* a)
      {
         allocator alloc;
         alloc.deallocate(a, sizeof(alias_array) + (a->n_alloc-1)*sizeof(AliasSet*));
      }

      void remove(AliasSet* alias)
      {
         --n_aliases;
         for (AliasSet **s = set->aliases, **end = s+n_aliases; s < end; ++s)
            if (*s == alias) {
               *s = set->aliases[n_aliases];
               break;
            }
      }

      void forget()
      {
         if (n_aliases > 0) {
            for (AliasSet **s = set->aliases, **end = s+n_aliases; s < end; ++s)
               (*s)->owner = nullptr;
            n_aliases = 0;
         }
      }

      AliasSet() : set(nullptr), n_aliases(0) {}

      ~AliasSet()
      {
         if (set) {
            if (n_aliases >= 0) {
               forget();
               deallocate(set);
            } else {
               owner->remove(this);
            }
         }
      }

      AliasSet(const AliasSet& s2)
      {
         if (__builtin_expect(s2.n_aliases < 0, 0)) {
            if (s2.owner) {
               enter(*s2.owner);
            } else {
               // even if the original owner has gone, we can still copy the aliases
               n_aliases = -1;
               owner = nullptr;
            }
         } else {
            set = nullptr;
            n_aliases = 0;
         }
      }

   private:
      void operator=(const AliasSet&) = delete;
   public:
      void enter(AliasSet& ow)
      {
         n_aliases = -1;
         owner = &ow;
         if (!ow.set)
            ow.set = allocate(3);
         else if (ow.n_aliases == ow.set->n_alloc)
            ow.set = reallocate(ow.set);
         ow.set->aliases[ow.n_aliases++] = this;
      }

      bool is_owner() const { return n_aliases >= 0; }

      typedef AliasSet** iterator;
      iterator begin() const { return set->aliases; }
      iterator end() const { return set->aliases + n_aliases; }

      friend void relocate(AliasSet* from, AliasSet* to)
      {
         to->set = from->set;
         to->n_aliases = from->n_aliases;
         to->relocated(from);
      }

      void swap(AliasSet& s)
      {
         std::swap(set, s.set);
         std::swap(n_aliases, s.n_aliases);
         relocated(&s);
         s.relocated(this);
      }

      shared_alias_handler* to_handler() { return reverse_cast(this, &shared_alias_handler::al_set); }

   protected:
      void relocated(AliasSet* from)
      {
         if (set) {
            if (is_owner()) {
               for (auto alias_ptr : *this) alias_ptr->owner = this;
            } else {
               AliasSet** it = owner->set->aliases;
               while (*it != from) ++it;
               *it = this;
            }
         }
      }
   };

   AliasSet al_set;

   template <typename Master>
   void CoW(Master* me, Int refc);

   bool preCoW(Int refc) const
   {
      return is_owner() || al_set.owner && refc > al_set.owner->n_aliases+1;
   }

   bool need_postCoW() const
   {
      return al_set.n_aliases > 0;
   }

   template <typename Master>
   void postCoW(Master* me, bool owner_checked = false);

   template <typename Master>
   void divorce_aliases(Master* me)
   {
      me->divorce(al_set.owner->to_handler());
      for (auto alias_ptr : *al_set.owner)
         if (alias_ptr != &al_set) me->divorce(alias_ptr->to_handler());
   }

   bool is_owner() const { return al_set.is_owner(); }

public:
   void make_mutable_alias(shared_alias_handler& owner)
   {
      if (al_set.n_aliases == 0)
         al_set.enter(owner.al_set);
   }

   friend void relocate(shared_alias_handler* from, shared_alias_handler* to)
   {
      relocate(&from->al_set, &to->al_set);
   }

   void swap(shared_alias_handler& s) { al_set.swap(s.al_set); }

   void drop()
   {
      assert(al_set.n_aliases < 0);
      al_set.owner->remove(&al_set);
      al_set.owner = nullptr;
      al_set.n_aliases = 0;
   }
};

class shared_object_secrets {
protected:
   template <typename prefix_type=nothing>
   struct rep {
      // reference counter: number of hosts
      Int refc;
      // number of objects
      std::pair<size_t, prefix_type> size_and_prefix;

      // prevent from being ever destroyed
      rep() : refc(1) { size_and_prefix.first = 0; }

      rep(const prefix_type& p) : refc(1), size_and_prefix(0, p) {}
   };

   static rep<> empty_rep;
};

/** Automatic pointer to shared data

    This class implements an automatic pointer to a data object that can be shared
    among several hosts. The data instance remains shared as far as all hosts it is attached to
    don't attempt to change it (read-only access). The host that wants to get read-write access
    to the data automatically obtains a new copy, while its former partners continue
    sharing the old one.

    In that way, a copy of the host always shares the data instance with the original host.
    Assignment to a host causes it to detach the old data instance and share the new one with
    the host being assigned from.

    The data instance is destroyed together with the last host is is attached to.

    @tmplparam Object type of the attached object
    @index tools Automatic Pointers
*/
template <typename Object, typename... TParams>
class shared_object
   : public mtagged_list_extract<typename mlist_wrap<TParams...>::type, AliasHandlerTag, nop_shared_alias_handler>::type
   , protected shared_object_secrets {
   friend class nop_shared_alias_handler;
   friend class shared_alias_handler;
protected:
   typedef typename mlist_wrap<TParams...>::type params;
   typedef typename mtagged_list_extract<params, AliasHandlerTag, nop_shared_alias_handler>::type alias_handler;
   typedef typename mtagged_list_extract<params, DivorceHandlerTag, nop_divorce_handler>::type divorce_handler;

   static constexpr bool copy_on_write = tagged_list_extract_integral<params, CopyOnWriteTag>(true);

   struct rep {
      /// the attached object itself
      Object obj;
      /// reference counter: number of hosts
      Int refc;
   private:
      static rep* allocate()
      {
         allocator alloc;
         rep* r = (rep*)alloc.allocate(sizeof(rep));
         r->refc = 1;
         return r;
      }

      static void deallocate(rep* r)
      {
         allocator alloc;
         alloc.deallocate(r, sizeof(rep));
      }

      static void empty(shared_object* owner)
      {
         if (owner) {
            rep* r = reverse_cast(&empty_rep.refc, &rep::refc);
            ++r->refc;
            owner->body = r;
         }
      }

      template <typename PointedObject>
      static void destroy(PointedObject* o)
      {
         destroy_at(o);
      }

   public:
      template <typename... Args>
      static rep* init(shared_object* owner, rep* r, std::true_type, Args&&... args)
      {
         construct_at(&r->obj, std::forward<Args>(args)...);
         return r;
      }

      template <typename... Args>
      static rep* init(shared_object* owner, rep* r, std::false_type, Args&&... args)
      {
         try {
            construct_at(&r->obj, std::forward<Args>(args)...);
         } catch (...) {
            deallocate(r);
            empty(owner);
            throw;
         }
         return r;
      }

      template <typename Operation>
      rep* apply(shared_object* owner, const Operation& op)
      {
         rep *r=allocate();
         try {
            op(&r->obj, obj);
         } catch (...) {
            deallocate(r);
            empty(owner);
            throw;
         }
         return r;
      }

      template <typename... Args>
      static rep* construct(shared_object* owner, Args&&... args)
      {
         return init(owner, allocate(), std::is_nothrow_constructible<Object, Args...>(), std::forward<Args>(args)...);
      }

      void destroy()
      {
         destroy(&obj);
      }

      static void destruct(rep *r)
      {
         r->destroy();
         deallocate(r);
      }
   };

   rep *body;
   divorce_handler divorce_hook;

   // Detache the object or delete it.
   void leave()
   {
      if (! --body->refc) rep::destruct(body);
   }

   // Create an own object instance and decrease the reference counter of the old one.
   void divorce()
   {
      --body->refc;
      body=divorce_hook(rep::construct(this, const_cast<const Object&>(body->obj)), std::true_type());
   }

   void divorce(alias_handler* al)
   {
      shared_object* o=static_cast<shared_object*>(al);
      o->body->refc--;
      o->body=body;
      body->refc++;
   }

   shared_object(rep* p) : body(p) {}
public:
   /// Create the attached object with its default constructor.
   shared_object()
      : body(rep::construct(nullptr)) {}

   /// Create the attached object from given arguments.
   template <typename... Args>
   explicit shared_object(Args&&... args)
      : body(rep::construct(nullptr, std::forward<Args>(args)...)) {}

   /// Share the object attached to @c s.
   shared_object(const shared_object& s)
      : alias_handler(s), body(s.body) { ++body->refc; }

   // overrule the greedy constructor template
   shared_object(shared_object& s)
      : shared_object(const_cast<const shared_object&>(s)) {}

   // no moves
   shared_object(shared_object&& s) = delete;

   ~shared_object() { leave(); }

   /// Detach or delete the old instance, share the one attached to @c s.
   shared_object& operator= (const shared_object& s)
   {
      ++s.body->refc;
      leave();
      body=divorce_hook(s.body, std::false_type());
      return *this;
   }

   shared_object& operator= (shared_object&& s) = delete;

   /// Detach or delete the old instance, create a new one from given arguments
   template <typename... Args>
   shared_object& replace(Args&&... args)
   {
      if (__builtin_expect(body->refc>1, 0)) {
         --body->refc;
         body=rep::construct(this, std::forward<Args>(args)...);
      } else {
         body->destroy();
         rep::init(this, body, std::is_nothrow_constructible<Object, Args...>(), std::forward<Args>(args)...);
      }
      return *this;
   }

   /// Enforce an own copy of the given object
   shared_object& assign_copy(const shared_object& s)
   {
      if (__builtin_expect(this != &s, 1)) {
         if (__builtin_expect(body->refc>1, 0)) {
            --body->refc;
            body=divorce_hook(rep::construct(this, const_cast<const Object&>(s.body->obj)), std::true_type());
         } else {
            body->obj=s.body->obj;
         }
      }
      return *this;
   }

   template <typename Operation>
   void apply(const Operation& op)
   {
      if (__builtin_expect(body->refc>1,0)) {
         --body->refc;
         body=divorce_hook(body->apply(this, op), std::true_type());
      } else {
         op(body->obj);
      }
   }

   void swap(shared_object& o)
   {
      static_cast<alias_handler&>(*this).swap(o);
      divorce_hook.swap(o.divorce_hook);
      std::swap(body,o.body);
   }

   friend void relocate(shared_object* from, shared_object* to)
   {
      to->body=from->body;
      relocate(static_cast<alias_handler*>(from), static_cast<alias_handler*>(to));
      relocate(&from->divorce_hook, &to->divorce_hook);
   }

   divorce_handler& get_divorce_handler() const
   {
      return const_cast<divorce_handler&>(divorce_hook);
   }

   static const shared_object* from_divorce_handler(const divorce_handler* h)
   {
      return reverse_cast(h, &shared_object::divorce_hook);
   }

   bool is_shared() const { return body->refc>1; }

   shared_object& enforce_unshared()
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0))
         alias_handler::CoW(this,body->refc);
      return *this;
   }

   Object* operator-> ()
   {
      enforce_unshared();
      return &body->obj;
   }
   Object& operator* ()
   {
      enforce_unshared();
      return body->obj;
   }
   Object* get() const { return &body->obj; }

   const Object* operator-> () const { return &body->obj; }
   const Object& operator* () const { return body->obj; }
};


class shared_array_placement {
public:
   explicit shared_array_placement(void* addr=nullptr)
      : p(addr) {}

   void* get() const { return p; }
   void set(void* addr) { p=addr; }
private:
   void* p;
};


/** Automatic pointer to a shared data array

    It has the same semantics as @see shared_object, with the main difference that
    there are several data objects packed in the array instance that is being shared.

    @tmplparam Object type of the object in the array
*/
template <typename Object, typename... TParams>
class shared_array
   : public mtagged_list_extract<typename mlist_wrap<TParams...>::type, AliasHandlerTag, nop_shared_alias_handler>::type
   , protected shared_object_secrets {
   friend class nop_shared_alias_handler;
   friend class shared_alias_handler;
protected:
   typedef typename mlist_wrap<TParams...>::type params;
   typedef typename mtagged_list_extract<params, AliasHandlerTag, nop_shared_alias_handler>::type alias_handler;
   static const bool copy_on_write=tagged_list_extract_integral<params, CopyOnWriteTag>(true);
   typedef typename mtagged_list_extract<params, PrefixDataTag, nothing>::type prefix_type;
   typedef typename mtagged_list_extract<params, DivorceHandlerTag, nop_divorce_handler>::type divorce_handler;

   /// Attached objects plus housekeeping
   struct rep : shared_object_secrets::rep<prefix_type> {
      /// data objects
      Object obj[1];

      static size_t total_size(size_t n)
      {
         return sizeof(rep)-sizeof(Object)+n*sizeof(Object);
      }
   private:
      typedef shared_object_secrets::rep<prefix_type> super;
      typedef typename std::conditional<std::is_same<prefix_type, nothing>::value, mlist<nothing>, prefix_type>::type prefix_init_arg;

      static rep* allocate(size_t n, const nothing&)
      {
         allocator alloc;
         rep* r = (rep*)alloc.allocate(total_size(n));
         r->refc=1;
         r->size_and_prefix.first=n;
         return r;
      }

      static rep* allocate(size_t n, const prefix_init_arg& p)
      {
         rep* r=allocate(n, nothing());
         construct_at(&r->size_and_prefix.second, p);
         return r;
      }

      static rep* allocate_copy(size_t n, const rep* src)
      {
         return allocate(n, src->size_and_prefix.second);
      }

      static rep* allocate(const shared_array_placement& place, size_t n, const nothing&)
      {
         rep* r = reinterpret_cast<rep*>(place.get());
         r->refc = -std::numeric_limits<Int>::max();     // must always stay negative
         r->size_and_prefix.first = n;
         return r;
      }

      static rep* allocate(const shared_array_placement& place, size_t n, const prefix_init_arg& p)
      {
         rep* r = allocate(place, n, nothing());
         construct_at(&r->size_and_prefix.second, p);
         return r;
      }

      static void deallocate(rep* r)
      {
         if (!std::is_trivially_destructible<prefix_type>::value)
            destroy_at(&r->size_and_prefix.second);
         if (__builtin_expect(r->refc >= 0, 1)) {
            allocator alloc;
            alloc.deallocate(r, total_size(r->size_and_prefix.first));
         }
      }

      static void destroy(Object* end, Object* first)
      {
         if (!std::is_trivially_destructible<Object>::value) {
            while (end > first)
               destroy_at(--end);
         }
      }

      static void empty(shared_array* owner)
      {
         if (owner) owner->body=construct(nullptr, 0);
      }

      // dummy tag for constructors copying values or creating them from scratch
      struct copy { };

      /// initialize all array elements from a given value
      template <typename... Args>
      static void init_from_value(shared_array* owner, rep* r, Object*& dst, Object* end, std::true_type, Args&&... args)
      {
         for (; dst != end; ++dst)
            construct_at(dst, std::forward<Args>(args)...);
      }

      template <typename... Args>
      static void init_from_value(shared_array* owner, rep* r, Object*& dst, Object* end, std::false_type, Args&&... args)
      {
         try {
            init_from_value(owner, r, dst, end, std::true_type(), std::forward<Args>(args)...);
         }
         catch (...) {
            destroy(dst, r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      static void init(shared_array* owner, rep* r, Object* dst, Object* end, copy = copy())
      {
         init_from_value(owner, r, dst, end, std::is_nothrow_default_constructible<Object>());
      }

      template <typename Arg1, typename... Args>
      static
      std::enable_if_t<std::is_constructible<Object, Arg1, Args...>::value &&
                       !(sizeof...(Args)==0 && looks_like_iterator<Arg1>::value)>
      init(shared_array* owner, rep* r, Object* dst, Object* end, Arg1&& arg1, Args&&... args)
      {
         init_from_value(owner, r, dst, end, std::is_nothrow_constructible<Object, Arg1, Args...>(), std::forward<Arg1>(arg1), std::forward<Args>(args)...);
      }

      template <typename Iterator>
      static bool go_on(Object* dst, Object* end, const Iterator& src, std::true_type)
      {
#if POLYMAKE_DEBUG
         if (!src.at_end()) {
            if (end && dst >= end) throw std::runtime_error("input sequence is longer than the allocated storage");
            return true;
         }
         return false;
#else
         return !src.at_end();
#endif
      }

      template <typename Iterator>
      static bool go_on(Object* dst, Object* end, const Iterator& src, std::false_type)
      {
         return dst != end;
      }

      template <typename Iterator>
      static bool go_on(Object* dst, Object* end, const Iterator& src)
      {
         return go_on(dst, end, src, bool_constant<check_iterator_feature<Iterator, end_sensitive>::value>());
      }

      /// Initialize array elements with values from a sequence
      template <typename Iterator>
      static void init_from_sequence(shared_array* owner, rep* r, Object*& dst, Object* end, Iterator&& src,
                                     std::enable_if_t<std::is_nothrow_constructible<Object, decltype(*src)>::value, copy>)
      {
         for (; go_on(dst, end, src); ++src, ++dst)
            construct_at(dst, *src);
      }

      template <typename Iterator>
      static void init_from_sequence(shared_array* owner, rep* r, Object*& dst, Object* end, Iterator&& src,
                                     std::enable_if_t<!std::is_nothrow_constructible<Object, decltype(*src)>::value, copy>)
      {
         try {
            for (; go_on(dst, end, src); ++src, ++dst)
               construct_at(dst, *src);
         }
         catch (...) {
            destroy(dst, r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      template <typename Iterator>
      static
      void init_from_sequence(shared_array* owner, rep* r, Object*& dst, Object* end, Iterator&& src, polymake::operations::move)
      {
         auto&& src_moving=enforce_movable_values(std::forward<Iterator>(src));
         init_from_sequence(owner, r, dst, end, src_moving, copy());
      }

      template <typename Iterator, typename CopyOrMove>
      static
      std::enable_if_t<assess_iterator_value<Iterator, can_initialize, Object>::value>
      init_from_iterator(shared_array* owner, rep* r, Object*& dst, Object* end, Iterator&& src, CopyOrMove)
      {
         init_from_sequence(owner, r, dst, end, std::forward<Iterator>(src), CopyOrMove());
      }

      template <typename Iterator, typename CopyOrMove>
      static
      std::enable_if_t<looks_like_iterator<Iterator>::value && !assess_iterator_value<Iterator, can_initialize, Object>::value>
      init_from_iterator(shared_array* owner, rep* r, Object*& dst, Object* end, Iterator&& src, CopyOrMove)
      {
         for (; go_on(dst, end, src); ++src)
            init_from_iterator(owner, r, dst, nullptr, entire_range<dense>(*src), CopyOrMove());
      }

      template <typename Iterator>
      static
      std::enable_if_t<assess_iterator_value<Iterator, can_assign_to, Object>::value>
      assign_from_iterator(Object*& dst, Object* end, Iterator&& src)
      {
         for (; go_on(dst, end, src); ++src, ++dst)
            *dst = *src;
      }

      template <typename Iterator>
      static
      std::enable_if_t<looks_like_iterator<Iterator>::value && !assess_iterator_value<Iterator, can_assign_to, Object>::value>
      assign_from_iterator(Object*& dst, Object* end, Iterator&& src)
      {
         for (; go_on(dst, end, src); ++src)
            assign_from_iterator(dst, nullptr, entire_range<dense>(*src));
      }

      template <typename Iterator, typename Operation>
      static
      std::enable_if_t<assess_iterator_value<Iterator, isomorphic_types, Object>::value>
      init_from_iterator_with_binop(shared_array* owner, rep* r, Object*& dst, Object* end, const Object*& src_obj, Iterator&& src2, const Operation& op)
      {
         const Object* dst_start=dst;
         init_from_sequence(owner, r, dst, end, make_binary_transform_iterator(src_obj, std::forward<Iterator>(src2), op), copy());
         src_obj += dst-dst_start;
      }

      template <typename Iterator, typename Operation>
      static
      std::enable_if_t<looks_like_iterator<Iterator>::value &&
                       !assess_iterator_value<Iterator, isomorphic_types, Object>::value>
      init_from_iterator_with_binop(shared_array* owner, rep* r, Object*& dst, Object* end, const Object*& src_obj, Iterator&& src2, const Operation& op)
      {
         for (; go_on(dst, end, src2); ++src2)
            init_from_iterator_with_binop(owner, r, dst, nullptr, src_obj, entire_range<dense>(*src2), op);
      }

      template <typename Iterator, typename Operation>
      static
      std::enable_if_t<assess_iterator_value<Iterator, isomorphic_types, Object>::value &&
                       check_iterator_feature<pure_type_t<Iterator>, end_sensitive>::value>
      assign_with_binop(Object*& dst, Object* end, Iterator&& src2, const Operation& op)
      {
         perform_assign(dst, src2, op);
      }

      template <typename Iterator, typename Operation>
      static
      std::enable_if_t<assess_iterator_value<Iterator, isomorphic_types, Object>::value &&
                       !check_iterator_feature<pure_type_t<Iterator>, end_sensitive>::value>
      assign_with_binop(Object*& dst, Object* end, Iterator&& src2, const Operation& op)
      {
         perform_assign(make_iterator_range(dst, end), src2, op);
      }

      template <typename Iterator, typename Operation>
      static
      std::enable_if_t<looks_like_iterator<Iterator>::value &&
                       !assess_iterator_value<Iterator, isomorphic_types, Object>::value>
      assign_with_binop(Object*& dst, Object* end, Iterator&& src2, const Operation& op)
      {
         for (; go_on(dst, end, src2); ++src2)
            assign_with_binop(dst, nullptr, entire_range<dense>(*src2), op);
      }

      template <typename Iterator>
      static
      std::enable_if_t<assess_iterator_value<Iterator, can_initialize, Object>::value>
      init_from_iterator_one_step(shared_array* owner, rep* r, Object*& dst, Iterator&& src)
      {
#if POLYMAKE_DEBUG
         if (!go_on(dst, nullptr, src))
            throw std::runtime_error("shared_array weave error: input sequence ends prematurely");
#endif
         init_from_value(owner, r, dst, dst+1, std::is_nothrow_constructible<Object, decltype(*src)>(), *src);
         ++src;
      }

      template <typename Iterator>
      static
      std::enable_if_t<looks_like_iterator<Iterator>::value && !assess_iterator_value<Iterator, can_initialize, Object>::value>
      init_from_iterator_one_step(shared_array* owner, rep* r, Object*& dst, Iterator& src)
      {
#if POLYMAKE_DEBUG
         if (!go_on(dst, nullptr, src))
            throw std::runtime_error("shared_array weave error: input sequence ends prematurely");
#endif
         init_from_iterator(owner, r, dst, nullptr, entire_range<dense>(*src), copy());
         ++src;
      }

      template <typename... Iterator>
      static constexpr void check_input_iterators(mlist<Iterator...>)
      {
         static_assert(mlist_and<check_iterator_feature<Iterator, end_sensitive>...>::value,
                       "all iterators but the last one must be end-sensitive");
      }

      template <typename CopyOrMove, typename... Iterator>
      static
      std::enable_if_t<is_among<CopyOrMove, copy, polymake::operations::move>::value &&
                       mlist_and_nonempty<looks_like_iterator<Iterator>...>::value>
      init(shared_array* owner, rep* r, Object* dst, Object* end, CopyOrMove, Iterator&&... src)
      {
         check_input_iterators(typename mlist_slice<mlist<Iterator...>, 0, int(sizeof...(Iterator))-1>::type());
         (void)std::initializer_list<bool>{ (init_from_iterator(owner, r, dst, end, std::forward<Iterator>(src), CopyOrMove()), true)... };
#if POLYMAKE_DEBUG
         if (dst && dst != end) throw std::runtime_error("shared_array construction error: input sequence ends prematurely");
#endif
      }

      template <typename... Iterator>
      static
      std::enable_if_t<mlist_and_nonempty<looks_like_iterator<Iterator>...>::value>
      init(shared_array* owner, rep* r, Object* dst, Object* end, Iterator&&... src)
      {
         init(owner, r, dst, end, copy(), std::forward<Iterator>(src)...);
      }

      static rep* construct_empty(std::true_type) PmNoSanitize(object-size)
      {
         return static_cast<rep*>(&empty_rep);
      }
      static rep* construct_empty(std::false_type) PmNoSanitize(object-size)
      {
         static super empty;
         return static_cast<rep*>(&empty);
      }

   public:
      template <typename... Args>
      static rep* construct(shared_array* owner, size_t n, Args&&... args)
      {
         rep* r;
         if (__builtin_expect(n != 0, 1)) {
            r=allocate(n, prefix_type());
            init(owner, r, r->obj, r->obj+n, std::forward<Args>(args)...);
         } else {
            r=construct_empty(std::is_same<prefix_type, nothing>());
            ++r->refc;
         }
         return r;
      }

      template <typename... Args>
      static rep* construct(shared_array* owner, const prefix_type& p, size_t n, Args&&... args)
      {
         rep* r=allocate(n, p);
         init(owner, r, r->obj, r->obj+n, std::forward<Args>(args)...);
         return r;
      }

      template <typename... Args>
      static rep* construct_copy(shared_array* owner, const rep* src, size_t n, Args&&... args)
      {
         rep* r=allocate_copy(n, src);
         init(owner, r, r->obj, r->obj+n, std::forward<Args>(args)...);
         return r;
      }

      template <typename Iterator, typename Operation>
      static rep* construct_copy_with_binop(shared_array* owner, const rep* src, size_t n, Iterator&& src2, const Operation& op)
      {
         rep* r=allocate_copy(n, src);
         Object* dst=r->obj;
         const Object* src_obj=src->obj;
         init_from_iterator_with_binop(owner, r, dst, dst+n, src_obj, std::forward<Iterator>(src2), op);
         return r;
      }

      template <typename Iterator, typename Operation>
      void assign_with_binop(Int n, Iterator&& src2, const Operation& op)
      {
         Object* dst=obj;
         assign_with_binop(dst, dst+n, std::forward<Iterator>(src2), op);
      }

      template <typename... Args>
      static rep* construct(shared_array* owner, const shared_array_placement& place, size_t n, Args&&... args)
      {
         rep* r=allocate(place, n, prefix_type());
         init(owner, r, r->obj, r->obj+n, std::forward<Args>(args)...);
         return r;
      }

      template <typename... Args>
      static rep* construct(shared_array* owner, const shared_array_placement& place, const prefix_type& p, size_t n, Args&&... args)
      {
         rep* r=allocate(place, n, p);
         init(owner, r, r->obj, r->obj+n, std::forward<Args>(args)...);
         return r;
      }

      // relocate or copy
      template <typename... Args>
      static rep* resize(shared_array *owner, rep* old, size_t n, Args&&... args)
      {
         rep* r=allocate_copy(n, old);
         const size_t n_copy=std::min(n, old->size_and_prefix.first);
         Object* dst=r->obj;
         Object* middle=dst+n_copy;
         Object* end=dst+n;
         Object* src_copy=nullptr;
         Object* src_end=nullptr;

         if (old->refc > 0) {
            init_from_sequence(owner, r, dst, middle, ptr_wrapper<const Object, false>(old->obj), copy());
         } else {
            src_copy=old->obj;
            src_end=src_copy+old->size_and_prefix.first;
            for (; dst!=middle;  ++src_copy, ++dst)
               relocate(src_copy, dst);
         }
         init(owner, r, middle, end, std::forward<Args>(args)...);
         if (old->refc <= 0) {
            destroy(src_end, src_copy);
            deallocate(old);
         }

         return r;
      }

      template <typename... Args>
      static rep* weave(shared_array *owner, rep *old, size_t n, size_t slice, Args&&... args)
      {
         rep* r=allocate_copy(n, old);
         Object* dst=r->obj;
         Object* end=dst+n;

         if (old->refc > 0) {
            ptr_wrapper<const Object, false> src_copy(old->obj);
            while (dst != end) {
               init_from_sequence(owner, r, dst, dst+slice, src_copy, copy());
               (void)std::initializer_list<bool>{ (init_from_iterator_one_step(owner, r, dst, args), true)... };
            }
         } else {
            Object* src_copy=old->obj;
            while (dst != end) {
               for (Object* slice_end=dst+slice; dst!=slice_end; ++src_copy, ++dst)
                  relocate(src_copy, dst);
               (void)std::initializer_list<bool>{ (init_from_iterator_one_step(owner, r, dst, args), true)... };
            }
            deallocate(old);
         }
#if POLYMAKE_DEBUG
         (void)std::initializer_list<bool>{ go_on(dst, end, args)... };
#endif
         return r;
      }

      template <typename Value>
      static
      std::enable_if_t<isomorphic_types<Value, Object>::value && can_assign_to<Value, Object>::value>
      assign(Object* dst, Object* end, const Value& val)
      {
         for (; dst != end; ++dst)
            *dst = val;
      }

      template <typename... Iterator>
      static
      std::enable_if_t<mlist_and_nonempty<looks_like_iterator<Iterator>...>::value>
      assign(Object* dst, Object* end, Iterator&&... src)
      {
         check_input_iterators(typename mlist_slice<mlist<Iterator...>, 0, int(sizeof...(Iterator))-1>::type());
         (void)std::initializer_list<bool>{ (assign_from_iterator(dst, end, std::forward<Iterator>(src)), true)... };
#if POLYMAKE_DEBUG
         if (dst && dst != end) throw std::runtime_error("shared_array assign error: input sequence ends prematurely");
#endif
      }

      static void destruct(rep *r)
      {
         destroy(r->obj+r->size_and_prefix.first, r->obj);
         deallocate(r);
      }
   };

   rep *body;
   divorce_handler divorce_hook;

   void leave()
   {
      if (--body->refc<=0) rep::destruct(body);
   }

   void divorce()
   {
      --body->refc;
      const Object* src=body->obj;
      body=divorce_hook(rep::construct(this, body->size_and_prefix.second, body->size_and_prefix.first, src), std::true_type());
   }

   void divorce(alias_handler *al)
   {
      shared_array *o=static_cast<shared_array*>(al);
      o->body->refc--;
      o->body=body;
      body->refc++;
   }
public:
   /// Create an empty array.
   shared_array()
      : body(rep::construct(nullptr, 0)) {}

   explicit shared_array(const prefix_type& p)
      : body(rep::construct(nullptr, p, 0)) {}

   /// Create an array with @a n value-initialized elements
   explicit shared_array(size_t n)
      : body(rep::construct(nullptr, n)) {}

   shared_array(const prefix_type& p, size_t n)
      : body(rep::construct(nullptr, p, n)) {}

   /// Create an array in a preallocated storage
   explicit shared_array(const shared_array_placement& place, size_t n)
      : body(rep::construct(nullptr, place, n)) {}

   shared_array(const shared_array_placement& place, const prefix_type& p, size_t n)
      : body(rep::construct(nullptr, place, p, n)) {}

   /// Create an array with @n elements initialized from given data.
   /// args either can be a list of arguments suitable for construction of a single element,
   /// in which case all elements are constructed identically,
   /// or it can be an iterator over a sequence of input values or nested containers thereof,
   /// in which case the array elements are constructed by copying or conversion, whatever applies.
   /// Note that the input iterator is passed down by reference where it's advanced towards the end of the sequence.
   template <typename... Args>
   shared_array(size_t n, Args&&... args)
      : body(rep::construct(nullptr, n, std::forward<Args>(args)...)) {}

   template <typename... Args>
   shared_array(const prefix_type& p, size_t n, Args&&... args)
      : body(rep::construct(nullptr, p, n, std::forward<Args>(args)...)) {}

   template <typename... Args>
   shared_array(const shared_array_placement& place, size_t n, Args&&... args)
      : body(rep::construct(nullptr, place, n, std::forward<Args>(args)...)) {}

   template <typename... Args>
   shared_array(const shared_array_placement& place, const prefix_type& p, size_t n, Args&&... args)
      : body(rep::construct(nullptr, place, p, n, std::forward<Args>(args)...)) {}

   shared_array(const shared_array& s)
      : alias_handler(s) , body(s.body) { ++body->refc; }

   static size_t alloc_size(Int n) { return rep::total_size(n); }
   
   /// the size of the object
   size_t size() const { return body->size_and_prefix.first; }

   ~shared_array() { leave(); }

   /// Detach or destroy the old array, attach the new one.
   shared_array& operator= (const shared_array& s)
   {
      ++s.body->refc;
      leave();
      body=divorce_hook(s.body, std::true_type());
      return *this;
   }

   void swap(shared_array& o)
   {
      static_cast<alias_handler&>(*this).swap(o);
      divorce_hook.swap(o.divorce_hook);
      std::swap(body,o.body);
   }

   friend void relocate(shared_array* from, shared_array* to)
   {
      to->body=from->body;
      relocate(static_cast<alias_handler*>(from), static_cast<alias_handler*>(to));
      relocate(&from->divorce_hook, &to->divorce_hook);
   }

   void clear()
   {
      if (size()) {
         leave();
         body=rep::construct(nullptr, 0);
      }
   }

   void resize(size_t n)
   {
      if (n != size()) {
         --body->refc;
         body=rep::resize(this, body, n);
      }
   }

   void resize(const shared_array_placement& place, size_t n)
   {
      leave();
      body=rep::construct(this, place, n);
   }

   template <typename... Args>
   void append(size_t n, Args&&... args)
   {
      assert(alias_handler::is_owner());
      if (n) {
         --body->refc;
         body=rep::resize(this, body, n+size(), std::forward<Args>(args)...);
         if (__builtin_expect(alias_handler::need_postCoW(), 0))
            alias_handler::postCoW(this,true);
      }
   }

   template <typename... Args>
   void weave(size_t n, size_t slice, Args&&... args)
   {
      assert(alias_handler::is_owner());
      if (n) {
         --body->refc;
         body=rep::weave(this, body, n+size(), slice, std::forward<Args>(args)...);
         if (__builtin_expect(alias_handler::need_postCoW(), 0))
            alias_handler::postCoW(this,true);
      }
   }

   divorce_handler& get_divorce_handler() const { return const_cast<divorce_handler&>(divorce_hook); }
   static const shared_array* from_divorce_handler(const divorce_handler *h)
   {
      return reverse_cast(h, &shared_array::divorce_hook);
   }

   bool is_shared() const { return body->refc>1; }

   /* Assign the values from an input dense sequence.
      If the attached array instance is shared or its size is not equal to n,
      it is detached rsp. destroyed and a new one is created and initialized from the
      input sequence. Otherwise assigns the input data to the array elements.
   */
   template <typename... Args>
   void assign(size_t n, Args&&... args)
   {
      const bool isCoW = copy_on_write && body->refc>1 && alias_handler::preCoW(body->refc);
      if (isCoW || size() != n) {
         rep* newbody=rep::construct_copy(this, body, n, std::forward<Args>(args)...);
         leave();
         body=newbody;
         if (isCoW) alias_handler::postCoW(this);
      } else {
         rep::assign(body->obj, body->obj+n, std::forward<Args>(args)...);
      }
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0) && alias_handler::preCoW(body->refc)) {
         rep* newbody=rep::construct_copy(this, body, size(), make_unary_transform_iterator(body->obj+0, op));
         leave();
         body=newbody;
         alias_handler::postCoW(this);
      } else {
         perform_assign(make_iterator_range(body->obj+0, body->obj+size()), op);
      }
   }

   template <typename Iterator, typename Operation>
   void assign_op(Iterator&& src2, const Operation& op)
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0) && alias_handler::preCoW(body->refc)) {
         rep *newbody=rep::construct_copy_with_binop(this, body, size(), std::forward<Iterator>(src2), op);
         leave();
         body=newbody;
         alias_handler::postCoW(this);
      } else {
         body->assign_with_binop(size(), std::forward<Iterator>(src2), op);
      }
   }

   shared_array& enforce_unshared()
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0))
         alias_handler::CoW(this,body->refc);
      return *this;
   }

   Object* operator* ()
   {
      enforce_unshared();
      return body->obj;
   }
   const Object* operator* () const { return body->obj; }

   prefix_type& get_prefix()
   {
      return body->size_and_prefix.second;
   }
   const prefix_type& get_prefix() const
   {
      return body->size_and_prefix.second;
   }
   static prefix_type& get_prefix(Object* obj, Int i = 0)
   {
      return reverse_cast(obj, i, &rep::obj)->size_and_prefix.second;
   }
   static const prefix_type& get_prefix(const Object* obj, Int i = 0)
   {
      return reverse_cast(obj, i, &rep::obj)->size_and_prefix.second;
   }
};

template <typename Master>
void shared_alias_handler::CoW(Master* me, Int refc)
{
   if (is_owner()) {
      me->divorce();
      al_set.forget();
   } else if (al_set.owner && refc > al_set.owner->n_aliases+1) {
      me->divorce();
      divorce_aliases(me);
   }
}

template <typename Master>
void shared_alias_handler::postCoW(Master *me, bool owner_checked)
{
   if (owner_checked || is_owner())
      al_set.forget();
   else
      divorce_aliases(me);
}

struct shared_clear {
   template <typename Object>
   void operator() (void *p, const Object&) const { new(p) Object; }
   template <typename Object>
   void operator() (Object& obj) const { obj.clear(); }
};

} // end namespace pm

namespace std {
   template <typename Object, typename... TParams> inline
   void swap(pm::shared_object<Object, TParams...>& s1, pm::shared_object<Object, TParams...>& s2) { s1.swap(s2); }

   template <typename Object, typename... TParams> inline
   void swap(pm::shared_array<Object, TParams...>& a1, pm::shared_array<Object, TParams...>& a2) { a1.swap(a2); }
} // end namespace std

#include "polymake/internal/alias.h"


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
