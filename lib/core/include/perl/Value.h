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

#include "polymake/GenericIO.h"
#include "polymake/internal/assoc.h"
#include "polymake/internal/CharBuffer.h"
#include "polymake/internal/Wary.h"
#include "polymake/internal/Array.h"
#include "polymake/optional"

#include <limits>
#include <cmath>
#include <string>
#include <typeinfo>
#include <memory>

// TODO: eliminate all references to SV outside the perl-aware code, use opaque ScalarHolder<ownership> instead

#ifndef POLYMAKE_WITHIN_PERL
struct sv;
#define SV ::sv
#endif

namespace pm { namespace perl {

class Undefined : public std::runtime_error {
public:
   Undefined();
};

class exception : public std::runtime_error {
public:
   exception();
   exception(const char* msg) : std::runtime_error(msg) {}
};

class SVHolder {
protected:
   SV* sv;

   SVHolder();

   explicit SVHolder(SV* sv_arg) noexcept
      : sv(sv_arg) {}

   SVHolder(SV* sv_arg, std::true_type);
   void set_copy(SV* sv_arg);
   void forget();
public:
   SV* get() const noexcept { return sv; }
   SV* get_temp();
   bool is_tuple() const;
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
      return const_string(s, strlen(s));
   }
   static SV* const_string(const AnyString& s)
   {
      return const_string(s.ptr, s.len);
   }

   static SV* const_string_with_int(const char* s, size_t l, int i);
   static SV* const_string_with_int(const char* s, int i)
   {
      return const_string_with_int(s, strlen(s), i);
   }
   static SV* const_string_with_int(const AnyString& s, int i)
   {
      return const_string_with_int(s.ptr, s.len, i);
   }

   static SV* const_int(int i);

   static Int convert_to_Int(SV* sv);
   static double convert_to_Float(SV* sv);
};

// forward declarations of mutual friends
class Value; class BigObject; class BigObjectType; class PropertyValue;
class Hash;  class OptionSet;  class ListResult;
class Stack;  class Main;

template <typename Element=Value> class ArrayOwner;
using Array = ArrayOwner<>;

template <typename T> class type_cache;

class ArrayHolder : public SVHolder {
   static SV* init_me(Int size);
protected:
   ArrayHolder(SV* sv_arg, std::true_type)
      : SVHolder(sv_arg, std::true_type()) {}
public:
   explicit ArrayHolder(Int reserve = 0)
      : SVHolder(init_me(reserve)) {}

   explicit ArrayHolder(const Value&);

   ArrayHolder(const SVHolder& x, Int reserve)
      : SVHolder(x)
   {
      upgrade(reserve);
   }

   explicit ArrayHolder(SV* sv_arg, ValueFlags flags = ValueFlags::is_trusted)
      : SVHolder(sv_arg)
   {
      if (flags * ValueFlags::not_trusted)
         verify();
      else if (flags * ValueFlags::allow_undef)
         upgrade(0);
   }

   void upgrade(Int size);
   void verify() const;
   void set_contains_aliases();

   Int size() const;
   void resize(Int n);
   SV* operator[] (Int i) const;

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

class istreambuf : public streambuf_ext {
public:
   istreambuf(SV* sv);

   size_type lines() override
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
      if (good() && CharBuffer::next_non_ws(&my_buf) >= 0)
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

template <typename X> struct TryCanned;
template <typename X> struct Canned;
template <typename X> struct Enum;
template <typename X> struct ReturningList;

template <typename Options=mlist<>> class ValueOutput;
template <typename Options=mlist<>> class ValueInput;

class ListValueInputBase {
protected:
   SV* arr_or_hash;
   SV* dim_sv;
   Int i;
   Int size_;
   Int cols_;
   Int dim_;
   bool sparse_;

   explicit ListValueInputBase(SV* sv);
   ~ListValueInputBase() { finish(); }
public:
   Int size() const { return size_; }

   void skip_item() { ++i; }
   void skip_rest() { i = size_; }
   bool at_end() const { return i >= size_; }

   Int get_dim(bool tell_size_if_dense) const
   {
      return dim_ >= 0 ? dim_ : tell_size_if_dense ? size_ : -1;
   }

   bool sparse_representation() const { return sparse_; }

