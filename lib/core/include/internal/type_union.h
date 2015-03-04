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

#ifndef POLYMAKE_INTERNAL_TYPE_UNION_H
#define POLYMAKE_INTERNAL_TYPE_UNION_H

#include "polymake/internal/type_manip.h"
#include <stdexcept>

namespace pm {
namespace virtuals {

void _nop();

template <typename Definitions>
class table {
   typedef typename Definitions::fpointer fpointer;

   struct _impl {
      static const int length=Definitions::length;

      template <int discr>
      void _init(fpointer* ptr, int2type<discr>)
      {
         *ptr=&Definitions::template defs<discr>::_do;
         _init(ptr-1, int2type<discr-1>());
      }

      void _init(fpointer* ptr, int2type<-1>)
      {
         *ptr=reinterpret_cast<fpointer>(&_nop);
      }

      fpointer _table[length+1];

      _impl() { _init(_table+length, int2type<length-1>()); }
   };

   static _impl const vt;
public:
   static fpointer call(int discr) { return vt._table[discr+1]; }
};

template <typename Definitions>
typename table<Definitions>::_impl const table<Definitions>::vt=typename table<Definitions>::_impl();

template <typename Mapping> struct mapping;

template <typename Mapping, int pos, int last=list_length<Mapping>::value-1>
struct mapping_impl : public mapping_impl<typename Mapping::tail, pos+1, last> {
   template <typename> friend struct mapping;
protected:
   mapping_impl()
   {
      typedef typename Mapping::head head;
      this->_table[pos]=head::value;
   }
};

template <typename Mapping, int last>
struct mapping_impl<Mapping, last, last> {
   template <typename> friend struct mapping;
protected:
   int _table[last+1];
   mapping_impl()
   {
      _table[last]=Mapping::value;
   }
public:
   int operator[] (int i) const { return _table[i]; }
};

template <typename Mapping>
struct mapping {
   static const mapping_impl<Mapping, 0> table;
};

template <typename Mapping>
mapping_impl<Mapping, 0> const mapping<Mapping>::table=mapping_impl<Mapping, 0>();

template <typename T,
          bool _is_mutable=is_mutable<T>::value,
          bool _is_reference=attrib<T>::is_reference>
struct basics {         // non-reference, const
   typedef T type;

   static const type& get(const char* obj)
   {
      return *reinterpret_cast<const type*>(obj);
   }

   static void construct_default(char* dst)
   {
      new(dst) type;
   }

   template <typename Source>
   static void construct(char* dst, const Source& src)
   {
      new(dst) type(src);
   }

   static void destroy(char* obj)
   {
      std::_Destroy(reinterpret_cast<type*>(obj));
   }
};

template <typename T>
struct basics<T, false, true> { // reference, const
   typedef typename deref<T>::type type;

   static const type& get(const char* obj)
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
   typedef basics<T, false, false> read_only;

   static typename read_only::type& get(char* obj)
   {
      return *reinterpret_cast<typename read_only::type*>(obj);
   }
   using read_only::get;
};

template <typename T>
struct basics<T, true, true>    // reference, non-const
   : basics<T, false, true> {
   typedef basics<T, false, true> read_only;

   static typename read_only::type& get(char* obj)
   {
      return **reinterpret_cast<typename read_only::type**>(obj);
   }
   using read_only::get;
};

template <typename T>
struct default_constructor {
   static void _do(char* dst)
   {
      basics<T>::construct_default(dst);
   }
};
template <typename T>
struct copy_constructor {
   static void _do(char* dst, const char* src)
   {
      basics<T>::construct(dst, basics<T>::get(src));
   }
};
template <typename T>
struct destructor {
   static void _do(char* obj)
   {
      basics<T>::destroy(obj);
   }
};

template <typename TypeList>
struct type_union_functions {
   template <int discr>
   struct basics : virtuals::basics<typename n_th<TypeList,discr>::type> { };

