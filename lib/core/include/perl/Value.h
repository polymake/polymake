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
#include <memory>

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

   explicit SVHolder(SV* sv_arg) noexcept
      : sv(sv_arg) {}

   SVHolder(SV* sv_arg, std::true_type);
   void set_copy(SV* sv_arg);
   void forget();
   bool is_tuple() const;
public:
   SV* get() const noexcept { return sv; }
   SV* get_temp();
};

class Scalar : public SVHolder {
public:
   Scalar() {}
   Scalar(const Scalar& x) : SVHolder(x.get(), std::true_type()) {}

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
class Stack;  class Main;

template <typename Element=Value> class ArrayOwner;
typedef ArrayOwner<> Array;

template <typename T> class type_cache;

class ArrayHolder : public SVHolder {
   static SV* init_me(int size);
protected:
   ArrayHolder(SV* sv_arg, std::true_type)
      : SVHolder(sv_arg, std::true_type()) {}
public:
   explicit ArrayHolder(int reserve=0)
      : SVHolder(init_me(reserve)) {}

   explicit ArrayHolder(const Value&);

   ArrayHolder(const SVHolder& x, int reserve)
      : SVHolder(x)
   {
      upgrade(reserve);
   }

   explicit ArrayHolder(SV* sv_arg, value_flags flags=value_trusted)
      : SVHolder(sv_arg)
   {
      if (flags & value_not_trusted)
         verify();
      else if (flags & value_allow_undef)
         upgrade(0);
   }

   void upgrade(int size);
   void verify() const;
   void set_contains_aliases();

   int size() const;
   int dim(bool& has_sparse_representation) const;
   int cols() const;
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

   HashHolder(SV* sv_arg, std::true_type) : SVHolder(sv_arg, std::true_type()) {}

   static SV* init_me();
   SV* fetch(const AnyString& key, bool create) const;
public:
   explicit HashHolder(SV* sv_arg)
      : SVHolder(sv_arg) {}

   void verify();

   bool exists(const AnyString& key) const;
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

template <typename Options=mlist<>> class ValueOutput;
template <typename Options=mlist<>> class ValueInput;

template <typename ElementType, typename Options=mlist<>>
class ListValueInput
   : public ArrayHolder
   , public GenericInputImpl< ListValueInput<ElementType, Options> >
   , public GenericIOoptions< ListValueInput<ElementType, Options>, Options, 1 > {
   int i, _size, _dim;
public:
   typedef ElementType value_type;

   ListValueInput(SV* sv_arg)
      : ArrayHolder(sv_arg, this->get_option(TrustedValue<std::true_type>()) ? value_trusted : value_not_trusted)
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
      if (this->get_option(CheckEOF<std::false_type>()) && !at_end())
         throw std::runtime_error("list input - size mismatch");
   }

   void skip_item() { ++i; }
   void skip_rest() { i=_size; }

   bool serialized_value() const { return is_tuple(); }

   bool sparse_representation()
   {
      if (mtagged_list_extract<Options, SparseRepresentation>::is_specified)
         return this->get_option(SparseRepresentation<std::false_type>());
      bool has_sparse_representation;
      _dim=ArrayHolder::dim(has_sparse_representation);
      return has_sparse_representation;
   }

   int cols(bool tell_size_if_dense);

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
      if (!this->get_option(TrustedValue<std::true_type>()) && (ix<0 || ix>=_dim))
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
      typedef typename mtagged_list_remove<Options, SparseRepresentation>::type
         cursor_options;
      typedef ListValueInput<typename deref<ObjectRef>::type::value_type, cursor_options>
         type;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      typedef typename mtagged_list_replace<
                typename mtagged_list_remove<Options, SparseRepresentation>::type,
                CheckEOF<std::true_type> >::type
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

template <typename Options, bool returning_list=tagged_list_extract_integral<Options, ReturningList>(false)>
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

   ListValueOutput& non_existent()
   {
      return *this << undefined();
   }

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
      store(x, bool_constant<ListValueOutput<Options>::stack_based>());
   }

   void
   fallback(const char* x, size_t l)
   {
      store_string(x, l, bool_constant<ListValueOutput<Options>::stack_based>());
   }

private:
   template <typename Data>
   void store(const Data& x, std::false_type)
   {
      ostream os(*this);
      os << x;
   }

   template <typename Data>
   void store(const Data& x, std::true_type)
   {
      SVHolder s;
      ostream os(s);
      os << x;
      this->push_temp(s);
   }

   void store_string(const char* x, size_t l, std::false_type);
   void store_string(const char* x, size_t l, std::true_type);
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
      typedef typename std::conditional<compress, ValueOutput, ListValueOutput<Options>>::type& type;
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
   ListValueOutput<Options>& begin_composite_impl(const T*, std::false_type)
   {
      ListValueOutput<Options>& pvl=static_cast<ListValueOutput<Options>&>(static_cast<super&>(*this));
      pvl.upgrade(list_length<typename object_traits<T>::elements>::value);
      return pvl;
   }