   bool is_ordered() const;

protected:
   SV* get_first() const;
   SV* get_next();
   Int get_index() const;
   void retrieve_key(std::string& dst) const;
   void finish();
};

template <typename ElementType, typename Options=mlist<>>
class ListValueInput
   : public ListValueInputBase
   , public GenericInputImpl< ListValueInput<ElementType, Options> >
   , public GenericIOoptions< ListValueInput<ElementType, Options>, Options, 1 > {
public:
   using value_type = ElementType;

   explicit ListValueInput(SV* sv)
      : ListValueInputBase(sv)
   {}

   template <typename T>
   ListValueInput& operator>> (T& x)
   {
      if (this->get_option(CheckEOF<std::false_type>()) && at_end())
         throw std::runtime_error("list input - size mismatch");
      retrieve(x, is_instance_of<ElementType, pair>());
      return *this;
   }

   Int cols(bool tell_size_if_dense);

   Int index(const Int index_bound) const
   {
      const Int ix = get_index();
      if (!this->get_option(TrustedValue<std::true_type>()) && (ix < 0 || ix >= index_bound))
         throw std::runtime_error("sparse input - index out of range");
      return ix;
   }

   void finish()
   {
      ListValueInputBase::finish();
      if (this->get_option(CheckEOF<std::false_type>()) && !at_end())
         throw std::runtime_error("list input - size mismatch");
   }

   template <typename X>
   ListValueInput& set_option(SparseRepresentation<X>) { return *this; }

   using GenericIOoptions<ListValueInput, Options, 1>::set_option;
private:
   template <typename T, bool anything>
   void retrieve(T&, bool_constant<anything>);

   template <typename T>
   void retrieve(std::pair<Int, T>& x, std::true_type)
   {
      if (!sparse_representation()) {
         retrieve(x, std::false_type());
      } else {
         // Maps with integer keys can be serialized as sparse containers
         x.first = get_index();
         retrieve(x.second, std::false_type());
      }
   }

   template <typename T>
   void retrieve(std::pair<std::string, T>& x, std::true_type)
   {
      if (is_ordered()) {
         retrieve(x, std::false_type());
      } else {
         retrieve_key(x.first);
         retrieve(x.second, std::false_type());
      }
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

   using SVHolder::is_tuple;

   template <typename ObjectRef>
   struct list_cursor {
      using cursor_options = typename mtagged_list_remove<Options, SparseRepresentation>::type;
      using type = ListValueInput<typename deref<ObjectRef>::type::value_type, cursor_options>;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      using cursor_options = typename mtagged_list_replace<
                typename mtagged_list_remove<Options, SparseRepresentation>::type,
                CheckEOF<std::true_type>>::type;
      using type = ListValueInput<void, cursor_options>;
   };

   template <typename T>
   decltype(auto) begin_list(T*)
   {
      return typename list_cursor<T>::type(sv);
   }

   template <typename T>
   decltype(auto) begin_composite(T*)
   {
      return typename composite_cursor<T>::type(sv);
   }
};

template <typename Options, bool returning_list = tagged_list_extract_integral<Options, ReturningList>(false)>
class ListValueOutput
   : public ArrayHolder
   , public GenericIOoptions< ListValueOutput<Options>, Options > {
   ListValueOutput();
public:
   using super = SVHolder;
   using super_arg = SV*;
   static const bool stack_based = false;

   template <typename T>
   ListValueOutput& operator<< (T&& x);

   ListValueOutput& non_existent()
   {
      return *this << Undefined();
   }

   void finish() const {}
};

template <typename Options>
class ValueOutput
   : public ListValueOutput<Options>::super
   , public GenericOutputImpl< ValueOutput<Options> >
   , public GenericIOoptions< ValueOutput<Options>, Options > {

   using super = typename ListValueOutput<Options>::super;
   using super_arg = typename ListValueOutput<Options>::super_arg;
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

   // TODO: investigate and remove
   bool is_tuple() const { return false; }

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
      using type = ListValueOutput<Options>&;
   };

   template <typename ObjectRef>
   struct composite_cursor {
      using Object = typename deref<ObjectRef>::type;
      static constexpr int
         total   = list_length<typename object_traits<Object>::elements>::value,
         ignored = list_accumulate_unary<list_count, ignore_in_composite, typename object_traits<Object>::elements>::value;
      static constexpr bool
         compress = ignored > 0 && total - ignored <= 1;
      using type = std::conditional_t<compress, ValueOutput, ListValueOutput<Options>>&;
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
      return begin_composite_impl((const T*)nullptr, bool_constant<composite_cursor<T>::compress>());
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
   Stack();
   Stack(SV** start);
   explicit Stack(Int reserve);
private:
   Stack(const Stack&) = delete;
   void operator= (const Stack&) = delete;

protected:
   void xpush(SV* x) const;
   void push(SV* x) const;
   void push(const AnyString& s) const;
   void extend(Int n);

public:
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

   explicit ListReturn(SV** stack_arg)
      : Stack(stack_arg) {}

   ListReturn(ListReturn&&) = default;

   template <typename T>
   ListReturn& operator<< (T&& x)
   {
      upgrade(1);
      store(std::forward<T>(x));
      return *this;
   }

   template <typename... T>
   ListReturn& operator<< (std::tuple<T...>&& t)
   {
      store(std::move(t), typename index_sequence_for<pure_type_t<decltype(t)>>::type());
      return *this;
   }

   void upgrade(Int size);

private:
   template <typename T>
   void store(T&& x);

   template <typename T, int... Indexes>
   void store(T&& t, std::index_sequence<Indexes...>)
   {
      upgrade(sizeof...(Indexes));
      (void)std::initializer_list<bool>{ (store(std::get<Indexes>(std::forward<T>(t))), true)... };
   }
};

template <typename Options>
class ListValueOutput<Options, true>
   : public ListReturn
   , public GenericIOoptions< ListValueOutput<Options, true>, Options > {
   ListValueOutput();
public:
   using super = Stack;
   using super_arg = SV**;
   static const bool stack_based=true;

   void finish() const {}
};

using ListSlurp = ValueOutput<mlist<ReturningList<std::true_type>>>;

} // end namespace perl

template <>
struct is_printable<perl::Value> : std::false_type {};
template <>
struct is_parseable<perl::Value> : std::false_type {};
template <>
struct is_printable<perl::BigObject> : std::false_type {};
template <>
struct is_writeable<perl::BigObject> : std::true_type {};

// forward declaration of a specialization
template <>
class Array<perl::BigObject>;

template <>
struct is_printable< Array<perl::BigObject> > : std::false_type {};
template <>
struct is_writeable< Array<perl::BigObject> > : std::true_type {};

namespace perl {

template <typename T,
          bool _has_generic=has_generic_type<T>::value>
struct generic_representative {
   using type = typename object_traits<T>::persistent_type;
};

template <typename T>
struct generic_representative<T, true> {
   using type = typename T::generic_type::persistent_type;
};

// primitive perl types which need special handling if returned by reference in lvalue context
using primitive_lvalues = mlist<bool, Int, double, std::string>;

template <typename T>
struct is_optional_value
   : is_instance_of<T, optional> {};

template <typename T>
struct numeric_traits : std::numeric_limits<type_behind_t<T>> {
   using real_type = type_behind_t<T>;
   static constexpr bool check_range = std::numeric_limits<real_type>::is_bounded && std::numeric_limits<real_type>::is_integer;
};

template <typename Target> class access;
template <typename Target> struct represents_BigObject : std::false_type {};

class Value
   : public SVHolder {
public:
   explicit Value(ValueFlags opt_arg=ValueFlags::is_trusted)
      : options(opt_arg)
   {}

   explicit Value(SV* sv_arg, ValueFlags opt_arg=ValueFlags::is_trusted) noexcept
      : SVHolder(sv_arg)
      , options(opt_arg)
   {}

   ValueFlags get_flags() const noexcept { return options; }

   struct Anchor {
      void store(SV* sv) noexcept;
      void store(const Value& v) noexcept { store(v.get()); }

      SV* stored;
   };
   struct NoAnchors {
      constexpr operator Anchor* () const { return nullptr; }
   };

   template <bool is_readonly> class Array_element_factory;
   template <typename Target> struct check_for_magic_storage;
   using nomagic_types = mlist<Undefined, AnyString, std::string, BigObject, BigObjectType, PropertyValue, Scalar, Array, Hash, ListReturn, pm::Array<BigObject>>;
   using nomagic_lvalue_types = mlist<Scalar, Array, Hash>;

protected:
   enum number_flags { not_a_number, number_is_zero, number_is_int, number_is_float, number_is_object };

   ValueFlags options;

   bool is_defined() const noexcept;
   bool is_TRUE() const;
   Int Int_value() const;
   Int enum_value(size_t s, bool expect_ref) const;
   double Float_value() const;
   bool is_plain_text(bool expect_numeric_scalar=false) const;

   struct canned_data_t {
      const std::type_info* ti;
      char* value;
      bool read_only;
   };

   static
   canned_data_t get_canned_data(SV*) noexcept;

   const std::type_info* get_canned_typeinfo() const noexcept { return get_canned_data(sv).ti; }
   char* get_canned_value() const noexcept { return get_canned_data(sv).value; }

   Int get_canned_dim(bool tell_size_if_dense) const;

   template <typename... AnchorList>
   static void store_anchors(Anchor* place, AnchorList&&... anchors) noexcept
   {
      (void)std::initializer_list<bool>{ ((place++)->store(anchors), true)... };
   }

   std::pair<void*, Anchor*> allocate_canned(SV* proto, int n_anchors) const;

   void* allocate_canned(SV* proto) const { return allocate_canned(proto, 0).first; }

   void mark_canned_as_initialized();

   Anchor* store_canned_ref_impl(void* obj, SV* type_descr, ValueFlags flags, int n_anchors) const;

   template <typename Numtype>
   static
   void assign_Int(Numtype& x, Int i, std::false_type) { x = i; }

   static
   void assign_Int(long& x, Int i, std::true_type) { x = i; }

   template <typename Numtype>
   static
   void assign_Int(Numtype& x, Int i, std::true_type)
   {
      if (i < min_value_as<Int>(mlist<Numtype>()) ||
          i > max_value_as<Int>(mlist<Numtype>()))
         throw std::runtime_error("input numeric property out of range");
      x = static_cast<typename numeric_traits<Numtype>::real_type>(i);
   }

   template <typename Numtype>
   static
   void assign_Float(Numtype& x, double d, std::false_type) { x=d; }

   template <typename Numtype>
   static
   void assign_Float(Numtype& x, double d, std::true_type)
   {
      if (d < min_value_as<double>(mlist<Numtype>()) ||
          d > max_value_as<double>(mlist<Numtype>()))
         throw std::runtime_error("input numeric property out of range");
      x = static_cast<typename numeric_traits<Numtype>::real_type>(lrint(d));
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
         assign_Int(x, Int_value(), bool_constant<numeric_traits<Numtype>::check_range>());
         break;
      case number_is_float:
         assign_Float(x, Float_value(), bool_constant<numeric_traits<Numtype>::check_range>());
         break;
      case number_is_object:
         assign_Int(x, Scalar::convert_to_Int(sv), bool_constant<numeric_traits<Numtype>::check_range>());
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

   std::false_type* retrieve(Int& x) const { num_input(x); return nullptr; }

   template <typename T>
   std::enable_if_t<std::is_enum<T>::value, std::false_type*>
   retrieve(T& x) const
   {
      const Int val = enum_value(sizeof(T), false);
      x = static_cast<T>(val);
      return nullptr;
   }

   std::false_type* retrieve(float& x) const
   {
      double xi;
      retrieve(xi);
      x=static_cast<float>(xi);
      return nullptr;
   }

   std::false_type* retrieve(Array& x) const;
   std::false_type* retrieve(BigObject& x) const;
   std::false_type* retrieve(BigObjectType& x) const;
   std::false_type* retrieve(pm::Array<BigObject>& x) const;

   template <typename Target>
   std::enable_if_t<represents_BigObject<Target>::value, std::false_type*>
   retrieve(Target& x) const;

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
      if (options * ValueFlags::not_trusted)
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
   std::enable_if_t<check_for_magic_storage<Target>::value && is_parseable<Target>::value>
   retrieve_nomagic(Target& x) const
   {
      if (is_plain_text(numeric_traits<Target>::is_specialized)) {
         parse(x);
      } else {
         retrieve(x, bool_constant<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
      }
   }

   template <typename Target>
   std::enable_if_t<check_for_magic_storage<Target>::value && !is_parseable<Target>::value>
   retrieve_nomagic(Target& x) const
   {
      retrieve(x, bool_constant<numeric_traits<Target>::is_specialized>(), has_serialized<Target>());
   }

   template <typename Target>
   std::enable_if_t<!check_for_magic_storage<Target>::value>
   retrieve_nomagic(Target& x) const
   {
      retrieve(x);
   }

   template <typename Target>
   std::enable_if_t<object_traits<Target>::is_persistent && std::is_destructible<Target>::value, bool>
   retrieve_with_conversion(Target& x) const
   {
      if (options * ValueFlags::allow_conversion) {
         using conv_f = Target (*)(const Value&);
         if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv))) {
            x=conversion(*this);
            return true;
         }
      }
      return false;
   }

