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

#ifndef POLYMAKE_INTERNAL_TYPE_UNION_H
#define POLYMAKE_INTERNAL_TYPE_UNION_H

#include "polymake/internal/type_manip.h"
#include <stdexcept>

namespace pm {

template <typename TypeList, typename heap_based=std::false_type>
class type_union;

namespace unions {

struct empty_union_def {
   static void invalid_op(char*, ...) __attribute__((noreturn));
   static void trivial_op(char*, ...);
};

template <typename TypeList, typename Operation>
class Function
   : public Function<typename mlist_wrap<TypeList>::type, Operation> {};

template <typename... T, typename Operation>
class Function<mlist<T...>, Operation> {
   using fpointer = typename Operation::fpointer;
   static constexpr int length = sizeof...(T);

   static const fpointer table[length+1];
public:
   static fpointer get(int discr) { return table[discr+1]; }
};

template <typename... T, typename Operation>
const typename Function<mlist<T...>, Operation>::fpointer
Function<mlist<T...>, Operation>::table[] = { Operation::no_op(), &Operation::template execute<T>... };

template <typename TypeList1, typename TypeList2>
class Mapping
   : public Mapping<TypeList1, typename mlist_wrap<TypeList2>::type> {};

template <typename TypeList1, typename... T>
class Mapping<TypeList1, mlist<T...>> {
   static const int table[sizeof...(T)];
public:
   static int get(int discr) { return table[discr]; }
};

template <typename TypeList1, typename... T>
const int Mapping<TypeList1, mlist<T...>>::table[] = { mlist_find<TypeList1, T>::pos... };

template <typename T, typename Arg>
using fits_as_init = bool_constant<(same_pure_type<T, Arg>::value &&
                                    (!std::is_lvalue_reference<T>::value ||
                                     (std::is_lvalue_reference<Arg>::value && is_const<T>::value >= is_const<Arg>::value)))>;

template <typename T,
          bool IsMutable=is_mutable<T>::value,
          bool IsReference=std::is_reference<T>::value>
struct basics {         // non-reference, const
   using type = T;

   static const type& get(const char* obj)
   {
      return *reinterpret_cast<const type*>(obj);
   }

   static void construct_default(char* dst)
   {
      new(dst) type;
   }

   template <typename Source>
   static void construct(char* dst, Source&& src)
   {
      new(dst) type(std::forward<Source>(src));
   }

   static void destroy(char* obj)
   {
      destroy_at(reinterpret_cast<type*>(obj));
   }
};

template <typename T>
struct basics<T, false, true> { // reference, const
   using type = std::remove_reference_t<T>;

   static std::add_const_t<type>& get(const char* obj)
   {
      return **reinterpret_cast<const type* const*>(obj);
   }

   static void construct_default(char* dst)
   {
      throw std::invalid_argument("can't create a default value for a reference");
   }

   static void construct(char* dst, const type& src)
   {
      *reinterpret_cast<const type**>(dst)=&src;
   }

