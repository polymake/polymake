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

#ifndef POLYMAKE_PERL_TYPES_H
#define POLYMAKE_PERL_TYPES_H

namespace pm { namespace perl {

using wrapper_type = SV* (*)(SV**);
using indirect_wrapper_type = SV* (*)(void*, SV**);
using destructor_type = void (*)(char*);
using copy_constructor_type = void (*)(void*, const char*);
using assignment_type = void (*)(char*, SV*, value_flags);
using conv_to_string_type = SV* (*)(const char*);
using conv_to_int_type = int (*)(const char*);
using conv_to_float_type = double (*)(const char*);
using conv_to_serialized_type = SV* (*)(const char*, SV*);
using container_resize_type = void (*)(char*, int);
using container_begin_type = void (*)(void*, char*);
using container_access_type = void (*)(char*, char*, int, SV*, SV*);
using container_store_type = void (*)(char*, char*, int, SV*);
using composite_access_type = void (*)(char*, SV*, SV*);
using composite_store_type = void (*)(char*, SV*);
using iterator_deref_type = SV* (*)(const char*);
using iterator_incr_type = void (*)(char*);
using provide_type = SV* (*)();

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

using recognizer_bait = polymake::perl_bindings::bait*;

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
   using TData = type_behind_t<T>;
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
   using helper_t = type_cache_helper<T>;

   static type_infos& get(SV* known_proto=nullptr)
   {
      static type_infos infos=helper_t::get(known_proto);
      return infos;
   }

   static type_infos& get_with_prescribed_pkg(SV* prescribed_pkg)
   {
      static type_infos infos=helper_t::get_with_prescribed_pkg(prescribed_pkg, bool_constant<object_traits<T>::is_lazy>());
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
   using TData = type_behind_t<T>;
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
   using recurse_down = TypeList_helper<type_list,next>;
   using T = typename n_th<type_list,i>::type;

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
      using Type = typename access<T>::type;
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
   using type_list = typename list2cons<Fptr>::type;

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
   // the signature of this function has to be compatible with indirect_wrapper_type
   static SV* get_flags(void*, SV**)
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