   template <typename Target>
   std::enable_if_t<!(object_traits<Target>::is_persistent && std::is_destructible<Target>::value), bool>
   retrieve_with_conversion(Target&) const
   {
      return false;
   }

   template <typename Target>
   std::enable_if_t<std::is_copy_assignable<Target>::value &&
                    !(represents_BigObject<Target>::value || std::is_enum<Target>::value), std::true_type*>
   retrieve(Target& x) const
   {
      if (!(options * ValueFlags::ignore_magic)) {
         const canned_data_t canned = get_canned_data(sv);
         if (canned.ti) {
            if (*canned.ti == typeid(Target)) {
               if (MaybeWary<Target>::value && options * ValueFlags::not_trusted)
                  maybe_wary(x)=*reinterpret_cast<const Target*>(canned.value);
               else
                  x=*reinterpret_cast<const Target*>(canned.value);
               return nullptr;
            }
            using ass_f = void (*)(Target&, const Value&);
            if (ass_f assignment=reinterpret_cast<ass_f>(type_cache<Target>::get_assignment_operator(sv))) {
               assignment(x, *this);
               return nullptr;
            }
            if (retrieve_with_conversion(x))
               return nullptr;
            if (type_cache<Target>::magic_allowed())
               throw std::runtime_error("invalid assignment of " + legible_typename(*canned.ti) + " to " + legible_typename<Target>());
         }
      }
      retrieve_nomagic(x);
      return nullptr;
   }

