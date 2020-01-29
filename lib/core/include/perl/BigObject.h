/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_PERL_BIGOBJECT_H
#define POLYMAKE_PERL_BIGOBJECT_H

#include <sstream>

namespace pm { namespace perl {

enum class property_kind { none, normal, temporary, attachment };

class PropertyOut {
private:
   Value val;
   AnyString construct_paths;
   property_kind t;
   void finish();
   void cancel();
protected:
   explicit PropertyOut(property_kind t_arg, const AnyString& construct_paths_arg = AnyString())
      : val(t_arg == property_kind::normal ? ValueFlags::read_only :
            t_arg == property_kind::temporary ? ValueFlags::allow_non_persistent | ValueFlags::read_only : ValueFlags::is_mutable)
      , construct_paths(construct_paths_arg)
      , t(t_arg) {}

public:
   PropertyOut(const PropertyOut&) = delete;
   PropertyOut(PropertyOut&&) = default;

   template <typename Source>
   void operator<< (Source&& x)
   {
      val << std::forward<Source>(x);
      finish();
   }

   ~PropertyOut()
   {
      if (__builtin_expect(t != property_kind::none, 0)) cancel();
   }

   friend class BigObject;
};

class BigObjectType {
public:
   // construct a parameterized object type
   template <typename... Params>
   BigObjectType(const AnyString& type_name, mlist<Params...> params)
      : obj_ref(TypeBuilder::build(type_name, params)) {}

   explicit BigObjectType(const AnyString& type_name)
      : BigObjectType(type_name, mlist<>()) {}

   BigObjectType() : obj_ref(nullptr) {}

   BigObjectType(const BigObjectType& o);
   ~BigObjectType();

   BigObjectType(BigObjectType&& o) noexcept
      : obj_ref(o.obj_ref)
   {
      o.obj_ref=nullptr;
   }

   bool valid() const { return obj_ref; }

   BigObjectType& operator= (const BigObjectType& o);

   BigObjectType& operator= (BigObjectType&& o) noexcept
   {
      std::swap(obj_ref, o.obj_ref);
      return *this;
   }

   std::string name() const;

   bool isa(const BigObjectType& o) const;

   bool isa(const AnyString& type_name) const;

   bool operator== (const BigObjectType& o) const;
   bool operator!= (const BigObjectType& o) const { return !operator==(o); }

protected:
   class TypeBuilder
      : protected FunCall {
      static AnyString app_method_name();
   public:
      using FunCall::FunCall;

      template <typename... Params>
      static
      SV* build(const AnyString& type_name, const mlist<Params...>& params)
      {
         TypeBuilder b(true, app_method_name(), 1 + count_args(type_name, params));
         b.push_current_application();
         b.push_arg(type_name);
         b.push_types(params);
         return b.call_scalar_context();
      }
   };

   explicit BigObjectType(SV* r)
      : obj_ref(r) {}

   SV* obj_ref;

   friend class Value;
   friend class BigObject;
   friend class pm::Array<BigObject>;
};


class BigObject {
   friend class Value;  friend class Main;
   friend class pm::Array<BigObject>;
protected:
   SV* obj_ref;

   explicit BigObject(SV* ref_arg)
      : obj_ref(ref_arg) {}

public:
   bool valid() const { return obj_ref; }

   // default constructor creates an invalid object
   BigObject() : obj_ref(nullptr) {}

   // construct an empty object of the given type and with an optional name
   explicit BigObject(const BigObjectType& type, const AnyString& name=AnyString());

   // construct an empty object of the type given as a string and with an optional name
   explicit BigObject(const AnyString& type_name, const AnyString& name=AnyString())
      : BigObject(BigObjectType(type_name), name) {}

   // construct an empty object of the given parametrized type and with an optional name
   template <typename... Params>
   BigObject(const AnyString& type_name, mlist<Params...> params, const AnyString& name=AnyString())
      : BigObject(BigObjectType(type_name, params), name) {}

   // construct a copy of another object with possible property conversion
   BigObject(const BigObjectType& type, const BigObject& src);

   BigObject(const AnyString& type_name, const BigObject& src)
      : BigObject(BigObjectType(type_name), src) {}

   template <typename... Params>
   BigObject(const AnyString& type_name, mlist<Params...> params, const BigObject& src)
      : BigObject(BigObjectType(type_name, params), src) {}

   // construct an exact copy (up to temporary properties and local extensions)
   BigObject copy() const;

   // doesn't copy the BigObject, but merely creates a second reference to it!
   BigObject(const BigObject& o);

   BigObject(BigObject&& o) noexcept
      : obj_ref(o.obj_ref)
   {
      o.obj_ref=nullptr;
   }

   ~BigObject();

   BigObject& operator= (const BigObject& o);

   BigObject& operator= (BigObject&& o) noexcept
   {
      std::swap(obj_ref, o.obj_ref);
      return *this;
   }

