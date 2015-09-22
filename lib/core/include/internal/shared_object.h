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

#ifndef POLYMAKE_INTERNAL_SHARED_OBJECT_H
#define POLYMAKE_INTERNAL_SHARED_OBJECT_H

#include "polymake/internal/iterators.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <limits>
#include <cassert>

namespace pm {

template <typename Ftype> class constructor;

template <typename> class CopyOnWrite;
template <typename> class Allocator;
template <typename> class PrefixData;
template <typename> class AliasHandler;
template <typename> class DivorceHandler;

class nop_shared_alias_handler {
protected:
   template <typename Master> static
   void CoW(Master *me, int)
   {
      me->divorce();
   }

   static bool preCoW(int) { return false; }
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
   Rep* operator() (Rep *body, bool2type<copied>) const { return body; }
   void swap(nop_divorce_handler&) {}
   friend void relocate(nop_divorce_handler*, nop_divorce_handler*) {}
};

class shared_alias_handler {
protected:
   struct AliasSet {
      struct alias_array {
         int n_alloc;
         AliasSet* aliases[1];
      };
      union {
         alias_array *set;
         AliasSet *owner;
      };
      long n_aliases;

      static alias_array* allocate(size_t n)
      {
         allocator alloc;
         alias_array *a=(alias_array*)alloc.allocate(sizeof(alias_array)+(n-1)*sizeof(AliasSet*));
         a->n_alloc=n;
         return a;
      }
      static alias_array* reallocate(alias_array *a)
      {
         alias_array *n=allocate(a->n_alloc+3);
         std::memcpy(n->aliases, a->aliases, a->n_alloc*sizeof(AliasSet*));
         deallocate(a);
         return n;
      }
      static void deallocate(alias_array *a)
      {
         allocator alloc;
         alloc.deallocate(reinterpret_cast<allocator::value_type*>(a),sizeof(alias_array)+(a->n_alloc-1)*sizeof(AliasSet*));
      }

      void remove(AliasSet *alias)
      {
         --n_aliases;
         for (AliasSet **s=set->aliases, **end=s+n_aliases; s<end; ++s)
            if (*s==alias) {
               *s=set->aliases[n_aliases];
               break;
            }
      }

      void forget()
      {
         for (AliasSet **s=set->aliases, **end=s+n_aliases; s<end; ++s) (*s)->owner=NULL;
         n_aliases=0;
      }

      AliasSet() : set(0), n_aliases(0) {}

      ~AliasSet()
      {
         if (set != NULL) {
            if (n_aliases>=0) {
               forget();
               deallocate(set);
            } else {
               owner->remove(this);
            }
         }
      }

      AliasSet(const AliasSet& s2)
      {
         if (__builtin_expect(s2.n_aliases<0, 0)) {
            if (s2.owner != NULL) {
               enter(*s2.owner);
            } else {
               // even if the original owner has gone, we can still copy the aliases
               n_aliases=-1;  owner=NULL;
            }
         } else {
            set=NULL; n_aliases=0;
         }
      }

   private:
      void operator=(const AliasSet&);
   public:
      void enter(AliasSet& ow)
      {
         n_aliases=-1;
         owner=&ow;
         if (!ow.set)
            ow.set=allocate(3);
         else if (ow.n_aliases==ow.set->n_alloc)
            ow.set=reallocate(ow.set);
         ow.set->aliases[ow.n_aliases++]=this;
      }

      bool is_owner() const { return n_aliases>=0; }

      typedef iterator_range<AliasSet**> iterator;
      iterator begin() const
      {
         return iterator(set->aliases, set->aliases+n_aliases);
      }

      friend void relocate(AliasSet *from, AliasSet *to)
      {
         to->set=from->set;
         to->n_aliases=from->n_aliases;
         to->relocated(from);
      }

      void swap(AliasSet& s)
      {
         std::swap(set,s.set);
         std::swap(n_aliases,s.n_aliases);
         relocated(&s);
         s.relocated(this);
      }