   template <typename Target>
   std::enable_if_t<!std::is_copy_assignable<Target>::value, std::true_type*>
   retrieve(Target& x) const
   {
      const canned_data_t canned = get_canned_data(sv);
      if (!(canned.ti && *canned.ti == typeid(Target) && !canned.read_only))
         throw Undefined();
      x = std::move(*reinterpret_cast<Target*>(canned.value));
   }

   template <typename Target>
   Target retrieve_copy(std::enable_if_t<mlist_contains<nomagic_types, Target>::value || !check_for_magic_storage<Target>::value, std::nullptr_t> = nullptr) const
   {
      Target x{};
      if (__builtin_expect(sv && is_defined(), 1)) {
         retrieve_nomagic(x);
      } else if (!(options * ValueFlags::allow_undef)) {
         throw Undefined();
      }
      return x;
   }

   // some code duplication with generic retrieve() is deliberate
   template <typename Target>
   Target retrieve_copy(std::enable_if_t<std::is_copy_constructible<Target>::value &&
                                         !mlist_contains<nomagic_types, Target>::value &&
                                         check_for_magic_storage<Target>::value, std::nullptr_t> = nullptr) const
   {
      if (__builtin_expect(!sv || !is_defined(), 0)) {
         if (!(options * ValueFlags::allow_undef))
            throw Undefined();
         return Target{};
      }
      if (!(options * ValueFlags::ignore_magic)) {
         const canned_data_t canned = get_canned_data(sv);
         if (canned.ti) {
            if (*canned.ti == typeid(Target))
               return *reinterpret_cast<const Target*>(canned.value);
            using conv_f = Target (*)(const Value&);
            if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv)))
               return conversion(*this);
            if (type_cache<Target>::magic_allowed())
               throw std::runtime_error("invalid conversion from " + legible_typename(*canned.ti) + " to " + legible_typename<Target>());
         }
      }
      Target x{};
      retrieve_nomagic(x);
      return x;
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
   template <typename SourceRef>
   Anchor* store_canned_value(SourceRef&& x, int n_anchors, std::false_type, std::false_type, std::false_type)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      if (options * ValueFlags::allow_non_persistent)
         return store_canned_value<Source>(std::forward<SourceRef>(x), type_cache<Source>::get_descr(), n_anchors);
      else
         return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(), 0);
   }

   // lazy type
   template <typename SourceRef, typename IsMasquerade, typename IsPersistent>
   Anchor* store_canned_value(SourceRef&& x, int n_anchors, IsMasquerade, std::true_type, IsPersistent)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(), 0);
   }

   // non-persistent regular type
   template <typename Source, typename IsMasquerade>
   Anchor* store_canned_ref(const Source& x, int n_anchors, IsMasquerade, std::false_type, std::false_type)
   {
      using Persistent = typename object_traits<Source>::persistent_type;
      if (options * ValueFlags::allow_non_persistent)
         return store_canned_ref(x, type_cache<Source>::get_descr(), n_anchors);
      else
         return store_canned_value<Persistent>(x, type_cache<Persistent>::get_descr(), 0);
   }

   // lazy type - never called
   template <typename Source, typename IsMasquerade, typename IsPersistent>
   Anchor* store_canned_ref(const Source& x, int n_anchors, IsMasquerade, std::true_type, IsPersistent)
   {
      return nullptr;
   }

   // persistent regular type
   template <typename SourceRef>
   Anchor* store_canned_value(SourceRef&& x, int n_anchors, std::false_type, std::false_type, std::true_type)
   {
      using Source = pure_type_t<SourceRef>;
      return store_canned_value<Source>(std::forward<SourceRef>(x), type_cache<Source>::get_descr(), n_anchors);
   }

   // persistent regular type
   template <typename Source>
   Anchor* store_canned_ref(const Source& x, int n_anchors, std::false_type, std::false_type, std::true_type)
   {
      return store_canned_ref(x, type_cache<Source>::get_descr(), n_anchors);
   }

   // masquerade type belonging to a generic family
   template <typename SourceRef>
   Anchor* store_canned_value(SourceRef&& x, int n_anchors, std::true_type, std::false_type, std::false_type)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      return store_canned_value<Persistent>(std::forward<SourceRef>(x), type_cache<Persistent>::get_descr(), 0);
   }

   // masquerade type without persistent substitute
   template <typename Source>
   Anchor* store_canned_value(const Source& x, int n_anchors, std::true_type, std::false_type, std::true_type)
   {
      store_as_perl(x);
      return nullptr;
   }

   // masquerade type without persistent substitute
   template <typename Source>
   Anchor* store_canned_ref(const Source& x, int n_anchors, std::true_type, std::false_type, std::true_type)
   {
      if (options * ValueFlags::allow_non_persistent) {
         return store_canned_ref(x, type_cache<Source>::get_descr(), n_anchors);
      }
      store_as_perl(x);
      return nullptr;
   }

   Anchor* store_primitive_ref(const bool& x,          SV* type_descr, int n_anchors);
   Anchor* store_primitive_ref(const Int& x,           SV* type_descr, int n_anchors);
   Anchor* store_primitive_ref(const double& x,        SV* type_descr, int n_anchors);
   Anchor* store_primitive_ref(const std::string& x,   SV* type_descr, int n_anchors);

   void set_string_value(const char* x);
   void set_string_value(const char* x, size_t l);
   void set_copy(const SVHolder& x);

   NoAnchors put_val(Int x, int=0);
   NoAnchors put_val(int x, int=0) { return put_val(Int(x)); }
   NoAnchors put_val(size_t x, int=0) { return put_val(Int(x)); }
   NoAnchors put_val(bool x, int=0);
   NoAnchors put_val(double x, int=0);
   NoAnchors put_val(const Undefined&, int=0);

   template <typename T>
   std::enable_if_t<std::is_enum<T>::value, NoAnchors>
   put_val(const T& x, int=0)
   {
      const Int val = static_cast<Int>(x);
      return put_val(val);
   }

   NoAnchors put_val(const AnyString& x, int=0)
   {
      if (x)
         set_string_value(x.ptr, x.len);
      else
         put_val(Undefined());
      return NoAnchors();
   }

   // need this one separately because otherwise the vile compiler coerces the string array to a boolean
   template <size_t n>
   NoAnchors put_val(const char (&x)[n], int=0)
   {
      set_string_value(x+0, n-1);
      return NoAnchors();
   }

   NoAnchors put_val(char x, int=0)
   {
      set_string_value(&x, 1);
      return NoAnchors();
   }

   NoAnchors put_val(const BigObject& x,            int=0);
   NoAnchors put_val(const BigObjectType& x,        int=0);
   NoAnchors put_val(const PropertyValue& x,     int=0);
   NoAnchors put_val(const Scalar& x,            int=0);
   NoAnchors put_val(const Array& x,             int=0);
   NoAnchors put_val(const Hash& x,              int=0);
   NoAnchors put_val(const ListReturn& x,        int=0);
   NoAnchors put_val(const pm::Array<BigObject>& x, int=0);

   template <typename SourceRef>
   std::enable_if_t<is_class_or_union<pure_type_t<SourceRef>>::value &&
                    !(is_derived_from_any<pure_type_t<SourceRef>, nomagic_types>::value ||
                      represents_BigObject<pure_type_t<SourceRef>>::value ||
                      is_optional_value<pure_type_t<SourceRef>>::value ||
                      !std::is_same<typename object_traits<pure_type_t<SourceRef>>::proxy_for, void>::value),
                    Anchor*>
   put_val(SourceRef&& x, int n_anchors)
   {
      using Source = pure_type_t<SourceRef>;
      using Persistent = typename object_traits<Source>::persistent_type;
      if (object_traits<Source>::is_lazy ||
          !(std::is_rvalue_reference<SourceRef&&>::value
            ? options * ValueFlags::allow_store_temp_ref
            : options * ValueFlags::allow_store_ref)) {
         // must store a copy
         return store_canned_value(std::forward<SourceRef>(x), n_anchors, is_masquerade<Source>(),
                                   bool_constant<object_traits<Source>::is_lazy>(), std::is_same<Source, Persistent>());
      } else {
         // can store a reference
         return store_canned_ref(x, n_anchors, is_masquerade<Source>(),
                                 bool_constant<object_traits<Source>::is_lazy>(), std::is_same<Source, Persistent>());
      }
   }

   template <typename SourceRef>
   std::enable_if_t<represents_BigObject<pure_type_t<SourceRef>>::value, NoAnchors>
   put_val(SourceRef&& x, int);

   template <typename Source, typename Deleter>
   Anchor* put_val(std::unique_ptr<Source, Deleter>&& ptr, int n_anchors)
   {
      if (SV* type_descr=type_cache<std::unique_ptr<Source, Deleter>>::get_descr()) {
         if (options * ValueFlags::allow_non_persistent) {
            return store_canned_value<std::unique_ptr<Source, Deleter>>(std::move(ptr), type_descr, n_anchors);
         } else {
            throw std::invalid_argument("can't store a pointer to an opaque C++ object");
         }
      } else {
         throw std::invalid_argument("can't store an opaque C++ type without perl binding");
      }
   }

   template <typename SourceRef>
   std::enable_if_t<is_optional_value<pure_type_t<SourceRef>>::value, Anchor*>
   put_val(SourceRef&& x, int n_anchors)
   {
      if (x) {
         return put_val(std::forward<SourceRef>(x).value(), n_anchors);
      } else {
         put_val(Undefined());
         return nullptr;
      }
   }

   template <typename SourceRef>
   std::enable_if_t<!std::is_same<typename object_traits<pure_type_t<SourceRef>>::proxy_for, void>::value, Anchor*>
   put_val(SourceRef&& x, int n_anchors)
   {
      SV* type_descr;
      using Source = pure_type_t<SourceRef>;
      if ((options & (ValueFlags::allow_non_persistent | ValueFlags::expect_lval | ValueFlags::read_only)) == 
                     (ValueFlags::allow_non_persistent | ValueFlags::expect_lval) &&
          (type_descr=type_cache<Source>::get_descr())) {
         return store_canned_value<Source>(std::move(x), type_descr, n_anchors);
      } else {
         return put_val(static_cast<const type_behind_t<Source>&>(x), 0);
      }
   }

