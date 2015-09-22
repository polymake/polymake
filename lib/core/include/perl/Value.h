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

#ifndef POLYMAKE_PERL_VALUE_H
#define POLYMAKE_PERL_VALUE_H

#include "polymake/GenericIO.h"
#include "polymake/internal/assoc.h"
#include "polymake/internal/CharBuffer.h"
#include "polymake/internal/Wary.h"
#include "polymake/internal/Array.h"

#include <limits>
#include <cmath>
#include <string>
#include <typeinfo>

// TODO: eliminate all references to SV outside the perl-aware code, use opaque ScalarHolder<ownership> instead

#ifndef POLYMAKE_WITHIN_PERL
struct sv;
#define SV ::sv
struct PerlInterpreter;
#endif

namespace pm { namespace perl {

class SVHolder {
protected:
   SV* sv;

   SVHolder();

   explicit SVHolder(SV* sv_arg)
      : sv(sv_arg) {}

   SVHolder(SV* sv_arg, True);
   void set_copy(SV* sv_arg);
   void forget();
   bool is_tuple() const;
public:
   SV* get() const { return sv; }
   SV* get_temp();
};

class Scalar : public SVHolder {
public:
   Scalar() {}
   Scalar(const Scalar& x) : SVHolder(x.get(), True()) {}

   Scalar& operator= (const Scalar& x)
   {
      set_copy(x.get());
      return *this;
   }
   ~Scalar() { forget(); }

   // The following static functions have nothing to do with this class as such;
   // it's just a suitable place to gather them withour polluting the namespace.
   // Each of them produces finally a scalar value or converts it to something else.

   static SV* undef();

   static SV* const_string(const char* s, size_t l);
   static SV* const_string(const char* s)
   {
      return const_string(s,strlen(s));
   }

   static SV* const_string_with_int(const char* s, size_t l, int i);
   static SV* const_string_with_int(const char* s, int i)
   {
      return const_string_with_int(s,strlen(s),i);
   }

   static int convert_to_int(SV* sv);
   static double convert_to_float(SV* sv);
};

// forward declarations of mutual friends
class Value; class Object; class ObjectType; class PropertyValue;
class Hash;  class OptionSet;  class ListResult;
class Stack;  class Main;  class FunCall;

template <typename Element=Value> class ArrayOwner;
template <typename Element> class Array_access;
typedef ArrayOwner<> Array;

template <typename T> class type_cache;

class ArrayHolder : public SVHolder {
   static SV* init_me(int size);
protected:
   ArrayHolder(SV* sv_arg, True) : SVHolder(sv_arg, True()) {}
public:
   explicit ArrayHolder(int reserve=0)
      : SVHolder(init_me(reserve)) {}

   explicit ArrayHolder(SV* sv_arg)
      : SVHolder(sv_arg) {}

   explicit ArrayHolder(const Value&);

   ArrayHolder(const SVHolder& x, int reserve)
      : SVHolder(x)
   {
      upgrade(reserve);
   }

   ArrayHolder(SV* sv_arg, value_flags flags)
      : SVHolder(sv_arg)
   {
      if (flags & value_not_trusted) verify();
   }

   void upgrade(int size);
   void verify() const;
   void set_contains_aliases();

   int size() const;
   int dim(bool& has_sparse_representation) const;
   void resize(int n);
   SV* operator[] (int i) const;

   SV* shift();
   SV* pop();
   void unshift(SV* x);
   void push(SV* x);

   void unshift(const SVHolder& x) { unshift(x.get()); }
   void push(const SVHolder& x) { push(x.get()); }
};

class HashHolder : public SVHolder {
protected:
   HashHolder() : SVHolder(init_me()) {}

   HashHolder(SV* sv_arg, True) : SVHolder(sv_arg, True()) {}

   bool exists(const char* key, size_t klen) const;
   static SV* init_me();
   Value _access(const char* key, size_t klen, bool create) const;
public:
   explicit HashHolder(SV* sv_arg)
      : SVHolder(sv_arg) {}

   void verify();

   bool exists(const std::string& key) const
   {
      return exists(key.c_str(), key.size());
   }

   template <size_t klen>
   bool exists (const char (&key)[klen]) const
   {
      return exists(key, klen-1);
   }
};

class istreambuf : public streambuf_with_input_width {
public:
   istreambuf(SV* sv);

   int lines()
   {
      return CharBuffer::count_lines(this);
   }
};

class ostreambuf : public std::streambuf {
public:
   ostreambuf(SV* sv);
   ~ostreambuf();
protected:
   SV* val;
   int_type overflow(int_type c=traits_type::eof());
};

template <typename StreamBuffer>
class BufferHolder {
protected:
   StreamBuffer my_buf;

