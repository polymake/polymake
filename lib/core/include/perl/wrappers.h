/* Copyright (c) 1997-2023
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

/* The following classes are designed solely for the automatically generated wrapper files.
   If you use them in a manually written code, you are doing it at your own risk!

   The changes in syntax and contents must be concerted with the corresponding routines
   in the package Polymake::Core::CPlusPlus
*/

namespace pm { namespace perl {

template <typename T>
using is_mutable = bool_constant<pm::is_mutable<T>::value && !object_traits<T>::is_always_const>;

class RegistratorQueue {
public:
   enum class Kind {
      // cf. CPlusPlus.pm
      function, embedded_rule, duplicate_class_instance
   };
   RegistratorQueue(const AnyString& name, Kind kind);

protected:
   SV* queue;
};

template <typename T, typename=void>
class Destroy {
public:
   static destructor_type func() { return nullptr; }
};

template <typename T>
class Destroy<T, std::enable_if_t<!(std::is_trivially_destructible<T>::value || is_masquerade<T>::value)>> {
   static void impl(char* p)
   {
      T* obj = reinterpret_cast<T*>(p);
      obj->~T();
   }
public:
   static destructor_type func() { return &impl; }
};

template <typename T, typename=void>
class Copy {
public:
   static copy_constructor_type func() { return nullptr; }
};

template <typename T>
class Copy<T, std::enable_if_t<std::is_same<T, typename object_traits<T>::persistent_type>::value &&
                               is_mutable<T>::value &&
                               !is_masquerade<T>::value &&
                               std::is_copy_constructible<T>::value>> {
   static void impl(void* place, const char* p)
   {
      const T* src = reinterpret_cast<const T*>(p);
      new(place) T(*src);
   }
public:
   static copy_constructor_type func() { return &impl; }
};



template <typename T, typename=void>
class Assign {
public:
   static assignment_type func() { return nullptr; }
};

template <typename T>
class Assign<T, std::enable_if_t<is_mutable<type_behind_t<T>>::value &&
                                 is_readable<type_behind_t<T>>::value &&
                                 std::is_copy_assignable<T>::value &&
                                 !std::is_same<T, Value>::value>> {
protected:
   template <typename, typename> friend class Assign;

   static void assign(T* dst, SV* sv, ValueFlags flags, std::true_type)
   {
      Value src(sv, flags);
      src >> *dst;
   }

   static void assign(T* dst, SV* sv, ValueFlags flags, std::false_type)
   {
      type_behind_t<T> x{};
      Assign<type_behind_t<T>>::assign(&x, sv, flags, std::true_type());
      *dst = std::move(x);
   }

   static void impl(char* p, SV* sv, ValueFlags flags)
   {
      T* dst = reinterpret_cast<T*>(p);
      assign(dst, sv, flags, std::is_same<typename object_traits<T>::proxy_for, void>());
   }
public:
   static assignment_type func() { return &impl; }
};


class Unprintable {
protected:
   static SV* impl(const char*);
public:
   static conv_to_string_type func() { return &impl; }
};

template <typename T, typename enabled=void>
class ToString : public Unprintable {};

template <typename T>
class ToString<T, std::enable_if_t<is_printable<type_behind_t<T>>::value>> {
protected:
   static SV* to_string(const type_behind_t<T>& src)
   {
      Value ret;
      ostream my_stream(static_cast<SVHolder&>(ret));
      PlainPrinter<> printer(my_stream);
      printer << src;
      return ret.get_temp();
   }
   static SV* impl(const char* p)
   {
      const T* src = reinterpret_cast<const T*>(p);
      return to_string(*src);
   }
public:
   static conv_to_string_type func() { return &impl; }
};


template <typename T, typename enabled=void>
class Serializable {
public:
   static constexpr ClassFlags flag_value() { return ClassFlags::none; }
   static conv_to_serialized_type conv() { return nullptr; }
   static type_reg_fn_type provide() { return nullptr; }
};

template <typename T>
class Serializable<T, std::enable_if_t<has_serialized<type_behind_t<T>>::value>> {
protected:
   static SV* store_serialized(const type_behind_t<T>& src, SV* holder)
   {
      Value ret(ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref | ValueFlags::read_only);
      ret.put(serialize(src), holder);
      return ret.get_temp();
   }

   static SV* impl(const char* p, SV* holder)
   {
      const T* src = reinterpret_cast<const T*>(p);
      return store_serialized(*src, holder);
   }
public:
   typedef pure_type_t<decltype(serialize(std::declval<const type_behind_t<T>&>()))> serialized_t;

   static constexpr ClassFlags flag_value()
   {
      return check_container_feature<serialized_t, sparse>::value
             ? ClassFlags::is_serializable | ClassFlags::is_sparse_serialized
             : ClassFlags::is_serializable;
   }