      shared_alias_handler* to_handler() { return reverse_cast(this, &shared_alias_handler::al_set); }

   protected:
      void relocated(AliasSet *from)
      {
         if (set) {
            if (is_owner()) {
               for (iterator it(set->aliases, set->aliases+n_aliases); !it.at_end(); ++it)
                  (*it)->owner=this;
            } else {
               AliasSet **it=owner->set->aliases;
               while (*it!=from) ++it;
               *it=this;
            }
         }
      }
   };

   AliasSet al_set;

   template <typename Master>
   void CoW(Master *me, long refc);

   bool preCoW(long refc) const
   {
      return is_owner() || al_set.owner && refc > al_set.owner->n_aliases+1;
   }

   bool need_postCoW() const
   {
      return al_set.n_aliases>0;
   }

   template <typename Master>
   void postCoW(Master *me, bool owner_checked=false);

   template <typename Master>
   void divorce_aliases(Master *me)
   {
      me->divorce(al_set.owner->to_handler());
      for (AliasSet::iterator it=al_set.owner->begin(); !it.at_end(); ++it)
         if (*it!=&al_set) me->divorce((*it)->to_handler());
   }

   bool is_owner() const { return al_set.is_owner(); }

public:
   void make_mutable_alias(shared_alias_handler& owner)
   {
      if (!al_set.n_aliases) al_set.enter(owner.al_set);
   }

   friend void relocate(shared_alias_handler *from, shared_alias_handler *to)
   {
      relocate(&from->al_set, &to->al_set);
   }

   void swap(shared_alias_handler& s) { al_set.swap(s.al_set); }

   void drop()
   {
      assert(al_set.n_aliases<0);
      al_set.owner->remove(&al_set); al_set.owner=NULL; al_set.n_aliases=0;
   }
};

class shared_object_secrets {
protected:
   template <typename prefix_type=nothing>
   struct rep {
      // reference counter: number of hosts
      long refc;
      // number of objects
      std::pair<size_t, prefix_type> size_and_prefix;

      // prevent from being ever destroyed
      rep() : refc(1) { size_and_prefix.first=0; }

      rep(const prefix_type& p) : refc(1), size_and_prefix(0,p) {}
   };

   static rep<> empty_rep;
};

class shared_pointer_secrets {
protected:
   struct rep {
      void *obj;
      long refc;

      // prevent from being ever destroyed
      rep() : obj(0), refc(1) {}
   };