public:
   template <typename SourceRef, typename... AnchorList>
   void put(SourceRef&& x, AnchorList&&... anchors)
   {
      Anchor* anchor_place = put_val(std::forward<SourceRef>(x), sizeof...(AnchorList));
      if (sizeof...(AnchorList) && anchor_place)
         store_anchors(anchor_place, std::forward<AnchorList>(anchors)...);
   }

   template <typename SourceRef, typename... AnchorList>
   std::enable_if_t<!mlist_contains<mlist_concat<primitive_lvalues, nomagic_lvalue_types>::type, pure_type_t<SourceRef>>::value>
   put_lvalue(SourceRef&& x, AnchorList&&... anchors)
   {
      put(std::forward<SourceRef>(x), std::forward<AnchorList>(anchors)...);
   }

   template <typename SourceRef, typename... AnchorList>
   std::enable_if_t<mlist_contains<primitive_lvalues, pure_type_t<SourceRef>>::value && std::is_lvalue_reference<SourceRef&&>::value>
   put_lvalue(SourceRef&& x, AnchorList&&... anchors)
   {
      using Source = pure_type_t<SourceRef>;
      Anchor* anchor_place = store_primitive_ref(x, type_cache<Source>::get_descr(), sizeof...(AnchorList));
      if (sizeof...(AnchorList) && anchor_place)
         store_anchors(anchor_place, std::forward<AnchorList>(anchors)...);
   }

   template <typename SourceRef, typename... AnchorList>
   std::enable_if_t<mlist_contains<primitive_lvalues, pure_type_t<SourceRef>>::value && !std::is_lvalue_reference<SourceRef&&>::value>
   put_lvalue(SourceRef&& x, AnchorList&&... anchors)
   {
      put_val(x);  // no anchors can ever be needed for a primitive scalar
   }

   template <typename Target>
   void parse(Target& x) const
   {
      if (options * ValueFlags::not_trusted)
         do_parse(x, mlist<TrustedValue<std::false_type>>());
      else
         do_parse(x, mlist<>());
   }

   template <typename Target>
   operator Target () const
   {
      return retrieve_copy<Target>();
   }

   explicit operator bool () const { return is_TRUE(); }
   bool operator! () const { return !is_TRUE(); }

   template <typename Target>
   friend
   std::enable_if_t<!is_effectively_const<Target>::value, bool>
   operator>> (const Value& me, Target&& x)
   {
      if (!me.sv || !me.is_defined()) {
         if (!(me.options * ValueFlags::allow_undef))
            throw Undefined();
         return false;
      }
      me.retrieve(x);
      return true;
   }

   template <typename Source>
   friend void operator<< (const Value& me, Source&& x)
   {
      const_cast<Value&>(me).put(std::forward<Source>(x));
   }

   template <typename Target>
   void* allocate(SV* proto)
   {
      return allocate_canned(type_cache<Target>::get_descr(proto));
   }

   SV* get_constructed_canned();

   template <typename Target>
   decltype(auto) get() const
   {
      return access<Target>::get(*this);
   }

   using SVHolder::get;

   template <typename T>
   Int get_dim(bool tell_size_if_dense) const
   {
      Int d = -1;
      if (is_plain_text()) {
         istream my_stream(sv);
         if (options * ValueFlags::not_trusted)
            d = PlainParser<mlist<TrustedValue<std::false_type>>>(my_stream).begin_list((T*)nullptr).get_dim(tell_size_if_dense);
         else
            d = PlainParser<>(my_stream).begin_list((T*)nullptr).get_dim(tell_size_if_dense);

      } else if (get_canned_typeinfo()) {
         d = get_canned_dim(tell_size_if_dense);

      } else {
         if (options * ValueFlags::not_trusted)
            d = ListValueInput<T, mlist<TrustedValue<std::false_type>>>(sv).get_dim(tell_size_if_dense);
         else
            d = ListValueInput<T>(sv).get_dim(tell_size_if_dense);
      }
      return d;
   }