   template <typename T>
   ValueOutput& begin_composite_impl(const T*, std::true_type)
   {
      return *this;
   }

public:
   template <typename T>
   typename composite_cursor<T>::type
   begin_composite(const T*)
   {
      return begin_composite_impl((const T*)0, bool_constant<composite_cursor<T>::compress>());
   }

   template <typename T>
   ListValueOutput<Options>& begin_sparse(const T* x)
   {
      return begin_list(x);
   }

   static constexpr int choose_sparse_representation() { return -1; }
};

class Stack {
protected:
   PerlInterpreter* pi;
   Stack();
   Stack(SV** start);
   explicit Stack(int reserve);
private:
   Stack(const Stack&) = delete;
   void operator= (const Stack&) = delete;

protected:
   void xpush(SV* x) const;

public:
   // FIXME: make protected
   void push(SV* x) const;

   // FIXME: remove
   Stack(bool room_for_object, int reserve);

   Stack(Stack&&) = default;

   void push(SVHolder& x) const
   {
      xpush(x.get());
   }
   void push_temp(SVHolder& x) const
   {
      xpush(x.get_temp());
   }

   void cancel();
};

class ListReturn : public Stack {
private:
   void operator= (const ListReturn&) = delete;
   ListReturn(const ListReturn&) = delete;
public:
   ListReturn() {}
   ListReturn(SV** stack_arg) : Stack(stack_arg) {}

   ListReturn(ListReturn&&) = default;

   template <typename T>
   ListReturn& operator<< (const T& x);

   void upgrade(int size);
};

template <typename Options>
class ListValueOutput<Options, true>
   : public ListReturn
   , public GenericIOoptions< ListValueOutput<Options, true>, Options > {
   ListValueOutput();
public:
   typedef Stack super;
   typedef SV** super_arg;
   static const bool stack_based=true;

   void finish() const {}
};

typedef ValueOutput<mlist<ReturningList<std::true_type>>> ListSlurp;

} // end namespace perl

template <>
struct is_printable<perl::Value> : std::false_type {};
template <>
struct is_parseable<perl::Value> : std::false_type {};
template <>
struct is_printable<perl::Object> : std::false_type {};
template <>
struct is_writeable<perl::Object> : std::true_type {};

// forward declaration of a specialization
template <>
class Array<perl::Object>;

template <>
struct is_printable< Array<perl::Object> > : std::false_type {};
template <>
struct is_writeable< Array<perl::Object> > : std::true_type {};

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
typedef mlist<bool, int, unsigned int, long, unsigned long, double, std::string> primitive_lvalues;

// TODO: remove this when Complements become stateful aliases
template <typename T, typename Model=typename object_traits<T>::model>
struct obscure_type : std::false_type {};

template <typename T>
struct obscure_type<T, is_container> {
   static const bool value= object_traits<T>::dimension==1 && !has_serialized<T>::value && !has_iterator<T>::value;
};

template <typename Top>
class MaybeUndefined : public Generic<Top> {};

template <typename T>
struct numeric_traits : std::numeric_limits<type_behind_t<T>> {
   typedef type_behind_t<T> real_type;
   static const bool check_range = std::numeric_limits<real_type>::is_bounded && std::numeric_limits<real_type>::is_integer;
};

template <typename Given, typename Target=Given>
class access;
template <typename Given, typename Target, bool _try_conv, bool _unwary=Unwary<typename attrib<Given>::minus_const>::value>
class access_canned;
template <typename Target>
struct check_for_magic_storage;