   // change the object type, possibly converting or discarding properties
   BigObject& cast(const BigObjectType& type);

   BigObject& cast(const AnyString& type_name)
   {
      return cast(BigObjectType(type_name));
   }

   std::string name() const;
   void set_name(const std::string& name);

   std::string description() const;
   void set_description(const std::string& value, bool append=false);

   template <bool append>
   class description_ostream {
      friend class BigObject;
      description_ostream(const description_ostream&) = delete;
      void operator=(const description_ostream&) = delete;
   protected:
      BigObject* obj;
      std::ostringstream content;
      PlainPrinter<> os;

      description_ostream(BigObject* obj_arg) : obj(obj_arg), os(content) {}
   public:
      description_ostream(description_ostream&& o)
         : obj(o.obj)
         , os(content)
      {
         o.obj = nullptr;
      }

      template <typename Source>
      PlainPrinter<>& operator<< (const Source& x)
      {
         return os << x;
      }

      ~description_ostream()
      {
         if (obj) obj->set_description(content.str(), append);
      }
   };

   description_ostream<false> set_description() { return this; }
   description_ostream<true> append_description() { return this; }

protected:
   SV* give_impl(const AnyString& name) const;
   SV* give_with_property_name_impl(const AnyString& name, std::string& given) const;
   SV* lookup_impl(const AnyString& name) const;
   SV* lookup_with_property_name_impl(const AnyString& name, std::string& given) const;

   void take_impl(const AnyString& name) const;
   SV* add_impl(const AnyString& name, SV* sub_obj, property_kind t) const;
public:
   template <typename... TOptions>
   PropertyValue give(const AnyString& name, const TOptions&... options) const
   {
      return PropertyValue(give_impl(name), options...);
   }

   template <typename... TOptions>
   PropertyValue give_with_property_name(const AnyString& name, std::string& given, const TOptions&... options) const
   {
      return PropertyValue(give_with_property_name_impl(name, given), options...);
   }

   template <typename... TOptions>
   PropertyValue lookup(const AnyString& name, const TOptions&... options) const
   {
      return PropertyValue(lookup_impl(name), ValueFlags::allow_undef, options...);
   }

   template <typename... TOptions>
   PropertyValue lookup_with_property_name(const AnyString& name, std::string& given, const TOptions&... options) const
   {
      return PropertyValue(lookup_with_property_name_impl(name, given), ValueFlags::allow_undef, options...);
   }

   BigObject lookup_multi(const AnyString& name, const std::string& subobj_name) const;

   BigObject give_multi(const AnyString& name, const OptionSet& props, property_kind t = property_kind::normal) const;

   BigObject lookup_multi(const AnyString& name, const OptionSet& props) const;

   pm::Array<BigObject> lookup_multi(const AnyString& name, all_selector) const;

   bool exists(const AnyString& name) const;

   PropertyOut take(const AnyString& name, property_kind t = property_kind::normal)
   {
      take_impl(name);
      return PropertyOut(t);
   }

   BigObject add(const AnyString& name, const BigObject& sub_obj, property_kind t = property_kind::normal)
   {
      return BigObject(add_impl(name, sub_obj.obj_ref, t));
   }

   BigObject add(const AnyString& name, property_kind t = property_kind::normal)
   {
      return BigObject(add_impl(name, nullptr, t));
   }

   void remove(const AnyString& name);

   void remove(const BigObject& sub_obj);

   PropertyValue get_attachment(const AnyString& name) const;

   PropertyOut attach(const AnyString& name, const AnyString& construct_paths = AnyString())
   {
      take_impl(name);
      return PropertyOut(property_kind::attachment, construct_paths);
   }

   void remove_attachment(const AnyString& name);

   BigObjectType type() const;
   bool isa(const BigObjectType& type) const;
   bool isa(const AnyString& type_name) const;

   static BigObject load(const std::string& filename);
   void save(const std::string& filename) const;

   BigObject parent() const;

   template <typename... Args>
   FunCall call_method(const AnyString& name, Args&&... args) const
   {
      return FunCall::call_method(name, obj_ref, std::forward<Args>(args)...);
   }

   VarFunCall prepare_call_method(const AnyString& name) const
   {
      return VarFunCall::prepare_call_method(name, obj_ref);
   }

   class Schedule {
   public:
      Schedule() : obj_ref(nullptr) {}
      ~Schedule();

      Schedule(const Schedule&);
      Schedule& operator= (const Schedule&);

      // swallow the result of call_method
      Schedule(FunCall&&);
      Schedule& operator= (FunCall&&);

      bool valid() const { return obj_ref; }

      ListResult list_new_properties() const;

      void apply(BigObject& o) const;

   protected:
      SV* obj_ref;

      friend class BigObject;
   };