   explicit BufferHolder(SV* sv) : my_buf(sv) {}
};

class istream : public BufferHolder<istreambuf>,
                public std::istream {
   friend class Value;
public:
   explicit istream(SV* sv);

   void finish()
   {
      if (good() && CharBuffer::next_non_ws(&my_buf)>=0)
         setstate(failbit);
   }
protected:
   std::runtime_error parse_error() const;
};

class ostream : public BufferHolder<ostreambuf>,
                public std::ostream {
public:
   explicit ostream(SVHolder& sv)
      : BufferHolder<ostreambuf>(sv.get()), std::ostream(&my_buf)
   {
      precision(10);
      exceptions(failbit | badbit);
   }
};

class undefined : public std::runtime_error {
public:
   undefined();
};

class exception : public std::runtime_error {
public:
   exception();
   exception(const char* msg) : std::runtime_error(msg) {}
};

SV* complain_obsolete_wrapper(const char* file, int line, const char* expr);

template <typename X> struct TryCanned;
template <typename X> struct Canned;
template <typename X> struct Enum;
template <typename X> struct ReturningList;

template <typename Options=void> class ValueOutput;
template <typename Options=void> class ValueInput;

template <typename ElementType, typename Options>
class ListValueInput
   : public ArrayHolder,
     public GenericInputImpl< ListValueInput<ElementType,Options> >,
     public GenericIOoptions< ListValueInput<ElementType,Options>, Options, 1 > {
   int i, _size, _dim;

   typedef GenericIOoptions< ListValueInput<ElementType,Options>, Options, 1 > _opts;
public:
   typedef ElementType value_type;

   ListValueInput(SV* sv_arg)
      : ArrayHolder(sv_arg, _opts::template get_option< TrustedValue<True> >::value ? value_trusted : value_not_trusted)
      , i(0)
      , _size(ArrayHolder::size())
      , _dim(-1)
   {}

   template <typename T> inline
   ListValueInput& operator>> (T& x);

   int size() const { return _size; }
   bool at_end() const { return i>=_size; }

   void finish()
   {
      if (_opts::template get_option< CheckEOF<False> >::value && !at_end())
         throw std::runtime_error("list input - size mismatch");
   }

   template <typename T>
   int lookup_lower_dim(bool tell_size_if_dense);

   void skip_item() { ++i; }
   void skip_rest() { i=_size; }

   bool serialized_value() const { return is_tuple(); }

   bool sparse_representation()
   {
      if (extract_bool_param<Options, SparseRepresentation>::specified)
         return extract_bool_param<Options, SparseRepresentation>::value;
      bool has_sparse_representation;
      _dim=ArrayHolder::dim(has_sparse_representation);
      return has_sparse_representation;
   }

   int lookup_dim(bool tell_size_if_dense)
   {
      return sparse_representation() ? _dim :
             tell_size_if_dense ? _size : -1;
   }

   int index()
   {
      if (!sparse_representation())
         throw std::runtime_error("dense/sparse input mismatch");
      int ix=-1;
      *this >> ix;
      if (!_opts::template get_option< TrustedValue<True> >::value && (ix<0 || ix>=_dim))
         throw std::runtime_error("sparse index out of range");
      return ix;
   }
};

template <typename Options>
class ValueInput
   : public SVHolder
   , public GenericInputImpl< ValueInput<Options> >
   , public GenericIOoptions< ValueInput<Options>, Options > {
   template <typename, typename> friend class ListValueInput;
public:
   ValueInput(SV* sv_arg)
      : SVHolder(sv_arg) {}

   template <typename Data>
   void fallback(Data& x)
   {
      istream is(sv);
      is >> x;  is.finish();
   }

   bool serialized_value() const { return is_tuple(); }

   template <typename ObjectRef>
   struct list_cursor {
      typedef typename remove_bool_param<Options,SparseRepresentation>::type
         cursor_options;
      typedef ListValueInput<typename deref<ObjectRef>::type::value_type, cursor_options>
         type;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      typedef typename replace_params<
              typename remove_bool_param<Options,
                                         SparseRepresentation>::type,
                                         CheckEOF<True> >::type
         cursor_options;
      typedef ListValueInput<void, cursor_options> type;
   };

   template <typename T>
   typename list_cursor<T>::type begin_list(T*)
   {
      return sv;
   }

   template <typename T>
   typename composite_cursor<T>::type begin_composite(T*)
   {
      return sv;
   }
};

template <typename Options, bool returning_list=extract_bool_param<Options, ReturningList>::value>
class ListValueOutput
   : public ArrayHolder
   , public GenericIOoptions< ListValueOutput<Options>, Options > {
   ListValueOutput();
public:
   typedef SVHolder super;
   typedef SV* super_arg;
   static const bool stack_based=false;

   template <typename T>
   ListValueOutput& operator<< (const T& x);

   void finish() const {}
};

template <typename Options>
class ValueOutput
   : public ListValueOutput<Options>::super
   , public GenericOutputImpl< ValueOutput<Options> >
   , public GenericIOoptions< ValueOutput<Options>, Options > {

   typedef typename ListValueOutput<Options>::super super;
   typedef typename ListValueOutput<Options>::super_arg super_arg;
public:
   template <typename Data>
   void fallback(const Data& x)
   {
      store(x, bool2type<ListValueOutput<Options>::stack_based>());
   }

   void
   fallback(const char* x, size_t l)
   {
      store_string(x, l, bool2type<ListValueOutput<Options>::stack_based>());
   }

private:
   template <typename Data>
   void store(const Data& x, False)
   {
      ostream os(*this);
      os << x;
   }

   template <typename Data>
   void store(const Data& x, True)
   {
      SVHolder s;
      ostream os(s);
      os << x;
      this->push_temp(s);
   }

   void store_string(const char* x, size_t l, False);
   void store_string(const char* x, size_t l, True);
public:
   ValueOutput() {}

   explicit ValueOutput(super_arg sv_arg)
      : super(sv_arg) {}

   template <typename ObjectRef>
   struct list_cursor {
      typedef ListValueOutput<Options>& type;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      typedef typename deref<ObjectRef>::type Object;
      static const int
         total  = list_length<typename object_traits<Object>::elements>::value,
         ignored= list_accumulate_unary<list_count, ignore_in_composite, typename object_traits<Object>::elements>::value;
      static const bool
         compress= ignored>0 && total-ignored<=1;
      typedef typename if_else<compress, ValueOutput, ListValueOutput<Options> >::type& type;
   };

   template <typename ObjectRef>
   struct sparse_cursor : list_cursor<ObjectRef> {};

   template <typename T>
   ListValueOutput<Options>& begin_list(const T* x)
   {
      ListValueOutput<Options>& pvl=static_cast<ListValueOutput<Options>&>(static_cast<super&>(*this));
      pvl.upgrade(x && !object_traits<T>::is_lazy && container_traits<T>::is_forward ? x->size() : 0);
      return pvl;
   }

private:
   template <typename T>
   ListValueOutput<Options>& _begin_composite(const T*, False)
   {
      ListValueOutput<Options>& pvl=static_cast<ListValueOutput<Options>&>(static_cast<super&>(*this));
      pvl.upgrade(list_length<typename object_traits<T>::elements>::value);
      return pvl;
   }

   template <typename T>
   ValueOutput& _begin_composite(const T*, True)
   {
      return *this;
   }

public:
   template <typename T>
   typename composite_cursor<T>::type
   begin_composite(const T*)
   {
      return _begin_composite((const T*)0, bool2type<composite_cursor<T>::compress>());
   }

   template <typename T>
   ListValueOutput<Options>& begin_sparse(const T* x)
   {
      return begin_list(x);
   }

   template <typename T>
   bool prefer_sparse_representation(const T&) const { return false; }
};

class Stack {
protected:
   PerlInterpreter* pi;
   Stack();
   Stack(SV** start);
private:
   // inhibited
   void operator= (const Stack&);
public:
   Stack(bool room_for_object, int reserve);