protected:
   template <typename Target>
   Target& parse_and_can() const
   {
      Value temp_can;
      Target* temp_val=new(temp_can.allocate<Target>(nullptr)) Target{};
      retrieve_nomagic(*temp_val);
      const_cast<Value&>(*this).sv = temp_can.get_constructed_canned();
      return *temp_val;
   }

   template <typename Target, typename Source,
             typename = std::enable_if_t<!std::is_same<pure_type_t<Target>, pure_type_t<Source>>::value>>
   Target& convert_and_can(Source& src) const
   {
      using data_t = std::remove_const_t<Target>;
      Value temp_can;
      data_t* temp_val=new(temp_can.allocate<data_t>(nullptr)) data_t{src};
      const_cast<Value&>(*this).sv = temp_can.get_constructed_canned();
      return *temp_val;
   }

   template <typename Target>
   Target& convert_and_can(Target& src) const
   {
      return src;
   }

   template <typename Target, typename Deleter>
   Target& convert_and_can(const std::unique_ptr<Target, Deleter>& src) const
   {
      return *src;
   }

   template <typename Target>
   Target& convert_and_can(const canned_data_t& canned) const
   {
      using conv_f = Target (*)(const Value&);
      if (conv_f conversion=reinterpret_cast<conv_f>(type_cache<Target>::get_conversion_operator(sv))) {
         Value temp_can;
         Target* temp_val=new(temp_can.allocate<Target>(nullptr)) Target{conversion(*this)};
         const_cast<Value&>(*this).sv = temp_can.get_constructed_canned();
         return *temp_val;
      }
      throw std::runtime_error("invalid conversion from " + legible_typename(*canned.ti) + " to " + legible_typename<Target>());
   }

   template <typename> friend class access;
   friend class ArrayHolder;
   friend class OptionSet;
   friend class ListResult;
   template <typename> friend class ValueOutput;
   template <typename, typename> friend class ListValueInput;
};