   template <bool is_readonly>
   class Array_element;
   template <bool is_readonly>
   class Array_element_factory;
};

template <bool is_readonly>
class BigObject::Array_element
   : public BigObject {
public:
   Array_element(Array_element&&) = default;

   Array_element& operator= (const BigObject& o);

   Array_element& operator= (const Array_element& o)
   {
      return operator=(static_cast<const BigObject&>(o));
   }

private:
   Array_element(SV* ref_arg, const BigObjectType& type);

   Array_element(const Array_element&) = delete;

   const BigObjectType& allowed_type;
   friend class BigObject;
};

template <bool is_readonly>
class BigObject::Array_element_factory {
public:
   typedef Int argument_type;
   typedef Array_element<is_readonly> result_type;

   explicit Array_element_factory(const ArrayHolder* array_arg=nullptr)
      : array(array_arg) {}

   result_type operator() (Int i) const;
protected:
   const ArrayHolder* array;
};


template <typename Target>
typename std::enable_if<represents_BigObject<Target>::value, std::false_type*>::type
Value::retrieve(Target& x) const
{
   BigObject obj;
   retrieve(obj);
   x = obj;
   return nullptr;
}

template <typename SourceRef>
std::enable_if_t<represents_BigObject<pure_type_t<SourceRef>>::value, Value::NoAnchors>
Value::put_val(SourceRef&& x, int)
{
   return put_val(static_cast<BigObject>(std::forward<SourceRef>(x)));
}

}

template <>
struct spec_object_traits<perl::BigObject>
   : spec_object_traits<is_opaque>
{
   // default constructor and destructor require a living perl interpreter
   static const bool allow_static=false;
};

template <>
struct spec_object_traits<perl::BigObjectType>
   : spec_object_traits<perl::BigObject> {};

template <bool is_readonly>
struct operation_cross_const_helper<perl::BigObject::Array_element_factory<is_readonly>> {
   typedef perl::BigObject::Array_element_factory<false> operation;
   typedef perl::BigObject::Array_element_factory<true> const_operation;
};

template <>
class Array<perl::BigObject>
   : public perl::ArrayOwner<perl::BigObject> {
   typedef perl::ArrayOwner<perl::BigObject> base_t;
public:
   /// Create an array of the given size and fill it with invalid objects.
   explicit Array(Int n = 0)
      : base_t(n) {}

   /// Create an array with a prescribed type for elements.
   /// It is populated with the given number of valid objects without properties.
   explicit Array(const perl::BigObjectType& type, Int n = 0);

   /// Construct an array from a sequence of BigObjects passed as arguments.
   /// The prescribed element type is unspecified; it can be deduced later on demand.
   Array(std::initializer_list<perl::BigObject> objects);

   /// Construct an array from a sequence of BigObjects passed as arguments.
   /// The elements must be compatible with the prescribed element type.
   Array(const perl::BigObjectType& type, std::initializer_list<perl::BigObject> objects);

   Array(const Array& other) = default;
   Array(Array&& other) = default;

   Array& operator= (const Array& other) = default;
   Array& operator= (Array&& other) = default;

   /// Create an elementwise copy.
   Array copy() const;

   /// Change the number of elements.
   /// If the array grows and the prescribed element type has been specified,
   /// new elements are created as valid objects without properties.
   /// Otherwise the new elements are in invalid state.
   void resize(Int n);

   /// Add an element at the end of the array.
   /// If must be compatible with the prescribed element type, unless the latter is unspecified.
   void push_back(perl::BigObject&& o);

   void push_back(const perl::BigObject& o)
   {
      push_back(perl::BigObject(o));
   }

   /// Tell the prescribed element type.
   /// If it has been unspecified until now, it will be deduced as the common type of all contained objects.
   /// It might turn out to be invalid if the objects are of incompatible types.
   const perl::BigObjectType& element_type() const;

   // prohibit low-level operations inherited from ArrayHolder
   void upgrade(Int) = delete;
   void set_contains_aliases() = delete;
   SV* shift() = delete;
   SV* pop() = delete;
   void unshift(SV*) = delete;
   void push(SV*) = delete;

protected:
   explicit Array(SV* sv_ref, perl::ValueFlags flags=perl::ValueFlags::is_trusted)
      : base_t(sv_ref, flags) {}

   mutable perl::BigObjectType el_type;

   friend class perl::BigObject;
   friend class perl::Value;
};

namespace perl {

inline
std::false_type* Value::retrieve(pm::Array<BigObject>& x) const
{
   return retrieve(static_cast<Array&>(static_cast<ArrayHolder&>(x)));
}

template <bool is_readonly>
inline
BigObject::Array_element<is_readonly> BigObject::Array_element_factory<is_readonly>::operator() (Int i) const
{
   return { (*array)[i], static_cast<const pm::Array<BigObject>*>(array)->el_type };
}

} }

#endif // POLYMAKE_PERL_BIGOBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