   struct length_def {
      static const int length=list_length<TypeList>::value;
   };
   struct default_constructor : length_def {
      template <int discr> struct defs : virtuals::default_constructor<typename n_th<TypeList,discr>::type> { };
      typedef void (*fpointer)(char*);
   };
   struct copy_constructor : length_def {
      template <int discr> struct defs : virtuals::copy_constructor<typename n_th<TypeList,discr>::type> { };
      typedef void (*fpointer)(char*, const char*);
   };
   struct destructor : length_def {
      template <int discr> struct defs : virtuals::destructor<typename n_th<TypeList,discr>::type> { };
      typedef void (*fpointer)(char*);
   };
};
} // end namespace virtuals

template <typename Object>
struct union_traits {
   static const int size=attrib<Object>::is_reference ? sizeof(typename deref<Object>::type*)
                                                      : sizeof(typename deref<Object>::type);

   template <typename Object2>
   struct isomorphic_types_helper
      : isomorphic_types_deref<Object, Object2> {};

   template <typename Head2, typename Tail2>
   struct isomorphic_types_helper< cons<Head2, Tail2> > {
      typedef isomorphic_types_helper<Head2> helper_for_head;
      typedef isomorphic_types_helper<Tail2> helper_for_tail;
      static const bool value = helper_for_head::value && helper_for_tail::value;
      typedef typename identical<typename helper_for_head::discriminant, typename helper_for_tail::discriminant>::type discriminant;
   };
};

template <typename Head, typename Tail>
struct union_traits< cons<Head, Tail> > {
   typedef union_traits<Head> traits_head;
   typedef union_traits<Tail> traits_tail;
   static const int size= traits_head::size >= traits_tail::size ? traits_head::size : traits_tail::size;

   template <typename Object2>
   struct isomorphic_types_helper {
      typedef typename traits_head::template isomorphic_types_helper<Object2> helper_for_head;
      typedef typename traits_tail::template isomorphic_types_helper<Object2> helper_for_tail;
      static const bool value = helper_for_head::value && helper_for_tail::value;
      typedef typename identical<typename helper_for_head::discriminant, typename helper_for_tail::discriminant>::type discriminant;
   };
};

template <typename TypeList, bool heap_based>
class type_union_base {
protected:
   POLYMAKE_ALIGN(char area[union_traits<TypeList>::size], 8);
};

template <typename TypeList>
class type_union_base<TypeList, true> {
protected:
   char *area;

   type_union_base()
      : area(new char[union_traits<TypeList>::size]) { }

   ~type_union_base() { delete[] area; }
};

template <typename TypeList, bool heap_based=false>
class type_union : protected type_union_base<TypeList, heap_based> {
protected:
   int discriminant;
   typedef virtuals::type_union_functions<TypeList> Functions;

   template <typename Source, int discr>
   void _init_from_value(const Source& src, int2type<discr>)
   {
      discriminant=discr;
      Functions::template basics<discr>::construct(this->area,src);
   }

   template <typename OtherList, bool other_heap>
   void _init_from_value(const type_union<OtherList,other_heap>& src, int2type<-1>)
   {
      _init_from_union(src, bool2type<list_mapping<OtherList, TypeList>::mismatch>());
   }

   template <typename OtherList, bool other_heap>
   void _init_from_union(const type_union<OtherList,other_heap>& src, False)
   {
      discriminant=virtuals::mapping< typename list_mapping<OtherList, TypeList>::type >::table[src.discriminant];
      virtuals::table<typename Functions::copy_constructor>::call(discriminant)(this->area,src.area);
   }

   template <typename Source>
   void _init(const Source& src, False)
   {
      _init_from_value(src, int2type<list_search<TypeList, Source, identical_minus_const_ref>::pos>());
   }

   template <typename Source>
   void _init(const Source& src, True)
   {
      discriminant=src.discriminant;
      virtuals::table<typename Functions::copy_constructor>::call(discriminant)(this->area,src.area);
   }