   static conv_to_serialized_type conv() { return &impl; }
   static type_reg_fn_type provide() { return &type_cache<serialized_t>::provide; }
};

// This one is used in overloaded operators for sparse proxies of primitive types.
// It should just retrieve the data element.
template <typename TProxy>
class Serializable<TProxy, std::enable_if_t<!std::is_same<TProxy, type_behind_t<TProxy>>::value &&
                                            std::is_arithmetic<type_behind_t<TProxy>>::value>>
   : public Serializable<type_behind_t<TProxy>> {
protected:
   static SV* impl(const char* p, SV*)
   {
      const TProxy* src = reinterpret_cast<const TProxy*>(p);
      Value ret;
      ret << static_cast<type_behind_t<TProxy>>(*src);
      return ret.get_temp();
   }

public:
   static conv_to_serialized_type conv() { return &impl; }
};

class ClassRegistratorBase
   : public RegistratorQueue {
   ClassRegistratorBase() = delete;
public:
   static const Kind kind = Kind::duplicate_class_instance;
protected:
   static
   SV* register_class(const AnyString& name, const AnyString& cpperl_file, int inst_num,
                      SV* someref, SV* generated_by,
                      const char* typeid_name,
                      bool is_mutable, ClassFlags kind,
                      SV* vtbl_sv);

   static
   SV* create_builtin_vtbl(
      const std::type_info& type,
      size_t obj_size,
      int primitive_lvalue,
      copy_constructor_type copy_constructor,
      assignment_type assignment,
      destructor_type destructor
   );

   static
   SV* create_scalar_vtbl(
      const std::type_info& type,
      size_t obj_size,
      copy_constructor_type copy_constructor,
      assignment_type assignment,
      destructor_type destructor,
      conv_to_string_type to_string,
      conv_to_serialized_type to_serialized,
      type_reg_fn_type provide_serialized_type,
      conv_to_Int_type to_Int,
      conv_to_Float_type to_Float
   );

   static
   SV* create_iterator_vtbl(
      const std::type_info& type,
      size_t obj_size,
      copy_constructor_type copy_constructor,
      destructor_type destructor,
      iterator_deref_type deref,
      iterator_incr_type incr,
      conv_to_bool_type at_end,
      conv_to_Int_type index
   );

   static
   SV* create_opaque_vtbl(
      const std::type_info& type,
      size_t obj_size,
      copy_constructor_type copy_constructor,
      assignment_type assignment,
      destructor_type destructor,
      conv_to_string_type to_string,
      conv_to_serialized_type to_serialized,
      type_reg_fn_type provide_serialized_type
   );

   static
   SV* create_container_vtbl(
      const std::type_info& type,
      size_t obj_size, int total_dimension, int own_dimension,
      copy_constructor_type copy_constructor,
      assignment_type assignment,
      destructor_type destructor,
      conv_to_string_type to_string,
      conv_to_serialized_type to_serialized,
      type_reg_fn_type provide_serialized_type,
      conv_to_Int_type size,
      container_resize_type resize,
      container_store_type store_at_ref,
      type_reg_fn_type provide_key_type,
      type_reg_fn_type provide_value_type
   );

   static
   void fill_iterator_access_vtbl(
      SV* vtbl, int i,
      size_t it_size, size_t cit_size,
      destructor_type it_destructor,
      destructor_type cit_destructor,
      container_begin_type begin,
      container_begin_type cbegin,
      container_access_type deref,
      container_access_type cderef
   );

   static
   void fill_random_access_vtbl(
      SV* vtbl,
      container_access_type random,
      container_access_type crandom
   );

   static
   SV* create_composite_vtbl(
      const std::type_info& type,
      size_t obj_size, int obj_dimension,
      copy_constructor_type copy_constructor,
      assignment_type assignment,
      destructor_type destructor,
      conv_to_string_type to_string,
      conv_to_serialized_type to_serialized,
      type_reg_fn_type provide_serialized_type,
      int n_members,
      provide_type provide_member_types,
      provide_type provide_member_descrs,
      provide_type provide_member_names,
      void (*fill)(composite_access_vtbl*)
   );

   template <typename T>
   static constexpr
   ClassFlags ordered_flag()
   {
      return is_ordered<T>::value ? ClassFlags::is_ordered : ClassFlags::none;
   }
};

template <typename T>
class Builtin
   : public ClassRegistratorBase {
   Builtin() = delete;
public:
   void add__me(const AnyString& name, const AnyString& cpperl_file, int inst_num) const
   {
      register_class(
         name, cpperl_file, inst_num,
         queue, nullptr,
         typeid(T).name(),
         false, ClassFlags::is_scalar | ordered_flag<T>(),
         create_builtin_vtbl(
            typeid(T), sizeof(T), mlist_contains<primitive_lvalues, T>::value,
            Copy<T>::func(),
            Assign<T>::func(),
            Destroy<T>::func()
         )
      );
   }
};

template <typename T, typename Model=typename object_traits<T>::model>
class ClassRegistrator;

template <>
class ClassRegistrator<Scalar, is_opaque> {};
template <>
class ClassRegistrator<Array, is_container> {};

template <typename T>
class ClassRegistrator<T, is_scalar>
   : public ClassRegistratorBase {
protected:
   typedef typename object_traits<T>::persistent_type persistent_type;

   template <typename Target, typename = void>
   struct conv;

   template <typename Target>
   struct conv<Target, std::enable_if_t<std::is_constructible<Target, T>::value>>
   {
      static Target func(const char* p)
      {
         const T* obj = reinterpret_cast<const T*>(p);
         return static_cast<Target>(*obj);
      }
   };

   template <typename Target>
   struct conv<Target, std::enable_if_t<!std::is_constructible<Target, T>::value &&
                                        std::is_constructible<Target, persistent_type>::value>>
   {
      static Target func(const char* p)
      {
         const T* obj = reinterpret_cast<const T*>(p);
         return static_cast<Target>(static_cast<const persistent_type&>(*obj));
      }
   };

   template <typename Target>
   struct conv<Target, std::enable_if_t<!std::is_constructible<Target, T>::value &&
                                        !std::is_constructible<Target, persistent_type>::value>>
   {
      static Target func(const char* p)
      {
         throw std::runtime_error("can't convert " + legible_typename<T>() + " to " + legible_typename<Target>());
      }
   };

public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(T).name(),
         is_mutable<T>::value,
         ClassFlags::is_scalar | Serializable<T>::flag_value() | ordered_flag<T>(),
         create_scalar_vtbl(
            typeid(T), sizeof(T),
            Copy<T>::func(),
            Assign<T>::func(),
            Destroy<T>::func(),
            ToString<T>::func(),
            Serializable<T>::conv(),
            Serializable<T>::provide(),
            &conv<Int>::func,
            &conv<double>::func
         )
      );
   }
};

template <typename T, bool is_iterator=check_iterator_feature<T, end_sensitive>::value>
class OpaqueClassRegistrator
   : public ClassRegistratorBase {
public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(T).name(),
         is_mutable<T>::value,
         ClassFlags::is_opaque | Serializable<T>::flag_value() | ordered_flag<T>(),
         create_opaque_vtbl(
            typeid(T), sizeof(T),
            Copy<T>::func(),
            Assign<T>::func(),
            Destroy<T>::func(),
            ToString<T>::func(),
            Serializable<T>::conv(),
            Serializable<T>::provide()
         )
      );
   }
};

template <typename TData, typename TDeleter>
class OpaqueClassRegistrator<std::unique_ptr<TData, TDeleter>, false>
   : public ClassRegistratorBase {
public:
   using Tptr = std::unique_ptr<TData, TDeleter>;

   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(Tptr).name(),
         false,
         ClassFlags::is_opaque | ClassFlags::none,
         create_opaque_vtbl(
            typeid(Tptr), sizeof(Tptr),
            nullptr,
            nullptr,
            Destroy<Tptr>::func(),
            Unprintable::func(),
            nullptr,
            nullptr
         )
      );
   }
};

template <typename T>
class OpaqueClassRegistrator<T, true>
   : public ClassRegistratorBase {
protected:
   static const bool read_only=attrib<typename iterator_traits<T>::reference>::is_const;

   static SV* deref(const char* p)
   {
      const T* it = reinterpret_cast<const T*>(p);
      Value ret((read_only ? ValueFlags::read_only : ValueFlags::is_mutable) |
                ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      ret.put_lvalue(**it);
      return ret.get_temp();
   }

   static void incr(char* p)
   {
      T* it = reinterpret_cast<T*>(p);
      ++(*it);
   }

   static bool at_end(const char* p)
   {
      const T* it = reinterpret_cast<const T*>(p);
      return it->at_end();
   }

   static Int index_impl(const char* p)
   {
      const T* it = reinterpret_cast<const T*>(p);
      return it->index();
   }

   static conv_to_Int_type index(std::false_type) { return nullptr; }
   static conv_to_Int_type index(std::true_type) { return &index_impl; }
   static conv_to_Int_type index() { return index(bool_constant<check_iterator_feature<T, indexed>::value>()); }

public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(T).name(),
         true,
         ClassFlags::is_opaque,
         create_iterator_vtbl(
            typeid(T), sizeof(T),
            Copy<T>::func(),
            Destroy<T>::func(),
            &deref,
            &incr,
            &at_end,
            index()
         )
      );
   }
};

