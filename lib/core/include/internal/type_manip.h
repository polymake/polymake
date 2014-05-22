/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_INTERNAL_TYPE_MANIP_H
#define POLYMAKE_INTERNAL_TYPE_MANIP_H

#include "polymake/internal/pool_allocator.h"

#include <tr1/type_traits>
#include <new>
#include <memory>
#include <limits>
#include <cstddef>

#if defined(__GNUC__)
#  define POLYMAKE_ALIGN(what,n) what __attribute__ ((aligned (n)))
#elif defined(__INTEL_COMPILER)
#  define POLYMAKE_ALIGN(what,n) __declspec(align(n)) what
#endif

namespace pm {

/** A piece of METALISP - binary choice
    @tmplparam Cond boolean value the choice depends on
    @tmplparam Then result type chosen in the @c true case
    @tmplparam Else result type chosen in the @c false case
*/
template <bool Cond, typename Then, typename Else>
struct if_else {
   typedef Then type;
};

template <typename Then, typename Else>
struct if_else<false,Then,Else> {
   typedef Else type;
};

template <typename Case>
struct choice {
   typedef Case type;
};

template <typename Then, typename Else>
struct choice< if_else<true, Then, Else> > {
   typedef Then type;
};

template <typename Then, typename Else>
struct choice< if_else<false, Then, Else> > : choice<Else> {};

template <typename, bool>
struct enable_if {};

template<typename T>
struct enable_if<T, true> {
   typedef T type;
};

template <typename, bool>
struct disable_if {};

template <typename T>
struct disable_if<T, false> {
   typedef T type;
};

template <typename X, typename Y>
struct project_first {
   typedef X type;
};

template <typename X, typename Y>
struct prefer_first {
   typedef X type;
};

template <typename Y>
struct prefer_first<void, Y> {
   typedef Y type;
};

template <typename X, typename Y>
struct project_second {
   typedef Y type;
};

template <typename X, typename Y>
struct prefer_second {
   typedef Y type;
};

template <typename X>
struct prefer_second<X, void> {
   typedef X type;
};

template <bool x> struct bool2type {
   typedef bool constant_type;
   static const bool value=x;
};
template <int x> struct int2type {
   typedef int constant_type;
   static const int value=x;
};
typedef bool2type<true> True;
typedef bool2type<false> False;

template <typename Class, typename Member, Member Class::*Ptr>
struct ptr2type {
   typedef Member Class::* constant_type;
};

template <typename T>
struct type2type {
   typedef T type;
};

template <typename T1, typename T2>
struct identical : False {
   typedef void type;
};

template <typename T>
struct identical<T,T> : True {
   typedef T type;
};

template <typename T>
struct is_pod : bool2type<std::tr1::is_pod<T>::value> {};
template <typename T>
struct has_trivial_destructor : bool2type<std::tr1::has_trivial_destructor<T>::value> {};

/** Definition of the input argument type

    For a given type, a suitable type of the function input argument is defined.
    Objects of user-defined classes should be passed as a const reference, while
    built-in types are more efficiently passed per value.
*/
template <typename T>
struct function_argument {
   typedef typename if_else<(is_pod<T>::value && sizeof(T)<=sizeof(double)), const T, const T&>::type type;
   typedef type const_type;
};

template <typename T>
struct function_argument<T&> {
   typedef T& type;
   typedef const T& const_type;
};

template <typename T>
struct function_argument<const T&> : function_argument<T&> {
   typedef const T& type;
};

template <>
struct function_argument<void> {
   typedef void* type;
   typedef void* const_type;
};

/** Removal of the pointer/reference modifier

    For a given pointer/reference type, the data type being referred to is revealed.
    For the sake of completeness, non-reference types yield themselves as result.
*/
template <typename C> struct attrib {
   typedef C minus_const_ref;
   typedef C minus_const;
   typedef C minus_ref;
   typedef const C& plus_const_ref;
   typedef const C plus_const;
   typedef const C toggle_const;
   typedef C& plus_ref;
   typedef C& toggle_ref;
   static const bool is_reference=false, is_const=false;
};
template <>
struct attrib<void> {
   typedef void minus_const_ref;
   typedef void minus_const;
   typedef void minus_ref;
   typedef void plus_const_ref;
   typedef void plus_const;
   typedef void plus_ref;
   typedef void toggle_const;
   typedef void toggle_ref;
   static const bool is_reference=false, is_const=false;
};
template <typename C>
struct attrib<const C> : attrib<C> {
   typedef const C minus_ref;
   typedef const C& plus_ref;
   typedef C toggle_const;
   typedef const C& toggle_ref;
   static const bool is_const=true;
};
template <typename C>
struct attrib<C&> : attrib<C> {
   typedef typename attrib<C>::plus_const& plus_const;
   typedef typename attrib<C>::minus_const& minus_const;
   typedef typename attrib<C>::toggle_const& toggle_const;
   typedef C toggle_ref;
   static const bool is_reference=true;
};

template <typename T>
struct pass_by_reference {
   static const bool value=attrib<typename function_argument<T>::type>::is_reference;
};

template <typename T>
struct pass_by_value {
   static const bool value=!attrib<typename function_argument<T>::type>::is_reference;
};

template <typename C>
struct is_pointer : False {};

template <typename C>
struct is_pointer<C*> : True {};

template <typename C>
struct deref : attrib<C> {
   typedef typename attrib<C>::minus_const_ref type;
};

template <typename C>
struct deref_ptr : attrib<C> {
   typedef typename attrib<C>::minus_const_ref type;
};

template <typename C>
struct deref_ptr<C*> : attrib<C> {
   typedef typename attrib<C>::minus_const_ref type;
   typedef const type* plus_const;
   typedef type* minus_const;
   typedef type& minus_const_ref;
};

template <typename T>
struct remove_unsigned {
   typedef T type;
};

template <>
struct remove_unsigned<unsigned char> {
   typedef signed char type;
};

template <>
struct remove_unsigned<unsigned short> {
   typedef short type;
};

template <>
struct remove_unsigned<unsigned int> {
   typedef int type;
};

template <>
struct remove_unsigned<unsigned long> {
   typedef long type;
};

template <typename T, typename From>
struct inherit_const {
   typedef typename if_else<deref<From>::is_const, typename attrib<T>::plus_const, T>::type type;
};

template <typename T, typename From>
struct inherit_ref {
   typedef typename inherit_const<typename deref<T>::minus_ref, From>::type QT;
   typedef typename if_else<(deref<T>::is_reference || deref<From>::is_reference), QT&, QT>::type type;
};

template <typename T, bool make_const>
struct assign_const {
   typedef typename if_else<make_const, typename attrib<T>::plus_const, typename attrib<T>::minus_const>::type type;
};

template <typename T, bool make_ref>
struct assign_ref {
   typedef typename if_else<make_ref, typename attrib<T>::plus_ref, typename attrib<T>::minus_ref>::type type;
};

template <typename T1, typename T2>
struct identical_minus_const {
   static const bool value=identical<typename attrib<T1>::minus_const, typename attrib<T2>::minus_const>::value;
   typedef typename if_else<value, typename assign_const<T1, attrib<T1>::is_const || attrib<T2>::is_const>::type,
                                   void>::type type;
};

template <typename T1, typename T2>
struct identical_minus_ref {
   static const bool value=identical<typename attrib<T1>::minus_ref, typename attrib<T2>::minus_ref>::value;
   typedef typename if_else<value, typename assign_ref<T1, attrib<T1>::is_reference && attrib<T2>::is_reference>::type,
                                   void>::type type;
};

template <typename T1, typename T2>
struct identical_minus_const_ref {
   static const bool value=identical<typename attrib<T1>::minus_const_ref, typename attrib<T2>::minus_const_ref>::value;
   typedef typename if_else<value, typename assign_const<
                                      typename assign_ref<T1, attrib<T1>::is_reference && attrib<T2>::is_reference>::type,
                                      attrib<T1>::is_const || attrib<T2>::is_const>::type,
                                   void>::type type;
};

template <typename Head, typename Tail>
struct cons {
   typedef Head head;
   typedef Head type;
   typedef Tail tail;
};

template <typename T>
struct list_head {
   typedef T type;
};
template <typename Head, typename Tail>
struct list_head< cons<Head,Tail> > {
   typedef Head type;
};

template <typename T>
struct list_tail {
   typedef void type;
};
template <typename Head, typename Tail>
struct list_tail< cons<Head,Tail> > {
   typedef Tail type;
};

template <typename> struct same {};

template <typename List>
struct list_length {
   static const int value=!identical<List,void>::value;
};

template <typename Head, typename Tail>
struct list_length< cons<Head,Tail> > {
   static const int value= list_length<Head>::value + list_length<Tail>::value;
};

template <typename List, int n> struct n_th;

template <typename T>
struct n_th<T,0> {
   typedef T type;
   typedef void tail;
   typedef T list;
};

template <typename Head, typename Tail, int n>
struct n_th<cons<Head, Tail>, n> : n_th<Tail, n-1> {};

template <typename Head, typename Tail>
struct n_th<cons<Head, Tail>, 0> {
   typedef Head type;
   typedef Tail tail;
   typedef cons<Head, Tail> list;
};

template <typename List, int n, typename New> struct subst_n_th;

template <typename T, typename New>
struct subst_n_th<T, 0, New> {
   typedef New type;
};

template <typename Head, typename Tail, int n, typename New>
struct subst_n_th<cons<Head, Tail>, n, New> {
   typedef cons<Head, typename subst_n_th<Tail, n-1, New>::type> type;
};

template <typename Head, typename Tail, typename New>
struct subst_n_th<cons<Head, Tail>, 0, New> {
   typedef cons<New, Tail> type;
};

/// derivation and conversion tests due to Andrei Alexandrescu

template <int n>
struct size_discriminant {
   typedef char (&type)[n];
};

namespace derivation {
   typedef size_discriminant<1>::type no;
   typedef size_discriminant<2>::type yes;

