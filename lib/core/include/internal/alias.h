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

#ifndef POLYMAKE_INTERNAL_ALIAS_H
#define POLYMAKE_INTERNAL_ALIAS_H

#include "polymake/internal/iterators.h"

#include <algorithm>
#include <memory>
#include <cstring>
#include <limits>
#include <cassert>

namespace pm {

template <typename T>
const type2type<T>& make_mutable_alias(T&, T&);
template <typename T> inline
void make_mutable_alias(const T&, const T&) {}

template <typename T, bool _apply=!identical<typename deref<T>::type, typename attrib<T>::minus_const_ref>::value>
struct demasq {
   typedef T type;
   static const bool apply=_apply;
};

template <typename T>
struct demasq<T,true> : T {
   typedef typename inherit_ref<typename object_traits<typename attrib<typename T::type>::minus_const_ref>::masquerade_for, T>::type
      arg_type;
   static const bool apply=true;
};

template <typename T>
struct deserves_special_alias {
   static T& data();
   struct helper {
      static derivation::yes Test(T&);
      static derivation::no Test(const type2type<T>&);
   };
   static const bool value= sizeof(helper::Test( make_mutable_alias(data(),data()) )) == sizeof(derivation::yes);
};

namespace object_classifier {
   enum { alias_obj, alias_primitive, alias_ref, alias_special,
          alias_temporary, alias_based, alias_masqueraded, alias_delayed_masquerade };

   template <typename ObjectRef> struct alias_kind_of;
};

template <typename T, int kind=object_classifier::alias_kind_of<T>::value> class alias;

namespace object_classifier {
   template <typename ObjectRef> 
   struct alias_kind_of {
      typedef typename deref<ObjectRef>::type T;
      static const int value=
         is_masquerade<typename demasq<ObjectRef>::type>::value
         ? (demasq<ObjectRef>::apply ? alias_delayed_masquerade : alias_masqueraded) :
         is_masquerade<ObjectRef>::value
         ? alias_masqueraded :
         deserves_special_alias<T>::value
         ? alias_special :
         (derived_from_instance2i<T,alias>::value && !check_container_feature<T,sparse>::value)
         ? alias_based :
         object_traits<T>::is_temporary
         ? alias_temporary :
         attrib<ObjectRef>::is_reference
         ? alias_ref :
         std::tr1::is_enum<T>::value || (is_pod<T>::value && sizeof(T) <= 2 * sizeof(T*))
         ? alias_primitive
         : alias_obj;
   };
};

template <typename T, int kind>  // kind==alias_obj
class alias {
   // alias to a transient value: keep a copy under a shared pointer
public:
   typedef alias alias_type;
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::plus_ref reference;
   typedef const value_type* const_pointer;
   typedef value_type* pointer;
   typedef const_reference arg_type;

   alias() {}
   alias(arg_type arg) : value(new(alloc.allocate(1)) value_type(arg)) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : value(*other) {}

   reference operator* () { return *value; }
   const_reference operator* () const { return *value; }
   pointer operator-> () { return value.operator->(); }
   const_pointer operator-> () const { return value.operator->(); }