   void push(SV* x) const;
   void push(SVHolder& x) const
   {
      push(x.get());
   }
   void push_temp(SVHolder& x) const
   {
      push(x.get_temp());
   }

   void cancel();
};

class ListReturn : public Stack {
private:
   // inhibited
   void operator= (const ListReturn&);
public:
   ListReturn() {}
   ListReturn(SV** stack_arg) : Stack(stack_arg) {}

   template <typename T>
   ListReturn& operator<< (const T& x);

   void upgrade(int size);
};

class ArgList : protected ArrayHolder {
   friend class FunCall;
public:
   ArgList(int reserve=0) : ArrayHolder(reserve) {}

   template <typename T>
   ArgList& operator<< (const T& x);
};

template <typename Options>
class ListValueOutput<Options, true>
   : public ListReturn,
     public GenericIOoptions< ListValueOutput<Options, true>, Options > {
   ListValueOutput();
public:
   typedef Stack super;
   typedef SV** super_arg;
   static const bool stack_based=true;

   void finish() const {}
};

typedef ValueOutput< ReturningList<True> > ListSlurp;

} // end namespace perl

template <>
struct is_printable<perl::Value> : False {};
template <>
struct is_parseable<perl::Value> : False {};
template <>
struct is_printable<perl::Object> : False {};
template <>
struct is_writeable<perl::Object> : True {};

// forward declaration of a specialization
template <>
class Array<perl::Object, void>;

template <>
struct is_printable< Array<perl::Object, void> > : False {};
template <>
struct is_writeable< Array<perl::Object, void> > : True {};

namespace perl {

template <typename T,
          bool _has_generic=has_generic_type<T>::value>
struct generic_representative {
   typedef typename object_traits<T>::persistent_type type;
};

template <typename T>
struct generic_representative<T, true> {
   typedef typename T::generic_type::persistent_type type;
};

// primitive perl types which need special handling if returned by reference in lvalue context
typedef list primitive_lvalues(bool, int, unsigned int, long, unsigned long, double, std::string);

template <typename T, typename Model=typename object_traits<T>::model>
struct obscure_type : False {};

template <typename T>
struct obscure_type<T, is_container> {
   static const bool value= object_traits<T>::dimension==1 && !has_serialized<T>::value && !has_iterator<T>::value;
};

template <typename Top>
class MaybeUndefined : public Generic<Top> {};

template <typename T>
struct numeric_traits : std::numeric_limits<T> {
   typedef T real_type;
   static const bool check_range = std::numeric_limits<T>::is_bounded && std::numeric_limits<T>::is_integer;
};

template <typename Base, typename E, typename Params>
struct numeric_traits< sparse_elem_proxy<Base, E, Params> > : numeric_traits<E> {
   typedef E real_type;
};

template <typename Given, typename Target=Given>
class access;
template <typename Given, typename Target, bool _try_conv, bool _unwary=Unwary<typename attrib<Given>::minus_const>::value>
class access_canned;
template <typename Target>
struct check_for_magic_storage;

class Value : public SVHolder {
public:
   struct Anchor {
      Anchor& store_anchor(SV*);
      Anchor& store_anchors(const Value& v) { return store_anchor(v.get()); }
      Anchor& operator()(const Value& v) { return store_anchor(v.get()); }