   template <typename Specific>
   struct test_impl {
      static const typename Specific::item *piece();
      struct helper : Specific {
         static no Test(...);
         using Specific::Test;
      };
      static const bool value= sizeof(helper::Test(piece())) == sizeof(yes);
   };

   template <typename Specific>
   struct test : bool2type< test_impl<Specific>::value > {};

   struct anything {
      template <typename T> anything(const T& ...);
   };

   template <typename Specific>
   struct conv_tools {
      static const typename Specific::item& piece();
      struct helper : Specific {
         static no Test(const anything& ...);
         using Specific::Test;
      };
   };

   template <typename Specific>
   struct conv_test : conv_tools<Specific> {
      typedef conv_tools<Specific> super;
      static const bool value= sizeof(super::helper::Test(super::piece(), 1)) == sizeof(yes);
   };
}

template <typename Child, typename Base>
struct derived_from_impl {
   static derivation::yes Test(const Base*);
   typedef Child item;
};

template <typename Child, typename Base>
struct derived_from : derivation::test< derived_from_impl<Child,Base> > {};

template <typename T>
struct derived_from<T, void> : False {};

template <typename T> struct least_derived { typedef T type; };
template <typename T> struct most_derived  { typedef T type; };

template <typename Head, typename Tail>
struct least_derived< cons<Head,Tail> > {
   typedef typename least_derived<Tail>::type C2;
   typedef typename if_else<derived_from<C2,Head>::value, Head,
                            typename if_else<derived_from<Head,C2>::value, C2, void>::type
                           >::type
      type;
};

template <typename Head, typename Tail>
struct most_derived< cons<Head,Tail> > {
   typedef typename most_derived<Tail>::type C2;
   typedef typename if_else<derived_from<C2,Head>::value, C2,
                            typename if_else<derived_from<Head,C2>::value, Head, void>::type
                           >::type
      type;
};

template <typename Concrete, template <typename> class Template>
struct derived_from_instance_impl {
   template <typename T>
   static derivation::yes Test(const Template<T>*);
   typedef Concrete item;
};
template <typename Concrete, template <typename> class Template>
struct derived_from_instance : derivation::test< derived_from_instance_impl<Concrete,Template> > {};

template <typename Concrete, template <typename,typename> class Template>
struct derived_from_instance2_impl {
   template <typename T1, typename T2>
   static derivation::yes Test(const Template<T1,T2>*);
   typedef Concrete item;
};
template <typename Concrete, template <typename,typename> class Template>
struct derived_from_instance2 : derivation::test< derived_from_instance2_impl<Concrete,Template> > {};

template <typename Concrete, template <typename,int> class Template>
struct derived_from_instance2i_impl {
   template <typename T1, int i2>
   static derivation::yes Test(const Template<T1,i2>*);
   typedef Concrete item;
};
template <typename Concrete, template <typename,int> class Template>
struct derived_from_instance2i : derivation::test< derived_from_instance2i_impl<Concrete,Template> > {};

template <typename Concrete, template <typename,typename,typename> class Template>
struct derived_from_instance3_impl {
   template <typename T1, typename T2, typename T3>
   static derivation::yes Test(const Template<T1,T2,T3>*);
   typedef Concrete item;
};
template <typename Concrete, template <typename,typename,typename> class Template>
struct derived_from_instance3 : derivation::test< derived_from_instance3_impl<Concrete,Template> > {};

template <typename Concrete, template <typename,typename,typename,typename> class Template>
struct derived_from_instance4_impl {
   template <typename T1, typename T2, typename T3, typename T4>
   static derivation::yes Test(const Template<T1,T2,T3,T4>*);
   typedef Concrete item;
};
template <typename Concrete, template <typename,typename,typename,typename> class Template>
struct derived_from_instance4 : derivation::test< derived_from_instance4_impl<Concrete,Template> > {};

template <typename Source, typename Target>
struct convertible_impl {
   typedef typename if_else<is_pod<Target>::value, Target, const Target&>::type arg_type;
   static derivation::yes Test(arg_type, int);
   typedef Source item;
};

template <typename Source, typename Target>
struct convertible_to : derivation::conv_test< convertible_impl<Source,Target> > {};

template <typename T>
struct convertible_to<T, void> : False {};

template <typename T>
struct convertible_to<void, T> : False {};

template <>
struct convertible_to<void, void> : True {};

template <typename Source, typename Target, typename ExpectedRet>
struct assignable_impl : derivation::conv_tools< convertible_impl<Source,ExpectedRet> > {
   typedef derivation::conv_tools< convertible_impl<Source,ExpectedRet> > super;
   struct mix_in : public Target {
      mix_in();
      using Target::operator=;
      const derivation::anything& operator=(const derivation::anything&);
   };
   static mix_in& dst();
   static const bool value= sizeof(super::helper::Test((dst()=super::piece()), 1))==sizeof(derivation::yes);
};

template <typename Source, typename Target, typename ExpectedRet=Target,
          bool _is_pod = identical<Source,void>::value || is_pod<Target>::value,
          bool _convertible = convertible_to<Source,Target>::value>
struct assignable_to : bool2type< assignable_impl<Source,Target,ExpectedRet>::value > {};

template <typename Source, typename Target, typename ExpectedRet, bool _convertible>
struct assignable_to<Source, Target, ExpectedRet, true, _convertible> : bool2type<_convertible> {};

template <typename Source, typename Target, typename ExpectedRet>
struct assignable_to<Source, Target, ExpectedRet, false, true> : True {};

template <typename T1, typename T2,
          typename common_base=typename least_derived< cons<typename deref<T1>::type, typename deref<T2>::type> >::type>
struct compatible_helper : True {
   typedef typename assign_const<common_base, attrib<T1>::is_const || attrib<T2>::is_const>::type value_type;
   typedef typename assign_ref<value_type, attrib<T1>::is_reference && attrib<T2>::is_reference>::type type;
};

template <typename T1, typename T2>
struct compatible_helper<T1,T2,void> : False {
   typedef void type;
};

template <typename T1, typename T2>
struct compatible : compatible_helper<T1, T2> {};

template <typename T1, typename T2>
struct compatible<T1*, T2*> : compatible<T1, T2> {
   typedef typename compatible<T1,T2>::type *type;
};

template <typename Concrete, template <typename> class Template>
struct is_instance_of : False {};

template <typename T, template <typename> class Template>
struct is_instance_of<Template<T>, Template> : True {};

template <typename Concrete, template <typename,typename> class Template>
struct is_instance2_of : False {};

template <typename T1, typename T2, template <typename,typename> class Template>
struct is_instance2_of<Template<T1,T2>, Template> : True {};

template <typename Concrete, template <typename,typename,typename> class Template>
struct is_instance3_of : False {};

template <typename T1, typename T2, typename T3, template <typename,typename,typename> class Template>
struct is_instance3_of<Template<T1,T2,T3>, Template> : True {};

template <typename Concrete, template <typename,typename,typename,typename> class Template>
struct is_instance4_of : False {};

template <typename T1, typename T2, typename T3, typename T4, template <typename,typename,typename,typename> class Template>
struct is_instance4_of<Template<T1,T2,T3,T4>, Template> : True {};

/* g++ < 4.1 had a bug in matching template template parameters in partial specializations.

   Given
     template <typename X, typename Y=Default> class FOO;
   and following specializations:
     template <template <typename> class Template, typename X> class BAR< Template<X> >;
     template <template <typename,typename> class Template, typename X, typename Y> class BAR< Template<X,Y> >;

   g++ < 4.1 would match both (and complain about ambiguity), while it should consider only the second.
*/

template <typename T>
struct has_1_template_type_param_impl {
   template <template <typename> class Template, typename T1>
   static derivation::yes Test(const Template<T1>*);
   typedef T item;
};

template <typename T,
          bool _answer=derivation::test< has_1_template_type_param_impl<T> >::value>
struct has_1_template_type_param : int2type<(int(_answer))> {};

template <typename T>
struct has_2_template_type_params_impl {
   template <template <typename,typename> class Template, typename T1, typename T2>
   static derivation::yes Test(const Template<T1,T2>*);
   typedef T item;
};

template <typename T,
          bool _answer=derivation::test< has_2_template_type_params_impl<T> >::value>
struct has_2_template_type_params : int2type<2> {};

template <typename T>
struct has_2_template_type_params<T,false> : has_1_template_type_param<T> {};

template <typename T>
struct has_3_template_type_params_impl {
   template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3>
   static derivation::yes Test(const Template<T1,T2,T3>*);
   typedef T item;
};

template <typename T,
          bool _answer=derivation::test< has_3_template_type_params_impl<T> >::value>
struct has_3_template_type_params : int2type<3> {};

template <typename T>
struct has_3_template_type_params<T,false> : has_2_template_type_params<T> {};

template <typename T>
struct has_4_template_type_params_impl {
   template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4>
   static derivation::yes Test(const Template<T1,T2,T3,T4>*);
   typedef T item;
};

template <typename T,
          bool _answer=derivation::test< has_4_template_type_params_impl<T> >::value>
struct has_4_template_type_params : int2type<4> {};

template <typename T>
struct has_4_template_type_params<T,false> : has_3_template_type_params<T> {};

template <typename T>
struct n_of_template_type_params : has_4_template_type_params<T> {};

template <typename T, int n, int i> struct extract_template_type_param_helper;
template <typename T, typename New, int n, int i> struct subst_template_type_param_helper;

template <template <typename> class Template, typename T>
struct extract_template_type_param_helper<Template<T>, 1, 0> {
   typedef T type;
};
template <template <typename> class Template, typename T, typename New>
struct subst_template_type_param_helper<Template<T>, New, 1, 0> {
   typedef Template<New> type;
};

template <template <typename,typename> class Template, typename T1, typename T2>
struct extract_template_type_param_helper<Template<T1,T2>, 2, 0> {
   typedef T1 type;
};
template <template <typename,typename> class Template, typename T1, typename T2>
struct extract_template_type_param_helper<Template<T1,T2>, 2, 1> {
   typedef T2 type;
};
template <template <typename,typename> class Template, typename T1, typename T2, typename New1>
struct subst_template_type_param_helper<Template<T1,T2>, New1, 2, 0> {
   typedef Template<New1,T2> type;
};
template <template <typename,typename> class Template, typename T1, typename T2, typename New2>
struct subst_template_type_param_helper<Template<T1,T2>, New2, 2, 1> {
   typedef Template<T1,New2> type;
};

template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3>
struct extract_template_type_param_helper<Template<T1,T2,T3>, 3, 0> {
   typedef T1 type;
};
template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3>
struct extract_template_type_param_helper<Template<T1,T2,T3>, 3, 1> {
   typedef T2 type;
};
template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3>
struct extract_template_type_param_helper<Template<T1,T2,T3>, 3, 2> {
   typedef T3 type;
};

template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename New1>
struct subst_template_type_param_helper<Template<T1,T2,T3>, New1, 3, 0> {
   typedef Template<New1,T2,T3> type;
};
template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename New2>
struct subst_template_type_param_helper<Template<T1,T2,T3>, New2, 3, 1> {
   typedef Template<T1,New2,T3> type;
};
template <template <typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename New3>
struct subst_template_type_param_helper<Template<T1,T2,T3>, New3, 3, 2> {
   typedef Template<T1,T2,New3> type;
};

template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4>
struct extract_template_type_param_helper<Template<T1,T2,T3,T4>, 4, 0> {
   typedef T1 type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4>
struct extract_template_type_param_helper<Template<T1,T2,T3,T4>, 4, 1> {
   typedef T2 type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4>
struct extract_template_type_param_helper<Template<T1,T2,T3,T4>, 4, 2> {
   typedef T3 type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4>
struct extract_template_type_param_helper<Template<T1,T2,T3,T4>, 4, 3> {
   typedef T4 type;
};

template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4, typename New1>
struct subst_template_type_param_helper<Template<T1,T2,T3,T4>, New1, 4, 0> {
   typedef Template<New1,T2,T3,T4> type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4, typename New2>
struct subst_template_type_param_helper<Template<T1,T2,T3,T4>, New2, 4, 1> {
   typedef Template<T1,New2,T3,T4> type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4, typename New3>
struct subst_template_type_param_helper<Template<T1,T2,T3,T4>, New3, 4, 2> {
   typedef Template<T1,T2,New3,T4> type;
};
template <template <typename,typename,typename,typename> class Template, typename T1, typename T2, typename T3, typename T4, typename New4>
struct subst_template_type_param_helper<Template<T1,T2,T3,T4>, New4, 4, 3> {
   typedef Template<T1,T2,T3,New4> type;
};

template <typename T, int i=0>
struct extract_template_type_param
   : extract_template_type_param_helper<T, n_of_template_type_params<T>::value, i> {};

template <typename T, typename New, int i=0>
struct subst_template_type_param
   : subst_template_type_param_helper<T, New, n_of_template_type_params<T>::value, i> {};

template <typename T1, typename T2>
void assert_overloaded(T1, T2) {}

template <typename T>
void assert_overloaded(T, T)
{
   typedef typename enable_if<T, false>::type method_must_be_overloaded __attribute__((unused));
}

#define AssertOVERLOADED(base,child,method) \
   if (false) assert_overloaded(&base::method, &child::method)


/* ---------------------------------------------
 *  Building blocks for flexible traits classes
 * --------------------------------------------- */

struct list;

template <typename T>
struct list2cons {
   typedef T type;
   typedef list return_type;
};

template <typename R>
struct list2cons< R() > {
   typedef void type;
   typedef R return_type;
};

template <typename R, typename T>
struct list2cons< R(T) > {
   typedef T type;
   typedef R return_type;
};

template <typename R, typename List>
struct list2tail : list2cons<List> {};

template <typename List>
struct list2tail<list,List*> : list2cons<List> {};

template <typename R, typename T1, typename T2>
struct list2cons< R(T1,T2) > {
   typedef cons<T1, typename list2tail<R,T2>::type> type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3>
struct list2cons< R(T1,T2,T3) > {
   typedef cons<T1, cons<T2, typename list2tail<R,T3>::type> > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4>
struct list2cons< R(T1,T2,T3,T4) > {
   typedef cons<T1, cons<T2, cons<T3, typename list2tail<R,T4>::type> > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct list2cons< R(T1,T2,T3,T4,T5) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, typename list2tail<R,T5>::type> > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct list2cons< R(T1,T2,T3,T4,T5,T6) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, typename list2tail<R,T6>::type> > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, typename list2tail<R,T7>::type> > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, typename list2tail<R,T8>::type> > > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8,T9) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, cons<T8, typename list2tail<R,T9>::type> > > > > > > > type;
   typedef R return_type;
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
struct list2cons< R(T1,T2,T3,T4,T5,T6,T7,T8,T9,T10) > {
   typedef cons<T1, cons<T2, cons<T3, cons<T4, cons<T5, cons<T6, cons<T7, cons<T8, cons<T9, typename list2tail<R,T10>::type> > > > > > > > > type;
   typedef R return_type;
};

template <typename T1, typename T2>
struct concat_list {
   typedef typename if_else<identical<T1,void>::value, T2,
                            typename if_else<identical<T2,void>::value, T1, cons<T1, T2> >::type>::type
      type;
};

template <typename Head, typename Tail, typename T>
struct concat_list<cons<Head,Tail>, T>
   : concat_list<Head, typename concat_list<Tail, T>::type> {};

template <typename Head, typename Tail>
struct concat_list<cons<Head,Tail>, void> {
   typedef cons<Head, Tail> type;
};

template <bool Cond, typename T, typename List>
struct concat_if : concat_list<T, List> {};

template <typename T, typename List>
struct concat_if<false, T, List> {
   typedef List type;
};

template <typename ParamList, template <typename> class Tag, typename Default>
struct extract_type_param_helper {
   typedef Default type;
   static const bool specified=false;
};

template <typename Value, template <typename> class Tag, typename Default>
struct extract_type_param_helper<Tag<Value>, Tag, Default> {
   typedef Value type;
   static const bool specified=true;
};

template <typename Head, typename Tail, template <typename> class Tag, typename Default>
struct extract_type_param_helper<cons<Head,Tail>, Tag, Default>
   : extract_type_param_helper<Tail, Tag, Default> {};

template <typename Value, typename Tail, template <typename> class Tag, typename Default>
struct extract_type_param_helper<cons<Tag<Value>, Tail>, Tag, Default> {
   typedef Value type;
   static const bool specified=true;
};

template <typename ParamList, template <typename> class Tag, typename Default=void>
struct extract_type_param
   : extract_type_param_helper<typename list2cons<ParamList>::type, Tag, Default> {};

template <typename ParamList, template <typename> class Tag, int Default=0>
struct extract_int_param {
   typedef extract_type_param<ParamList, Tag, int2type<Default> > extractor;
   typedef typename extractor::type wrapped;
   static const int value=wrapped::value;
   static const bool specified=extractor::specified;
};

template <typename ParamList, template <typename> class Tag, bool Default=false>
struct extract_bool_param {
   typedef extract_type_param<ParamList, Tag, bool2type<Default> > extractor;
   typedef typename extractor::type wrapped;
   static const bool value=wrapped::value;
   static const bool specified=extractor::specified;
};

template <typename ParamList, template <typename> class Tag, template <typename,typename> class Operation, typename Second>
struct modify_type_param_helper
   : concat_list< ParamList, Tag<typename Operation<void,Second>::type> > {};

template <typename Value, template <typename> class Tag, template <typename,typename> class Operation, typename Second>
struct modify_type_param_helper<Tag<Value>, Tag, Operation, Second> {
   typedef Tag<typename Operation<Value,Second>::type> type;
};

template <typename Head, typename Tail, template <typename> class Tag, template <typename,typename> class Operation, typename Second>
struct modify_type_param_helper<cons<Head,Tail>, Tag, Operation, Second>
   : concat_list< Head, typename modify_type_param_helper<Tail,Tag,Operation,Second>::type > {};

template <typename Value, typename Tail, template <typename> class Tag, template <typename,typename> class Operation, typename Second>
struct modify_type_param_helper<cons<Tag<Value>, Tail>, Tag, Operation, Second>
   : concat_list< Tag<typename Operation<Value,Second>::type>, Tail > {};

template <typename ParamList, template <typename> class Tag, template <typename,typename> class Operation, typename Second>
struct modify_type_param
   : modify_type_param_helper<typename list2cons<ParamList>::type, Tag, Operation, Second> {};

template <typename ParamList, template <typename> class Tag, typename NewValue>
struct replace_type_param : modify_type_param<ParamList, Tag, project_second, NewValue> {};

template <typename ParamList, template <typename> class Tag, int NewValue>
struct replace_int_param : replace_type_param<ParamList, Tag, int2type<NewValue> > {};

template <typename ParamList, template <typename> class Tag, bool NewValue>
struct replace_bool_param : replace_type_param<ParamList, Tag, bool2type<NewValue> > {};

template <typename ParamList, template <typename> class Tag, typename NewValue>
struct append_type_param : modify_type_param<ParamList, Tag, prefer_first, NewValue> {};

template <typename ParamList, template <typename> class Tag, int NewValue>
struct append_int_param : append_type_param<ParamList, Tag, int2type<NewValue> > {};

template <typename ParamList, template <typename> class Tag, bool NewValue>
struct append_bool_param : append_type_param<ParamList, Tag, bool2type<NewValue> > {};

template <typename ParamList, template <typename> class Tag>
struct remove_type_param_helper {
   typedef ParamList type;
};

template <typename Value, template <typename> class Tag>
struct remove_type_param_helper<Tag<Value>, Tag> {
   typedef void type;
};

template <typename Head, typename Tail, template <typename> class Tag>
struct remove_type_param_helper<cons<Head,Tail>, Tag>
   : concat_list<Head, typename remove_type_param_helper<Tail,Tag>::type> {};

template <typename Value, typename Tail, template <typename> class Tag>
struct remove_type_param_helper<cons<Tag<Value>, Tail>, Tag> {
   typedef Tail type;
};

template <typename ParamList, template <typename> class Tag>
struct remove_type_param
   : remove_type_param_helper<typename list2cons<ParamList>::type, Tag> {};

template <typename ParamList, template <typename> class Tag>
struct remove_int_param : remove_type_param<ParamList, Tag> {};

template <typename ParamList, template <typename> class Tag>
struct remove_bool_param : remove_type_param<ParamList, Tag> {};

template <typename ParamList, typename ParamInstance>
struct extract_param {
   typedef void type;
   static const bool specified=false;
};

template <typename ParamList, template <typename> class Tag, typename Default>
struct extract_param<ParamList, Tag<Default> >
   : extract_type_param<ParamList, Tag, Default> {};

template <typename ParamList, typename ParamInstance>
struct replace_params_helper {
   typedef ParamList type;
};

template <typename ParamList, template <typename> class Tag, typename Value>
struct replace_params_helper<ParamList, Tag<Value> >
   : replace_type_param<ParamList, Tag, Value> {};

template <typename ParamList, template <typename> class Tag, typename Value, typename Tail>
struct replace_params_helper<ParamList, cons<Tag<Value>, Tail> >
   : replace_params_helper< typename replace_params_helper< ParamList,Tag<Value> >::type, Tail > {};

template <typename ParamList, typename ParamInstance>
struct replace_params
   : replace_params_helper<ParamList, typename list2cons<ParamInstance>::type> {};

template <typename ParamList, typename ParamInstance>
struct append_params_helper {
   typedef ParamList type;
};

template <typename ParamList, template <typename> class Tag, typename Value>
struct append_params_helper<ParamList, Tag<Value> >
   : append_type_param<ParamList, Tag, Value> {};

template <typename ParamList, template <typename> class Tag, typename Value, typename Tail>
struct append_params_helper<ParamList, cons<Tag<Value>, Tail> >
   : append_params_helper< typename append_params_helper< ParamList,Tag<Value> >::type, Tail > {};

template <typename ParamList, typename ParamInstance>
struct append_params
   : append_params_helper<ParamList, typename list2cons<ParamInstance>::type> {};

template <typename ParamInstance, typename GoodInstances>
struct filter_bool_params {
   typedef extract_param<typename list2cons<GoodInstances>::type, ParamInstance> helper;
   typedef typename helper::type good_param;
   typedef typename extract_template_type_param<ParamInstance>::type given_param;
   typedef typename if_else<(helper::specified && good_param::value==given_param::value), ParamInstance, void>::type type;
};

template <typename Head, typename Tail, typename GoodInstances>
struct filter_bool_params<cons<Head,Tail>, GoodInstances>
   : concat_list<typename filter_bool_params<Head,GoodInstances>::type,
                 typename filter_bool_params<Tail,GoodInstances>::type> {};

template <typename GoodInstances>
struct filter_bool_params<void, GoodInstances> {
   typedef void type;
};

/** Determine element pairs yielding Predicate::value==true
    The primary template handles the case of $First$ and $Second$ being scalars,
    while the general case of $First$ being a list is handled in specializations.

    list_search is a simplified function, it terminates as soon as one matching
    pair is found.
 */
template <typename First, typename Second, template <typename,typename> class Predicate>
struct list_search {
   /// whether at least one pair found
   static const bool value=Predicate<First,Second>::value;
   /// where it was found
   static const int pos= value ? 0 : -1;
   /// elements from First matching the predicate
   typedef typename if_else<value, First, void>::type positive;
   /// complement
   typedef typename if_else<value, void, First>::type negative;
   /// Second if there is at least one matching pair
   typedef typename if_else<value, Second, void>::type positive2;
   /// complement
   typedef typename if_else<value, void, Second>::type negative2;
};

template <typename Second, template <typename,typename> class Predicate>
struct list_search<void, Second, Predicate> : False {
   static const int pos=-1;
   typedef void positive;
   typedef void negative;
   typedef void positive2;
   typedef Second negative2;
};

template <typename First, template <typename,typename> class Predicate>
struct list_search<First, void, Predicate> : False {
   static const int pos=-1;
   typedef void positive;
   typedef First negative;
   typedef void positive2;
   typedef void negative2;
};

// to avoid ambiguity
template <template <typename,typename> class Predicate>
struct list_search<void, void, Predicate> : False {
   static const int pos=-1;
   typedef void positive;
   typedef void negative;
   typedef void positive2;
   typedef void negative2;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate,
          bool _head_matches=Predicate<Head,Second>::value>
struct list_search_helper {
   static const bool value=_head_matches; // true
   static const int pos=0;
   typedef Head positive;
   typedef Tail negative;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_helper<Head, Tail, Second, Predicate, false>
   : list_search<Tail, Second, Predicate> {
   typedef list_search<Tail, Second, Predicate> _super;
   static const int pos= _super::value ? _super::pos+1 : -1;
   typedef typename concat_list<Head, typename _super::negative>::type negative;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search<cons<Head, Tail>, Second, Predicate>
   : list_search_helper<Head, Tail, Second, Predicate> {};

// to avoid ambiguity
template <typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search<cons<Head, Tail>, void, Predicate> : False {
   static const int pos=-1;
   typedef void positive;
   typedef cons<Head,Tail> negative;
   typedef void positive2;
   typedef void negative2;
};

/** More powerful than list_search: find all pairs from $First$ and $Second$
    (both may be lists or scalars.) */
template <typename First, typename Second, template <typename,typename> class Predicate>
struct list_search_all : list_search<First, Second, Predicate> {};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_all_first_helper {
   typedef list_search_all<Head,Second,Predicate> head;
   typedef list_search_all<Tail,Second,Predicate> tail;
   static const bool value=head::value || tail::value;
   typedef typename concat_list<typename head::positive, typename tail::positive>::type positive;
   typedef typename concat_list<typename head::negative, typename tail::negative>::type negative;
   typedef typename if_else<value, Second, void>::type positive2;
   typedef typename if_else<value, void, Second>::type negative2;
};

template <typename First, typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search_all_second_helper {
   typedef list_search_all<First,Head,Predicate> head;
   typedef list_search_all<First,Tail,Predicate> tail;
   static const bool value=head::value || tail::value;
   typedef typename if_else<value,First,void>::type positive;
   typedef typename if_else<value,void,First>::type negative;
   typedef typename concat_list<typename head::positive2, typename tail::positive2>::type positive2;
   typedef typename concat_list<typename head::negative2, typename tail::negative2>::type negative2;
};

template <typename Head, typename Tail, typename Second, template <typename,typename> class Predicate>
struct list_search_all<cons<Head,Tail>, Second, Predicate>
   : list_search_all_first_helper<Head, Tail, Second, Predicate> {};

template <typename First, typename Head, typename Tail, template <typename,typename> class Predicate>
struct list_search_all<First, cons<Head, Tail>, Predicate>
   : list_search_all_second_helper<First, Head, Tail, Predicate> {};

template <typename Head, typename Tail, typename Head2, typename Tail2, template <typename,typename> class Predicate>
struct list_search_all<cons<Head,Tail>, cons<Head2, Tail2>, Predicate>
   : list_search_all_first_helper<Head, Tail, cons<Head2, Tail2>, Predicate> {
   typedef list_search_all_second_helper<cons<Head,Tail>, Head2, Tail2, Predicate> h2;
   typedef typename h2::positive2 positive2;
   typedef typename h2::negative2 negative2;
};

template <typename List, typename Item>
struct list_contains : list_search<typename list2cons<List>::type, Item, identical> {};

/// Eliminate the elements of Second that evaluate Predicate::value==TRUE
///  when combined with some element(s) from First.
template <typename First, typename Second, template <typename,typename> class Predicate>
struct merge_list
   : concat_list<First, typename list_search_all<First, Second, Predicate>::negative2> {};

template <typename T, typename Old, typename New>
struct list_replace : if_else<identical<T,Old>::value, New, T> {};

template <typename Head, typename Tail, typename Old, typename New>
struct list_replace<cons<Head,Tail>, Old, New> {
   typedef cons<typename if_else<identical<Head,Old>::value, New, Head>::type, typename list_replace<Tail,Old,New>::type> type;
};

template <template <typename> class Operation, typename T>
struct list_transform_unary : Operation<T> {};

template <template <typename> class Operation>
struct list_transform_unary<Operation, void> {
   typedef void type;
};

template <template <typename> class Operation, typename Head, typename Tail>
struct list_transform_unary<Operation, cons<Head,Tail> >
   : concat_list<typename Operation<Head>::type,
                 typename list_transform_unary<Operation, Tail>::type> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, T1, same<T2> > : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, same<T1>, T2> : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename T1, typename T2>
struct list_transform_binary<Operation, same<T1>, same<T2> > : Operation<T1,T2> {};

template <template <typename,typename> class Operation, typename Head1, typename Tail1, typename Head2, typename Tail2>
struct list_transform_binary<Operation, cons<Head1,Tail1>, cons<Head2,Tail2> >
   : concat_list<typename Operation<Head1, Head2>::type,
                 typename list_transform_binary<Operation, Tail1, Tail2>::type> {};

template <template <typename,typename> class Operation, typename Head1, typename Tail1, typename T2>
struct list_transform_binary<Operation, cons<Head1,Tail1>, same<T2> >
   : concat_list<typename Operation<Head1, T2>::type,
                 typename list_transform_binary<Operation, Tail1, same<T2> >::type> {};

template <template <typename,typename> class Operation, typename T1, typename Head2, typename Tail2>
struct list_transform_binary<Operation, same<T1>, cons<Head2,Tail2> >
   : concat_list<typename Operation<T1, Head2>::type,
                 typename list_transform_binary<Operation, same<T1>, Tail2>::type> {};

template <template <typename> class Operation>
struct apply_unary {
   template <typename T>
   struct op {
      typedef Operation<T> type;
   };
};

template <template <typename,typename> class Operation>
struct apply_binary {
   template <typename T1, typename T2>
   struct op {
      typedef Operation<T1,T2> type;
   };
};

template <template <typename, typename> class Operation, typename T>
struct list_accumulate : T {};

template <template <typename, typename> class Operation>
struct list_accumulate<Operation, void> : Operation<void,void> {};

template <template <typename, typename> class Operation, typename Head, typename Tail>
struct list_accumulate<Operation, cons<Head,Tail> >
   : Operation<Head, list_accumulate<Operation, Tail> > {};

template <typename T1, typename T2>
struct list_count {
   static const int value= T1::value + T2::value;
};

template <>
struct list_count<void,void> {
   static const int value=0;
};

template <typename T1, typename T2>
struct list_and {
   static const bool value= T1::value && T2::value;
};

template <>
struct list_and<void,void> {
   static const bool value=false;
};

template <typename T1, typename T2>
struct list_or {
   static const bool value= T1::value || T2::value;
};

template <>
struct list_or<void,void> {
   static const bool value=false;
};

template <template <typename,typename> class AccumulatingOperation, template <typename> class Operation, typename T>
struct list_accumulate_unary
   : list_accumulate<AccumulatingOperation, typename list_transform_unary<apply_unary<Operation>::template op, T>::type> {};

template <template <typename,typename> class AccumulatingOperation, template <typename,typename> class Operation, typename T1, typename T2>
struct list_accumulate_binary
   : list_accumulate<AccumulatingOperation, typename list_transform_binary<apply_binary<Operation>::template op, T1, T2>::type> {};

template <typename T1, typename T2>
struct list_logical_or {
   typedef T1 type;
};

template <typename T2>
struct list_logical_or<void, T2> {
   typedef T2 type;
};

template <typename T1, typename T2>
struct list_logical_and {
   typedef T2 type;
};

template <typename T2>
struct list_logical_and<void, T2> {
   typedef void type;
};

template <typename First, typename Second, template <typename, typename> class Predicate=identical>
struct list_mapping {
   typedef int2type< list_search<Second, First, Predicate>::pos > type;
   static const bool mismatch= list_search<Second, First, Predicate>::pos < 0;
};

template <typename Head, typename Tail, typename Second, template <typename, typename> class Predicate>
struct list_mapping< cons<Head, Tail>, Second, Predicate> {
   typedef list_mapping<Head, Second, Predicate> first;
   typedef list_mapping<Tail, Second, Predicate> second;
   typedef cons< typename first::type, typename second::type > type;
   static const bool mismatch= first::mismatch || second::mismatch;
};

/* -----------------------------------
 *  Structural description of objects
 * ----------------------------------- */

struct is_scalar {};
struct is_container {};
struct is_composite {};
struct is_opaque {};
struct is_not_object {};

namespace object_classifier {
   enum { is_not_object, is_opaque, is_scalar };

   namespace _impl {
      struct bait {};
      size_discriminant<is_opaque>::type analyzer_f(...);

      template <typename T>
      typename enable_if<size_discriminant<is_scalar>::type, std::numeric_limits<T>::is_specialized>::type analyzer_f(const T*, bait*);
   }
   typedef _impl::bait *bait;

   template <typename T, typename _disable=void> struct kind_of;

   template <typename T>
   struct kind_of<T, typename disable_if<void, std::tr1::is_enum<T>::value || is_pointer<T>::value>::type>
   {
      static const int value=sizeof(analyzer_f((const T*)0, bait(0)));
   };

   template <typename T>
   struct kind_of<T*, void>
   {
      static const int value=is_not_object;
   };

   template <typename T>
   struct kind_of<T, typename enable_if<void, std::tr1::is_enum<T>::value>::type>
   {
      static const int check_numeric=sizeof(analyzer_f((const T*)0, bait(0)));
      static const int value= check_numeric==is_scalar ? is_scalar : is_not_object;
   };

   template <>
   struct kind_of<void, void>
   {
      static const int value=is_not_object;
   };

   template <typename T, int kind=kind_of<T>::value>
   struct what_is {
      static const int value=kind;
   };
} // end namespace object_classifier

// to be specialized for overwriting model-specific properties
template <typename T> struct spec_object_traits;

struct default_object_traits {
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true;
   static const bool IO_ends_with_eol=false;
   // allow creation of static objects, destructor wouldn't make any troubles when called in the global exit sequence
   static const bool allow_static=true;
   typedef void masquerade_for;
};

template <>
struct spec_object_traits<is_scalar> : default_object_traits {
   typedef is_scalar model;
};

template <>
struct spec_object_traits<is_not_object> : default_object_traits {
   typedef is_not_object model;
};

template <>
struct spec_object_traits<is_opaque> : default_object_traits {
   typedef is_opaque model;
};

template <typename T>
class Serialized : public T {
public:
   Serialized& operator= (const T& x) { static_cast<T&>(*this)=x; return *this; }
protected:
   Serialized() throw();
   ~Serialized() throw();
};

#if defined(__GLIBCXX__) && !defined(_GLIBCXX_TR1_TYPE_TRAITS)
#  define POLYMAKE_IsClass std::tr1::__is_union_or_class
#else
#  define POLYMAKE_IsClass std::tr1::is_class
#endif

template <typename T, bool _enable=POLYMAKE_IsClass<T>::value>
struct has_serialized_impl : bool2type<identical<typename spec_object_traits< Serialized<T> >::model, is_composite>::value> {};

template <typename T>
struct has_serialized_impl<T, false> : False {};

template <typename T>
struct has_serialized : has_serialized_impl<T> {};

template <typename T>
struct has_serialized< Serialized<T> > : False {};

template <typename T> inline
Serialized<T>& serialize(T& x, typename enable_if<void**, has_serialized<T>::value>::type =0)
{
   return static_cast<Serialized<T>&>(x);
}

template <typename T> inline
const Serialized<T>& serialize(const T& x, typename enable_if<void**, has_serialized<T>::value>::type =0)
{
   return static_cast<const Serialized<T>&>(x);
}

template <>
struct spec_object_traits<is_composite> {
   typedef is_composite model;
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true;
   static const bool allow_static=true;
   typedef void masquerade_for;
};

template <typename T>
struct spec_object_traits< cons<Serialized<T>, int2type<object_classifier::is_opaque> > > : spec_object_traits<is_not_object> {};

template <typename T>
struct spec_object_traits< cons<T, int2type<object_classifier::is_not_object> > > : spec_object_traits<is_not_object> {};

template <typename T>
struct spec_object_traits< cons<T, int2type<object_classifier::is_opaque> > > : spec_object_traits<is_opaque> {};

template <typename T>
struct spec_object_traits< cons<T, int2type<object_classifier::is_scalar> > > : spec_object_traits<is_scalar> {
   static
   bool is_zero(typename function_argument<T>::type x)
   {
      return !x;
   }

   static
   bool is_one(typename function_argument<T>::type x)
   {
      return x==1;
   }

   static const T& zero()
   {
      static const T zero_v(0);
      return zero_v;
   }
   static const T& one()
   {
      static const T one_v(1);
      return one_v;
   }
};

template <typename T>
struct spec_object_traits :
   spec_object_traits< cons<T, int2type<object_classifier::what_is<T>::value> > > {};

template <typename T, typename Model,
          bool _fulfilled=derived_from< spec_object_traits<T>, spec_object_traits<Model> >::value>
struct spec_or_model_traits : spec_object_traits<T> {
   static const bool is_specialized=true;
};

template <typename T, typename Model>
struct spec_or_model_traits<T, Model, false> : spec_object_traits<Model> {
   static const bool is_specialized=false;
};

template <typename Top>
class Generic {
protected:
   Generic() {}
   Generic(const Generic&) {}
public:
   typedef Top top_type;
   typedef void generic_type;
   typedef Top persistent_type;
   typedef Top concrete_type;

   const Top& top() const
   {
      return static_cast<const Top&>(*this);
   }
   Top& top()
   {
      return static_cast<Top&>(*this);
   }
};

// to be specialized for overwriting generic family-specific properties
template <typename T>
struct generic_object_traits : spec_object_traits<typename T::generic_type> {};

#define DeclTypedefCHECK(Typename)                                      \
   template <typename T>                                                \
   struct has_##Typename##_impl {                                       \
      template <typename T2>                                            \
      static typename size_discriminant<(2-identical<typename T2::Typename,void>::value)>::type Test(const T2*, typename T2::Typename* =0); \
      typedef T item;                                                   \
   };                                                                   \
   template <typename T>                                                \
   struct has_##Typename : derivation::test< has_##Typename##_impl<T> > {}

#define DeclNestedTemplate1CHECK(Templname)                             \
   template <typename T>                                                \
   struct has_nested_##Templname##_impl {                               \
      template <template <typename> class> struct templ_wrapper {};     \
      template <typename T2>                                            \
      static derivation::yes Test(const T2*, const templ_wrapper<T2::template Templname>* =0); \
      typedef T item;                                                   \
   };                                                                   \
   template <typename T>                                                \
   struct has_nested_##Templname  : derivation::test< has_nested_##Templname##_impl<T> > {}

#define DeclNestedTemplate2CHECK(Templname)                             \
   template <typename T>                                                \
   struct has_nested_##Templname##_impl {                               \
      template <template <typename,typename> class> struct templ_wrapper {}; \
      template <typename T2>                                            \
      static derivation::yes Test(const T2*, const templ_wrapper<T2::template Templname>* =0); \
      typedef T item;                                                   \
   };                                                                   \
   template <typename T>                                                \
   struct has_nested_##Templname  : derivation::test< has_nested_##Templname##_impl<T> > {}

DeclTypedefCHECK(generic_type);
DeclTypedefCHECK(persistent_type);

template <typename T, bool _has_generic=has_generic_type<T>::value,
                      bool _has_persistent=has_persistent_type< spec_object_traits<T> >::value>
struct choose_generic_object_traits : spec_object_traits<T> {
   typedef void generic_type;
   typedef T persistent_type;
   typedef typename spec_object_traits<T>::model generic_tag;
};

template <typename T>
struct choose_generic_object_traits<T, false, true> : spec_object_traits<T> {
   typedef void generic_type;
   typedef typename spec_object_traits<T>::model generic_tag;
};

template <typename T>
struct choose_generic_object_traits<T, true, false> : generic_object_traits<T> {
   typedef typename T::generic_type generic_type;
   typedef generic_object_traits<T> gen_traits;
   static const bool is_persistent=
      gen_traits::is_persistent && identical<typename gen_traits::masquerade_for,void>::value && !(gen_traits::is_temporary || gen_traits::is_lazy);
   typedef typename if_else<is_persistent, T, typename generic_type::persistent_type>::type persistent_type;
};

template <typename T>
struct choose_generic_object_traits<T, true, true> : generic_object_traits<T> {
   typedef typename T::generic_type generic_type;
   typedef generic_object_traits<T> gen_traits;
   static const bool is_persistent=identical<T, typename spec_object_traits<T>::persistent_type>::value;
};

// to be specialized by "proxy" types like pseudo-containers
template <typename T>
struct redirect_object_traits : choose_generic_object_traits<T> {};

template <typename T, typename Model=typename redirect_object_traits<T>::model>
struct object_traits : redirect_object_traits<T> {
   static const int total_dimension=0, nesting_level=0;
};

enum IO_separator_kind { IO_sep_inherit, IO_sep_containers, IO_sep_enforce };

template <typename T>
struct object_traits<T,is_container> : redirect_object_traits<T> {
   static const int
      total_dimension = object_traits<typename T::value_type>::total_dimension + object_traits<T>::dimension,
      nesting_level = object_traits<typename T::value_type>::nesting_level + 1;
   static const int is_resizeable= object_traits<T>::is_temporary || object_traits<T>::is_always_const ? 0 : redirect_object_traits<T>::is_resizeable;
   static const bool
      IO_separate_elements_with_eol=
         object_traits<T>::IO_separator==IO_sep_inherit &&
            object_traits<typename T::value_type>::IO_ends_with_eol
      || object_traits<T>::IO_separator==IO_sep_containers &&
            object_traits<typename T::value_type>::total_dimension >= 1
      || object_traits<T>::IO_separator==IO_sep_enforce,
      IO_ends_with_eol=
         IO_separate_elements_with_eol
      || object_traits<typename T::value_type>::IO_ends_with_eol
      || object_traits<T>::dimension>1;
};

template <>
struct spec_object_traits<is_container> {
   typedef is_container model;
   static const int dimension=1, is_resizeable=1;
   static const bool is_lazy=false, is_temporary=false, is_always_const=false, is_persistent=true, allow_sparse=false;
   static const bool allow_static=true;
   static const IO_separator_kind IO_separator=IO_sep_containers;
   typedef void masquerade_for;
};

template <typename T>
struct max_dimension {
   static const int value=object_traits<typename deref<T>::type>::total_dimension;
   static const bool IO_ends_with_eol=object_traits<typename deref<T>::type>::IO_ends_with_eol;
};

template <int X, int Y, bool _ge=(X>=Y)>
struct const_compare {
   static const int max=X, min=Y;
};
template <int X, int Y>
struct const_compare<X,Y,false> {
   static const int max=Y, min=X;
};
template <int X, int Y, bool _ge=(X>=0)>
struct const_first_nonnegative {
   static const int value=X;
};
template <int X, int Y>
struct const_first_nonnegative<X,Y,false> {
   static const int value=Y;
};

template <typename Head, typename Tail>
struct max_dimension< cons<Head, Tail> > {
   static const int value=const_compare<max_dimension<Head>::value, max_dimension<Tail>::value>::max;
   static const bool IO_ends_with_eol=max_dimension<Head>::IO_ends_with_eol || max_dimension<Tail>::IO_ends_with_eol;
};

template <typename Object>
struct object_traits<Object,is_composite> : redirect_object_traits<Object> {
   static const int total_dimension=max_dimension<typename object_traits::elements>::value,
                    nesting_level=0;
   static const bool IO_ends_with_eol=max_dimension<typename object_traits::elements>::IO_ends_with_eol;
};

template <typename ObjectRef, int n, int m=0,
          int l=list_length<typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements>::value>
struct visitor_n_th : visitor_n_th<ObjectRef, n, m+1, l> {
   typedef visitor_n_th<ObjectRef, n, m+1, l> super;
   super&
   operator<< (typename attrib<typename n_th<typename super::elements, m>::type>::plus_const_ref)
   {
      return *this;
   }
};

template <typename ObjectRef, int n, int l>
struct visitor_n_th<ObjectRef, n, n, l> : visitor_n_th<ObjectRef, n, n+1, l> {
   typedef visitor_n_th<ObjectRef, n, n+1, l> super;
   super&
   operator<< (typename super::value_ref val)
   {
      this->value=&val;
      return *this;
   }
};

template <typename ObjectRef, int n, int l>
struct visitor_n_th<ObjectRef, n, l, l> {
   typedef typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements elements;
   typedef typename n_th<elements, n>::type element;
   typedef typename inherit_const<typename attrib<element>::minus_const_ref, ObjectRef>::type* value_ptr;
   typedef typename inherit_const<typename attrib<element>::minus_const_ref, ObjectRef>::type& value_ref;
   value_ptr value;

