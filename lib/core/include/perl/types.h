/* Copyright (c) 1997-2022
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

#include <functional>

namespace pm { namespace perl {

using wrapper_type = SV* (*)(SV**);
using type_reg_fn_type = std::pair<SV*, SV*> (*)(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by);
using destructor_type = void (*)(char*);
using copy_constructor_type = void (*)(void*, const char*);
using assignment_type = void (*)(char*, SV*, ValueFlags);
using conv_to_string_type = SV* (*)(const char*);
using conv_to_bool_type = bool (*)(const char*);
using conv_to_Int_type = Int (*)(const char*);
using conv_to_Float_type = double (*)(const char*);
using conv_to_serialized_type = SV* (*)(const char*, SV*);
using container_resize_type = void (*)(char*, Int);
using container_begin_type = void (*)(void*, char*);
using container_access_type = void (*)(char*, char*, Int, SV*, SV*);
using container_store_type = void (*)(char*, char*, Int, SV*);
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

struct bait {};
struct unrecognized : std::false_type {};

inline unrecognized recognize(pm::perl::type_infos&, bait, ...) { return unrecognized{}; }
inline SV* member_names(bait, ...) { return pm::perl::Scalar::undef(); }

template <typename T> class Class;

// don't try to create any magic glue for these guys
inline unrecognized recognize(pm::perl::type_infos&, bait, pm::perl::BigObject*, pm::perl::BigObject*) { return unrecognized{}; }
inline unrecognized recognize(pm::perl::type_infos&, bait, pm::perl::BigObjectType*, pm::perl::BigObjectType*) { return unrecognized{}; }
inline unrecognized recognize(pm::perl::type_infos&, bait, pm::Array<pm::perl::BigObject>*, pm::Array<pm::perl::BigObject>*) { return unrecognized{}; }

} }
namespace pm { namespace perl {

using recognizer_bait = polymake::perl_bindings::bait;
using unrecognized = polymake::perl_bindings::unrecognized;

extern AnyString class_with_prescribed_pkg;
extern AnyString relative_of_known_class;

struct type_infos {
   SV* descr = nullptr;
   SV* proto = nullptr;
   bool magic_allowed = false;

   void set_proto_with_prescribed_pkg(SV* prescribed_pkg, SV* app_stash_ref, const std::type_info&, SV* super_proto = nullptr);
   void set_proto(SV* known_proto=nullptr);
   void set_descr();
   bool set_descr(const std::type_info&);
};

template <typename T>
struct known_type {
   using data_t = type_behind_t<T>;
   static const bool is_proxy=!std::is_same<T, data_t>::value;

   using recognize_result = decltype(recognize(std::declval<type_infos&>(), recognizer_bait(), (data_t*)nullptr, (data_t*)nullptr));

   static constexpr bool value = !std::is_same<recognize_result, unrecognized>::value
                                 || (is_proxy && mlist_contains<primitive_lvalues, data_t>::value);
   static constexpr bool exact_match = recognize_result::value && !is_proxy;
};

using suppress_registration_for = mlist<BigObject, BigObjectType, pm::Array<BigObject>, OptionSet>;

template <typename T, typename=void>
class type_cache_helper {
   // primary template: nothing known
   // either a special class with prescribed perl package, or a built-in type, or something really undeclared
public:
   static const bool prefer_early_registration = Value::check_for_magic_storage<T>::value;
protected:
   static type_infos init(SV* known_proto, SV* generated_by)
   {
      type_infos infos;
      if (infos.set_descr(typeid(remove_unsigned_t<T>)))
         infos.set_proto(known_proto);
      return infos;
   }

   static type_infos init(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      type_infos infos;
      assert(generated_by);
      infos.set_proto_with_prescribed_pkg(prescribed_pkg, app_stash_ref, typeid(T));
      infos.descr=polymake::perl_bindings::Class<T>::register_it(class_with_prescribed_pkg, infos.proto, generated_by);
      return infos;
   }

};

template <typename T>
class type_cache_helper<T, std::enable_if_t<mlist_contains<suppress_registration_for, T>::value>> {
public:
   static const bool prefer_early_registration = false;
protected:
   static type_infos init(SV*, ...)
   {
      return type_infos{};
   }
};

template <typename T>
class type_cache_helper<T, std::enable_if_t<std::is_enum<T>::value>>
   : public type_cache_helper<std::make_signed_t<T>> {};

class type_cache_base {
protected:
   static char* get_function_wrapper(SV* src, SV* descr, int auto_func_index);
   static char* get_assignment_operator(SV* src, SV* descr);
   static char* get_conversion_operator(SV* src, SV* descr);
};

template <typename T>
class type_cache
   : protected type_cache_base
   , public type_cache_helper<T> {
protected:
   using helper_t = type_cache_helper<T>;

   static_assert(!object_traits<T>::is_lazy, "lazy types may not cross the C++/perl boundary");

   static type_infos& data(SV* known_proto, SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      static type_infos infos = prescribed_pkg ? helper_t::init(prescribed_pkg, app_stash_ref, generated_by) : helper_t::init(known_proto, generated_by);
      return infos;
   }

public:
   static SV* get_descr(SV* known_proto = nullptr)
   {
      return data(known_proto, nullptr, nullptr, nullptr).descr;
   }
   static SV* get_proto(SV* known_proto = nullptr)
   {
      return data(known_proto, nullptr, nullptr, nullptr).proto;
   }
   // for ClassRegistrator
   static std::pair<SV*, SV*> provide(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      const type_infos& ti = data(nullptr, prescribed_pkg, app_stash_ref, generated_by);
      return { ti.proto, ti.descr };
   }

   static bool magic_allowed()
   {
      return data(nullptr, nullptr, nullptr, nullptr).magic_allowed;
   }

   static char* get_assignment_operator(SV* src)
   {
      return type_cache_base::get_assignment_operator(src, get_descr());
   }

   static char* get_conversion_operator(SV* src)
   {
      return type_cache_base::get_conversion_operator(src, get_descr());
   }
};

template <typename T, typename Representative>
class type_cache_via {
public:
   static const bool prefer_early_registration=true;
protected:
   static type_infos init(SV* known_proto, SV* generated_by)
   {
      type_infos infos;
      assert(known_proto==nullptr);
      infos.proto=type_cache<Representative>::get_proto();
      infos.magic_allowed=type_cache<Representative>::magic_allowed();
      infos.descr=infos.proto ? polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, infos.proto, generated_by) : nullptr;
      return infos;
   }

   static type_infos init(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      type_infos infos;
      infos.set_proto_with_prescribed_pkg(prescribed_pkg, app_stash_ref, typeid(T), type_cache<Representative>::get_proto());
      infos.descr=polymake::perl_bindings::Class<T>::register_it(class_with_prescribed_pkg, infos.proto, generated_by);
      return infos;
   }
};

template <typename T>
class type_cache_helper<T, std::enable_if_t<(!known_type<T>::value &&
                                             !known_type<T>::exact_match &&
                                             known_type<typename object_traits<T>::persistent_type>::value &&
                                             std::is_same<typename object_traits<T>::proxy_for, void>::value)>>
   : public type_cache_via<T, typename object_traits<T>::persistent_type> {};

template <typename T>
class type_cache_helper<T, std::enable_if_t<(!known_type<T>::value &&
                                             !known_type<T>::exact_match &&
                                             !known_type<typename object_traits<T>::persistent_type>::value &&
                                             known_type<typename generic_representative<T>::type>::value &&
                                             std::is_same<typename object_traits<T>::proxy_for, void>::value)>>
   : public type_cache_via<T, typename generic_representative<T>::type> {};

template <typename T>
class type_cache_helper<T, std::enable_if_t<(known_type<T>::value &&
                                             !known_type<T>::exact_match &&
                                             !std::is_same<typename object_traits<T>::proxy_for, void>::value)>> {
public:
   static const bool prefer_early_registration=true;
protected:
   using data_t = type_behind_t<T>;
   static type_infos init(SV* known_proto, SV* generated_by)
   {
      type_infos infos;
      assert(!known_proto);
      infos.proto = type_cache<data_t>::get_proto();
      infos.magic_allowed = true;
      infos.descr = polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, infos.proto, generated_by);
      return infos;
   }

   static type_infos init(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      return init(nullptr, generated_by);
   }
};

template <typename T>
class type_cache_helper<T, std::enable_if_t<(known_type<T>::value &&
                                             !known_type<T>::exact_match &&
                                             std::is_same<typename object_traits<T>::proxy_for, void>::value)>> {
public:
   static const bool prefer_early_registration=true;
protected:
   static type_infos init(SV* known_proto, SV* generated_by)
   {
      type_infos infos;
      assert(!known_proto);
      recognize(infos, recognizer_bait(), (T*)nullptr, (T*)nullptr);
      infos.descr = polymake::perl_bindings::Class<T>::register_it(relative_of_known_class, infos.proto, generated_by);
      return infos;
   }

   static type_infos init(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      return init(nullptr, generated_by);
   }
};

template <typename T>
class type_cache_helper<T, std::enable_if_t<(known_type<T>::value &&
                                             known_type<T>::exact_match &&
                                             std::is_same<typename object_traits<T>::proxy_for, void>::value)>> {
public:
   static const bool prefer_early_registration=false;
protected:
   static type_infos init(SV* known_proto, SV* generated_by)
   {
      type_infos infos;
      if (known_proto)
         infos.set_proto(known_proto);
      else
         recognize(infos, recognizer_bait(), (T*)nullptr, (T*)nullptr);
      if (infos.magic_allowed)
         infos.set_descr();
      return infos;
   }

   static type_infos init(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      return init(nullptr, nullptr);
   }
};

template <typename type_list, int i>
struct TypeList_helper {
   static constexpr int next = i+1 < list_length<type_list>::value ? i+1 : i;
   using recurse_down = TypeList_helper<type_list, next>;
   using T = typename n_th<type_list, i>::type;

   static void gather_type_protos(ArrayHolder& arr)
   {
      SV* proto = type_cache<pure_type_t<T>>::get_proto();
      arr.push(proto ? proto : Scalar::undef());
      if (next > i) recurse_down::gather_type_protos(arr);
   }

   static void gather_type_descrs(ArrayHolder& arr)
   {
      SV* descr = type_cache<pure_type_t<T>>::get_descr();
      arr.push(descr ? descr : Scalar::undef());
      if (next > i) recurse_down::gather_type_descrs(arr);
   }
};

template <int i>
struct TypeList_helper<void, i> {
   static void gather_type_protos(ArrayHolder&) {}
   static void gather_type_descrs(ArrayHolder&) {}
};

template <typename Fptr>
class TypeListUtils {
   // TODO: use func_args_t and variadic pack instead of recursion, cf. store_cross_apps in wrappers.h
   using type_list = typename list2cons<Fptr>::type;

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
   static constexpr int type_cnt = list_length<type_list>::value;

   // instantiate the perl-side descriptors of all types in type_list
   static SV* provide_types()
   {
      static SV* types = gather_type_protos();
      return types;
   }

   static SV* provide_descrs()
   {
      static SV* descrs = gather_type_descrs();
      return descrs;
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