   template <typename Source, int discr>
   void _assign_value(const Source& src, int2type<discr>)
   {
      virtuals::table<typename Functions::destructor>::call(discriminant)(this->area);
      discriminant=discr;
      Functions::template basics<discr>::construct(this->area,src);
   }

   template <typename OtherList, bool other_heap>
   void _assign_value(const type_union<OtherList,other_heap>& src, int2type<-1>)
   {
      _assign_union(src, bool2type<list_mapping<OtherList, TypeList>::mismatch>());
   }

   template <typename OtherList, bool other_heap>
   void _assign_union(const type_union<OtherList,other_heap>& src, False)
   {
      const int discr=virtuals::mapping< typename list_mapping<OtherList, TypeList>::type >::table[src.discriminant];
      virtuals::table<typename Functions::destructor>::call(discriminant)(this->area);
      discriminant=discr;
      virtuals::table<typename Functions::copy_constructor>::call(discriminant)(this->area,src.area);
   }

   template <typename Source>
   void _assign(const Source& src, False)
   {
      _assign_value(src, int2type<list_search<TypeList, Source, identical_minus_const_ref>::pos>());
   }

   template <typename Source>
   void _assign(const Source& src, True)
   {
      virtuals::table<typename Functions::destructor>::call(discriminant)(this->area);
      _init(src, True());
   }
public:
   type_union() : discriminant(-1) { }

   type_union(const type_union& src)
   {
      _init(src, True());
   }

   template <typename Source>
   type_union(const Source& src)
   {
      _init(src, derived_from<Source,type_union>());
   }

   template <typename Source>
   Source* init()
   {
      const int discr=list_search<TypeList, Source, identical>::pos;
      if (discriminant>=0) {
         if (discriminant != discr) return 0;
      } else {
         discriminant=discr;
         Functions::template basics<discr>::construct_default(this->area);
      }
      return &Functions::template basics<discr>::get(this->area);
   }

   ~type_union()
   {
      virtuals::table<typename Functions::destructor>::call(discriminant)(this->area);
   }

   type_union& operator= (const type_union& src)
   {
      _assign(src, True());
      return *this;
   }

   template <typename Source>
   type_union& operator= (const Source& src)
   {
      _assign(src, derived_from<Source,type_union>());
      return *this;
   }

   template <typename T>
   T* as()
   {
      const int discr=list_search<TypeList, T, identical_minus_const_ref>::pos;
      if (discriminant != discr) return 0;
      return &Functions::template basics<discr>::get(this->area);
   }

   template <typename T>
   typename attrib<T>::plus_const* as() const
   {
      const int discr=list_search<TypeList, T, identical_minus_const_ref>::pos;
      if (discriminant != discr) return 0;
      return &Functions::template basics<discr>::get(this->area);
   }

   int get_discriminant() const { return discriminant; }
   bool valid() const { return discriminant>=0; }

   template <typename,bool> friend class type_union;
};

template <typename TypeList, bool heap_based, typename Object2>
struct isomorphic_types< type_union<TypeList, heap_based>, Object2>
   : union_traits<TypeList>::template isomorphic_types_helper<Object2> {};

template <typename Object1, typename TypeList, bool heap_based>
struct isomorphic_types<Object1, type_union<TypeList, heap_based> >
   : union_traits<TypeList>::template isomorphic_types_helper<Object1> {
   typedef typename reverse_cons<typename union_traits<TypeList>::template isomorphic_types_helper<Object1>::discriminant>::type discriminant;
};

template <typename TypeList1, bool heap_based1, typename TypeList2, bool heap_based2>
struct isomorphic_types< type_union<TypeList1, heap_based1>, type_union<TypeList2, heap_based2> >
   : union_traits<TypeList1>::template isomorphic_types_helper<TypeList2> {};

} // end namespace pm

#endif // POLYMAKE_INTERNAL_TYPE_UNION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