   visitor_n_th() : value(NULL) {}
};

template <typename T, int n> inline
typename visitor_n_th<T,n>::value_ref
visit_n_th(T& obj, int2type<n>)
{
   visitor_n_th<T, n> vis;
   object_traits<typename attrib<T>::minus_const_ref>::visit_elements(obj,vis);
   return *vis.value;
}

template <typename ObjectRef, typename Model=typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::model>
struct is_mutable_helper {
   static const bool value=!attrib<ObjectRef>::is_const;
};

template <typename ObjectRef>
struct is_mutable : is_mutable_helper<ObjectRef> {};

template <typename ObjectRef>
struct is_mutable_helper<ObjectRef, is_composite> {
   static const bool value=
      is_mutable_helper<ObjectRef, void>::value &&
      list_accumulate_unary<list_and, is_mutable, typename object_traits<typename attrib<ObjectRef>::minus_const_ref>::elements>::value;
};

template <typename T,
          typename _hidden=typename object_traits<typename deref<T>::type>::masquerade_for>
struct is_masquerade : True {
   typedef typename if_else<is_masquerade<_hidden>::value, typename is_masquerade<_hidden>::hidden_type,
                                                           typename attrib<_hidden>::minus_const_ref>::type
      hidden_type;
   // the rest is for the appropriate specialization of the class alias
   typedef typename if_else<attrib<_hidden>::is_const || attrib<_hidden>::is_reference, _hidden, T>::type
      inherit_from;
   typedef typename inherit_ref< typename inherit_ref<hidden_type,_hidden>::type,
                                 typename if_else< object_traits<hidden_type>::is_always_const, typename attrib<T>::plus_const, T>::type>::type
      hidden_stored_type;
};

template <typename T>
struct is_masquerade<T, void> : False {
   typedef void hidden_type;
};

// common case: everything is bad
template <typename T1, typename T2,
          typename Model1=typename object_traits<T1>::model, typename Model2=typename object_traits<T2>::model>
struct isomorphic_types : False {
   typedef void discriminant;
};

template <typename T1, typename T2, typename Model>
struct isomorphic_types<T1, T2, Model, Model> : True {
   typedef cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag> discriminant;
};

template <typename T1, typename T2>
struct isomorphic_types<T1, T2, is_container, is_container> {
   typedef typename T1::value_type V1;
   typedef typename T2::value_type V2;
   static const bool value=isomorphic_types<V1,V2>::value;
   typedef typename choice< if_else< value, cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag>,
                            if_else< isomorphic_types<T1,V2>::value, cons<is_scalar, typename object_traits<T2>::generic_tag>,
                            if_else< isomorphic_types<V1,T2>::value, cons<typename object_traits<T1>::generic_tag, is_scalar>,
                                     void > > > >::type
      discriminant;
};

template <typename T1, typename T2, typename Model1>
struct isomorphic_types<T1, T2, Model1, is_container> : False {
   typedef typename if_else< isomorphic_types<T1, typename T2::value_type>::value,
                             cons<is_scalar, typename object_traits<T2>::generic_tag>, void>::type
      discriminant;
};

template <typename T1, typename T2, typename Model2>
struct isomorphic_types<T1, T2, is_container, Model2> : False {
   typedef typename if_else< isomorphic_types<typename T1::value_type, T2>::value,
                             cons<typename object_traits<T1>::generic_tag, is_scalar>, void>::type
      discriminant;
};

template <typename T1, typename T2>
struct isomorphic_types_helper : isomorphic_types<typename deref<T1>::type, typename deref<T2>::type> {};

template <typename T1, typename T2>
struct isomorphic_types<T1, T2, is_composite, is_composite> {
   static const bool value=
      list_accumulate_binary<list_and, isomorphic_types_helper, typename object_traits<T1>::elements, typename object_traits<T2>::elements>::value;
   typedef typename if_else<value, cons<typename object_traits<T1>::generic_tag, typename object_traits<T2>::generic_tag>, void>::type
      discriminant;
};

// specializations thereof may define an internal type field_type pointing to itself or another type
// fulfilling the requirements to an algebraic field.
template <typename T>
struct algebraic_traits {};

template <>
struct algebraic_traits<double> {
   typedef double field_type;
};

template <typename T, typename field_type=T>
struct is_field_impl : False {};

template <typename T>
struct is_field_impl<T, typename algebraic_traits<T>::field_type> : True {};

template <typename T>
struct is_field : is_field_impl<T> {};

template <typename T, typename T2=T>
struct has_zero_value_impl : False {};

// we assume that every class for which algebraic_traits is specialized has also a zero value
template <typename T>
struct has_zero_value_impl<T, typename cons<T, typename algebraic_traits<T>::field_type>::head> : True {};

template <typename T>
struct has_zero_value : has_zero_value_impl<T> {};

template <typename T>
struct is_gcd_domain : bool2type<std::numeric_limits<T>::is_integer & std::numeric_limits<T>::is_exact> {};

template <typename T>
struct is_field_of_fractions : bool2type<!std::numeric_limits<T>::is_integer & std::numeric_limits<T>::is_exact> {};

/* -----------------------------------------------
 *  conversion between generic and concrete types
 * ----------------------------------------------- */

template <typename T, bool _has_generic=has_generic_type<typename deref<T>::type>::value, bool _itself_generic=false>
struct Concrete : True {
   typedef typename attrib<T>::minus_const_ref type;
};

template <typename T>
struct Concrete<T, true, false>
   : Concrete<T, false, identical<typename deref<T>::type, typename deref<T>::type::generic_type>::value> {};

template <typename T>
struct Concrete<T, false, true> : False {
   typedef typename deref<T>::type::concrete_type type;
};

template <typename T> inline
typename enable_if<const T, Concrete<T>::value>::type&
concrete(const T& x)
{
   return x;
}
template <typename T> inline
typename enable_if<T, Concrete<T>::value>::type&
concrete(T& x)
{
   return x;
}
template <typename T> inline
typename disable_if<const typename Concrete<T>::type, Concrete<T>::value>::type&
concrete(const T& x)
{
   return static_cast<const typename Concrete<T>::type&>(x);
}
template <typename T> inline
typename disable_if<typename Concrete<T>::type, Concrete<T>::value>::type&
concrete(T& x)
{
   return static_cast<typename Concrete<T>::type&>(x);
}

template <typename ObjectRef,
          bool _is_lazy=object_traits<typename Concrete<ObjectRef>::type>::is_lazy>
struct Diligent : True {
   static const bool add_ref=is_masquerade<typename Concrete<ObjectRef>::type>::value;
   typedef typename inherit_ref<typename Concrete<ObjectRef>::type,
                                typename if_else<add_ref, typename attrib<ObjectRef>::plus_const_ref,
                                                          typename attrib<ObjectRef>::plus_const>::type>::type
      type;
};

template <typename ObjectRef>
struct Diligent<ObjectRef, true> : False {
   typedef const typename object_traits<typename Concrete<ObjectRef>::type>::persistent_type type;
};

template <typename T> inline
typename enable_if<const T&, Diligent<T>::value>::type
diligent(const T& x)
{
   return concrete(x);
}

template <typename T> inline
typename disable_if<typename Diligent<T>::type, Diligent<T>::value>::type
diligent(const T& x)
{
   return typename Diligent<T>::type(x);
}

template <typename Result, typename Source, typename Generic>
struct inherit_generic_helper {
   typedef typename Source::template rebind_generic<Result>::type type;
};

template <typename Result, typename Source>
struct inherit_generic_helper<Result, Source, void> {
   struct type {};
};

template <typename Result, typename Source>
struct inherit_generic
   : inherit_generic_helper<Result, Source, typename object_traits<Source>::generic_type> {};

template <typename Result, typename Head, typename Tail>
struct inherit_generic<Result, cons<Head, Tail> > {
   typedef typename least_derived< cons<typename inherit_generic<Result, Head>::type,
                                        typename inherit_generic<Result, Tail>::type> >::type try_type;
   typedef typename if_else<identical<try_type,void>::value, typename inherit_generic_helper<Result,Head,void>::type, try_type>::type type;
};

/* ----------------------------------------
 *  tools for alias and masquerade objects 
 * ---------------------------------------- */

template <typename T>
struct const_equivalent {
   typedef T type;
};

template <typename ObjectRef>
struct effectively_const {
   typedef typename deref<ObjectRef>::type Object;
   static const bool value= object_traits<Object>::is_always_const || (!object_traits<Object>::is_temporary && deref<ObjectRef>::is_const);
};

template <typename ObjectRef1, typename ObjectRef2>
struct coherent_const {
   typedef typename choice< if_else< effectively_const<ObjectRef1>::value, ObjectRef1,
                            if_else< effectively_const<ObjectRef2>::value, typename deref<ObjectRef1>::plus_const,
                            if_else< object_traits<typename deref<ObjectRef1>::type>::is_temporary, typename deref<ObjectRef1>::minus_const_ref,
                                     ObjectRef1 > > > >::type
      first_type;
   typedef typename choice< if_else< effectively_const<ObjectRef2>::value, ObjectRef2,
                            if_else< effectively_const<ObjectRef1>::value, typename deref<ObjectRef2>::plus_const,
                            if_else< object_traits<typename deref<ObjectRef2>::type>::is_temporary, typename deref<ObjectRef2>::minus_const_ref,
                                     ObjectRef2 > > > >::type
      second_type;
};

template <typename T>
struct non_const_helper { typedef T type; };

template <typename T>
struct non_const_helper<const T> : assign_const<T, effectively_const<const T>::value> {};

template <typename T> inline
typename non_const_helper<const T>::type& non_const(const T& x)
{
   return const_cast<typename non_const_helper<T>::type&>(x);
}

template <typename T> inline
T& non_const(T& x) { return x; }

template <template <typename> class Masquerade, typename OrigRef>
struct masquerade : inherit_ref<Masquerade<typename deref<OrigRef>::type>, OrigRef> {};

template <template <typename,typename> class Masquerade, typename OrigRef, typename Second>
struct masquerade2 : inherit_ref<Masquerade<typename deref<OrigRef>::type, Second>, OrigRef> {};

template <template <typename,typename,typename> class Masquerade, typename OrigRef, typename Second, typename Third>
struct masquerade3 : inherit_ref<Masquerade<typename deref<OrigRef>::type, Second, Third>, OrigRef> {};

template <template <typename> class Masquerade, typename OrigRef>
struct deref< masquerade<Masquerade,OrigRef> > : deref< typename masquerade<Masquerade,OrigRef>::type > {
   typedef masquerade<Masquerade, typename attrib<OrigRef>::plus_const> plus_const;
};

template <template <typename,typename> class Masquerade, typename OrigRef, typename Second>
struct deref< masquerade2<Masquerade,OrigRef,Second> > : deref< typename masquerade2<Masquerade,OrigRef,Second>::type > {
   typedef masquerade2<Masquerade, typename attrib<OrigRef>::plus_const, Second> plus_const;
};

template <template <typename,typename,typename> class Masquerade, typename OrigRef, typename Second, typename Third>
struct deref< masquerade3<Masquerade,OrigRef,Second,Third> > : deref< typename masquerade3<Masquerade,OrigRef,Second,Third>::type > {
  typedef masquerade3<Masquerade, typename attrib<OrigRef>::plus_const, Second, Third> plus_const;
};

template <typename ObjRef, bool _is_ref=(deref<ObjRef>::is_reference || is_masquerade<ObjRef>::value)>
struct lvalue_arg {
   typedef typename deref<ObjRef>::type object;
   typedef typename if_else<( object_traits<object>::is_temporary &&
                              !object_traits<object>::is_always_const &&
                              !object_traits<object>::is_lazy ),
                            object, object&>::type
      type;
};

template <typename ObjRef>
struct lvalue_arg<ObjRef, true> {
   typedef typename deref<ObjRef>::minus_const type;
};

template <>
struct lvalue_arg<void, false> {
   typedef type2type<void> type;
};

template <typename ObjRef>
struct temp_ref {
   typedef typename deref<ObjRef>::type object;
   typedef typename if_else<object_traits<object>::is_always_const || object_traits<object>::is_lazy,
                            typename deref<ObjRef>::plus_const_ref,
           typename if_else<object_traits<object>::is_temporary,
                            ObjRef,
                            typename deref<ObjRef>::plus_ref>::type >::type
      type;
};

/* ----------------------
 *  memory layout tricks
 * ---------------------- */

template <typename T> inline
void relocate(T* from, T* to, True)
{
   *to=*from;
}

template <typename T> inline
void relocate(T* from, T* to, False)
{
   new(to) T(*from);
   std::_Destroy(from);
}

template <typename T> inline
void relocate(T* from, T* to)
{
   relocate(from, to, is_pod<T>());
}

// Move a pointer from a data member of a class to the containing class object.
// data_member may also point to some Member's parent class.
template <typename Source, typename Member, typename Owner> inline
Owner* reverse_cast(Source* data_member, Member Owner::* member_ptr)
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<char*>(&(reinterpret_cast<Owner*>(fict_addr)->*member_ptr))
            -reinterpret_cast<char*>(fict_addr);
   return reinterpret_cast<Owner*>( reinterpret_cast<char*>(static_cast<Member*>(data_member)) - offset );
}

template <typename Source, typename Member, typename Owner> inline
const Owner* reverse_cast(const Source* data_member, Member Owner::* member_ptr)
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<const char*>(&(reinterpret_cast<const Owner*>(fict_addr)->*member_ptr))
            -reinterpret_cast<const char*>(fict_addr);
   return reinterpret_cast<const Owner*>( reinterpret_cast<const char*>(static_cast<const Member*>(data_member)) - offset );
}

template <typename Source, typename Member, typename Owner> inline
const Owner* reverse_cast(const Source* data_member, const Member Owner::* member_ptr)
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<const char*>(&(reinterpret_cast<const Owner*>(fict_addr)->*member_ptr))
            -reinterpret_cast<const char*>(fict_addr);
   return reinterpret_cast<const Owner*>( reinterpret_cast<const char*>(static_cast<const Member*>(data_member)) - offset );
}

template <typename Member, typename Source, typename Owner, int _size> inline
Owner* reverse_cast(Source* data_member, int i, Member (Owner::* member_ptr)[_size])
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<char*>((reinterpret_cast<Owner*>(fict_addr)->*member_ptr)+0)
            -reinterpret_cast<char*>(fict_addr);
   return reinterpret_cast<Owner*>( reinterpret_cast<char*>(static_cast<Member*>(data_member)-i) - offset );
}

template <typename Member, typename Source, typename Owner, int _size> inline
const Owner* reverse_cast(const Source* data_member, int i, Member (Owner::* member_ptr)[_size])
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<const char*>((reinterpret_cast<const Owner*>(fict_addr)->*member_ptr)+0)
            -reinterpret_cast<const char*>(fict_addr);
   return reinterpret_cast<const Owner*>( reinterpret_cast<const char*>(static_cast<const Member*>(data_member)-i) - offset );
}

template <typename Member, typename Source, typename Owner, int _size> inline
const Owner* reverse_cast(const Source* data_member, int i, const Member (Owner::* member_ptr)[_size])
{
   const ptrdiff_t fict_addr=8,
      offset=reinterpret_cast<const char*>((reinterpret_cast<const Owner*>(fict_addr)->*member_ptr)+0)
            -reinterpret_cast<const char*>(fict_addr);
   return reinterpret_cast<const Owner*>( reinterpret_cast<const char*>(static_cast<const Member*>(data_member)-i) - offset );
}

// the same, but with additional static cast to a class derived from the owner

template <typename Class, typename Source, typename Member, typename Owner> inline
Class* reverse_cast(Source* data_member, Member Owner::* member_ptr)
{
   return static_cast<Class*>(reverse_cast(data_member, member_ptr));
}

template <typename Class, typename Source, typename Member, typename Owner> inline
const Class* reverse_cast(const Source* data_member, Member Owner::* member_ptr)
{
   return static_cast<const Class*>(reverse_cast(data_member, member_ptr));
}

template <typename Class, typename Source, typename Member, typename Owner> inline
const Class* reverse_cast(const Source* data_member, const Member Owner::* member_ptr)
{
   return static_cast<const Class*>(reverse_cast(data_member, member_ptr));
}

template <typename T> inline
bool is_zero(const T& x) { return object_traits<T>::is_zero(x); }

template <typename T> inline
bool is_one(const T& x) { return object_traits<T>::is_one(x); }

template <typename T> inline
const T& zero_value() { return object_traits<T>::zero(); }

template <typename T> inline
const T& one_value() { return object_traits<T>::one(); }

} // end namespace pm

namespace polymake {
   using pm::bool2type;
   using pm::int2type;
   using pm::ptr2type;
   using pm::type2type;
   using pm::non_const;
   using pm::reverse_cast;
   using pm::is_zero;
   using pm::is_one;
   using pm::zero_value;
   using pm::one_value;
}

#if POLYMAKE_DEBUG
# define POLYMAKE_DEBUG_METHOD(Class,Name) static void (Class::* Name##_dummy)() const __attribute__((used)) =&Class::Name
#endif

#endif // POLYMAKE_INTERNAL_TYPE_MANIP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