class Value
   : public SVHolder {
public:
   explicit Value(value_flags opt_arg=value_trusted)
      : options(opt_arg)
   {}

   explicit Value(SV* sv_arg, value_flags opt_arg=value_trusted) noexcept
      : SVHolder(sv_arg)
      , options(opt_arg)
   {}

   value_flags get_flags() const noexcept { return options; }

   struct Anchor {
      void store(SV* sv) noexcept;
      void store(const Value& v) noexcept { store(v.get()); }

      SV* stored;
   };
   struct NoAnchors {
      constexpr operator Anchor* () const { return nullptr; }
   };

   template <bool is_readonly>
   class Array_element_factory;

protected:
   enum number_flags { not_a_number, number_is_zero, number_is_int, number_is_float, number_is_object };

   value_flags options;

   bool is_defined() const noexcept;
   bool is_TRUE() const;
   long int_value() const;
   long enum_value() const;
   double float_value() const;
   bool is_plain_text(bool expect_numeric_scalar=false) const;

   typedef std::pair<const std::type_info*, char*> canned_data_t;

   static
   canned_data_t get_canned_data(SV*) noexcept;

   const std::type_info* get_canned_typeinfo() const noexcept { return get_canned_data(sv).first; }
   char* get_canned_value() const noexcept { return get_canned_data(sv).second; }

   int get_canned_dim(bool tell_size_if_dense) const;

   static
   void store_anchors(Anchor* place) noexcept {}

   template <typename TAnchor1, typename... TMoreAnchors>
   static
   void store_anchors(Anchor* place, TAnchor1&& anchor1, TMoreAnchors&&... more_anchors) noexcept
   {
      place->store(anchor1);
      store_anchors(++place, std::forward<TMoreAnchors>(more_anchors)...);
   }

   std::pair<void*, Anchor*> allocate_canned(SV* proto, int n_anchors) const;

   void* allocate_canned(SV* proto) const { return allocate_canned(proto, 0).first; }

   void mark_canned_as_initialized();

   Anchor* store_canned_ref_impl(void* obj, SV* type_descr, value_flags flags, int n_anchors) const;

   template <typename Numtype>
   static
   void assign_int(Numtype& x, long i, std::false_type) { x=i; }

   static
   void assign_int(long& x, long i, std::true_type) { x=i; }

   template <typename Numtype>
   static
   void assign_int(Numtype& x, long i, std::true_type)
   {
      if (i < min_value_as<long>(mlist<Numtype>()) ||
          i > max_value_as<long>(mlist<Numtype>()))
         throw std::runtime_error("input numeric property out of range");
      x=typename numeric_traits<Numtype>::real_type(i);
   }

   template <typename Numtype>
   static
   void assign_float(Numtype& x, double d, std::false_type) { x=d; }

   template <typename Numtype>
   static
   void assign_float(Numtype& x, double d, std::true_type)
   {
      if (d < min_value_as<double>(mlist<Numtype>()) ||
          d > max_value_as<double>(mlist<Numtype>()))
         throw std::runtime_error("input numeric property out of range");
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
         assign_int(x, int_value(), bool_constant<numeric_traits<Numtype>::check_range>());
         break;
      case number_is_float:
         assign_float(x, float_value(), bool_constant<numeric_traits<Numtype>::check_range>());
         break;
      case number_is_object:
         assign_int(x, Scalar::convert_to_int(sv), bool_constant<numeric_traits<Numtype>::check_range>());
         break;
      case not_a_number:
         throw std::runtime_error("invalid value for an input numerical property");
      }
   }

   std::false_type* retrieve(std::string& x) const;
   std::false_type* retrieve(AnyString &x) const;
   std::false_type* retrieve(char &x) const;
   std::false_type* retrieve(double& x) const;
   std::false_type* retrieve(bool& x) const;

   std::false_type* retrieve(int& x) const { num_input(x); return nullptr; }
   std::false_type* retrieve(unsigned int& x) const { num_input(x); return nullptr; }
   std::false_type* retrieve(long& x) const { num_input(x); return nullptr; }
   std::false_type* retrieve(unsigned long& x) const { num_input(x); return nullptr; }

   std::false_type* retrieve(float& x) const
   {
      double xi;
      retrieve(xi);
      x=static_cast<float>(xi);
      return NULL;
   }

   std::false_type* retrieve(Array& x) const;
   std::false_type* retrieve(Object& x) const;
   std::false_type* retrieve(ObjectType& x) const;
   std::false_type* retrieve(pm::Array<Object>& x) const;

   template <typename Target, typename Options>
   void do_parse(Target& x, Options) const
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
   void retrieve(Target& x, std::false_type, Serializable) const
   {
      if (options & value_not_trusted)
         ValueInput<mlist<TrustedValue<std::false_type>>>(sv) >> x;
      else
         ValueInput<>(sv) >> x;
   }

   // numeric scalar type, non-serializable
   template <typename Target>
   void retrieve(Target& x, std::true_type, std::false_type) const
   {
      num_input(x);
   }

   // numeric scalar type, serializable
   template <typename Target>
   void retrieve(Target& x, std::true_type, std::true_type) const
   {
      if (is_tuple())
         retrieve(x, std::false_type(), std::true_type());
      else
         num_input(x);
   }

   template <typename Target>
   typename std::enable_if<check_for_magic_storage<Target>::value && is_parseable<Target>::value, void>::type
   retrieve_nomagic(Target& x) const
   {
      if (is_plain_text(numeric_traits<Target>::is_specialized)) {
         parse(x);
      } else {
         retrieve(x, bool_constant<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
      }
   }

   template <typename Target>
   typename std::enable_if<check_for_magic_storage<Target>::value && !is_parseable<Target>::value, void>::type
   retrieve_nomagic(Target& x) const
   {
      retrieve(x, bool_constant<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
   }

   template <typename Target>
   typename std::enable_if<!check_for_magic_storage<Target>::value, void>::type
   retrieve_nomagic(Target& x) const
   {
      retrieve(x);
   }

   template <typename Target>
   typename std::enable_if<object_traits<Target>::is_persistent && std::is_destructible<Target>::value, bool>::type
   retrieve_with_conversion(Target& x) const
   {
      if (options & value_allow_conversion) {
         typedef Target (*conv_f)(const Value&);
         if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv))) {
            x=conversion(*this);
            return true;
         }
      }
      return false;
   }

   template <typename Target>
   typename std::enable_if<!(object_traits<Target>::is_persistent && std::is_destructible<Target>::value), bool>::type
   retrieve_with_conversion(Target&) const
   {
      return false;
   }

   template <typename Target>
   std::true_type* retrieve(Target& x) const
   {
      if (!(options & value_ignore_magic)) {
         const canned_data_t canned=get_canned_data(sv);
         if (canned.first) {
            if (*canned.first == typeid(Target)) {
               if (MaybeWary<Target>::value && (options & value_not_trusted))
                  maybe_wary(x)=*reinterpret_cast<const Target*>(canned.second);
               else
                  x=*reinterpret_cast<const Target*>(canned.second);
               return nullptr;
            }
            typedef void (*ass_f)(Target&, const Value&);
            if (ass_f assignment=reinterpret_cast<ass_f>(type_cache<Target>::get_assignment_operator(sv))) {
               assignment(x, *this);
               return nullptr;
            }
            if (retrieve_with_conversion(x))
               return nullptr;
            if (type_cache<Target>::magic_allowed())
               throw std::runtime_error("invalid assignment of " + legible_typename(*canned.first) + " to " + legible_typename<Target>());
         }
      }
      retrieve_nomagic(x);
      return nullptr;
   }

   template <typename Source>
   void store_as_perl(const Source& x)
   {
      static_cast<ValueOutput<>&>(static_cast<SVHolder&>(*this)) << x;
   }

   template <typename Stored, typename SourceRef>
   Anchor* store_canned_value(SourceRef&& x, SV* type_descr, int n_anchors)
   {
      if (type_descr) {
         auto place=allocate_canned(type_descr, n_anchors);
         new(place.first) Stored(std::forward<SourceRef>(x));
         mark_canned_as_initialized();
         return place.second;
      }
      store_as_perl(x);
      return nullptr;
   }

   template <typename Source>
   Anchor* store_canned_ref(const Source& x, SV* type_descr, int n_anchors)
   {
      if (type_descr)
         return store_canned_ref_impl((void*)&x, type_descr, options, n_anchors);
      store_as_perl(x);
      return nullptr;
   }

   // non-persistent regular type
   template <typename SourceRef, typename PerlPkg>
   Anchor* store_canned_value(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors, std::false_type, std::false_type, std::false_type)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      if (options & value_allow_non_persistent)
         return store_canned_value<Source>(std::forward<SourceRef>(x), type_cache<Source>::get_descr(prescribed_pkg), n_anchors);
      else
         return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(0), 0);
   }

   // lazy type
   template <typename SourceRef, typename PerlPkg, typename IsMasquerade, typename IsPersistent>
   Anchor* store_canned_value(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors, IsMasquerade, std::true_type, IsPersistent)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(0), 0);
   }

   // non-persistent regular type
   template <typename Source, typename PerlPkg, typename IsMasquerade>
   Anchor* store_canned_ref(const Source& x, PerlPkg prescribed_pkg, int n_anchors, IsMasquerade, std::false_type, std::false_type)
   {
      using Persistent = typename object_traits<Source>::persistent_type;
      if (options & value_allow_non_persistent)
         return store_canned_ref(x, type_cache<Source>::get_descr(prescribed_pkg), n_anchors);
      else
         return store_canned_value<Persistent>(x, type_cache<Persistent>::get_descr(0), 0);
   }

   // lazy type - never called
   template <typename Source, typename PerlPkg, typename IsMasquerade, typename IsPersistent>
   Anchor* store_canned_ref(const Source& x, PerlPkg prescribed_pkg, int n_anchors, IsMasquerade, std::true_type, IsPersistent)
   {
      return nullptr;
   }

   // persistent regular type
   template <typename SourceRef, typename PerlPkg>
   Anchor* store_canned_value(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors, std::false_type, std::false_type, std::true_type)
   {
      using Source = pure_type_t<SourceRef>;
      return store_canned_value<Source>(std::forward<SourceRef>(x), type_cache<Source>::get_descr(prescribed_pkg), n_anchors);
   }

   // persistent regular type
   template <typename Source, typename PerlPkg>
   Anchor* store_canned_ref(const Source& x, PerlPkg prescribed_pkg, int n_anchors, std::false_type, std::false_type, std::true_type)
   {
      return store_canned_ref(x, type_cache<Source>::get_descr(prescribed_pkg), n_anchors);
   }

   // masquerade type belonging to a generic family
   template <typename SourceRef, typename PerlPkg>
   Anchor* store_canned_value(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors, std::true_type, std::false_type, std::false_type)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(0), 0);
   }

   // masquerade type without persistent substitute
   template <typename Source, typename PerlPkg>
   Anchor* store_canned_value(const Source& x, PerlPkg prescribed_pkg, int n_anchors, std::true_type, std::false_type, std::true_type)
   {
      // TODO: allow storing of references to masquerade types in Object::take because they are to be converted immediately
      store_as_perl(x);
      return nullptr;
   }

   // masquerade type without persistent substitute
   template <typename Source, typename PerlPkg>
   Anchor* store_canned_ref(const Source& x, PerlPkg prescribed_pkg, int n_anchors, std::true_type, std::false_type, std::true_type)
   {
      if (options & value_allow_non_persistent) {
         return store_canned_ref(x, type_cache<Source>::get_descr(prescribed_pkg), n_anchors);
      }
      store_as_perl(x);
      return nullptr;
   }

   Anchor* store_primitive_ref(const bool& x,          SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const int& x,           SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const unsigned int& x,  SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const long& x,          SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const unsigned long& x, SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const double& x,        SV* type_descr, int n_anchors, bool take_ref);
   Anchor* store_primitive_ref(const std::string& x,   SV* type_descr, int n_anchors, bool take_ref);

   void set_string_value(const char* x);
   void set_string_value(const char* x, size_t l);
   void set_copy(const SVHolder& x);

   NoAnchors put_val(int x, int=0, int=0) { return put_val(static_cast<long>(x)); }
   NoAnchors put_val(unsigned int x, int=0, int=0) { return put_val(static_cast<unsigned long>(x)); }
   NoAnchors put_val(long x, int=0, int=0);
   NoAnchors put_val(unsigned long x, int=0, int=0);
   NoAnchors put_val(bool x, int=0, int=0);
   NoAnchors put_val(double x, int=0, int=0);
   NoAnchors put_val(const undefined&, int=0, int=0);

   NoAnchors put_val(const AnyString& x, int=0, int=0)
   {
      if (x)
         set_string_value(x.ptr, x.len);
      else
         put_val(undefined());
      return NoAnchors();
   }

   // need this one separately because otherwise the vile compiler coerces the string array to a boolean
   template <size_t n>
   NoAnchors put_val(const char (&x)[n], int=0, int=0)
   {
      set_string_value(x+0, n-1);
      return NoAnchors();
   }

   NoAnchors put_val(char x, int=0, int=0)
   {
      set_string_value(&x, 1);
      return NoAnchors();
   }

   NoAnchors put_val(const Object& x,            int=0, int=0);
   NoAnchors put_val(const ObjectType& x,        int=0, int=0);
   NoAnchors put_val(const PropertyValue& x,     int=0, int=0);
   NoAnchors put_val(const Scalar& x,            int=0, int=0);
   NoAnchors put_val(const Array& x,             int=0, int=0);
   NoAnchors put_val(const Hash& x,              int=0, int=0);
   NoAnchors put_val(const ListReturn& x,        int=0, int=0);
   NoAnchors put_val(const pm::Array<Object>& x, int=0, int=0);

   typedef mlist<undefined, AnyString, std::string, Object, ObjectType, PropertyValue, Scalar, Array, Hash, ListReturn, pm::Array<Object>> nomagic_types;
   typedef mlist<Scalar, Array, Hash> nomagic_lvalue_types;

   template <typename Source, typename PerlPkg>
   typename std::enable_if<obscure_type<Source>::value, Anchor*>::type
   put_val(const Source& x, PerlPkg prescribed_pkg, int n_anchors)
   {
      // obscure type (something weird, like a set complement)
      if (SV* type_descr=type_cache<Source>::get_descr(prescribed_pkg)) {
         if ((options & (value_allow_non_persistent | value_allow_store_ref)) ==
             (value_allow_non_persistent | value_allow_store_ref)) {
            return store_canned_ref_impl((void*)&x, type_descr, options | value_read_only, n_anchors);
         } else {
            throw std::invalid_argument("can't store a copy of an obscure C++ object");
         }
      } else {
         throw std::invalid_argument("can't store an obscure C++ type without perl binding");
      }
   }

   template <typename SourceRef, typename PerlPkg>
   typename std::enable_if<is_class_or_union<pure_type_t<SourceRef>>::value &&
                           !(is_derived_from_any<pure_type_t<SourceRef>, nomagic_types>::value ||
                             obscure_type<pure_type_t<SourceRef>>::value ||
                             is_derived_from_instance_of<pure_type_t<SourceRef>, MaybeUndefined>::value ||
                             !std::is_same<typename object_traits<pure_type_t<SourceRef>>::proxy_for, void>::value),
                           Anchor*>::type
   put_val(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      if (object_traits<Source>::is_lazy ||
          !(options & (std::is_rvalue_reference<SourceRef&&>::value ? value_allow_store_temp_ref : value_allow_store_ref))) {
         // must store a copy
         return store_canned_value(std::forward<SourceRef>(x), prescribed_pkg, n_anchors,
                                   is_masquerade<Source>(), bool_constant<object_traits<Source>::is_lazy>(), std::is_same<Source, Persistent>());
      } else {
         // can store a reference
         return store_canned_ref(x, prescribed_pkg, n_anchors,
                                 is_masquerade<Source>(), bool_constant<object_traits<Source>::is_lazy>(), std::is_same<Source, Persistent>());
      }
   }

   template <typename Source>
   Anchor* put_val(std::unique_ptr<Source>&& ptr, int, int n_anchors)
   {
      if (SV* type_descr=type_cache<std::unique_ptr<Source>>::get_descr(0)) {
         if (options & value_allow_non_persistent) {
            return store_canned_value<std::unique_ptr<Source>>(std::move(ptr), type_descr, n_anchors);
         } else {
            throw std::invalid_argument("can't store a pointer to an opaque C++ object");
         }
      } else {
         throw std::invalid_argument("can't store an opaque C++ type without perl binding");
      }
   }

   // currently only helpers for associative containers, see assoc.h
   template <typename Source, typename PerlPkg>
   Anchor* put_val(const MaybeUndefined<Source>& x, PerlPkg prescribed_pkg, int n_anchors)
   {
      if (x.top().defined()) {
         return put_val(x.top().get_val(), prescribed_pkg, n_anchors);
      } else {
         put_val(undefined());
         return nullptr;
      }
   }

   template <typename SourceRef, typename PerlPkg>
   typename std::enable_if<!std::is_same<typename object_traits<pure_type_t<SourceRef>>::proxy_for, void>::value, Anchor*>::type
   put_val(SourceRef&& x, PerlPkg prescribed_pkg, int n_anchors)
   {
      SV* type_descr;
      using Source = pure_type_t<SourceRef>;
      if ((options & (value_allow_non_persistent | value_expect_lval | value_read_only)) == 
                     (value_allow_non_persistent | value_expect_lval) &&
          (type_descr=type_cache<Source>::get_descr(prescribed_pkg))) {
         return store_canned_value<Source>(std::move(x), type_descr, n_anchors);
      } else {
         return put_val(static_cast<const type_behind_t<Source>&>(x), prescribed_pkg, 0);
      }
   }