   static void destroy(char* dst) { }
};

template <typename T>
struct basics<T, true, false>   // non-reference, non-const
   : basics<T, false, false> {

   static T& get(char* obj)
   {
      return *reinterpret_cast<T*>(obj);
   }
   using basics<T, false, false>::get;
};

template <typename T>
struct basics<T, true, true>    // reference, non-const
   : basics<T, false, true> {

   static std::remove_reference_t<T>& get(char* obj)
   {
      return **reinterpret_cast<std::remove_reference_t<T>**>(obj);
   }
   using basics<T, false, true>::get;
};

template <typename F>
struct for_defined_only {
   using fpointer = F*;
   static fpointer no_op()
   {
      return reinterpret_cast<fpointer>(&empty_union_def::invalid_op);
   }
};

template <typename F>
struct for_any_state {
   using fpointer = F*;
   static fpointer no_op()
   {
      return reinterpret_cast<fpointer>(&empty_union_def::trivial_op);
   }
};

struct default_constructor : for_defined_only<void(char*)> {
   template <typename T>
   static void execute(char* dst)
   {
      basics<T>::construct_default(dst);
   }
};

struct copy_constructor : for_any_state<void(char*, const char*)> {
   template <typename T>
   static void execute(char* dst, const char* src)
   {
      basics<T>::construct(dst, basics<T>::get(src));
   }
};

struct move_constructor : for_any_state<void(char*, char*)> {
   template <typename T>
   static void execute(char* dst, char* src)
   {
      basics<T>::construct(dst, std::move(basics<T>::get(src)));
   }
};

struct destructor : for_any_state<void(char*)> {
   template <typename T>
   static void execute(char* obj)
   {
      basics<T>::destroy(obj);
   }
};

template <typename T, typename TypeList,
          bool is_possible = is_derived_from_instance_of<T, type_union>::value>
struct is_smaller_union : std::false_type {};

template <typename T, typename TypeList>
struct is_smaller_union<T, TypeList, true> {
   using type_list = typename mget_template_parameter<typename is_derived_from_instance_of<T, type_union>::instance_type, 0>::type;
   static constexpr bool value = mlist_is_included<type_list, TypeList>::value && !std::is_same<type_list, TypeList>::value;
};

} // end namespace unions

template <typename T>
struct union_element_traits {
   struct type {
      using element_type = pure_type_t<T>;
      static constexpr bool homogeneous = true;
      static constexpr size_t size = std::is_reference<T>::value ? sizeof(element_type*) : sizeof(element_type);
      static constexpr size_t alignment = std::is_reference<T>::value ? alignof(element_type*) : alignof(element_type);
      static constexpr bool trivial_destructor = std::is_trivially_destructible<element_type>::value;
   };
};

template <typename Traits1, typename Traits2>
struct combine_union_traits {
   struct type {
      using element_type = typename Traits1::element_type;
      static constexpr bool homogeneous = Traits2::homogeneous && isomorphic_types<element_type, typename Traits2::element_type>::value;
      static constexpr size_t size = Traits1::size >= Traits2::size ? Traits1::size : Traits2::size;
      static constexpr size_t alignment = Traits1::alignment >= Traits2::alignment ? Traits1::alignment : Traits2::alignment;
      static constexpr bool trivial_destructor = Traits1::trivial_destructor && Traits2::trivial_destructor;
   };
};

template <typename TypeList>
using union_traits = typename mlist_fold_transform<typename mlist_reverse<TypeList>::type, union_element_traits, combine_union_traits>::type;

template <typename TypeList, bool heap_based>
class type_union_area {
protected:
   alignas(union_traits<TypeList>::alignment) char area[union_traits<TypeList>::size];
};

template <typename TypeList>
class type_union_area<TypeList, true> {
protected:
   char* area;

   type_union_area()
      : area(new char[union_traits<TypeList>::size]) { }

   ~type_union_area() { delete[] area; }
};

template <typename TypeList, typename heap_based>
class type_union : protected type_union_area<TypeList, heap_based::value> {
protected:
   int discriminant;

   template <int discr>
   using basics = unions::basics<typename mlist_at<TypeList, discr>::type>;

   template <typename Operation>
   using function = unions::Function<TypeList, Operation>;

   template <typename Source>
   using mapping = unions::Mapping<TypeList, typename unions::is_smaller_union<pure_type_t<Source>, TypeList>::type_list>;

   void destroy(std::true_type) {}

   void destroy(std::false_type)
   {
      function<unions::destructor>::get(discriminant)(this->area);
   }

   void destroy()
   {
      destroy(bool_constant<union_traits<TypeList>::trivial_destructor>());
   }

   template <typename Source, int discr>
   void init_from_value(Source&& src, int_constant<discr>)
   {
      discriminant = discr;
      basics<discr>::construct(this->area, std::forward<Source>(src));
   }

   template <typename Source>
   std::enable_if_t<std::is_lvalue_reference<Source>::value && unions::is_smaller_union<pure_type_t<Source>, TypeList>::value>
   init_from_value(Source&& src, int_constant<-1>)
   {
      discriminant=mapping<Source>::get(src.discriminant);
      function<unions::copy_constructor>::get(discriminant)(this->area, src.area);
   }

   template <typename Source>
   std::enable_if_t<!std::is_lvalue_reference<Source>::value && unions::is_smaller_union<pure_type_t<Source>, TypeList>::value>
   init_from_value(Source&& src, int_constant<-1>)
   {
      discriminant=mapping<Source>::get(src.discriminant);
      function<unions::move_constructor>::get(discriminant)(this->area, src.area);
   }

   template <typename Source>
   void init_impl(Source&& src, std::false_type)
   {
      init_from_value(std::forward<Source>(src), int_constant<mlist_find_if<TypeList, unions::fits_as_init, Source>::pos>());
   }

   template <typename Source>
   std::enable_if_t<std::is_lvalue_reference<Source>::value>
   init_impl(Source&& src, std::true_type)
   {
      discriminant=src.discriminant;
      function<unions::copy_constructor>::get(discriminant)(this->area, src.area);
   }