      SV* stored;
   };
   struct NoAnchor : Anchor {
      NoAnchor& store_anchor(SV*)           { return *this; }
      NoAnchor& store_anchors(const Value&) { return *this; }
      NoAnchor& operator()(const Value&) { return *this; }
   };

   explicit Value(value_flags opt_arg=value_trusted, unsigned int n_achors_arg=0)
      : n_anchors(n_achors_arg)
      , options(opt_arg)
   {}

   explicit Value(SV* sv_arg)
      : SVHolder(sv_arg)
      , n_anchors(0)
      , options(value_trusted)
   {}

   Value(SV* sv_arg, value_flags opt_arg, unsigned int n_achors_arg=0)
      : SVHolder(sv_arg)
      , n_anchors(n_achors_arg)
      , options(opt_arg)
   {}

   value_flags get_flags() const { return options; }
   unsigned int get_num_anchors() const { return n_anchors; }
protected:
   enum number_flags { not_a_number, number_is_zero, number_is_int, number_is_float, number_is_object };

   unsigned int n_anchors : 8;
   value_flags options : 8;

   Value(SV* sv_arg, const Array_access<Value>&)
      : SVHolder(sv_arg)
      , n_anchors(0)
      , options(value_not_trusted)
   {}

   static bool on_stack(const char* val, const char* frame_upper_bound);

   bool is_defined() const;
   bool is_TRUE() const;
   long int_value() const;
   long enum_value() const;
   double float_value() const;
   bool is_plain_text(bool expect_numeric_scalar=false) const;

   void set_perl_type(SV* proto);

   typedef std::pair<const std::type_info*, char*> canned_data_t;
   static
   canned_data_t get_canned_data(SV*);

   const std::type_info* get_canned_typeinfo() const { return get_canned_data(sv).first; }
   char* get_canned_value() const { return get_canned_data(sv).second; }

   int get_canned_dim(bool tell_size_if_dense) const;

   void* allocate_canned(SV* proto) const;
   Anchor* first_anchor_slot() const;
   Anchor* store_canned_ref(SV* descr, void* obj, value_flags flags) const;

   template <typename Numtype> static
   void assign_int(Numtype& x, long i, False, True) { x=i; }

   static void assign_int(long& x, long i, True, True) { x=i; }

   template <typename Numtype> static
   void assign_int(Numtype& x, long i, True, True)
   {
      if (i < numeric_traits<Numtype>::min() || i > numeric_traits<Numtype>::max())
         throw std::runtime_error("input integer property out of range");
      x=typename numeric_traits<Numtype>::real_type(i);
   }

   template <typename Numtype> static
   void assign_int(Numtype& x, long i, True, False)
   {
      if (i<0 || static_cast<unsigned long>(i) > numeric_traits<Numtype>::max())
         throw std::runtime_error("input integer property out of range");
      x=typename numeric_traits<Numtype>::real_type(i);
   }

   template <typename Numtype> static
   void assign_float(Numtype& x, double d, False) { x=d; }

   template <typename Numtype> static
   void assign_float(Numtype& x, double d, True)
   {
      if (d < numeric_traits<Numtype>::min() || d > numeric_traits<Numtype>::max())
         throw std::runtime_error("input integer property out of range");
      x=typename numeric_traits<Numtype>::real_type(lrint(d));
   }

   number_flags classify_number() const;

   template <typename Numtype>
   void num_input(Numtype& x) const
   {
      switch (classify_number()) {
      case number_is_zero:
         x= 0;
         break;
      case number_is_int:
         assign_int(x, int_value(), bool2type<numeric_traits<Numtype>::check_range>(),
                                    bool2type<numeric_traits<Numtype>::is_signed>());
         break;
      case number_is_float:
         assign_float(x, float_value(), bool2type<numeric_traits<Numtype>::check_range>());
         break;
      case number_is_object:
         assign_int(x, Scalar::convert_to_int(sv), bool2type<numeric_traits<Numtype>::check_range>(),
                                                   bool2type<numeric_traits<Numtype>::is_signed>());
         break;
      case not_a_number:
         throw std::runtime_error("invalid value for an input numerical property");
      }
   }

   False* retrieve(std::string& x) const;
   False* retrieve(const char* &x) const;
   False* retrieve(char &x) const;
   False* retrieve(double& x) const;
   False* retrieve(bool& x) const;

   False* retrieve(int& x) const { num_input(x); return NULL; }
   False* retrieve(unsigned int& x) const { num_input(x); return NULL; }
   False* retrieve(long& x) const { num_input(x); return NULL; }
   False* retrieve(unsigned long& x) const { num_input(x); return NULL; }

   False* retrieve(float& x) const
   {
      double xi;
      retrieve(xi);
      x=static_cast<float>(xi);
      return NULL;
   }

   False* retrieve(Array& x) const;
   False* retrieve(Object& x) const;
   False* retrieve(ObjectType& x) const;
   False* retrieve(pm::Array<Object>& x) const;

   SV* store_instance_in() const;

   template <typename Target>
   void cache_instance(const Target& x, False) const {}

   template <typename Target>
   void cache_instance(const Target& x, True) const
   {
      if (SV* store_in_sv=store_instance_in()) {
         Value store_in(store_in_sv);
         store_in << x;
      }
   }