   value_type& get_object() { return *value; }

protected:
   std::allocator<value_type> alloc;
   shared_pointer<value_type> value;

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_primitive> {
   // alias to a constant primitive data item: keep a verbatim copy
public:
   typedef alias alias_type;
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_const_ref reference;
   typedef reference const_reference;
   typedef const value_type* pointer;
   typedef pointer const_pointer;
   typedef typename function_argument<value_type>::type arg_type;

   alias() {}
   alias(arg_type arg) : val(arg) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : val(*other) {}

   const_reference operator* () const { return val; }
   const_pointer operator-> () const { return &val; }
protected:
   value_type val;

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_ref> {
   // alias to a persistent object: keep a reference
public:
   typedef alias alias_type;
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_ref reference;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::minus_ref* pointer;
   typedef const value_type* const_pointer;
   typedef reference arg_type;

   alias() : ptr(0) {}
   alias(arg_type arg) : ptr(&arg) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : ptr(other.ptr) {}

   reference operator* () { return *ptr; }
   const_reference operator* () const { return *ptr; }
   pointer operator-> () { return ptr; }
   const_pointer operator-> () const { return ptr; }

   reference get_object() { return *ptr; }
protected:
   pointer ptr;

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_special> {
   // special alias requires extra treatment in the constructor
public:
   typedef alias alias_type;
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_ref reference;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::minus_ref* pointer;
   typedef const value_type* const_pointer;
   typedef typename function_argument<T>::type arg_type;

   alias() {}
   alias(arg_type arg) : val(arg) { make_mutable_alias(val,arg); }

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : val(other.val) {}

   reference operator* () { return val; }
   const_reference operator* () const { return val; }
   pointer operator-> () { return &val; }
   const_pointer operator-> () const { return &val; }

   value_type& get_object() { return val; }
protected:
   value_type val;

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_temporary> {
   // alias to a temporary object: keep an internal copy
public:
   typedef alias alias_type;
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename if_else<object_traits<value_type>::is_always_const, const_reference, typename attrib<T>::plus_ref>::type reference;
   typedef const value_type* const_pointer;
   typedef typename if_else<attrib<reference>::is_const, const_pointer, typename attrib<T>::minus_ref*>::type pointer;
   typedef const_reference arg_type;

   alias() : init(false) {}
   alias(arg_type arg) : init(true)
   {
      // if the constructor throws an exception, alias' destructor won't be called, hence it's safe to set init=true up front
      new(allocate()) value_type(arg);
   }

   alias(const alias& other) : init(other.init)
   {
      if (init) new(allocate()) value_type(*other);
   }

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;

   alias(alt_alias_arg_type& other) : init(other.init)
   {
      if (init) new(allocate()) value_type(*other);
   }

   alias& operator= (const alias& other)
   {
      if (this != &other) {
         if (init) {
            std::_Destroy(ptr());
            init=false;
         }
         if (other.init) {
            new(allocate()) value_type(*other);
            init=true;
         }
      }
      return *this;
   }

   ~alias() { if (init) std::_Destroy(ptr()); }

   reference operator* () { return *ptr(); }
   const_reference operator* () const { return *ptr(); }
   pointer operator-> () { return ptr(); }
   const_pointer operator-> () const { return ptr(); }

   value_type& get_object() { return *ptr(); }

protected:
   POLYMAKE_ALIGN(char area[sizeof(value_type)], 8);
   bool init;

   void* allocate() { return area; }
   value_type*       ptr()       { return reinterpret_cast<value_type*>(area); }
   const value_type* ptr() const { return reinterpret_cast<const value_type*>(area); }

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_based>
   : public deref<T>::type::alias_type {
   typedef typename deref<T>::type::alias_type _super;
public:
   typedef typename attrib<T>::minus_const_ref value_type;
   typedef typename attrib<T>::plus_ref reference;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename attrib<T>::minus_ref* pointer;
   typedef const value_type* const_pointer;
   typedef const_reference arg_type;

   alias() {}
   alias(typename _super::arg_type arg) : _super(arg) {}
   alias(arg_type arg) : _super(arg) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : _super(other) {}

   reference operator* () { return static_cast<reference>(static_cast<_super&>(*this)); }
   const_reference operator* () const { return static_cast<const_reference>(static_cast<const _super&>(*this)); }
   pointer operator-> () { return &operator*(); }
   const_pointer operator-> () const { return &operator*(); }

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_masqueraded>
   : public alias<typename is_masquerade<T>::hidden_stored_type> {
   // masquerade: keep an alias to a hidden type
   typedef alias<typename is_masquerade<T>::hidden_stored_type> _super;
public:
   typedef alias alias_type;
   typedef typename attrib<T>::plus_const_ref const_reference;
   typedef typename if_else<(object_traits<typename is_masquerade<T>::hidden_type>::is_always_const ||
                             object_traits<typename attrib<T>::minus_const_ref>::is_always_const),
                            const_reference, typename attrib<T>::plus_ref>::type
      reference;
   typedef const typename attrib<T>::minus_ref* const_pointer;
   typedef typename if_else<attrib<reference>::is_const, const_pointer, typename attrib<T>::minus_ref*>::type pointer;
   typedef reference arg_type;

   alias() {}
   alias(arg_type arg)
      : _super(reinterpret_cast<typename is_masquerade<T>::hidden_stored_type>(arg)) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : _super(other) {}

   reference operator* () { return reinterpret_cast<reference>(_super::operator*()); }
   const_reference operator* () const { return reinterpret_cast<const_reference>(_super::operator*()); }
   pointer operator-> () { return &operator*(); }
   const_pointer operator-> () const { return &operator*(); }

   template <typename,int> friend class alias;
};

template <typename T>
class alias<T, object_classifier::alias_delayed_masquerade>
   : public alias<typename demasq<T>::arg_type> {
   // delayed masquerade: keep an alias to a given source type
   typedef typename demasq<T>::type M;
   typedef alias<typename demasq<T>::arg_type> _super;
public:
   typedef alias alias_type;
   typedef typename attrib<M>::plus_const_ref const_reference;
   typedef typename if_else<(object_traits<typename is_masquerade<M>::hidden_type>::is_always_const ||
                             object_traits<typename attrib<M>::minus_const_ref>::is_always_const),
                            const_reference, typename attrib<M>::plus_ref>::type
      reference;
   typedef const typename attrib<M>::minus_ref* const_pointer;
   typedef typename if_else<attrib<reference>::is_const, const_pointer, typename attrib<M>::minus_ref*>::type pointer;

   alias() {}
   alias(typename _super::arg_type arg) : _super(arg) {}

   typedef typename if_else<attrib<T>::is_const, const alias<typename attrib<T>::minus_const>, type2type<T> >::type alt_alias_arg_type;
   alias(alt_alias_arg_type& other) : _super(other) {}

   reference operator* () { return reinterpret_cast<reference>(_super::operator*()); }
   const_reference operator* () const { return reinterpret_cast<const_reference>(_super::operator*()); }
   pointer operator-> () { return &operator*(); }
   const_pointer operator-> () const { return &operator*(); }

   template <typename,int> friend class alias;
};

// keeps nothing
template <>
class alias<nothing, object_classifier::alias_obj>
{
public:
   typedef alias alias_type;
   typedef const nothing& reference;
   typedef reference const_reference;
   typedef const nothing* pointer;
   typedef pointer const_pointer;
   typedef reference arg_type;

   alias() {}
   alias(arg_type) {}

   const_reference operator* () const { return std::pair<nothing,nothing>::first; }
   const_pointer operator-> () const { return &operator*(); }
};

template <typename T, int kind>
struct function_argument< alias<T,kind> > {
   typedef typename alias<T,kind>::arg_type type;
};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_SHARED_OBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