   template <typename Source>
   std::enable_if_t<!std::is_lvalue_reference<Source>::value>
   init_impl(Source&& src, std::true_type)
   {
      discriminant=src.discriminant;
      function<unions::move_constructor>::get(discriminant)(this->area, src.area);
   }

   template <typename Source>
   void assign_impl(Source&& src, std::false_type)
   {
      destroy();
      init_from_value(std::forward<Source>(src), int_constant<mlist_find_if<TypeList, unions::fits_as_init, Source>::pos>());
   }

   template <typename Source>
   void assign_impl(Source&& src, std::true_type)
   {
      destroy();
      init_impl(std::forward<Source>(src), std::true_type());
   }

public:
   type_union() : discriminant(-1) { }

   type_union(const type_union& src)
   {
      init_impl(src, std::true_type());
   }

   type_union(type_union&& src)
   {
      init_impl(std::move(src), std::true_type());
   }

   template <typename Source,
             typename=std::enable_if_t<(is_derived_from<pure_type_t<Source>, type_union>::value ||
                                        mlist_find_if<TypeList, unions::fits_as_init, Source>::value ||
                                        unions::is_smaller_union<pure_type_t<Source>, TypeList>::value)>>
   explicit type_union(Source&& src)
   {
      init_impl(std::forward<Source>(src), is_derived_from<pure_type_t<Source>, type_union>());
   }

   template <typename Source, typename=std::enable_if_t<mlist_find_if<TypeList, unions::fits_as_init, Source>::value>>
   Source* init()
   {
      constexpr int discr=mlist_find_if<TypeList, unions::fits_as_init, Source>::pos;
      if (discriminant>=0) {
         if (discriminant != discr) return nullptr;
      } else {
         discriminant=discr;
         basics<discr>::construct_default(this->area);
      }
      return &basics<discr>::get(this->area);
   }

   ~type_union()
   {
      destroy();
   }

   type_union& operator= (const type_union& src)
   {
      assign_impl(src, std::true_type());
      return *this;
   }

   type_union& operator= (type_union&& src)
   {
      assign_impl(std::move(src), std::true_type());
      return *this;
   }

   template <typename Source>
   std::enable_if_t<(is_derived_from<pure_type_t<Source>, type_union>::value ||
                     mlist_find_if<TypeList, unions::fits_as_init, Source>::value ||
                     unions::is_smaller_union<pure_type_t<Source>, TypeList>::value), type_union&>
   operator= (Source&& src)
   {
      assign_impl(std::forward<Source>(src), is_derived_from<pure_type_t<Source>, type_union>());
      return *this;
   }

   template <typename T, typename=std::enable_if_t<mlist_find_if<TypeList, same_pure_type, T>::value>>
   T* as()
   {
      constexpr int discr=mlist_find_if<TypeList, same_pure_type, T>::pos;
      if (discriminant != discr) return nullptr;
      return &basics<discr>::get(this->area);
   }

   template <typename T, typename=std::enable_if_t<mlist_find_if<TypeList, same_pure_type, T>::value>>
   std::add_const_t<T>* as() const
   {
      constexpr int discr=mlist_find_if<TypeList, same_pure_type, T>::pos;
      if (discriminant != discr) return nullptr;
      return &basics<discr>::get(this->area);
   }

   int get_discriminant() const { return discriminant; }
   bool valid() const { return discriminant>=0; }

   template <typename, typename> friend class type_union;
};

template <typename TypeList, typename heap_based, typename Object2>
struct isomorphic_types<type_union<TypeList, heap_based>, Object2>
   : std::conditional_t<union_traits<TypeList>::homogeneous, isomorphic_types<typename union_traits<TypeList>::element_type, Object2>, anisomorphic_types> {};

template <typename Object1, typename TypeList, typename heap_based>
struct isomorphic_types<Object1, type_union<TypeList, heap_based> >
   : std::conditional_t<union_traits<TypeList>::homogeneous, isomorphic_types<Object1, typename union_traits<TypeList>::element_type>, anisomorphic_types> {};

template <typename TypeList1, typename heap_based1, typename TypeList2, typename heap_based2>
struct isomorphic_types< type_union<TypeList1, heap_based1>, type_union<TypeList2, heap_based2> >
   : std::conditional_t<union_traits<TypeList1>::homogeneous && union_traits<TypeList2>::homogeneous,
                        isomorphic_types<typename union_traits<TypeList1>::element_type, typename union_traits<TypeList2>::element_type>, anisomorphic_types> {};

} // end namespace pm

namespace polymake {

using pm::type_union;

}

#endif // POLYMAKE_INTERNAL_TYPE_UNION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