   template <typename Options, typename Target>
   void do_parse(Target& x) const
   {
      istream my_stream(sv);
      PlainParser<Options> parser(my_stream);
      try {
         parser >> x;
         my_stream.finish();
      } catch (const std::ios::failure&) {
         throw my_stream.parse_error();
      }
   }

   // opaque type
   template <typename Target, typename Serializable>
   void retrieve(Target& x, False, Serializable) const
   {
      if (options & value_not_trusted)
         ValueInput< TrustedValue<False> >(sv) >> x;
      else
         ValueInput<>(sv) >> x;

      cache_instance(x, Serializable());
   }

   // numeric scalar type, non-serializable
   template <typename Target>
   void retrieve(Target& x, True, False) const
   {
      num_input(x);
   }

   // numeric scalar type, serializable
   template <typename Target>
   void retrieve(Target& x, True, True) const
   {
      if (is_tuple())
         retrieve(x, False(), True());
      else
         num_input(x);
   }

   template <typename Target>
   typename enable_if<void, (check_for_magic_storage<Target>::value && is_parseable<Target>::value)>::type
   retrieve_nomagic(Target& x) const
   {
      if (is_plain_text(numeric_traits<Target>::is_specialized)) {
         parse(x);
      } else {
         retrieve(x, bool2type<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
      }
   }

   template <typename Target>
   typename enable_if<void, (check_for_magic_storage<Target>::value && !is_parseable<Target>::value)>::type
   retrieve_nomagic(Target& x) const
   {
      retrieve(x, bool2type<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
   }

   template <typename Target>
   typename disable_if<void, check_for_magic_storage<Target>::value>::type
   retrieve_nomagic(Target& x) const
   {
      retrieve(x);
   }

   template <typename Target>
   True* retrieve(Target& x) const
   {
      if (!(options & value_ignore_magic)) {
         const canned_data_t canned=get_canned_data(sv);
         if (canned.first) {
            if (*canned.first == typeid(Target)) {
               if (MaybeWary<Target>::value && (options & value_not_trusted))
                  maybe_wary(x)=*reinterpret_cast<const Target*>(canned.second);
               else
                  x=*reinterpret_cast<const Target*>(canned.second);
               return NULL;
            }
            typedef void (*ass_f)(Target&, const Value&);
            if (ass_f assignment=reinterpret_cast<ass_f>(type_cache<Target>::get_assignment_operator(sv))) {
               assignment(x, *this);
               return NULL;
            }
         }
      }
      retrieve_nomagic(x);
      return NULL;
   }

   template <typename Source>
   void store_as_perl(const Source& x)
   {
      static_cast<ValueOutput<>&>(static_cast<SVHolder&>(*this)) << x;
      set_perl_type(type_cache<typename object_traits<Source>::persistent_type>::get_proto());
   }

   template <typename Stored, typename Source>
   void store(const Source& x)
   {
      new(allocate_canned(type_cache<Stored>::get_descr())) Stored(x);
   }

   template <typename Source>
   Anchor* store_ref(const Source& x)
   {
      return store_canned_ref(type_cache<Source>::get_descr(), (void*)&x, options);
   }

   template <typename Source>
   Anchor* store_magic(const Source& x, False, False, False)  // non-persistent regular type
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      if (options & value_allow_non_persistent) {
         store<Source>(x);
         return n_anchors ? first_anchor_slot() : NULL;
      } else {
         store<Persistent>(x);
         return NULL;
      }
   }

   template <typename Source, typename IsMasquerade, typename IsPersistent>
   Anchor* store_magic(const Source& x, IsMasquerade, True, IsPersistent)          // lazy type
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      store<Persistent>(x);
      return NULL;
   }

   template <typename Source, typename IsMasquerade>
   Anchor* store_magic_ref(const Source& x, IsMasquerade, False, False)          // non-persistent regular type
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      if (options & value_allow_non_persistent) {
         return store_ref(x);
      } else {
         store<Persistent>(x);
         return NULL;
      }
   }

   template <typename Source, typename IsMasquerade, typename IsPersistent>
   Anchor* store_magic_ref(const Source& x, IsMasquerade, True, IsPersistent)      // lazy type
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      store<Persistent>(x);
      return NULL;
   }

   template <typename Source>
   Anchor* store_magic(const Source& x, False, False, True)          // persistent regular type
   {
      store<Source>(x);
      return NULL;
   }

   template <typename Source>
   Anchor* store_magic_ref(const Source& x, False, False, True)
   {
      return store_ref(x);
   }

   template <typename Source>
   Anchor* store_magic(const Source& x, True, False, False)         // masquerade type belonging to a generic family
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      store<Persistent>(x);
      return NULL;
   }

   template <typename Source>
   Anchor* store_magic(const Source& x, True, False, True)         // masquerade type without persistent substitute
   {
      store_as_perl(x);
      return NULL;
   }

   template <typename Source>
   Anchor* store_magic_ref(const Source& x, True, False, True)
   {
      if (options & value_allow_non_persistent) {
         return store_ref(x);
      } else {
         store_as_perl(x);
         return NULL;
      }
   }

   Anchor* store_primitive_ref(const bool& x,          SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const int& x,           SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const unsigned int& x,  SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const long& x,          SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const unsigned long& x, SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const double& x,        SV* descr, bool take_ref);
   Anchor* store_primitive_ref(const std::string& x,   SV* descr, bool take_ref);

   void set_string_value(const char* x);
   void set_string_value(const char* x, size_t l);
   void set_copy(const SVHolder& x);