public:
   template <typename SourceRef, typename PerlPkg, typename... AnchorList>
   void put(SourceRef&& x, PerlPkg prescribed_pkg, AnchorList&&... anchors)
   {
      Anchor* anchor_place{ put_val(std::forward<SourceRef>(x), prescribed_pkg, sizeof...(AnchorList)) };
      if (sizeof...(AnchorList) && anchor_place)
         store_anchors(anchor_place, std::forward<AnchorList>(anchors)...);
   }

   template <typename SourceRef, typename PerlPkg, typename OwnerType, typename... AnchorList>
   typename std::enable_if<!mlist_contains<mlist_concat<primitive_lvalues, nomagic_lvalue_types>::type, pure_type_t<SourceRef>>::value, void>::type
   put_lvalue(SourceRef&& x, PerlPkg prescribed_pkg, const Value* owner, OwnerType*, AnchorList&&... anchors)
   {
      typedef pure_type_t<SourceRef> Source;
      if (std::is_same<Source, typename access<OwnerType>::value_type>::value &&
          std::is_lvalue_reference<SourceRef&&>::value &&
          reinterpret_cast<const Source*>(owner->get_canned_value()) == &x) {
         forget();
         sv=owner->sv;
      } else {
         put(std::forward<SourceRef>(x), prescribed_pkg, std::forward<AnchorList>(anchors)...);
         if (owner) get_temp();
      }
   }

   template <typename SourceRef, typename... AnchorList>
   typename std::enable_if<mlist_contains<primitive_lvalues, pure_type_t<SourceRef>>::value, void>::type
   put_lvalue(SourceRef&& x, int, const Value* owner, void*, AnchorList&&... anchors)
   {
      typedef pure_type_t<SourceRef> Source;
      Anchor* anchor_place=store_primitive_ref(x, type_cache<Source>::get_descr(0), sizeof...(AnchorList), std::is_lvalue_reference<SourceRef&&>::value);
      if (sizeof...(AnchorList) && anchor_place)
         store_anchors(anchor_place, std::forward<AnchorList>(anchors)...);
      if (owner) get_temp();
   }

   template <typename SourceRef, typename... AnchorList>
   typename std::enable_if<mlist_contains<nomagic_lvalue_types, pure_type_t<SourceRef>>::value, void>::type
   put_lvalue(SourceRef&& x, int, const Value*, void*, AnchorList&&... anchors)
   {
      forget();
      sv=x.get();
   }

   template <typename Target>
   void parse(Target& x) const
   {
      if (options & value_not_trusted)
         do_parse(x, mlist<TrustedValue<std::false_type>>());
      else
         do_parse(x, mlist<>());
   }

   // some code duplication with generic retrieve() is deliberate
   template <typename Target>
   operator Target () const
   {
      if (check_for_magic_storage<Target>::value) {
         if (!sv || !is_defined()) {
            if (!(options & value_allow_undef))
               throw undefined();
            return Target{};
         }
         if (!(options & value_ignore_magic)) {
            const canned_data_t canned=get_canned_data(sv);
            if (canned.first) {
               if (*canned.first == typeid(Target))
                  return reinterpret_cast<const Target&>(*canned.second);
               typedef Target (*conv_f)(const Value&);
               if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv)))
                  return conversion(*this);
               if (type_cache<Target>::magic_allowed())
                  throw std::runtime_error("invalid conversion from " + legible_typename(*canned.first) + " to " + legible_typename<Target>());
            }
         }
         Target x{};
         retrieve_nomagic(x);
         return x;
      } else {
         Target x{};
         *this >> x;
         return x;
      }
   }

   explicit operator bool () const { return is_TRUE(); }
   bool operator! () const { return !is_TRUE(); }

   template <typename Target>
   friend
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

   template <typename Source>
   friend
   void operator<< (const Value& me, Source&& x)
   {
      const_cast<Value&>(me).put(std::forward<Source>(x), 0);
   }

   template <typename Target>
   void* allocate(SV* proto)
   {
      return allocate_canned(type_cache<Target>::get_descr_for_proto(proto));
   }

   SV* get_constructed_canned();

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
            d=PlainParser<mlist<TrustedValue<std::false_type>>>(my_stream).begin_list((T*)0).lookup_dim(tell_size_if_dense);
         else
            d=PlainParser<>(my_stream).begin_list((T*)0).lookup_dim(tell_size_if_dense);

      } else if (get_canned_typeinfo()) {
         d=get_canned_dim(tell_size_if_dense);

      } else {
         if (options & value_not_trusted)
            d=ListValueInput<T, mlist<TrustedValue<std::false_type>>>(sv).lookup_dim(tell_size_if_dense);
         else
            d=ListValueInput<T>(sv).lookup_dim(tell_size_if_dense);
      }
      return d;
   }

   template <typename, typename> friend class access;
   template <typename, typename, bool, bool> friend class access_canned;
   template <typename> friend struct check_for_magic_storage;
   friend class ArrayHolder;
   friend class OptionSet;
   friend class ListResult;
   template <typename> friend class ValueOutput;
   template <typename, typename> friend class ListValueInput;
};