template <typename T>
class ClassRegistrator<T, is_opaque>
   : public OpaqueClassRegistrator<T> {};

template <typename Mode, bool TEnableSparse>
struct transform_input_mode {
   typedef Mode type;
};

template <int TResizeable>
struct transform_input_mode<io_test::as_sparse<TResizeable>, false> {
   typedef io_test::as_array<TResizeable, true> type;
};

template <typename T, bool TEnable = true, bool TEnableSparse = true, bool TMutable = TEnable && !object_traits<T>::is_always_const>
struct input_mode
   : transform_input_mode<typename io_test::input_mode<T, false>::type, TEnableSparse> {};

template <typename T, bool TEnable, bool TEnableSparse>
struct input_mode<T, TEnable, TEnableSparse, false> {
   typedef std::false_type type;
};

template <typename T, int TDim = object_traits<T>::dimension>
struct container_helper;

template <typename T>
struct container_helper<T, 1> {
   typedef T type;
   static type& streamline(T& x) { return x; }
   static const type& streamline(const T& x) { return x; }
};

template <typename T>
struct container_helper<T, 2> {
   typedef Rows<T> type;
   static type& streamline(T& x) { return rows(x); }
   static const type& streamline(const T& x) { return rows(x); }
};

template <typename T, typename Category=typename container_traits<typename container_helper<T>::type>::category>
class ContainerClassRegistrator
   : public ClassRegistratorBase {
public:
   typedef container_helper<T> helper;
   typedef typename helper::type Obj;
   static constexpr bool
        is_associative = is_assoc_container<Obj>::value,
             is_sparse = check_container_feature<Obj, sparse>::value,
      is_sparse_native = check_container_feature<T, sparse>::value,
              like_set = is_among<typename object_traits<Obj>::generic_tag, is_set, is_unordered_set>::value;
   using iterator_feature = std::conditional_t<is_associative, end_sensitive, mlist<>>;
   using iterator = typename ensure_features<Obj, iterator_feature>::iterator;
   using const_iterator = typename ensure_features<Obj, iterator_feature>::const_iterator;

   static const bool allow_non_const_access= !object_traits<T>::is_always_const &&
                                             !std::is_same<iterator, const_iterator>::value &&
                                             !attrib<typename iterator_traits<iterator>::reference>::is_const &&
                                             !object_traits<typename iterator_traits<iterator>::value_type>::is_always_const;

   typedef bool_constant<allow_non_const_access> non_const_access;

   static constexpr int element_dim = object_traits<typename Obj::value_type>::total_dimension;
   static_assert(!std::is_same<pure_type_t<typename Obj::value_type>, int>::value, "found container with int elements - use Int instead");
protected:
   static Int size_impl(const char* p)
   {
      const T* obj = reinterpret_cast<const T*>(p);
      return helper::streamline(*obj).size();
   }

   static Int dim(const char* p)
   {
      const T* obj = reinterpret_cast<const T*>(p);
      return helper::streamline(*obj).dim();
   }

   static void resize_impl(char* p, Int n)
   {
      T* obj = reinterpret_cast<T*>(p);
      helper::streamline(*obj).resize(n);
   }

   static void fixed_size(char* p, Int n)
   {
      T* obj = reinterpret_cast<T*>(p);
      if (get_dim(helper::streamline(*obj)) != n)
         throw std::runtime_error("size mismatch");
   }

   static void clear_by_resize(char* p, Int)
   {
      T* obj = reinterpret_cast<T*>(p);
      helper::streamline(*obj).clear();
   }

   template <typename E>
   static void check_insertion(const Obj&, const E&, std::false_type) {}

   static void check_insertion(const Obj& obj, Int x, std::true_type)
   {
      if (x < 0 || x >= obj.dim())
         throw std::runtime_error("element out of range");
   }

   static void push_back(char* p_obj, char* p_it, Int, SV* src)
   {
      T* obj = reinterpret_cast<T*>(p_obj);
      iterator* it = reinterpret_cast<iterator*>(p_it);
      typename Obj::value_type x{};
      Value v(src);
      v >> x;
      check_insertion(helper::streamline(*obj), x,
                      bool_constant<check_container_feature<Obj, sparse_compatible>::value>());
      helper::streamline(*obj).insert(*it, x);
   }

   static void insert(char* p_obj, char*, Int, SV* src)
   {
      T* obj = reinterpret_cast<T*>(p_obj);
      typename item4insertion<typename Obj::value_type>::type x{};
      Value v(src);
      v >> x;
      check_insertion(helper::streamline(*obj), x,
                      bool_constant<check_container_feature<Obj, sparse_compatible>::value>());
      helper::streamline(*obj).insert(x);
   }

   static void store_dense(char*, char* p_it, Int, SV* src)
   {
      iterator* it = reinterpret_cast<iterator*>(p_it);
      Value v(src, ValueFlags::not_trusted);
      v >> **it;
      ++(*it);
   }

   static void store_sparse(char* p_obj, char* p_it, Int index, SV* src)
   {
      T* obj = reinterpret_cast<T*>(p_obj);
      iterator* it = reinterpret_cast<iterator*>(p_it);
      Value v(src, ValueFlags::not_trusted);
      typename Obj::value_type x{};
      v >> x;
      if (!is_zero(x)) {
         if (!it->at_end() && it->index() == index) {
            **it = x; ++(*it);
         } else {
            obj->insert(*it, index, x);
         }
      } else {
         if (!it->at_end() && it->index() == index)
            obj->erase((*it)++);
      }
   }

   template <typename Iterator, bool non_const>
   struct do_it {
      typedef typename assign_const<T, !non_const>::type* ObjPtr;

      static void begin(void* it_place, char* p)
      {
         ObjPtr obj = reinterpret_cast<ObjPtr>(p);
         new(it_place) Iterator(ensure(helper::streamline(*obj), iterator_feature()).begin());
      }

      static void rbegin(void* it_place, char* p)
      {
         ObjPtr obj = reinterpret_cast<ObjPtr>(p);
         new(it_place) Iterator(ensure(helper::streamline(*obj), iterator_feature()).rbegin());
      }

      static void deref(char*, char* p_it, Int, SV* dst, SV* container_sv)
      {
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         Value pv(dst, (non_const ? ValueFlags::is_mutable : ValueFlags::read_only) | ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
         pv.put_lvalue(**it, container_sv);
         ++(*it);
      }

      static void deref_pair(char*, char* p_it, Int i, SV* dst, SV* container_sv)
      {
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         if (i <= 0) {
            // i==-1: FIRSTKEY;  i==0: NEXTKEY
            if (i == 0) ++(*it);
            if (!it->at_end()) {
               Value pv(dst, ValueFlags::read_only | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
               pv.put((*it)->first, container_sv);
            }
         } else {
            // i==1: fetch value
            Value pv(dst, (non_const ? ValueFlags::is_mutable : ValueFlags::read_only) | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
            pv.put((*it)->second, container_sv);
         }
      }
   };

   template <typename Iterator, bool Dim = element_dim>
   struct do_sparse {
      static void deref(char* p_obj, char* p_it, Int index, SV* dst, SV* container_sv)
      {
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         if (it->at_end() || index < it->index()) {
            Value pv(dst);
            pv.put(Undefined());
         } else {
            do_it<Iterator, true>::deref(p_obj, p_it, index, dst, container_sv);
         }
      }
   };

   template <typename Iterator>
   struct do_sparse<Iterator, 0> {
      static void deref(char* p_obj, char* p_it, Int index, SV* dst, SV* container_sv)
      {
         T* obj = reinterpret_cast<T*>(p_obj);
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         Value pv(dst, ValueFlags::expect_lval | ValueFlags::allow_non_persistent);
         auto x = Obj::reference::construct(sparse_proxy_it_base<Obj, Iterator>(*obj, *it, index));
         if (x.exists()) ++(*it);
         pv.put(std::move(x), container_sv);
      }
   };

   template <typename Iterator, bool TDim=element_dim>
   struct do_const_sparse {
      static void deref(char* p_obj, char* p_it, Int index, SV* dst, SV* container_sv)
      {
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         if (it->at_end() || index < it->index()) {
            Value pv(dst);
            pv.put(Undefined());
         } else {
            do_it<Iterator, false>::deref(p_obj, p_it, index, dst, container_sv);
         }
      }
   };

   template <typename Iterator>
   struct do_const_sparse<Iterator, 0> {
      static void deref(char*, char* p_it, Int index, SV* dst, SV* container_sv)
      {
         Iterator* it = reinterpret_cast<Iterator*>(p_it);
         Value pv(dst, ValueFlags::read_only | ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
         if (!it->at_end() && index == it->index()) {
            pv.put_lvalue(**it, container_sv);
            ++(*it);
         } else {
            pv.put(zero_value<typename Obj::value_type>());
         }
      }
   };

   static conv_to_Int_type size(std::false_type) { return &size_impl; }
   static conv_to_Int_type size(std::true_type)  { return &dim; }

   static container_resize_type resize(io_test::as_list<Obj>)
   {
      return &clear_by_resize;
   }
   static container_resize_type resize(io_test::as_set)
   {
      return &clear_by_resize;
   }
   template <bool allow_sparse>
   static container_resize_type resize(io_test::as_array<1, allow_sparse>)
   {
      return &resize_impl;
   }
   template <bool allow_sparse>
   static container_resize_type resize(io_test::as_array<0, allow_sparse>)
   {
      return &fixed_size;
   }
   static container_resize_type resize(io_test::as_sparse<-1>) { return nullptr; }
   static container_resize_type resize(std::false_type) { return nullptr; }

   static container_store_type store_at_ref(io_test::as_list<Obj>)
   {
      return &push_back;
   }
   static container_store_type store_at_ref(io_test::as_set)
   {
      return &insert;
   }
   template <int resizeable, bool allow_sparse>
   static container_store_type store_at_ref(io_test::as_array<resizeable, allow_sparse>)
   {
      return &store_dense;
   }
   template <int resizeable>
   static container_store_type store_at_ref(io_test::as_sparse<resizeable>)
   {
      return &store_sparse;
   }
   static container_store_type store_at_ref(std::false_type) { return nullptr; }

   static destructor_type it_destructor(std::true_type) { return Destroy<iterator>::func(); }
   static destructor_type it_destructor(std::false_type) { return Destroy<const_iterator>::func(); }

   static container_begin_type begin(std::true_type)
   {
      return &do_it<iterator, true>::begin;
   }
   static container_begin_type begin(std::false_type)
   {
      return &do_it<const_iterator, false>::begin;
   }

   static container_access_type deref(std::false_type, std::false_type, std::true_type)
   {
      return &do_it<iterator, true>::deref;
   }
   static container_access_type deref(std::false_type, std::false_type, std::false_type)
   {
      return &do_it<const_iterator, false>::deref;
   }
   static container_access_type deref(std::true_type, std::false_type, std::true_type)
   {
      return &do_it<iterator, true>::deref_pair;
   }
   static container_access_type deref(std::true_type, std::false_type, std::false_type)
   {
      return &do_it<const_iterator, false>::deref_pair;
   }
   static container_access_type deref(std::false_type, std::true_type, std::true_type)
   {
      return &do_sparse<iterator>::deref;
   }
   static container_access_type deref(std::false_type, std::true_type, std::false_type)
   {
      return &do_const_sparse<const_iterator>::deref;
   }

   static type_reg_fn_type provide_key_type(std::true_type)
   {
      return &type_cache<typename T::key_type>::provide;
   }
   static type_reg_fn_type provide_key_type(std::false_type)
   {
      return &type_cache<typename object_traits<typename T::value_type>::persistent_type>::provide;
   }
   static type_reg_fn_type provide_value_type(std::true_type)
   {
      return &type_cache<typename T::mapped_type>::provide;
   }
   static type_reg_fn_type provide_value_type(std::false_type)
   {
      return &type_cache<typename object_traits<typename Obj::value_type>::persistent_type>::provide;
   }

   static SV* create_vtbl()
   {
      SV* vtbl=create_container_vtbl(
         typeid(T), sizeof(T),
         object_traits<T>::total_dimension, object_traits<T>::dimension,
         Copy<T>::func(),
         Assign<T>::func(),
         Destroy<T>::func(),
         ToString<T>::func(),
         Serializable<T>::conv(),
         Serializable<T>::provide(),
         size(bool_constant<is_sparse>()),
         resize(typename input_mode<Obj>::type()),
         store_at_ref(typename input_mode<Obj, !is_associative, element_dim==0>::type()),
         provide_key_type(bool_constant<is_associative>()),
         provide_value_type(bool_constant<is_associative>())
      );
      fill_iterator_access_vtbl(
         vtbl, 0,
         sizeof(iterator), sizeof(const_iterator),
         it_destructor(non_const_access()),
         it_destructor(std::false_type()),
         begin(non_const_access()),
         begin(std::false_type()),
         deref(bool_constant<is_associative>(), bool_constant<is_sparse>(), non_const_access()),
         deref(bool_constant<is_associative>(), bool_constant<is_sparse>(), std::false_type())
      );
      return vtbl;
   }

   static SV* register_me(const AnyString& name, const AnyString& cpperl_file, int inst_num,
                          SV* someref, SV* generated_by, SV* vtbl)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(T).name(),
         is_mutable<T>::value,
         ClassFlags::is_container | Serializable<T>::flag_value() | ordered_flag<T>()
         | (is_sparse && !std::is_same<T, Obj>::value ? ClassFlags::is_sparse_serialized : ClassFlags::none)
         | (is_associative   ? ClassFlags::is_assoc_container :
            is_sparse_native ? ClassFlags::is_sparse_container :
            like_set         ? ClassFlags::is_set : ClassFlags::none),
         vtbl);
   }

public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_me(name, cpperl_file, inst_num, someref, generated_by, create_vtbl());
   }
};

template <typename T>
class ContainerClassRegistrator<T, bidirectional_iterator_tag>
   : public ContainerClassRegistrator<T, forward_iterator_tag> {
   typedef ContainerClassRegistrator<T, forward_iterator_tag> super;
public:
   typedef typename super::Obj::reverse_iterator reverse_iterator;
   typedef typename super::Obj::const_reverse_iterator const_reverse_iterator;
protected:
   static container_begin_type rbegin(std::true_type)
   {
      return &super::template do_it<reverse_iterator, true>::rbegin;
   }
   static container_begin_type rbegin(std::false_type)
   {
      return &super::template do_it<const_reverse_iterator, false>::rbegin;
   }

   static container_access_type rderef(std::false_type, std::true_type)
   {
      return &super::template do_it<reverse_iterator, true>::deref;
   }
   static container_access_type rderef(std::false_type, std::false_type)
   {
      return &super::template do_it<const_reverse_iterator, false>::deref;
   }
   static container_access_type rderef(std::true_type, std::true_type)
   {
      return &super::template do_sparse<reverse_iterator>::deref;
   }
   static container_access_type rderef(std::true_type, std::false_type)
   {
      return &super::template do_const_sparse<const_reverse_iterator>::deref;
   }

   static destructor_type rit_destructor(std::true_type) { return Destroy<reverse_iterator>::func(); }
   static destructor_type rit_destructor(std::false_type) { return Destroy<const_reverse_iterator>::func(); }
public:
   static SV* create_vtbl(std::false_type)
   {
      SV* vtbl=super::create_vtbl();
      super::fill_iterator_access_vtbl(
         vtbl, 2,
         sizeof(reverse_iterator), sizeof(const_reverse_iterator),
         rit_destructor(typename super::non_const_access()),
         rit_destructor(std::false_type()),
         rbegin(typename super::non_const_access()),
         rbegin(std::false_type()),
         rderef(bool_constant<super::is_sparse>(), typename super::non_const_access()),
         rderef(bool_constant<super::is_sparse>(), std::false_type())
      );
      return vtbl;
   }

   static SV* create_vtbl(std::true_type)
   {
      return super::create_vtbl();
   }
   static SV* create_vtbl()
   {
      return create_vtbl(bool_constant<super::is_associative>());
   }
public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return super::register_me(name, cpperl_file, inst_num, someref, generated_by, create_vtbl());
   }
};