public:
   NoAnchor* put(int x,            const char* =NULL, int=0) { put(long(x));            return NULL; }
   NoAnchor* put(unsigned int x,   const char* =NULL, int=0) { put((unsigned long)(x)); return NULL; }
   NoAnchor* put(long x,           const char* =NULL, int=0);
   NoAnchor* put(unsigned long x,  const char* =NULL, int=0);
   NoAnchor* put(bool x,           const char* =NULL, int=0);
   NoAnchor* put(double x,         const char* =NULL, int=0);
   NoAnchor* put(const undefined&, const char* =NULL, int=0);

   NoAnchor* put(const char* x,    const char* =NULL, int=0)
   {
      if (x)
         set_string_value(x);
      else
         put(undefined(), NULL, 0);
      return NULL;
   }

   template <size_t ll>
   NoAnchor* put(const char (&x)[ll], const char* =NULL, int=0)
   {
      set_string_value(x, ll-1);
      return NULL;
   }

   NoAnchor* put(const std::string& x, const char* =NULL, int=0)
   {
      set_string_value(x.c_str(), x.size());
      return NULL;
   }

   NoAnchor* put(char x, const char* =NULL, int=0)
   {
      set_string_value(&x, 1);
      return NULL;
   }

   NoAnchor* put(const Object& x,            const char* =NULL, int=0);
   NoAnchor* put(const ObjectType& x,        const char* =NULL, int=0);
   NoAnchor* put(const PropertyValue& x,     const char* =NULL, int=0);
   NoAnchor* put(const Scalar& x,            const char* =NULL, int=0);
   NoAnchor* put(const Array& x,             const char* =NULL, int=0);
   NoAnchor* put(const Hash& x,              const char* =NULL, int=0);
   NoAnchor* put(const ListReturn& x,        const char* =NULL, int=0);
   NoAnchor* put(const pm::Array<Object>& x, const char* =NULL, int=0);

   template <typename Source, typename PerlPkg>
   typename enable_if<Anchor*, obscure_type<Source>::value>::type
   put(const Source& x, const char* fup, PerlPkg prescribed_pkg)
   {
      // obscure type (something weird, like a set complement)
      if (fup && (options & value_allow_non_persistent) &&
          type_cache<Source>::magic_allowed(prescribed_pkg)) {
         return store_canned_ref(type_cache<Source>::get_descr(), (void*)&x, value_flags(options | value_read_only));
      } else {
         throw std::invalid_argument("can't store an obscure C++ type without perl binding");
      }
   }

   template <typename Source, typename PerlPkg>
   typename disable_if<Anchor*, (obscure_type<Source>::value ||
                                 derived_from_instance<Source, MaybeUndefined>::value ||
                                 is_instance3_of<Source,sparse_elem_proxy>::value ||
                                 is_pointer<Source>::value)>::type
   put(const Source& x, const char* fup, PerlPkg prescribed_pkg)
   {
      typedef typename object_traits<Source>::persistent_type Persistent;
      if (type_cache<Source>::magic_allowed(prescribed_pkg)) {
         if (fup && !object_traits<Source>::is_lazy) {
            const char* const val=reinterpret_cast<const char*>(&x);
            if (!on_stack(val,fup)) {
               // the wrapped function has returned a reference to an object stored elsewhere
               return store_magic_ref(x, is_masquerade<Source>(), bool2type<object_traits<Source>::is_lazy>(), identical<Source,Persistent>());
            }
         }
         // taking reference not allowed or just temporary object
         return store_magic(x, is_masquerade<Source>(), bool2type<object_traits<Source>::is_lazy>(), identical<Source,Persistent>());
      } else {
         store_as_perl(x);
         return NULL;
      }
   }

   template <typename Source>
   typename enable_if<Anchor*, identical<typename object_traits<typename deref<Source>::type>::model, is_opaque>::value>::type
   put(Source* ptr, const char* =NULL, int=0)
   {
      if ((options & value_allow_non_persistent) &&
          type_cache<Source>::magic_allowed(0)) {
         return store_ref(*ptr);
      } else {
         throw std::invalid_argument("can't store an opaque C++ type without perl binding");
      }
   }

   template <typename Source, typename PerlPkg>
   NoAnchor* put(const MaybeUndefined<Source>& x, const char* fup, PerlPkg prescribed_pkg)
   {
      if (x.top().defined())
         put(*x.top(), fup, prescribed_pkg);
      else
         put(undefined(), NULL, 0);
      return NULL;
   }

   template <typename Base, typename E, typename Params, typename PerlPkg>
   Anchor* put(const sparse_elem_proxy<Base,E,Params>& x, const char*, PerlPkg prescribed_pkg)
   {
      if ((options & (value_allow_non_persistent | value_expect_lval | value_read_only)) == 
                     (value_allow_non_persistent | value_expect_lval) &&
          type_cache< sparse_elem_proxy<Base,E,Params> >::magic_allowed(prescribed_pkg)) {
         store< sparse_elem_proxy<Base,E,Params> >(x);
         return first_anchor_slot();
      } else {
         return put(x.get(), NULL, prescribed_pkg);
      }
   }

   template <typename Source, typename PerlPkg, typename OwnerType>
   typename disable_if<Anchor*, list_contains<primitive_lvalues,Source>::value>::type
   put_lval(const Source& x, const char* fup, PerlPkg prescribed_pkg, const Value* owner, OwnerType*)
   {
      if (identical<Source, typename access<OwnerType>::value_type>::value &&
          reinterpret_cast<const Source*>(owner->get_canned_value()) == &x) {
         forget();
         sv=owner->sv;
         return NULL;
      } else {
         Anchor* anchor=put(x, fup, prescribed_pkg);
         if (owner) get_temp();
         return anchor;
      }
   }

   template <typename Source, typename OwnerType>
   typename enable_if<Anchor*, list_contains<primitive_lvalues, Source>::value>::type
   put_lval(const Source& x, const char* fup, int, const Value* owner, OwnerType*)
   {
      const char* const val=reinterpret_cast<const char*>(&x);
      Anchor* anchor=store_primitive_ref(x, type_cache<Source>::get_descr(), !on_stack(val,fup));
      if (owner) get_temp();
      return anchor;
   }

   NoAnchor* put_lval(const Scalar& x, const char*, int, const Value*, void*);
   NoAnchor* put_lval(const Array& x,  const char*, int, const Value*, void*);
   NoAnchor* put_lval(const Hash& x,   const char*, int, const Value*, void*);

   template <typename Target>
   void parse(Target& x) const
   {
      if (options & value_not_trusted)
         do_parse< TrustedValue<False> >(x);
      else
         do_parse<void>(x);
   }

   // some code duplication with generic retrieve() is deliberate
   template <typename Target>
   operator Target () const
   {
      if (check_for_magic_storage<Target>::value) {
         if (!sv || !is_defined()) {
            if (options & value_allow_undef) return Target();
            throw undefined();
         }
         if (!(options & value_ignore_magic)) {
            const canned_data_t canned=get_canned_data(sv);
            if (canned.first) {
               if (*canned.first == typeid(Target))
                  return reinterpret_cast<const Target&>(*canned.second);
               typedef Target (*conv_f)(const Value&);
               if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv)))
                  return conversion(*this);
            }
         }
         Target x;
         retrieve_nomagic(x);
         return x;
      } else {
         Target x=Target();  // avoid 'may be used uninitialized' warning for POD types
         // may be overloaded; if not, some specialized retrieve instance will be called
         *this >> x;
         return x;
      }
   }

   operator void* () const { return is_TRUE() ? (void*)1 : 0; }
   bool operator! () const { return !is_TRUE(); }

   template <typename Target> friend
   bool operator>> (const Value& me, Target& x)
   {
      if (!me.sv || !me.is_defined()) {
         if (!(me.options & value_allow_undef))
            throw undefined();
         return false;
      }
      me.retrieve(x);
      return true;
   }

   template <typename Source> friend
   void operator<< (const Value& me, const Source& x)
   {
      const_cast<Value&>(me).put(x, NULL, 0);
   }

   template <size_t ll> friend
   void operator<< (const Value& me, const char (&x)[ll])
   {
      const_cast<Value&>(me).set_string_value(x, ll-1);
   }

   template <typename Target>
   void* allocate(SV* proto)
   {
      return allocate_canned(type_cache<Target>::get_descr(proto));
   }

   template <typename Target>
   typename access<Target>::return_type get() const
   {
      return access<Target>::get(*this);
   }

   template <typename Given, typename Target>
   typename access<Given, Target>::return_type get() const
   {
      return access<Given, Target>::get(*this);
   }
   using SVHolder::get;

   template <typename T>
   int lookup_dim(bool tell_size_if_dense)
   {
      int d=-1;
      if (is_plain_text()) {
         istream my_stream(sv);
         if (options & value_not_trusted)
            d=PlainParser< TrustedValue<False> >(my_stream).begin_list((T*)0).lookup_dim(tell_size_if_dense);
         else
            d=PlainParser<>(my_stream).begin_list((T*)0).lookup_dim(tell_size_if_dense);

      } else if (get_canned_typeinfo()) {
         d=get_canned_dim(tell_size_if_dense);

      } else {
         if (options & value_not_trusted)
            d=ListValueInput< T, TrustedValue<False> >(sv).lookup_dim(tell_size_if_dense);
         else
            d=ListValueInput<T, void>(sv).lookup_dim(tell_size_if_dense);
      }
      return d;
   }

   template <typename, typename> friend class access;
   template <typename, typename, bool, bool> friend class access_canned;
   template <typename> friend struct check_for_magic_storage;
   friend class ArrayHolder;
   template <typename> friend class Array_access;
   friend class OptionSet;
   friend class ListResult;
   template <typename> friend class ValueOutput;
   template <typename, typename> friend class ListValueInput;
};

