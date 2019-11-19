/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_PERL_OBJECT_H
#define POLYMAKE_PERL_OBJECT_H

#include <sstream>

namespace pm { namespace perl {

enum property_type { done_, normal_, temporary, attachment };

class PropertyOut {
private:
   Value val;
   AnyString construct_paths;
   property_type t;
   void finish();
   void cancel();
protected:
   explicit PropertyOut(property_type t_arg, const AnyString& construct_paths_arg = AnyString())
      : val(t_arg == normal_ ? ValueFlags::read_only :
            t_arg == temporary ? ValueFlags::allow_non_persistent | ValueFlags::read_only : ValueFlags::is_mutable)
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
      if (__builtin_expect(t != done_, 0)) cancel();
   }

   friend class Object;
};

class ObjectType {
public:
   // construct a parameterized object type
   template <typename... Params>
   ObjectType(const AnyString& type_name, mlist<Params...> params)
      : obj_ref(TypeBuilder::build(type_name, params)) {}

   explicit ObjectType(const AnyString& type_name)
      : ObjectType(type_name, mlist<>()) {}

   ObjectType() : obj_ref(nullptr) {}

   ObjectType(const ObjectType& o);
   ~ObjectType();

   ObjectType(ObjectType&& o) noexcept
      : obj_ref(o.obj_ref)
   {
      o.obj_ref=nullptr;
   }

   bool valid() const { return obj_ref; }

   ObjectType& operator= (const ObjectType& o);

   ObjectType& operator= (ObjectType&& o) noexcept
   {
      std::swap(obj_ref, o.obj_ref);
      return *this;
   }

   std::string name() const;

   bool isa(const ObjectType& o) const;

   bool isa(const AnyString& type_name) const;

   bool operator== (const ObjectType& o) const;
   bool operator!= (const ObjectType& o) const { return !operator==(o); }

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

   explicit ObjectType(SV* r)
      : obj_ref(r) {}

   SV* obj_ref;

   friend class Value;
   friend class Object;
   friend class pm::Array<Object>;
};


class Object {
   friend class Value;  friend class Main;
   friend class pm::Array<Object>;
protected:
   SV* obj_ref;

   explicit Object(SV* ref_arg)
      : obj_ref(ref_arg) {}

public:
   bool valid() const { return obj_ref; }

   // default constructor creates an invalid object
   Object() : obj_ref(nullptr) {}

   // construct an empty object of the given type and with an optional name
   explicit Object(const ObjectType& type, const AnyString& name=AnyString());

   // construct an empty object of the type given as a string and with an optional name
   explicit Object(const AnyString& type_name, const AnyString& name=AnyString())
      : Object(ObjectType(type_name), name) {}

   // construct an empty object of the given parametrized type and with an optional name
   template <typename... Params>
   Object(const AnyString& type_name, mlist<Params...> params, const AnyString& name=AnyString())
      : Object(ObjectType(type_name, params), name) {}

   // construct a copy of another object with possible property conversion
   Object(const ObjectType& type, const Object& src);

   Object(const AnyString& type_name, const Object& src)
      : Object(ObjectType(type_name), src) {}

   template <typename... Params>
   Object(const AnyString& type_name, mlist<Params...> params, const Object& src)
      : Object(ObjectType(type_name, params), src) {}

   // construct an exact copy (up to temporary properties and local extensions)
   Object copy() const;

   // doesn't copy the Object, but merely creates a second reference to it!
   Object(const Object& o);

   Object(Object&& o) noexcept
      : obj_ref(o.obj_ref)
   {
      o.obj_ref=nullptr;
   }

   ~Object();

   Object& operator= (const Object& o);

   Object& operator= (Object&& o) noexcept
   {
      std::swap(obj_ref, o.obj_ref);
      return *this;
   }

   // change the object type, possibly converting or discarding properties
   Object& cast(const ObjectType& type);

   Object& cast(const AnyString& type_name)
   {
      return cast(ObjectType(type_name));
   }

   std::string name() const;
   void set_name(const std::string& name);

   std::string description() const;
   void set_description(const std::string& value, bool append=false);

   template <bool append>
   class description_ostream {
      friend class Object;
   protected:
      mutable Object* obj;
      mutable std::ostringstream content;
      mutable PlainPrinter<> os;

      description_ostream(Object* obj_arg) : obj(obj_arg), os(content) {}
   public:
      description_ostream(const description_ostream& o) : obj(o.obj), os(content) { o.obj=NULL; }

      template <typename Source>
      PlainPrinter<>& operator<< (const Source& x) const
      {
         return os << x;
      }

      ~description_ostream() { if (obj) obj->set_description(content.str(), append); }
   };

   description_ostream<false> set_description() { return this; }
   description_ostream<true> append_description() { return this; }

protected:
   SV* give_impl(const AnyString& name) const;
   SV* give_with_property_name_impl(const AnyString& name, std::string& given) const;
   SV* lookup_impl(const AnyString& name) const;
   SV* lookup_with_property_name_impl(const AnyString& name, std::string& given) const;

   void take_impl(const AnyString& name) const;
   SV* add_impl(const AnyString& name, SV* sub_obj, property_type t) const;
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

   Object lookup_multi(const AnyString& name, const std::string& subobj_name) const;

   Object give_multi(const AnyString& name, const OptionSet& props, property_type t = normal_) const;

   Object lookup_multi(const AnyString& name, const OptionSet& props) const;

   pm::Array<Object> lookup_multi(const AnyString& name, all_selector) const;