   static rep null_rep;
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
template <typename Object, typename Params=void>
class shared_object
   : public extract_param<Params, AliasHandler<nop_shared_alias_handler> >::type,
     protected shared_object_secrets {
   friend class nop_shared_alias_handler;
   friend class shared_alias_handler;
protected:
   typedef typename extract_param<Params, AliasHandler<nop_shared_alias_handler> >::type alias_handler;
   typedef typename extract_param<Params, DivorceHandler<nop_divorce_handler> >::type divorce_handler;
   typedef typename extract_param<Params, Allocator< std::allocator<Object> > >::type Alloc;

   static const bool copy_on_write=extract_bool_param<Params, CopyOnWrite, true>::value;

   struct rep {
      /// the attached object itself
      Object obj;
      /// reference counter: number of hosts
      long refc;
   private:
      typedef typename Alloc::template rebind<rep>::other RepAlloc;

      static rep* allocate()
      {
         RepAlloc alloc;
         rep *r=(rep*)alloc.allocate(1);
         r->refc=1;
         return r;
      }

      static void deallocate(rep *r)
      {
         RepAlloc alloc;
         alloc.deallocate(r,1);
      }

      static void empty(shared_object *owner)
      {
         if (owner) {
            rep* r=reverse_cast(&empty_rep.refc, &rep::refc);
            ++r->refc;
            owner->body=r;
         }
      }

      template <typename PointedObject>
      static void destroy(PointedObject *o) { std::_Destroy(o); }

      template <typename PointedObject>
      static void destroy(PointedObject** o)
      {
         Alloc alloc;
         std::_Destroy(*o);
         alloc.deallocate(*o, 1);
      }

   public:
      static rep* init(rep *r, const Object& o, shared_object *owner)
      {
         try {
            new(&r->obj) Object(o);
         } catch (...) {
            deallocate(r);
            empty(owner);
            throw;
         }
         return r;
      }

      template <typename Fptr>
      static rep* init(rep *r, const constructor<Fptr>& c, shared_object *owner)
      {
         try {
            c(&r->obj);
         } catch (...) {
            deallocate(r);
            empty(owner);
            throw;
         }
         return r;
      }

      template <typename Operation>
      rep* apply(const Operation& op, shared_object *owner)
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

      template <typename Arg>
      static rep* construct(const Arg& arg, shared_object* owner=NULL)
      {
         return init(allocate(), arg, owner);
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
      body=divorce_hook(rep::construct(body->obj, this), True());
   }

   void divorce(alias_handler *al)
   {
      shared_object *o=static_cast<shared_object*>(al);
      o->body->refc--;
      o->body=body;
      body->refc++;
   }

   shared_object(rep* p) : body(p) {}
public:
   /// Create the attached object with its default constructor.
   shared_object()
      : body(rep::construct(constructor<Object()>())) {}

   // Create the attached object with its copy constructor.
   shared_object(const Object& obj_arg)
      : body(rep::construct(obj_arg)) {}

   template <typename Fptr>
   shared_object(const constructor<Fptr>& c)
      : body(rep::construct(c)) {}

   // Share the object attached to @c s.
   shared_object(const shared_object& s)
      : alias_handler(s), body(s.body) { ++body->refc; }

   ~shared_object() { leave(); }

   // Detach or delete the old instance, shares the one attached to @c s.
   shared_object& operator= (const shared_object& s)
   {
      ++s.body->refc;
      leave();
      body=divorce_hook(s.body, False());
      return *this;
   }

   template <typename Fptr>
   shared_object& operator= (const constructor<Fptr>& c)
   {
      if (__builtin_expect(body->refc>1,0)) {
         --body->refc;
         body=rep::construct(c, this);
      } else {
         body->destroy();
         rep::init(body, c, this);
      }
      return *this;
   }

   shared_object& assign_copy(const shared_object& s)
   {
      if (__builtin_expect(body->refc>1,0)) {
         --body->refc;
         body=divorce_hook(rep::construct(s.body->obj, this), True());
      } else {
         body->obj=s.body->obj;
      }
      return *this;
   }

   template <typename Operation>
   void apply(const Operation& op)
   {
      if (__builtin_expect(body->refc>1,0)) {
         --body->refc;
         body=divorce_hook(body->apply(op,this), True());
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

   divorce_handler& get_divorce_handler() const { return const_cast<divorce_handler&>(divorce_hook); }
   static const shared_object* from_divorce_handler(const divorce_handler *h)
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

/** Like in boost, but not MT-safe (and hence without overhead) */
template <typename Object, typename Params=void>
class shared_pointer
   : public shared_object<Object*, typename append_params<Params, list(CopyOnWrite<False>, Allocator< std::allocator<Object> >)>::type>,
     protected shared_pointer_secrets {
   typedef shared_object<Object*, typename append_params<Params, list(CopyOnWrite<False>, Allocator< std::allocator<Object> >)>::type> super;
   typedef typename super::rep rep;
public:
   shared_pointer()
      : super(reinterpret_cast<rep*>(&null_rep)) { ++this->body->refc; }

   shared_pointer(Object *o)
      : super(o) {}

   shared_pointer& operator= (Object* o)
   {
      if (__builtin_expect(o != NULL, 1)) {
         if (this->body->refc==1) {
            this->body->destroy();
            this->body->obj=o;
         } else {
            this->body->refc--;
            this->body=rep::construct(o);
         }
      } else {
         this->leave();
         this->body=reinterpret_cast<rep*>(&null_rep);
         ++this->body->refc;
      }
      return *this;
   }

   using super::swap;

   shared_pointer& swap(Object* &op)
   {
      Object *o=this->body->obj;
      if (__builtin_expect(this->body->refc==1, 1)) {
         if (__builtin_expect(op != NULL, 1)) {
            if (o) this->body->obj=op;
            else this->body=rep::construct(op);
         } else if (o) {
            rep::destruct(this->body);
            this->body=reinterpret_cast<rep*>(&null_rep);
         }
      } else {
         this->body->refc--;
         this->body= op ? rep::construct(op) : reinterpret_cast<rep*>(&null_rep);
      }
      op=o;
      return *this;
   }

   Object& operator* () { return *this->body->obj; }
   const Object& operator* () const { return *this->body->obj; }
   Object* operator-> () { return this->body->obj; }
   const Object* operator-> () const { return this->body->obj; }

   operator Object* () { return this->body->obj; }
   operator const Object* () const { return this->body->obj; }

   bool operator== (const shared_pointer& other) const { return this->body->obj == other.body->obj; }
   bool operator!= (const shared_pointer& other) const { return this->body->obj != other.body->obj; }

   template <typename OtherObject>
   typename enable_if< shared_pointer<OtherObject>&, derived_from<OtherObject,Object>::value ||
                                                     derived_from<Object,OtherObject>::value>::type
   cast()
   {
      return reinterpret_cast<shared_pointer<OtherObject>&>(*this);
   }

   template <typename OtherObject>
   typename enable_if<const shared_pointer<OtherObject>&, derived_from<OtherObject,Object>::value ||
                                                          derived_from<Object,OtherObject>::value>::type
   cast() const
   {
      return reinterpret_cast<const shared_pointer<OtherObject>&>(*this);
   }

   static shared_pointer new_object()
   {
      typename extract_type_param<Params, Allocator, std::allocator<Object> >::type alloc;
      return new(alloc.allocate(1)) Object;
   }
};

template <typename T, typename Params>
struct deref_ptr< shared_pointer<T, Params> > : deref_ptr<T> {};

template <typename T, typename Params>
struct deref_ptr< const shared_pointer<T, Params> > : deref_ptr<const T> {};

/** Automatic pointer to a shared data array

    It has the same semantics as @see shared_object, with the main difference that
    there are several data objects packed in the array instance that is being shared.

    @tmplparam Object type of the object in the array
*/
template <typename Object, typename Params=void>
class shared_array
   : public extract_param<Params, AliasHandler<nop_shared_alias_handler> >::type,
     protected shared_object_secrets {
   friend class nop_shared_alias_handler;
   friend class shared_alias_handler;
protected:
   typedef typename extract_param<Params, AliasHandler<nop_shared_alias_handler> >::type alias_handler;
   static const bool copy_on_write=extract_bool_param<Params, CopyOnWrite, true>::value;
   typedef typename extract_type_param<Params, PrefixData, nothing>::type prefix_type;
   typedef typename extract_param<Params, DivorceHandler<nop_divorce_handler> >::type divorce_handler;

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

      static rep* allocate(size_t n)
      {
         allocator alloc;
         rep *r=(rep*)alloc.allocate(total_size(n));
         r->refc=1;
         r->size_and_prefix.first=n;
         return r;
      }

      static rep* allocate(size_t n, const prefix_type& p)
      {
         rep *r=allocate(n);
         new(&r->size_and_prefix.second) prefix_type(p);
         return r;
      }

      static rep* allocate_copy(size_t n, const rep*, True)
      {
         return allocate(n);
      }

      static rep* allocate_copy(size_t n, const rep *old, False)
      {
         return allocate(n, old->size_and_prefix.second);
      }

      static rep* allocate(void *place, size_t n)
      {
         rep *r=reinterpret_cast<rep*>(place);
         r->refc=-std::numeric_limits<long>::max();     // must always stay negative
         r->size_and_prefix.first=n;
         return r;
      }

      static rep* allocate(void *place, size_t n, const prefix_type& p)
      {
         rep *r=allocate(place,n);
         new(&r->size_and_prefix.second) prefix_type(p);
         return r;
      }

      static void deallocate(rep *r)
      {
         if (!has_trivial_destructor<prefix_type>::value)
            std::_Destroy(&r->size_and_prefix.second);
         if (__builtin_expect(r->refc>=0, 1)) {
            allocator alloc;
            alloc.deallocate(reinterpret_cast<allocator::value_type*>(r),total_size(r->size_and_prefix.first));
         }
      }

      static void destroy(Object *end, Object *first)
      {
         if (!has_trivial_destructor<Object>::value)
            while (end > first)
               std::_Destroy(--end);
      }

      static void empty(shared_array *owner)
      {
         if (owner) owner->body=construct();
      }

      /// Initialize array elements with a constructor
      template <typename Ftpr>
      static Object* init(rep *r, Object *dst, Object *end, const constructor<Ftpr>& c, shared_array *owner)
      {
         try {
            for (; dst != end; ++dst)
               c(dst);
            return dst;
         }
         catch (...) {
            destroy(dst,r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      /// Initialize array elements with values from a sequence
      template <typename Iterator>
      static Object* init(rep *r, Object *dst, Object *end, Iterator src, shared_array *owner)
      {
         try {
            for (; dst != end; ++src, ++dst)
               new(dst) Object(*src);
            return dst;
         }
         catch (...) {
            destroy(dst,r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      template <typename Iterator>
      static Object* init(rep *r, Object *dst, const Iterator& src, True, shared_array *owner)
      {
         try {
            new(dst) Object(*src);
            ++dst;
            return dst;
         }
         catch (...) {
            destroy(dst,r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      template <typename Iterator>
      static Object* init(rep *r, Object *dst, const Iterator& src, False, shared_array *owner)
      {
         try {
            typedef typename ensure_features<typename iterator_traits<Iterator>::value_type,
                                             cons<end_sensitive, dense> >::const_iterator
               sub_iterator;
            for (sub_iterator it=ensure(*src, (cons<end_sensitive, dense>*)0).begin();
                 !it.at_end(); ++it)
               dst=init(r, dst, it, bool2type<isomorphic_types<Object, typename iterator_traits<sub_iterator>::value_type>::value>(), owner);
            return dst;
         }
         catch (...) {
            destroy(dst,r->obj);
            deallocate(r);
            empty(owner);
            throw;
         }
      }

      static rep* construct_empty(True)
      {
         return static_cast<rep*>(&empty_rep);
      }
      static rep* construct_empty(False)
      {
         static super *e=new(allocate(0)) super;
         return static_cast<rep*>(e);
      }

   public:
      static rep* construct()
      {
         rep *r=construct_empty(identical<prefix_type,nothing>());
         ++r->refc;
         return r;
      }

      static rep* construct(const prefix_type& p)
      {
         return allocate(0,p);
      }

      template <typename Init>
      static rep* construct(size_t n, const Init& src, shared_array* owner=NULL)
      {
         rep *r=allocate(n);
         init(r, r->obj, r->obj+n, src, owner);
         return r;
      }

      template <typename Init>
      static rep* construct(const prefix_type& p, size_t n, const Init& src, shared_array* owner=NULL)
      {
         rep *r=allocate(n,p);
         init(r, r->obj, r->obj+n, src, owner);
         return r;
      }

      template <typename Init>
      static rep* construct_copy(size_t n, const Init& src, const rep *old, shared_array* owner=NULL)
      {
         rep *r=allocate_copy(n,old,identical<prefix_type,nothing>());
         init(r, r->obj, r->obj+n, src, owner);
         return r;
      }

      template <typename Init>
      static rep* construct(void *place, size_t n, const Init& src, shared_array* owner=NULL)
      {
         rep *r=allocate(place,n);
         init(r, r->obj, r->obj+n, src, owner);
         return r;
      }

      template <typename Init>
      static rep* construct(void *place, const prefix_type& p, size_t n, const Init& src, shared_array* owner=NULL)
      {
         rep *r=allocate(place,n,p);
         init(r, r->obj, r->obj+n, src, owner);
         return r;
      }

      // relocate or copy
      template <typename Init>
      static rep* resize(size_t n, rep* old, const Init& src, shared_array *owner)
      {
         rep *r=allocate_copy(n,old,identical<prefix_type,nothing>());
         const size_t n_copy=std::min(n,old->size_and_prefix.first);
         Object *dst=r->obj, *middle=dst+n_copy, *end=dst+n;
			Object *src_copy = NULL, *src_end = NULL;

         if (old->refc > 0) {
            init(r, dst, middle, const_cast<const Object*>(old->obj), owner);
         } else {
            src_copy=old->obj, src_end=src_copy+old->size_and_prefix.first;
            for (; dst!=middle;  ++src_copy, ++dst)
               relocate(src_copy,dst);
         }
         init(r, middle, end, src, owner);
			if (old->refc <= 0) {
            destroy(src_end, src_copy);
				deallocate(old);
			}

         return r;
      }

      template <typename Iterator>
      static rep* weave(size_t n, size_t slice, rep *old, Iterator& src, shared_array *owner)
      {
         rep *r=allocate_copy(n,old,identical<prefix_type,nothing>());
         Object *dst=r->obj, *end=dst+n;

         if (old->refc > 0) {
            const Object *src_copy=old->obj;
            while (dst != end) {
               dst=init(r, dst, dst+slice, src_copy, owner);
               src_copy+=slice;
               dst=init(r, dst, src, bool2type<isomorphic_types<Object, typename iterator_traits<Iterator>::value_type>::value>(), owner);
               ++src;
            }
         } else {
            Object *src_copy=old->obj;
            while (dst != end) {
               for (Object *slice_end=dst+slice; dst!=slice_end; ++src_copy, ++dst)
                  relocate(src_copy,dst);
               dst=init(r, dst, src, bool2type<isomorphic_types<Object, typename iterator_traits<Iterator>::value_type>::value>(), owner);
               ++src;
            }
            deallocate(old);
         }
         return r;
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

   void divorce(False)
   {
      --body->refc;
      const Object *src=body->obj;
      body=divorce_hook(rep::construct(body->size_and_prefix.second, body->size_and_prefix.first, src, this), True());
   }

   void divorce(True)
   {
      --body->refc;
      const Object *src=body->obj;
      body=divorce_hook(rep::construct(body->size_and_prefix.first, src, this), True());
   }

   void divorce() { divorce(identical<prefix_type,nothing>()); }

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
      : body(rep::construct()) {}

   explicit shared_array(const prefix_type& p)
      : body(rep::construct(p)) {}

   explicit shared_array(size_t n)
      : body(rep::construct(n, constructor<Object()>())) {}

   shared_array(const prefix_type& p, size_t n)
      : body(rep::construct(p, n, constructor<Object()>())) {}

   explicit shared_array(void *place, size_t n)
      : body(rep::construct(place, n, constructor<Object()>())) {}

   shared_array(void *place, const prefix_type& p, size_t n)
      : body(rep::construct(place, p, n, constructor<Object()>())) {}

   template <typename Iterator>
   shared_array(size_t n, const Iterator& src)
      : body(rep::construct(n, src)) {}

   template <typename Iterator>
   shared_array(const prefix_type& p, size_t n, const Iterator& src)
      : body(rep::construct(p, n, src)) {}

   template <typename Iterator>
   shared_array(void *place, size_t n, const Iterator& src)
      : body(rep::construct(place, n, src)) {}

   template <typename Iterator>
   shared_array(void *place, const prefix_type& p, size_t n, const Iterator& src)
      : body(rep::construct(place, p, n, src)) {}

   shared_array(const shared_array& s)
      : alias_handler(s), body(s.body) { ++body->refc; }

   static size_t alloc_size(int n) { return rep::total_size(n); }
   
   /// the size of the object
   size_t size() const { return body->size_and_prefix.first; }

   ~shared_array() { leave(); }

   /// Detach or destroy the old array, attach the new one.
   shared_array& operator= (const shared_array& s)
   {
      ++s.body->refc;
      leave();
      body=divorce_hook(s.body, True());
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
         body=rep::construct();
      }
   }

   void resize(size_t n)
   {
      if (n != body->size_and_prefix.first) {
         --body->refc;
         body=rep::resize(n,body,constructor<Object()>(),this);
      }
   }

   void resize(void *place, size_t n)
   {
      leave();
      body=rep::construct(place, n, constructor<Object()>(),this);
   }

   template <typename Iterator>
   void append(size_t n, Iterator src)
   {
      assert(alias_handler::is_owner());
      if (n) {
         --body->refc;
         body=rep::resize(n+body->size_and_prefix.first,body,src,this);
         if (__builtin_expect(alias_handler::need_postCoW(),0))
            alias_handler::postCoW(this,true);
      }
   }

   template <typename Iterator>
   void weave(size_t n, size_t slice, Iterator src)
   {
      assert(alias_handler::is_owner());
      if (n) {
         --body->refc;
         body=rep::weave(n+body->size_and_prefix.first,slice,body,src,this);
         if (__builtin_expect(alias_handler::need_postCoW(),0))
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
   template <typename Iterator>
   void assign(size_t n, Iterator src)
   {
      const bool CoW=copy_on_write && body->refc>1 && alias_handler::preCoW(body->refc);
      if (CoW || body->size_and_prefix.first != n) {
         rep *newbody=rep::construct_copy(n,src,body);
         leave();
         body=newbody;
         if (CoW) alias_handler::postCoW(this);
      } else {
         for (Object *dst=body->obj, *end=dst+n;  dst!=end;  ++src, ++dst)
            *dst = *src;
      }
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0) && alias_handler::preCoW(body->refc)) {
         rep *newbody=rep::construct_copy(body->size_and_prefix.first, make_unary_transform_iterator(body->obj,op), body);
         leave();
         body=newbody;
         alias_handler::postCoW(this);
      } else {
         perform_assign(make_iterator_range(body->obj, body->obj+body->size_and_prefix.first), op);
      }
   }

   template <typename Iterator, typename Operation>
   void assign_op(Iterator src2, const Operation& op)
   {
      if (copy_on_write && __builtin_expect(body->refc>1,0) && alias_handler::preCoW(body->refc)) {
         rep *newbody=rep::construct_copy(body->size_and_prefix.first, make_binary_transform_iterator(body->obj,src2,op), body);
         leave();
         body=newbody;
         alias_handler::postCoW(this);
      } else {
         perform_assign(make_iterator_range(body->obj, body->obj+body->size_and_prefix.first), src2, op);
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
   static prefix_type& get_prefix(Object* obj, int i=0)
   {
      return reverse_cast(obj, i, &rep::obj)->size_and_prefix.second;
   }
   static const prefix_type& get_prefix(const Object* obj, int i=0)
   {
      return reverse_cast(obj, i, &rep::obj)->size_and_prefix.second;
   }
};

template <typename Master>
void shared_alias_handler::CoW(Master *me, long refc)
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
   template <typename Object, typename Params> inline
   void swap(pm::shared_object<Object,Params>& s1, pm::shared_object<Object,Params>& s2) { s1.swap(s2); }

   template <typename Object, typename Params> inline
   void swap(pm::shared_array<Object,Params>& a1, pm::shared_array<Object,Params>& a2) { a1.swap(a2); }
} // end namespace std

#include "polymake/internal/alias.h"
#include "polymake/internal/constructors.h"

#endif // POLYMAKE_INTERNAL_SHARED_OBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