template <typename Target>
struct Value::check_for_magic_storage
   : std::remove_pointer_t<decltype(Value().retrieve(std::declval<Target&>()))> {};

template <size_t S>
class ArgValues {
   using value_tuple = typename mlist2tuple<typename mreplicate<Value, S>::type>::type;
   value_tuple values;
   ArgValues(const ArgValues&) = delete;

   template <size_t... I>
   ArgValues(SV** stack, ValueFlags opt_arg, std::index_sequence<I...>)
      : values(Value(stack[I], opt_arg)...) {}

public:
   explicit ArgValues(SV** stack, ValueFlags opt_arg=ValueFlags::is_trusted)
      : ArgValues(stack, opt_arg, std::make_index_sequence<S>()) {}

   template <size_t I, typename Target>
   decltype(auto) get() const
   {
      return std::get<I>(values).template get<Target>();
   }
};

template <bool is_readonly>
class Value::Array_element_factory {
public:
   using argument_type = Int;
   using result_type = Value;

   explicit Array_element_factory(const ArrayHolder* array_arg = nullptr)
      : array(array_arg) {}

   result_type operator() (Int i) const
   {
      return result_type((*array)[i], (is_readonly ? ValueFlags::read_only : ValueFlags::is_mutable) | ValueFlags::not_trusted);
   }

protected:
   const ArrayHolder* array;
};

}

template <bool is_readonly>
struct operation_cross_const_helper<perl::Value::Array_element_factory<is_readonly>> {
   using operation = perl::Value::Array_element_factory<false>;
   using const_operation = perl::Value::Array_element_factory<true>;
};

namespace perl {

inline ArrayHolder::ArrayHolder(const Value& v)
   : SVHolder(v.sv)
{
   if (v.options * ValueFlags::not_trusted) verify();
}

template <typename ElementType, typename Options>
template <typename T, bool anything>
void ListValueInput<ElementType, Options>::retrieve(T& x, bool_constant<anything>)
{
   Value elem(get_next(), this->get_option(TrustedValue<std::true_type>()) ? ValueFlags::is_trusted : ValueFlags::not_trusted);
   elem >> x;
}

template <typename ElementType, typename Options>
Int ListValueInput<ElementType, Options>::cols(bool tell_size_if_dense)
{
   if (cols_ < 0) {
      if (SV* first_sv = get_first()) {
         Value first(first_sv, this->get_option(TrustedValue<std::true_type>()) ? ValueFlags::is_trusted : ValueFlags::not_trusted);
         cols_ = first.get_dim<ElementType>(tell_size_if_dense);
      }
   }
   return cols_;
}

template <typename Options>
void ValueOutput<Options>::store_string(const char* x, size_t l, std::false_type)
{
   static_cast<Value*>(static_cast<super*>(this))->set_string_value(x, l);
}

template <typename Options>
void ValueOutput<Options>::store_string(const char* x, size_t l, std::true_type)
{
   Value v;
   v.set_string_value(x,l);
   this->push_temp(v);
}

template <typename Options, bool returning_list>
template <typename T>
ListValueOutput<Options, returning_list>&
ListValueOutput<Options, returning_list>::operator<< (T&& x)
{
   Value elem;
   elem << std::forward<T>(x);
   push(elem);
   return *this;
}

template <typename T>
void ListReturn::store(T&& x)
{
   Value result;
   result << std::forward<T>(x);
   push(result.get_temp());
}

template <typename Element>
class ArrayOwner
   : public ArrayHolder
   , public modified_container_impl< ArrayOwner<Element>,
                                     mlist< ContainerTag< sequence >,
                                            OperationTag< typename Element::template Array_element_factory<false> > > > {
   friend class Value;
protected:
   explicit ArrayOwner(SV* sv_arg, ValueFlags flags=ValueFlags::is_trusted)
      : ArrayHolder(sv_arg, flags) {}

   explicit ArrayOwner(const Value& v)
      : ArrayHolder(v) {}
public:
   ArrayOwner() {}
   explicit ArrayOwner(Int n) { resize(n); }

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
      return Value(fetch(key, false), ValueFlags::not_trusted | ValueFlags::allow_undef);
   }

   Value operator[] (const AnyString& key)
   {
      return Value(fetch(key, true), ValueFlags::not_trusted | ValueFlags::allow_undef | ValueFlags::allow_non_persistent);
   }

protected:
   void store_values() {}