template <bool is_readonly>
class Value::Array_element_factory {
public:
   typedef int argument_type;
   typedef Value result_type;

   explicit Array_element_factory(const ArrayHolder* array_arg=nullptr)
      : array(array_arg) {}

   result_type operator() (int i) const
   {
      return result_type((*array)[i], (is_readonly ? value_read_only : value_mutable) | value_not_trusted);
   }

protected:
   const ArrayHolder* array;
};

}

template <bool is_readonly>
struct operation_cross_const_helper<perl::Value::Array_element_factory<is_readonly>> {
   typedef perl::Value::Array_element_factory<false> operation;
   typedef perl::Value::Array_element_factory<true> const_operation;
};

namespace perl {

SV* make_string_array(int size, ...);

inline ArrayHolder::ArrayHolder(const Value& v)
   : SVHolder(v.sv)
{
   if (v.options & value_not_trusted) verify();
}

template <typename Target>
struct check_for_magic_storage {
   struct helper {
      static derivation::yes Test(std::true_type*);
      static derivation::no Test(std::false_type*);
      static Target& piece();
   };
   static const bool value= sizeof(helper::Test(Value().retrieve(helper::piece()))) == sizeof(derivation::yes);
};

template <typename ElementType, typename Options>
template <typename T> inline
ListValueInput<ElementType, Options>&
ListValueInput<ElementType, Options>::operator>> (T& x)
{
   if (this->get_option(CheckEOF<std::false_type>()) && at_end())
      throw std::runtime_error("list input - size mismatch");
   Value elem((*this)[i++], this->get_option(TrustedValue<std::true_type>()) ? value_trusted : value_not_trusted);
   elem >> x;
   return *this;
}

template <typename ElementType, typename Options>
inline
int ListValueInput<ElementType, Options>::cols(bool tell_size_if_dense)
{
   const int c=ArrayHolder::cols();
   if (c>=0) return c;
   if (_size==0) return tell_size_if_dense-1;
   Value first_elem((*this)[0], this->get_option(TrustedValue<std::true_type>()) ? value_trusted : value_not_trusted);
   return first_elem.lookup_dim<ElementType>(tell_size_if_dense);
}

template <typename Options> inline
void ValueOutput<Options>::store_string(const char* x, size_t l, std::false_type)
{
   static_cast<Value*>(static_cast<super*>(this))->set_string_value(x,l);
}

template <typename Options> inline
void ValueOutput<Options>::store_string(const char* x, size_t l, std::true_type)
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

template <typename Element>
class ArrayOwner
   : public ArrayHolder
   , public modified_container_impl< ArrayOwner<Element>,
                                     mlist< ContainerTag< sequence >,
                                            OperationTag< typename Element::template Array_element_factory<false> > > > {
   friend class Value;
protected:
   explicit ArrayOwner(SV* sv_arg, value_flags flags=value_trusted)
      : ArrayHolder(sv_arg, flags) {}

   explicit ArrayOwner(const Value& v)
      : ArrayHolder(v) {}
public:
   ArrayOwner() {}
   explicit ArrayOwner(int n) { resize(n); }

   ArrayOwner(const ArrayOwner& x)
      : ArrayHolder(x.get(), std::true_type()) {}

   ArrayOwner& operator= (const ArrayOwner& x)
   {
      set_copy(x.get());
      return *this;
   }

   ArrayOwner(ArrayOwner&& x) noexcept
      : ArrayHolder(x.sv)
   {
      x.sv=nullptr;
   }

   ArrayOwner& operator= (ArrayOwner&& x) noexcept
   {
      forget();
      sv=x.sv;
      x.sv=nullptr;
      return *this;
   }

   ~ArrayOwner() noexcept { forget(); }

   using ArrayHolder::size;
   using modified_container_impl<ArrayOwner>::operator[];

   bool empty() const
   {
      return size()==0;
   }

   sequence get_container() const
   {
      return sequence(0, size());
   }
   typename Element::template Array_element_factory<false> get_operation()
   {
      return typename Element::template Array_element_factory<false>(this);
   }
   typename Element::template Array_element_factory<true> get_operation() const
   {
      return typename Element::template Array_element_factory<true>(this);
   }

   void clear() { resize(0); }
};

class OptionSet
   : public HashHolder {
public:
   OptionSet()
      : HashHolder() { this->get_temp(); }
   
   OptionSet(const Value& v)
      : HashHolder(v.sv)
   {
      verify();
   }

   // create an option set on the fly, to be passed to a function or user method
   template <typename FirstVal, typename... MoreArgs>
   OptionSet(const AnyString& first_key, FirstVal&& first_val, MoreArgs&&... more_args)
      : OptionSet()
   {
      store_values(first_key, std::forward<FirstVal>(first_val), std::forward<MoreArgs>(more_args)...);
   }

public:
   Value operator[] (const AnyString& key) const
   {
      return Value(fetch(key, false), value_not_trusted | value_allow_undef);
   }

   Value operator[] (const AnyString& key)
   {
      return Value(fetch(key, true), value_not_trusted | value_allow_undef | value_allow_non_persistent);
   }

protected:
   void store_values() {}

   template <typename FirstVal, typename... MoreArgs>
   void store_values(const AnyString& first_key, FirstVal&& first_val, MoreArgs&&... more_args)
   {
      Value v(fetch(first_key, true), value_allow_non_persistent | value_allow_store_any_ref);
      v.put(std::forward<FirstVal>(first_val), 0);
      store_values(std::forward<MoreArgs>(more_args)...);
   }
};

class Hash : public HashHolder {
public:
   Hash() {}
   Hash(const Hash& x)
      : HashHolder(x.get(), std::true_type()) {}

   Hash& operator= (const Hash& x)
   {
      set_copy(x.get());
      return *this;
   }

   ~Hash() { forget(); }

   Value operator[] (const AnyString& key) const
   {
      return Value(fetch(key, false), value_not_trusted | value_allow_undef);
   }
   Value operator[] (const AnyString& key)
   {
      return Value(fetch(key, true), value_not_trusted | value_allow_undef | value_allow_non_persistent);
   }
};

inline Value::NoAnchors Value::put_val(const Scalar& x,      int, int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const Array& x,       int, int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const Hash& x,        int, int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const ListReturn& x,  int, int) { forget(); sv=nullptr; return NoAnchors(); }

} }

#endif // POLYMAKE_PERL_VALUE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