SV* make_string_array(int size, ...);

inline ArrayHolder::ArrayHolder(const Value& v) : SVHolder(v.sv)
{
   if (v.options & value_not_trusted) verify();
}

template <typename Target>
struct check_for_magic_storage {
   struct helper {
      static derivation::yes Test(True*);
      static derivation::no Test(False*);
      static Target& piece();
   };
   static const bool value= sizeof(helper::Test(Value().retrieve(helper::piece()))) == sizeof(derivation::yes);
};

template <typename ElementType, typename Options>
template <typename T> inline
ListValueInput<ElementType, Options>&
ListValueInput<ElementType, Options>::operator>> (T& x)
{
   if (_opts::template get_option< CheckEOF<False> >::value && at_end())
      throw std::runtime_error("list input - size mismatch");
   Value elem((*this)[i++], _opts::template get_option< TrustedValue<True> >::value ? value_trusted : value_not_trusted );
   elem >> x;
   return *this;
}

template <typename ElementType, typename Options>
template <typename T> inline
int ListValueInput<ElementType, Options>::lookup_lower_dim(bool tell_size_if_dense)
{
   if (_size==0) return 0;
   Value first_elem((*this)[0], _opts::template get_option< TrustedValue<True> >::value ? value_trusted : value_not_trusted);
   return first_elem.lookup_dim<T>(tell_size_if_dense);
}