   template <typename FirstVal, typename... MoreArgs>
   void store_values(const AnyString& first_key, FirstVal&& first_val, MoreArgs&&... more_args)
   {
      Value v(fetch(first_key, true), ValueFlags::allow_non_persistent | ValueFlags::allow_store_any_ref);
      v.put(std::forward<FirstVal>(first_val));
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
      return Value(fetch(key, false), ValueFlags::not_trusted | ValueFlags::allow_undef);
   }
   Value operator[] (const AnyString& key)
   {
      return Value(fetch(key, true), ValueFlags::not_trusted | ValueFlags::allow_undef | ValueFlags::allow_non_persistent);
   }
};

inline Value::NoAnchors Value::put_val(const Scalar& x,      int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const Array& x,       int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const Hash& x,        int) { set_copy(x); return NoAnchors(); }
inline Value::NoAnchors Value::put_val(const ListReturn& x,  int) { forget(); sv=nullptr; return NoAnchors(); }

template <typename Target>
class access {
public:
   using type = Target;
   using return_type = std::remove_const_t<type>;

   static return_type get(const Value& v)
   {
      return v.operator return_type();
   }
};

template <>
class access<void> {
public:
   using type = void;

   static const Value& get(const Value& v)
   {
      return v;
   }
};

template <typename T>
class access<T()>
   : public access<void> {};

template <>
class access<SV*> {
public:
   using type = void;

   static SV* get(const Value& v)
   {
      return v.get();
   }
};

template <>
class access<OptionSet>
   : public access<void> {};

template <typename Given, typename Target>
class access<Target(Given)> {
public:
   using type = Given;
   using given_type = std::remove_const_t<Given>;
   using return_type = std::remove_const_t<Target>;

   static return_type get(const Value& v)
   {
      return static_cast<return_type>(v.operator given_type());
   }
};

// TODO: add a declaration for representative of HashMaps when CPlusPlus.pm learns to generate them for anonymous hash maps

template <typename Target>
using canned_may_be_missing = is_instance_of<Target, pm::Array>;

template <typename Given>
using ignore_constness = is_instance_of<std::remove_const_t<Given>, std::unique_ptr>;

template <typename Target, typename Given>
class access<Target(Canned<Given&>)> {
public:
   using type = Given&;
   static constexpr bool enforce_const = !ignore_constness<Given>::value &&
                                         (std::is_const<Given>::value || !std::is_same<std::remove_const_t<Given>, Target>::value);
   using return_type = std::conditional_t<enforce_const, std::add_const_t<Target>, Target>;

   static return_type& get(const Value& v)
   {
      const Value::canned_data_t canned=Value::get_canned_data(v.sv);
      constexpr bool maybe_missing = std::is_const<Given>::value && canned_may_be_missing<std::remove_const_t<Given>>::value;
      if (maybe_missing && !canned.ti)
         return parse(v, bool_constant<maybe_missing>());
      assert(canned.value && *canned.ti == typeid(Given));
      if (!ignore_constness<Given>::value && !std::is_const<return_type>::value && canned.read_only)
         throw std::runtime_error("read-only object " + legible_typename<Given>() + " can't be bound to a non-const lvalue reference");
      return v.convert_and_can<return_type>(*reinterpret_cast<Given*>(canned.value));
   }
private:
   static return_type& parse(const Value& v, std::true_type)
   {
      return v.parse_and_can<std::remove_const_t<Target>>();
   }
   static return_type& parse(const Value& v, std::false_type);  // undefined
};

template <typename Target>
class access<TryCanned<Target>> {
public:
   using type = Target&;

   static type get(const Value& v)
   {
      const Value::canned_data_t canned=Value::get_canned_data(v.sv);
      if (!canned.ti)
         return v.parse_and_can<std::remove_const_t<Target>>();
      if (*canned.ti == typeid(Target)) {
         if (canned.read_only && !std::is_const<Target>::value)
            throw std::runtime_error("read-only object " + legible_typename<Target>() + " can't be bound to a non-const lvalue reference");
         return *reinterpret_cast<Target*>(canned.value);
      }
      if (!std::is_const<Target>::value) {
         throw std::runtime_error("object " + legible_typename(*canned.ti) + " can't be bound to a non-const lvalue reference to " + legible_typename<Target>());
      }
      return v.convert_and_can<std::remove_const_t<Target>>(canned);
   }
};

template <typename Target>
class access<Canned<Target&>>
   : public access<Target(Canned<Target&>)> {};

template <typename Target>
class access<Canned<const Target&>>
   : public access<Target(Canned<const Target&>)> {};

template <typename Target>
class access<Canned<Target>> {
public:
   using type = Target;

   static Target&& get(const Value& v)
   {
      const Value::canned_data_t canned=Value::get_canned_data(v.sv);
      assert(canned.ti && canned.value && *canned.ti == typeid(Target) && !canned.read_only);
      return std::move(*reinterpret_cast<Target*>(canned.value));
   }
};

template <typename Target, typename Deleter>
class access<Canned<const std::unique_ptr<Target, Deleter>&>>
   : public access<Target(Canned<const std::unique_ptr<Target, Deleter>&>)> {};

template <typename Target>
class access<Canned<Wary<Target>&>> {
public:
   using type = Target&;

   static Wary<Target>& get(const Value& v)
   {
      return wary(access<Canned<Target&>>::get(v));
   }
};

template <typename Target>
class access<Canned<const Wary<Target>&>> {
public:
   using type = const Target&;

   static const Wary<Target>& get(const Value& v)
   {
      return wary(access<Canned<const Target&>>::get(v));
   }
};

template <typename Target>
class access<Canned<Wary<Target>>> {
public:
   using type = Target;

   static Wary<Target>&& get(const Value& v)
   {
      return wary(access<Canned<Target>>::get(v));
   }
};

template <typename Target>
class access<Enum<Target>> {
public:
   using type = Target;

   static Target get(const Value& v)
   {
      return static_cast<Target>(v.enum_value(sizeof(Target), true));
   }
};

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
