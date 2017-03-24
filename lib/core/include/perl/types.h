/* Copyright (c) 1997-2017
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

#ifndef POLYMAKE_PERL_TYPES_H
#define POLYMAKE_PERL_TYPES_H

namespace pm { namespace perl {

typedef SV* (*wrapper_type)(SV**);
typedef SV* (*indirect_wrapper_type)(void*, SV**);
typedef void (*destructor_type)(char*);
typedef void (*copy_constructor_type)(void*, const char*);
typedef void (*assignment_type)(char*, SV*, value_flags);
typedef SV* (*conv_to_string_type)(const char*);
typedef int (*conv_to_int_type)(const char*);
typedef double (*conv_to_float_type)(const char*);
typedef SV* (*conv_to_serialized_type)(const char*, SV*);
typedef void (*container_resize_type)(char*, int);
typedef void (*container_begin_type)(void*, char*);
typedef void (*container_access_type)(char*, char*, int, SV*, SV*);
typedef void (*container_store_type)(char*, char*, int, SV*);
typedef void (*composite_access_type)(char*, SV*, SV*);
typedef void (*composite_store_type)(char*, SV*);
typedef SV* (*iterator_deref_type)(const char*);
typedef void (*iterator_incr_type)(char*);
typedef SV* (*provide_type)();

struct composite_access_vtbl {
   composite_access_type get[2];
   composite_store_type store;
};

struct type_infos;

} }
namespace polymake { namespace perl_bindings {

struct bait { int dummy[1]; };

template <bool exact_match>
struct recognized : bait { int dummies[1+exact_match]; };

inline bait* recognize(pm::perl::type_infos&, bait*, ...) { return nullptr; }
inline SV* member_names(bait*, ...) { return pm::perl::Scalar::undef(); }

template <typename T> class Class;

// don't try to create any magic glue for these guys
inline bait* recognize(pm::perl::type_infos&, bait*, pm::perl::Object*, pm::perl::Object*) { return nullptr; }
inline bait* recognize(pm::perl::type_infos&, bait*, pm::perl::ObjectType*, pm::perl::ObjectType*) { return nullptr; }
inline bait* recognize(pm::perl::type_infos&, bait*, pm::Array<pm::perl::Object>*, pm::Array<pm::perl::Object>*) { return nullptr; }

} }
namespace pm { namespace perl {

typedef polymake::perl_bindings::bait* recognizer_bait;

extern AnyString class_with_prescribed_pkg;
extern AnyString relative_of_known_class;

struct type_infos {
   SV* descr = nullptr;
   SV* proto = nullptr;
   bool magic_allowed = false;

   void set_proto(SV* prescribed_pkg, const std::type_info&, SV* super_proto=nullptr);
   void set_proto(SV* known_proto=nullptr);
   bool set_descr();
   bool set_descr(const std::type_info&);
};

template <typename T>
struct known_type {
   typedef type_behind_t<T> TData;
   static const bool is_proxy=!std::is_same<T, TData>::value;

   static const size_t recog_size=sizeof(*recognize(std::declval<type_infos&>(), recognizer_bait(0), (TData*)0, (TData*)0));

   static const bool value = recog_size >= sizeof(polymake::perl_bindings::recognized<false>)
                             || (is_proxy && mlist_contains<primitive_lvalues, TData>::value),
               exact_match = recog_size == sizeof(polymake::perl_bindings::recognized<true>) && !is_proxy;
};

template <typename T,
          bool is_known=known_type<T>::value,
          bool is_exact_match=known_type<T>::exact_match,
          bool has_persistent=known_type<typename object_traits<T>::persistent_type>::value,
          bool has_generic=known_type<typename generic_representative<T>::type>::value,
          bool is_proxy=!std::is_same<typename object_traits<T>::proxy_for, void>::value>
class type_cache_helper {
protected:
   // primary template: nothing known
   // either a special class with prescribed perl package, or a built-in type, or something really undeclared

   static type_infos get(SV* known_proto)
   {
      type_infos infos;
      if (!is_among<T, Object, pm::Array<Object>, ObjectType>::value &&
          infos.set_descr(typeid(typename remove_unsigned<T>::type)))
         infos.set_proto(known_proto);
      return infos;
   }

   // non-lazy type
   static type_infos get_with_prescribed_pkg(SV* prescribed_pkg, std::false_type)
   {
      type_infos infos;
      infos.set_proto(prescribed_pkg, typeid(T));
      infos.descr=polymake::perl_bindings::Class<T>::register_it(class_with_prescribed_pkg, infos.proto);
      return infos;
   }

   // lazy type
   static type_infos get_with_prescribed_pkg(SV* prescribed_pkg, std::true_type)
   {
      return type_infos{};
   }
};

class type_cache_base {
protected:
   static wrapper_type get_function_wrapper(SV* src, SV* descr, int auto_func_index);
   static wrapper_type get_assignment_operator(SV* src, SV* descr);
   static wrapper_type get_conversion_operator(SV* src, SV* descr);
   static wrapper_type get_conversion_constructor(SV* src, SV* descr);
};

template <typename T>
class type_cache
   : protected type_cache_base
   , protected type_cache_helper<T> {
protected:
   typedef type_cache_helper<T> super;

   static type_infos& get(SV* known_proto=nullptr)
   {
      static type_infos infos=super::get(known_proto);
      return infos;
   }

   static type_infos& get_with_prescribed_pkg(SV* prescribed_pkg)
   {
      static type_infos infos=super::get_with_prescribed_pkg(prescribed_pkg, bool_constant<object_traits<T>::is_lazy>());
      return infos;
   }
public:
   static SV* get_descr(int)
   {
      return get().descr;
   }
   static SV* get_descr(SV* prescribed_pkg)
   {
      return get_with_prescribed_pkg(prescribed_pkg).descr;
   }

   static SV* get_descr_for_proto(SV* known_proto) { return get(known_proto).descr; }
   static SV* get_proto(SV* known_proto=nullptr) { return get(known_proto).proto; }
   static SV* provide() { return get_proto(); }       // for ClassRegistrator
   static SV* provide_descr() { return get_descr(0); } // for ClassRegistrator

   static bool magic_allowed()
   {
      return get().magic_allowed;
   }

   static wrapper_type get_assignment_operator(SV* src)
   {
      return type_cache_base::get_assignment_operator(src, get_descr(0));
   }

   static wrapper_type get_conversion_operator(SV* src)
   {
      return type_cache_base::get_conversion_operator(src, get_descr(0));
   }

   static wrapper_type get_conversion_constructor(SV* src)
   {
      return type_cache_base::get_conversion_constructor(src, get_descr(0));
   }
};

template <typename T, typename Representative>
class type_cache_via {
protected:
   static SV* get_descr(SV* proto, std::false_type)
   {
      return polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, proto);
   }
   static SV* get_descr(SV* proto, std::true_type)
   {
      return proto;
   }

   static type_infos get(SV* known_proto)
   {
      assert(known_proto==nullptr);
      type_infos infos;
      infos.proto=type_cache<Representative>::get_proto();
      infos.magic_allowed=type_cache<Representative>::magic_allowed();
      infos.descr=infos.proto ? get_descr(infos.proto, bool_constant<object_traits<T>::is_lazy>()) : nullptr;
      return infos;
   }

   // non-lazy type
   static type_infos get_with_prescribed_pkg(SV* prescribed_pkg, std::false_type)
   {
      type_infos infos;
      infos.set_proto(prescribed_pkg, typeid(T), type_cache<Representative>::get_proto());
      infos.descr=polymake::perl_bindings::Class<T>::register_it(class_with_prescribed_pkg, infos.proto);
      return infos;
   }

   static type_infos get_with_prescribed_pkg(SV* prescribed_pkg, std::true_type)
   {
      return type_infos{};
   }
};

template <typename T, bool has_generic>
class type_cache_helper<T, false, false, true, has_generic, false>
   : public type_cache_via<T, typename object_traits<T>::persistent_type> {};

template <typename T>
class type_cache_helper<T, false, false, false, true, false>
   : public type_cache_via<T, typename generic_representative<T>::type> {};

template <typename T, bool has_persistent, bool has_generic>
class type_cache_helper<T, true, false, has_persistent, has_generic, true> {
protected:
   typedef type_behind_t<T> TData;
   static type_infos get(SV* known_proto)
   {
      assert(known_proto==nullptr);
      type_infos infos;
      infos.proto=type_cache<TData>::get_proto();
      infos.magic_allowed=true;
      infos.descr=polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, infos.proto);
      return infos;
   }
};

template <typename T, bool has_persistent, bool has_generic>
class type_cache_helper<T, true, false, has_persistent, has_generic, false> {
protected:
   static type_infos get(SV* known_proto)
   {
      assert(known_proto==nullptr);
      type_infos infos;
      recognize(infos, recognizer_bait(0), (T*)0, (T*)0);
      infos.descr=polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, infos.proto);
      return infos;
   }
};

template <typename T, bool has_persistent, bool has_generic>
class type_cache_helper<T, true, true, has_persistent, has_generic, false> {
protected:
   static type_infos get(SV* known_proto)
   {
      type_infos infos;
      if (known_proto)
         infos.set_proto(known_proto);
      else
         recognize(infos, recognizer_bait(0), (T*)0, (T*)0);
      if (infos.magic_allowed)
         infos.set_descr();
      return infos;
   }
};

template <typename T>
struct is_lvalue : bool_constant<attrib<T>::is_reference && !attrib<T>::is_const> {};

template <typename T>
struct is_mutable : bool_constant<pm::is_mutable<T>::value && !object_traits<T>::is_always_const> {};

template <typename type_list, int i>
struct TypeList_helper {
   static const int next= i+1 < list_length<type_list>::value ? i+1 : i;
   typedef TypeList_helper<type_list,next> recurse_down;
   typedef typename n_th<type_list,i>::type T;

   static bool push_types(Stack& stack)
   {
      if (SV* const Tproto=type_cache<T>::get_proto()) {
         stack.push(Tproto);
         return next==i || recurse_down::push_types(stack);
      } else {
         return false;
      }
   }

   static void gather_flags(ArrayHolder& arr)
   {
      type_cache<typename attrib<T>::minus_const_ref>::get_proto();
      if (is_lvalue<T>::value) {
         Value lvalue_arg;
         lvalue_arg << i;
         arr.push(lvalue_arg);
      }
      if (next>i) recurse_down::gather_flags(arr);
   }

   static void gather_type_names(ArrayHolder& arr)
   {
      typedef typename access<T>::type Type;
      arr.push(Scalar::const_string_with_int(typeid(Type).name(), attrib<Type>::is_const));
      if (next>i) recurse_down::gather_type_names(arr);
   }

   static void gather_type_protos(ArrayHolder& arr)
   {
      SV* proto=type_cache<pure_type_t<T>>::get_proto();
      arr.push(proto ? proto : Scalar::undef());
      if (next>i) recurse_down::gather_type_protos(arr);
   }

   static void gather_type_descrs(ArrayHolder& arr)
   {
      SV* descr=type_cache<pure_type_t<T>>::get_descr(0);
      arr.push(descr ? descr : Scalar::undef());
      if (next>i) recurse_down::gather_type_descrs(arr);
   }
};

template <int i>
struct TypeList_helper<void,i> {
   static bool push_types(Stack&) { return true; }
   static void gather_flags(ArrayHolder&) {}
   static void gather_type_names(ArrayHolder&) {}
   static void gather_type_protos(ArrayHolder&) {}
   static void gather_type_descrs(ArrayHolder&) {}
};

template <typename Fptr>
class TypeListUtils {
   typedef typename list2cons<Fptr>::type type_list;

   static SV* gather_flags()
   {
      ArrayHolder arr(1+list_accumulate_unary<list_count, is_lvalue, type_list>::value);
      Value void_result;
      void_result << (std::is_same<typename list2cons<Fptr>::return_type, void>::value ||
                      std::is_same<typename list2cons<Fptr>::return_type, ListReturn>::value);
      arr.push(void_result);
      TypeList_helper<type_list,0>::gather_flags(arr);
      return arr.get();
   }

   static SV* gather_type_names()
   {
      ArrayHolder arr(list_length<type_list>::value);
      TypeList_helper<type_list, 0>::gather_type_names(arr);
      return arr.get();
   }

   static SV* gather_type_protos()
   {
      ArrayHolder arr(list_length<type_list>::value);
      TypeList_helper<type_list, 0>::gather_type_protos(arr);
      arr.set_contains_aliases();
      return arr.get();
   }

   static SV* gather_type_descrs()
   {
      ArrayHolder arr(list_length<type_list>::value);
      TypeList_helper<type_list, 0>::gather_type_descrs(arr);
      arr.set_contains_aliases();
      return arr.get();
   }

public:
   static const int type_cnt=list_length<type_list>::value;

   static bool push_types(Stack& stack)
   {
      return TypeList_helper<type_list, 0>::push_types(stack);
   }

   // build an array with void-return and arg-is-lvalue flags;
   // the signature of this function has to be compatible with wrapper_type
   static SV* get_flags(SV**)
   {
      static SV* ret=gather_flags();
      return ret;
   }

   static SV* get_type_names()
   {
      static SV* types=gather_type_names();
      return types;
   }

   // instantiate the perl-side descriptors of all types in type_list
   static SV* provide_types()
   {
      static SV* types=gather_type_protos();
      return types;
   }

   static SV* provide_descrs()
   {
      static SV* descrs=gather_type_descrs();
      return descrs;
   }
};

} }

#endif // POLYMAKE_PERL_TYPES_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
