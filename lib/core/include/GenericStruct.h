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

#ifndef POLYMAKE_GENERIC_STRUCT_H
#define POLYMAKE_GENERIC_STRUCT_H

#include "polymake/internal/operations.h"
#include "polymake/internal/comparators.h"

#define DeclStructFIELD(name,tn,...)                                                    \
   name##_sUpEr_;                                                                       \
   typedef tn pm::concat_list<name##_sUpEr_,__VA_ARGS__>::type name##_fIeLdS_;          \
   __VA_ARGS__ name;                                                                    \
   template <typename Me, typename Visitor> static                                      \
   void _vIsItFiElDs_(Me& me, Visitor& v, int_constant<pm::list_length<name##_sUpEr_>::value>) \
   {                                                                                    \
      _vIsItFiElDs_(me, v << me.name, int_constant<pm::list_length<name##_fIeLdS_>::value>()); \
   }                                                                                    \
   template <typename Me, typename Operation> static                                    \
   void _aPpLy2FiElDs_(Me& me1, Me& me2, const Operation& op, int_constant<pm::list_length<name##_sUpEr_>::value>) \
   {                                                                                    \
      typedef pm::binary_op_builder<Operation, typename pm::inherit_const<__VA_ARGS__,Me>::type*, typename pm::inherit_const<__VA_ARGS__,Me>::type*> opb; \
      opb::create(op)(me1.name,me2.name);                                               \
      _aPpLy2FiElDs_(me1,me2,op,int_constant<pm::list_length<name##_fIeLdS_>::value>()); \
   }                                                                                    \
   template <typename Me> static                                                        \
   void _rElOcFiElDs_(Me* from, Me* to, int_constant<pm::list_length<name##_sUpEr_>::value>) \
   {                                                                                    \
      pm::relocate(&from->name, &to->name);                             \
      _rElOcFiElDs_(from,to,int_constant<pm::list_length<name##_fIeLdS_>::value>());    \
   }                                                                                    \
   static const char* _fIeLdNaMe_(size_t& l, int_constant<pm::list_length<name##_sUpEr_>::value>) \
   {                                                                                    \
      l=sizeof(#name)-1;                                                                \
      return #name;                                                                     \
   }                                                                                    \
   template <typename Me> static                                                        \
   __VA_ARGS__ Me::* field_ptr(Me*, int_constant<pm::list_length<name##_sUpEr_>::value>) \
   {                                                                                    \
      return &Me::name;                                                                 \
   }                                                                                    \
   typedef name##_fIeLdS_

#define DeclFIELD(name,...) DeclStructFIELD(name,,__VA_ARGS__)
#define DeclTemplFIELD(name,...) DeclStructFIELD(name,typename,__VA_ARGS__)

#define DeclSTRUCT(...)                                                 \
   typedef void __VA_ARGS__ _sTrUcTFiElDs_;                             \
   template <typename Me, typename Visitor> static                      \
   void _vIsItFiElDs_(Me&, Visitor&, int_constant<pm::list_length<_sTrUcTFiElDs_>::value>) {} \
   template <typename Me, typename Operation> static                    \
   void _aPpLy2FiElDs_(Me&, Me&, const Operation& op, int_constant<pm::list_length<_sTrUcTFiElDs_>::value>) {} \
   template <typename Me> static                                        \
   void _rElOcFiElDs_(Me*, Me*, int_constant<pm::list_length<_sTrUcTFiElDs_>::value>) {} \
   template <int n>                                                     \
   static const char* _fIeLdNaMe_(size_t&, int_constant<n>) { return 0; } \
public:                                                                 \
   typedef _sTrUcTFiElDs_ field_types;                                  \
   template <typename> friend class pm::GenericStruct

namespace pm {

template <typename Struct>
class GenericStruct : public Generic<Struct> {
public:
   typedef GenericStruct generic_type;

   template <typename Visitor>
   void visit_fields(Visitor& v)
   {
      Struct::_vIsItFiElDs_(this->top(), v, int_constant<0>());
   }
   template <typename Visitor>
   void visit_fields(Visitor& v) const
   {
      Struct::_vIsItFiElDs_(this->top(), v, int_constant<0>());
   }
   template <typename Operation>
   void apply_to_fields(Struct& o, const Operation& op)
   {
      Struct::_aPpLy2FiElDs_(this->top(), o, op, int_constant<0>());
   }
   template <typename Operation>
   void apply_to_fields(const Struct& o, const Operation& op) const
   {
      Struct::_aPpLy2FiElDs_(this->top(), o, op, int_constant<0>());
   }
   friend void relocate(Struct *from, Struct *to)
   {
      Struct::_rElOcFiElDs_(from, to, int_constant<0>());
   }
   template <int n>
   static const char* get_field_name(size_t& l, int_constant<n>)
   {
      return Struct::_fIeLdNaMe_(l, int_constant<n>());
   }

   template <typename Result>
   struct rebind_generic {
      typedef GenericStruct<Result> type;
   };
};

template <typename Struct>
struct spec_object_traits< GenericStruct<Struct> >
   : spec_or_model_traits<Struct,is_composite> {

   typedef is_composite generic_tag;
   typedef typename Struct::field_types elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& x, Visitor& v)
   {
      x.visit_fields(v);
   }
};

template <typename Struct> inline
bool operator== (const GenericStruct<Struct>& a, const GenericStruct<Struct>& b)
{
   return operations::eq<const Struct&, const Struct&>()(a.top(), b.top());
}

template <typename Struct> inline
bool operator!= (const GenericStruct<Struct>& a, const GenericStruct<Struct>& b)
{
   return !(a==b);
}

}
namespace polymake {

using pm::GenericStruct;

}
namespace std {

template <typename Struct> inline
void swap(pm::GenericStruct<Struct>& s1, pm::GenericStruct<Struct>& s2)
{
   s1.top().apply_to_fields(s2.top(), polymake::operations::swap_op());
}

}

#endif // POLYMAKE_GENERIC_STRUCT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