template <typename Options> inline
void ValueOutput<Options>::store_string(const char* x, size_t l, False)
{
   static_cast<Value*>(static_cast<super*>(this))->set_string_value(x,l);
}

template <typename Options> inline
void ValueOutput<Options>::store_string(const char* x, size_t l, True)
{
   Value v;
   v.set_string_value(x,l);
   this->push_temp(v);
}

template <typename Options, bool returning_list>
template <typename T> inline
ListValueOutput<Options, returning_list>&
ListValueOutput<Options, returning_list>::operator<< (const T& x)
{
   Value elem;
   elem << x;
   push(elem);
   return *this;
}

template <typename T> inline
ListReturn& ListReturn::operator<< (const T& x)
{
   Value elem;
   elem << x;
   push_temp(elem);
   return *this;
}

template <typename T> inline
ArgList& ArgList::operator<< (const T& x)
{
   Value elem;
   elem << x;
   push(elem);
   return *this;
}

template <typename Element>
class Array_access {
protected:
   ArrayHolder ary;
public:
   Array_access(SV* sv_arg=NULL) : ary(sv_arg) {}

   typedef int argument_type;
   typedef Element result_type;
   Element operator() (int i) const
   {
      return Element(ary[i], *this);
   }
};

template <typename Element>
class ArrayOwner : public ArrayHolder,
                   public modified_container_impl< ArrayOwner<Element>,
                                                   list ( Container< sequence >,
                                                          Operation< Array_access<Element> > ) > {
   friend class Value;
protected:
   explicit ArrayOwner(SV* sv_arg) : ArrayHolder(sv_arg) {}
   explicit ArrayOwner(const Value& v) : ArrayHolder(v) {}
public:
   ArrayOwner() {}
   explicit ArrayOwner(int n) { resize(n); }

   ArrayOwner(const ArrayOwner& x) : ArrayHolder(x.get(), True()) {}

   ArrayOwner& operator= (const ArrayOwner& x)
   {
      set_copy(x.get());
      return *this;
   }

   ~ArrayOwner() { forget(); }

   using ArrayHolder::size;
   using modified_container_impl<ArrayOwner>::operator[];

   bool empty() const { return size()==0; }

   sequence get_container() const { return sequence(0,size()); }
   Array_access<Element> get_operation() const { return sv; }

   void clear() { resize(0); }
};

class OptionSet : public HashHolder {
public:
   OptionSet() : HashHolder() { this->get_temp(); }
   
   OptionSet(const Value& v)
      : HashHolder(v.sv) { verify(); }
public:
   template <size_t klen>
   Value operator[] (const char (&key)[klen]) const
   {
      return _access(key, klen-1, false);
   }
   Value operator[] (const std::string& key) const
   {
      return _access(key.c_str(), key.size(), false);
   }

   friend class Hash;
};

class Hash : public HashHolder {
public:
   Hash() {}
   Hash(const Hash& x) : HashHolder(x.get(), True()) {}

   Hash& operator= (const Hash& x)
   {
      set_copy(x.get());
      return *this;
   }

   ~Hash() { forget(); }

   template <size_t klen>
   Value operator[] (const char (&key)[klen]) const
   {
      return _access(key, klen-1, false);
   }
   Value operator[] (const std::string& key) const
   {
      return _access(key.c_str(), key.size(), false);
   }

   template <size_t klen>
   Value operator[] (const char (&key)[klen])
   {
      return _access(key, klen-1, true);
   }
   Value operator[] (const std::string& key)
   {
      return _access(key.c_str(), key.size(), true);
   }
};

inline Value::NoAnchor* Value::put(const Scalar& x,      const char*, int) { set_copy(x); return NULL; }
inline Value::NoAnchor* Value::put(const Array& x,       const char*, int) { set_copy(x); return NULL; }
inline Value::NoAnchor* Value::put(const Hash& x,        const char*, int) { set_copy(x); return NULL; }
inline Value::NoAnchor* Value::put(const ListReturn& x,  const char*, int) { forget(); sv=NULL; return NULL; }
inline Value::NoAnchor* Value::put_lval(const Scalar& x, const char*, int, const Value*, void*) { forget(); sv=x.get(); return NULL; }
inline Value::NoAnchor* Value::put_lval(const Array& x,  const char*, int, const Value*, void*) { forget(); sv=x.get(); return NULL; }
inline Value::NoAnchor* Value::put_lval(const Hash& x,   const char*, int, const Value*, void*) { forget(); sv=x.get(); return NULL; }

} }

#endif // POLYMAKE_PERL_VALUE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