   bool exists(const AnyString& name) const;

   PropertyOut take(const AnyString& name, property_type t = normal_)
   {
      take_impl(name);
      return PropertyOut(t);
   }

   Object add(const AnyString& name, const Object& sub_obj, property_type t = normal_)
   {
      return Object(add_impl(name, sub_obj.obj_ref, t));
   }

   Object add(const AnyString& name, property_type t = normal_)
   {
      return Object(add_impl(name, nullptr, t));
   }

   void remove(const AnyString& name);

   void remove(const Object& sub_obj);

   PropertyValue get_attachment(const AnyString& name) const;

   PropertyOut attach(const AnyString& name, const AnyString& construct_paths = AnyString())
   {
      take_impl(name);
      return PropertyOut(attachment, construct_paths);
   }

   void remove_attachment(const AnyString& name);

   ObjectType type() const;
   bool isa(const ObjectType& type) const;
   bool isa(const AnyString& type_name) const;

   static Object load(const std::string& filename);
   void save(const std::string& filename) const;

   Object parent() const;

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

      void apply(Object& o) const;

   protected:
      SV* obj_ref;

      friend class Object;
   };

   template <bool is_readonly>
   class Array_element;
   template <bool is_readonly>
   class Array_element_factory;
};

template <bool is_readonly>
class Object::Array_element
   : public Object {
public:
   Array_element(Array_element&&) = default;

   Array_element& operator= (const Object& o);

   Array_element& operator= (const Array_element& o)
   {
      return operator=(static_cast<const Object&>(o));
   }

private:
   Array_element(SV* ref_arg, const ObjectType& type);

   Array_element(const Array_element&) = delete;

   const ObjectType& allowed_type;
   friend class Object;
};

template <bool is_readonly>
class Object::Array_element_factory {
public:
   typedef int argument_type;
   typedef Array_element<is_readonly> result_type;

   explicit Array_element_factory(const ArrayHolder* array_arg=nullptr)
      : array(array_arg) {}

   result_type operator() (int i) const;
protected:
   const ArrayHolder* array;
};


template <typename Target>
typename std::enable_if<represents_big_Object<Target>::value, std::false_type*>::type
Value::retrieve(Target& x) const
{
   Object obj;
   retrieve(obj);
   x = obj;
   return nullptr;
}

template <typename SourceRef>
std::enable_if_t<represents_big_Object<pure_type_t<SourceRef>>::value, Value::NoAnchors>
Value::put_val(SourceRef&& x, int)
{
   return put_val(static_cast<Object>(std::forward<SourceRef>(x)));
}

}

template <>
struct spec_object_traits<perl::Object>
   : spec_object_traits<is_opaque>
{
   // default constructor and destructor require a living perl interpreter
   static const bool allow_static=false;
};

template <>
struct spec_object_traits<perl::ObjectType>
   : spec_object_traits<perl::Object> {};

template <bool is_readonly>
struct operation_cross_const_helper<perl::Object::Array_element_factory<is_readonly>> {
   typedef perl::Object::Array_element_factory<false> operation;
   typedef perl::Object::Array_element_factory<true> const_operation;
};

template <>
class Array<perl::Object>
   : public perl::ArrayOwner<perl::Object> {
   typedef perl::ArrayOwner<perl::Object> base_t;
public:
   /// Create an array of the given size and fill it with invalid objects.
   explicit Array(int n=0)
      : base_t(n) {}

   /// Create an array with a prescribed type for elements.
   /// It is populated with the given number of valid objects without properties.
   explicit Array(const perl::ObjectType& type, int n=0);

   /// Construct an array from a sequence of Objects passed as arguments.
   /// The prescribed element type is unspecified; it can be deduced later on demand.
   Array(std::initializer_list<perl::Object> objects);

   /// Construct an array from a sequence of Objects passed as arguments.
   /// The elements must be compatible with the prescribed element type.
   Array(const perl::ObjectType& type, std::initializer_list<perl::Object> objects);

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
   void resize(int n);

   /// Add an elemnet at the end of the array.
   /// If must be compatible with the prescribed element type, unless the latter is unspecified.
   void push_back(perl::Object&& o);

   void push_back(const perl::Object& o)
   {
      push_back(perl::Object(o));
   }

   /// Tell the prescribed element type.
   /// If it has been unspecified until now, it will be deduced as the common type of all contained objects.
   /// It might turn out to be invalid if the objects are of incompatible types.
   const perl::ObjectType& element_type() const;

   // prohibit low-level operations inherited from ArrayHolder
   void upgrade(int) = delete;
   void set_contains_aliases() = delete;
   int dim(bool&) = delete;
   int cols() const = delete;
   SV* shift() = delete;
   SV* pop() = delete;
   void unshift(SV*) = delete;
   void push(SV*) = delete;

protected:
   explicit Array(SV* sv_ref, perl::ValueFlags flags=perl::ValueFlags::is_trusted)
      : base_t(sv_ref, flags) {}

   mutable perl::ObjectType el_type;

   friend class perl::Object;
   friend class perl::Value;
};

namespace perl {

inline
std::false_type* Value::retrieve(pm::Array<Object>& x) const
{
   return retrieve(static_cast<Array&>(static_cast<ArrayHolder&>(x)));
}

template <bool is_readonly>
inline
Object::Array_element<is_readonly> Object::Array_element_factory<is_readonly>::operator() (int i) const
{
   return { (*array)[i], static_cast<const pm::Array<Object>*>(array)->el_type };
}

} }

#endif // POLYMAKE_PERL_OBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