template <typename T>
class ContainerClassRegistrator<T, random_access_iterator_tag>
   : public ContainerClassRegistrator<T, bidirectional_iterator_tag> {
   typedef ContainerClassRegistrator<T, bidirectional_iterator_tag> super;
protected:
   static void random_impl(char* p_obj, char*, Int index, SV* dst, SV* container_sv)
   {
      T* obj = reinterpret_cast<T*>(p_obj);
      index = index_within_range(super::helper::streamline(*obj), index);
      Value pv(dst, ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      pv.put_lvalue(super::helper::streamline(*obj)[index], container_sv);
   }

   static void crandom(char* p_obj, char*, Int index, SV* dst, SV* container_sv)
   {
      const T* obj = reinterpret_cast<const T*>(p_obj);
      index = index_within_range(super::helper::streamline(*obj), index);
      Value pv(dst, ValueFlags::read_only | ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      pv.put_lvalue(super::helper::streamline(*obj)[index], container_sv);
   }

   static void random_sparse(char* p_obj, char*, Int index, SV* dst, SV* container_sv)
   {
      T* obj = reinterpret_cast<T*>(p_obj);
      index = index_within_range(super::helper::streamline(*obj), index);
      Value pv(dst, ValueFlags::expect_lval | ValueFlags::allow_non_persistent);
      pv.put(super::helper::streamline(*obj)[index], container_sv);
   }

   static container_access_type random(std::false_type, std::true_type)
   {
      return &random_impl;
   }
   static container_access_type random(std::true_type, std::true_type)
   {
      return &random_sparse;
   }
   template <typename is_sparse>
   static container_access_type random(is_sparse, std::false_type)
   {
      return &crandom;
   }
public:
   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      SV* vtbl=super::create_vtbl();
      super::fill_random_access_vtbl(
         vtbl,
         random(bool_constant<super::is_sparse>(), typename super::non_const_access()),
         random(bool_constant<super::is_sparse>(), std::false_type())
      );
      return super::register_me(name, cpperl_file, inst_num, someref, generated_by, vtbl);
   }
};

template <typename T>
class ClassRegistrator<T, is_container>
   : public ContainerClassRegistrator<T> {};

template <typename T, int n = 0, int l = list_length<typename object_traits<T>::elements>::value>
struct CompositeClassRegistrator {
   typedef typename n_th<typename object_traits<T>::elements, n>::type member_type;
   static const bool allow_non_const_access = !attrib<member_type>::is_const &&
                                              !object_traits<typename deref<member_type>::type>::is_always_const;

   static void get_impl(char* p, SV* dst, SV* container_sv)
   {
      T* obj = reinterpret_cast<T*>(p);
      Value pv(dst, ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      pv.put_lvalue(visit_n_th(*obj, int_constant<n>()), container_sv);
   }

   static void cget(char* p, SV* dst, SV* container_sv)
   {
      const T* obj = reinterpret_cast<const T*>(p);
      Value pv(dst, ValueFlags::read_only | ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      pv.put_lvalue(visit_n_th(*obj, int_constant<n>()), container_sv);
   }

   static void store_impl(char* p, SV* src)
   {
      T* obj = reinterpret_cast<T*>(p);
      Value v(src, ValueFlags::not_trusted);
      v >> visit_n_th(*obj, int_constant<n>());
   }

   static composite_access_type get(std::true_type)  { return &get_impl; }
   static composite_access_type get(std::false_type) { return &cget; }
   static composite_store_type store(std::true_type)  { return &store_impl; }
   static composite_store_type store(std::false_type) { return nullptr; }

   static void init(composite_access_vtbl* acct)
   {
      acct->get[0]=get(bool_constant<allow_non_const_access>());
      acct->get[1]=get(std::false_type());
      acct->store=store(bool_constant<allow_non_const_access>());
      CompositeClassRegistrator<T, n+1, l>::init(++acct);
   }

   static SV* provide_member_names() { return member_names(recognizer_bait(), (T*)nullptr); }
};

template <typename T, int l>
struct CompositeClassRegistrator<T, l, l> {
   static void init(composite_access_vtbl*) {}
};

template <typename T>
struct get_persistent_type {
   typedef typename object_traits<T>::persistent_type type;
};

template <typename T>
class ClassRegistrator<T, is_composite>
   : public ClassRegistratorBase {
public:
   typedef typename list_transform_unary<get_persistent_type, typename object_traits<T>::elements>::type elements;
   static_assert(!list_search<elements, int, std::is_same>::value, "found composite type with int member - use Int instead");

   static SV* register_it(const AnyString& name, SV* someref, SV* generated_by,
                          const AnyString& cpperl_file = AnyString(), int inst_num = 0)
   {
      return register_class(
         name, cpperl_file, inst_num, someref, generated_by,
         typeid(T).name(),
         is_mutable<T>::value,
         ClassFlags::is_composite | Serializable<T>::flag_value() | ordered_flag<T>(),
         create_composite_vtbl(
            typeid(T), sizeof(T), object_traits<T>::total_dimension,
            Copy<T>::func(),
            Assign<T>::func(),
            Destroy<T>::func(),
            ToString<T>::func(),
            Serializable<T>::conv(),
            Serializable<T>::provide(),
            list_length<elements>::value,
            &TypeListUtils<elements>::provide_types,
            &TypeListUtils<elements>::provide_descrs,
            &CompositeClassRegistrator<T>::provide_member_names,
            &CompositeClassRegistrator<T>::init
         )
      );
   }
};

} }
namespace polymake { namespace perl_bindings {

template <typename T>
class Class
   : public pm::perl::ClassRegistrator<T> {
   Class() = delete;
public:
   void add__me(const AnyString& name, const AnyString& cpperl_file, int inst_num) const
   {
      this->register_it(name, this->queue, nullptr, cpperl_file, inst_num);
   }
};

} }
namespace pm { namespace perl {

class ClassTemplate {
   ClassTemplate() = delete;
public:
   static void add__me(const AnyString& name);
};

struct CrossApp
   : public AnyString {
   using AnyString::AnyString;
};

template <typename T>
struct static_class {
   using type = T;
   static const bool skip_first_arg = true;
};

template <typename T, typename... Given>
struct static_class<T(Given...)> {
   using type = T;
   static const bool skip_first_arg = sizeof...(Given) != 0;
};

template <typename T>
using static_class_t = typename static_class<T>::type;

class FunctionWrapperBase
   : public RegistratorQueue {
   FunctionWrapperBase() = delete;
protected:
   void register_it(bool is_template, wrapper_type wrapper, const AnyString& uniq_name, const AnyString& cpperl_file, int inst_num,
                    SV* arg_types, SV* cross_apps, type_reg_fn_type result_type_reg) const;

   template <typename... T>
   static SV* store_type_names(mlist<T...>)
   {
      // filter out void's
      using types = typename mlist_transform_unary<typename mlist_concat<T...>::type, access>::type;
      ArrayHolder arr(mlist_length<types>::value);
      push_type_names(arr, types());
      return arr.get();
   }

   template <int N>
   static SV* store_type_names(int_constant<N>)
   {
      return Scalar::const_int(N);
   }

   template <typename T>
   static constexpr int arg_ref_kind()
   {
      return std::is_lvalue_reference<T>::value
             ? (is_const<T>::value ? arg_is_const_ref : arg_is_lval_ref) :
             (std::is_enum<pure_type_t<T>>::value || 
              mlist_contains<primitive_lvalues, pure_type_t<T>>::value ||
              mlist_contains<Value::nomagic_types, pure_type_t<T>>::value)
             ? arg_is_const_ref
             : arg_is_univ_ref;
   }

   template <typename... T>
   static void push_type_names(ArrayHolder& arr, mlist<T...>)
   {
      (void)std::initializer_list<bool>{ (arr.push(Scalar::const_string_with_int(typeid(T).name(), arg_ref_kind<T>())), true)... };
   }

   template <typename... T>
   static SV* store_cross_apps(const std::tuple<T...>& names)
   {
      if (sizeof...(T) == 0) return nullptr;
      ArrayHolder arr(static_cast<Int>(sizeof...(T)));
      push_names(arr, names, typename index_sequence_for<std::tuple<T...>>::type());
      return arr.get();
   }

   template <typename... T, size_t... Indexes>
   static void push_names(ArrayHolder& arr, const std::tuple<T...>& names, std::index_sequence<Indexes...>)
   {
      (void)std::initializer_list<bool>{ (arr.push(Scalar::const_string(std::get<Indexes>(names))), true)... };
   }

   template <typename T>
   static decltype(auto) result_type_registrator(SV* prescribed_pkg, SV* app_stash_ref, SV* generated_by)
   {
      return type_cache<T>::provide(prescribed_pkg, app_stash_ref, generated_by);
   }

   template <typename ArgTypes>
   static constexpr bool skip_first_arg(bool is_static, ArgTypes)
   {
      return is_static && static_class<typename mlist_head<ArgTypes>::type>::skip_first_arg;
   }
};

struct FunctionCaller {
   static const bool is_static=false;
   static const RegistratorQueue::Kind kind = RegistratorQueue::Kind::function;
   // tags for choosing the caller template
   // suffix _t denotes a variant with explicit template parameters
   enum class FuncKind {
      free, free_t,          // free function, possibly namespace-qualified
      meth, meth_t,          // object method
      stat, stat_t           // static method
   };
};
struct StaticFunctionCaller {
   static const bool is_static=true;
   static const RegistratorQueue::Kind kind = RegistratorQueue::Kind::function;
};

template <typename Fptr, Fptr fptr>
struct CallerViaPtr {
   static const bool is_static=false;
   static const RegistratorQueue::Kind kind = RegistratorQueue::Kind::embedded_rule;

   template <size_t... I_, typename... T_>
   decltype(auto) operator()(const ArgValues<sizeof...(T_)>& args_,
                             mlist<>, mlist<T_...>, std::index_sequence<I_...>) const
   {
      return fptr(args_.template get<I_, T_>()...);
   }
   template <typename Consumer_, size_t... I_, typename... T_>
   decltype(auto) operator()(const Consumer_ consumer_, const ArgValues<sizeof...(T_)>& args_,
                             mlist<>, mlist<T_...>, std::index_sequence<I_...>) const
   {
      return consumer_(fptr(args_.template get<I_, T_>()...), args_);
   }
};

struct SpecialOperator : FunctionCaller {
   struct NoImpl {
      template <typename... T>
      Undefined operator() (T&&...) const;

      static wrapper_type get_wrapper_ptr() { return nullptr; }
   };
};

template <typename Caller, typename... T>
struct caller_type {
   using type = Caller;
};

template <typename Caller, typename... T>
struct caller_type<Caller, mlist<T...>, std::enable_if_t<is_derived_from<Caller, SpecialOperator>::value>> {
   using type = typename Caller::template Impl<T...>;
};

template <size_t... anchors>
class ConsumeRetScalar {
public:
   template <size_t n_args, typename Ret>
   SV* operator() (Ret&& ret, const ArgValues<n_args>& args) const
   {
      Value result(ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      result.put(std::forward<Ret>(ret), args.template get<anchors, SV*>()...);
      return result.get_temp();
   }
};

template <typename Arg0, size_t... anchors>
class ConsumeRetLvalue {
public:
   template <size_t n_args, typename Ret>
   std::enable_if_t<(std::is_lvalue_reference<Ret&&>::value && std::is_same<Ret, typename access<Arg0>::type>::value), SV*>
   operator() (Ret&& ret, const ArgValues<n_args>& args) const
   {
      if (&ret == &unwary(args.template get<0, Arg0>()))
         return args.template get<0, SV*>();
      return put_lval(std::forward<Ret>(ret), args);
   }

   template <size_t n_args, typename Ret>
   std::enable_if_t<!(std::is_lvalue_reference<Ret&&>::value && std::is_same<Ret, typename access<Arg0>::type>::value), SV*>
   operator() (Ret&& ret, const ArgValues<n_args>& args) const
   {
      return put_lval(std::forward<Ret>(ret), args);
   }
private:
   template <size_t n_args, typename Ret>
   SV* put_lval(Ret&& ret, const ArgValues<n_args>& args) const
   {
      ValueFlags read_only_flag = is_const<Ret>::value ? ValueFlags::read_only : ValueFlags::is_mutable;
      Value result(read_only_flag | ValueFlags::expect_lval | ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref);
      result.put_lvalue(std::forward<Ret>(ret), args.template get<anchors, SV*>()...);
      return result.get_temp();
   }
};

template <typename Caller, Returns returns, int n_explicit, typename... T>
class FunctionWrapper;

template <typename Caller, Returns returns, int n_explicit, typename... T, size_t... anchors>
class FunctionWrapper<Caller, returns, n_explicit, mlist<T...>, std::index_sequence<anchors...>>
   : public FunctionWrapperBase {
public:
   static const Kind kind = Caller::kind;
protected:
   FunctionWrapper() = delete;

   static constexpr size_t n_args = sizeof...(T) - n_explicit - Caller::is_static;

   using arg_values = ArgValues<n_args>;
   using all_types = mlist<T...>;
   using explicit_params = typename mlist_slice<all_types, 0, n_explicit>::type;
   using arg_types = typename mlist_slice<all_types, n_explicit>::type;
   using arg_pure_types = mlist_pure_types<typename mlist_transform_unary<arg_types, access>::type>;
   using indexes = std::make_index_sequence<n_args>;
   using caller_t = typename caller_type<Caller, all_types, void>::type;
   using result_type = decltype(caller_t()(std::declval<arg_values>(),
                                           explicit_params(), arg_types(), indexes()));

   using result_discr = typename mselect< std::enable_if<is_among<result_type, SV*>::value, mlist<result_type>>,
                                          std::enable_if<std::is_same<result_type, void>::value || returns == Returns::empty, mlist<void>>,
                                          std::enable_if<is_among<result_type, Undefined, ListReturn>::value, mlist<void>>,
                                          std::enable_if<is_instance_of<result_type, std::tuple>::value || returns == Returns::list, std::tuple<>>,
                                          bool_constant<returns == Returns::lvalue>
                                        >::type;

   template <typename Result, bool enable = !mlist_contains<arg_pure_types, Result>::value &&
                                            !object_traits<Result>::is_lazy &&
                                            is_among<result_discr, std::true_type, std::false_type>::value>
   struct needs_result_type_registrator
      : std::false_type {};

   template <typename Result>
   struct needs_result_type_registrator<Result, true>
      : bool_constant<type_cache<Result>::prefer_early_registration> {};

   static SV* call(SV** stack)
   {
      const arg_values args{stack + skip_first_arg(Caller::is_static, arg_types())};
      return consume_result(args, result_discr());
   }

   // void function or ListReturn which stores all value directly in the stack
   static SV* consume_result(const arg_values& args, mlist<void>)
   {
      Caller()(args, explicit_params(), arg_types(), indexes());
      return nullptr;
   }

   // regular function or method returning some value
   static SV* consume_result(const arg_values& args, std::false_type)
   {
      return Caller()(ConsumeRetScalar<anchors...>{}, args, explicit_params(), arg_types(), indexes());
   }

   // regular function or method returning an lvalue
   static SV* consume_result(const arg_values& args, std::true_type)
   {
      return Caller()(ConsumeRetLvalue<typename mlist_head<arg_types>::type, anchors...>{},
                      args, explicit_params(), arg_types(), indexes());
   }

   // function returning a list of values
   static SV* consume_result(const arg_values& args, std::tuple<>)
   {
      std::conditional_t<is_instance_of<pure_type_t<result_type>, std::tuple>::value, ListReturn, ListSlurp> results{};
      results << Caller()(args, explicit_params(), arg_types(), indexes());
      return nullptr;
   }

   // special Caller consuming the value itself, e.g. a constructor
   static SV* consume_result(const arg_values& args, mlist<SV*>)
   {
      return Caller()(args, explicit_params(), arg_types(), indexes());
   }

   static wrapper_type get_wrapper_ptr()
   {
      return &call;
   }

   static type_reg_fn_type get_type_registrator(std::true_type)
   {
      return &result_type_registrator<pure_type_t<result_type>>;
   }

   static type_reg_fn_type get_type_registrator(std::false_type)
   {
      return nullptr;
   }

public:
   template <typename... Attrs>
   void add__me(const AnyString& uniq_name, const AnyString& cpperl_file, int inst_num, Attrs&&... attrs) const
   {
      std::tuple<Attrs...> attr_tuple{ std::forward<Attrs>(attrs)... };
      register_it(kind == Kind::function,
                  std::conditional_t<std::is_same<result_type, Undefined>::value, caller_t, FunctionWrapper>::get_wrapper_ptr(),
                  uniq_name, cpperl_file, inst_num,
                  store_type_names(std::conditional_t<kind == Kind::function, mlist<T...>, int_constant<int(sizeof...(T))>>()),
                  store_cross_apps(forward_all_args_of_type<CrossApp>(attr_tuple)),
                  get_type_registrator(needs_result_type_registrator<pure_type_t<result_type>>()));
   }
};

template <typename Arg> class AnchorArg;

template <typename T>
struct unwrap_anchor
   : std::false_type {
   using type = T;
};

template <typename T>
struct unwrap_anchor<AnchorArg<T>>
   : std::true_type {
   using type = T;
};

template <typename Caller, Returns returns, int n_explicit, typename... T>
class FunctionWrapper<Caller, returns, n_explicit, mlist<T...>>
   : public FunctionWrapper<Caller, returns, n_explicit, typename mlist_transform_unary<mlist<T...>, unwrap_anchor>::type,
                            typename index_sequence_for<mlist_filter_unary<typename mlist_slice<mlist<T...>, n_explicit>::type, unwrap_anchor>>::type> {};

template <typename T>
struct add_try_canned {
   using type = std::conditional_t<Value::check_for_magic_storage<pure_type_t<T>>::value && std::is_lvalue_reference<T>::value,
                                   TryCanned<std::remove_reference_t<T>>,
                                   pure_type_t<T>>;

   static_assert(Value::check_for_magic_storage<pure_type_t<T>>::value ||
                 !(std::is_lvalue_reference<T>::value && !std::is_const<std::remove_reference_t<T>>::value),
                 "primitive data types and perl objects can't be bound to non-const lvalue references");
};

template <typename Fptr>
struct regular_func_arg_list;

template <typename Ret, typename... Args>
struct regular_func_arg_list<Ret(*)(Args...)>
   : mlist_transform_unary<mlist<Args...>, add_try_canned> {};

template <typename Fptr, Fptr fptr>
using RegularFunctionWrapper =
   FunctionWrapper<CallerViaPtr<Fptr, fptr>, Returns::normal, 0,
                   typename regular_func_arg_list<Fptr>::type, std::index_sequence<>>;

class EmbeddedRule
   : public RegistratorQueue {
private:
   EmbeddedRule() = delete;
public:
   static const Kind kind = Kind::embedded_rule;
   void add__me(const AnyString& text, const AnyString& source_line) const;
};

struct OperatorCallerName4perl(new) : FunctionCaller {
   template <size_t... I_, typename T0_, typename... T_>
   SV* operator()(const ArgValues<sizeof...(T_)+1>& args_,
                  mlist<>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const
   {
      Value result;
      new(result.allocate<T0_>(args_.template get<0, SV*>())) T0_(args_.template get<I_, T_>()...);
      return result.get_constructed_canned();
   } 
};

struct OperatorCallerName4perl(assign) : SpecialOperator {

   template <typename Target, typename Source,
             bool enabled=can_assign_to<typename access<Source>::type, Target>::value>
   struct Impl : NoImpl {};

   template <typename Target, typename Source>
   struct Impl<Target, Source, true> : NoImpl {

      static void call(Target& dst, const Value& src)
      {
         if (MaybeWary<Target>::value && src.get_flags() * ValueFlags::not_trusted)
            maybe_wary(dst)=src.get<Source>();
         else
            dst=src.get<Source>();
      }
      static wrapper_type get_wrapper_ptr() { return reinterpret_cast<wrapper_type>(&call); }
   };
};

struct OperatorCallerName4perl(convert) : SpecialOperator {

   template <typename Target, typename Source,
             bool enabled=std::is_constructible<Target, typename access<Source>::type>::value>
   struct Impl : NoImpl {};

   template <typename Target, typename Source>
   struct Impl<Target, Source, true> : NoImpl {

      static Target call(const Value& src)
      {
         return Target(src.get<Source>());
      }
      static wrapper_type get_wrapper_ptr() { return reinterpret_cast<wrapper_type>(&call); }
   };
};


UnaryOperatorCallerBody4perl(-, neg);
UnaryOperatorCallerBody4perl(~, com);
UnaryOperatorCallerBody4perl(++, inc);
UnaryOperatorCallerBody4perl(--, dec);
UnaryOperatorCallerBody4perl(!is_zero, boo);
UnaryOperatorCallerBody4perl(is_zero, not);

BinaryOperatorCallerBody4perl(+, add);
BinaryOperatorCallerBody4perl(-, sub);
BinaryOperatorCallerBody4perl(*, mul);
BinaryOperatorCallerBody4perl(/, div);
BinaryOperatorCallerBody4perl(%, mod);
BinaryOperatorCallerBody4perl(&, and);
BinaryOperatorCallerBody4perl(|, _or);
BinaryOperatorCallerBody4perl(^, xor);
BinaryOperatorCallerBody4perl(<<, lsh);
BinaryOperatorCallerBody4perl(>>, rsh);

BinaryOperatorCallerBody4perl(+=, Add);
BinaryOperatorCallerBody4perl(-=, Sub);
BinaryOperatorCallerBody4perl(*=, Mul);
BinaryOperatorCallerBody4perl(/=, Div);
BinaryOperatorCallerBody4perl(%=, Mod);
BinaryOperatorCallerBody4perl(&=, And);
BinaryOperatorCallerBody4perl(|=, _Or);
BinaryOperatorCallerBody4perl(^=, Xor);
BinaryOperatorCallerBody4perl(<<=, Lsh);
BinaryOperatorCallerBody4perl(>>=, Rsh);

BinaryOperatorCallerBody4perl(==, _eq);
BinaryOperatorCallerBody4perl(!=, _ne);
BinaryOperatorCallerBody4perl(<,  _lt);
BinaryOperatorCallerBody4perl(<=, _le);
BinaryOperatorCallerBody4perl(>,  _gt);
BinaryOperatorCallerBody4perl(>=, _ge);

struct OperatorCallerName4perl(brk) : public FunctionCaller {
   template <typename T0_, typename T1_>
   decltype(auto) operator()(const ArgValues<2>& args_,
                             mlist<>, mlist<T0_, T1_>, std::index_sequence<0, 1>) const
   {
      return args_.template get<0, T0_>()[args_.template get<1, T1_>()];
   }
   template <typename Consumer_, typename T0_, typename T1_>
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const ArgValues<2>& args_,
                             mlist<>, mlist<T0_, T1_>, std::index_sequence<0, 1>) const
   {
      return consumer_(args_.template get<0, T0_>()[args_.template get<1, T1_>()], args_);
   }
};

struct OperatorCallerName4perl(cal) : public FunctionCaller {
   template <size_t... I_, typename T0_, typename... T_>
   decltype(auto) operator()(const ArgValues<sizeof...(T_)+1>& args_,
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const
   {
      return args_.template get<0, T0_>()(args_.template get<I_, T_>()...);
   }
   template <typename Consumer_, size_t... I_, typename T0_, typename... T_>
   decltype(auto) operator()(const Consumer_ consumer_,                 \
                             const ArgValues<sizeof...(T_)+1>& args_,
                             mlist<>, mlist<T0_, T_...>, std::index_sequence<0, I_...>) const
   {
      return consumer_(args_.template get<0, T0_>()(args_.template get<I_, T_>()...), args_);
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
