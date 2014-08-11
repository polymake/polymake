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

#ifndef POLYMAKE_PERL_OBJECT_H
#define POLYMAKE_PERL_OBJECT_H

#include <sstream>

namespace pm { namespace perl {

enum property_type { _done, _normal, temporary, attachment };

class PropertyOut {
private:
   Value val;
   PerlInterpreter* pi;
   property_type t;
   void finish();
   void cancel();
protected:
   explicit PropertyOut(PerlInterpreter* pi_arg, property_type t_arg)
      : val(value_flags(t_arg==_normal ? value_read_only :
                        t_arg==temporary ? value_allow_non_persistent|value_read_only : 0)),
        pi(pi_arg),
        t(t_arg) {}
public:
   template <typename Source>
   void operator<< (const Source& x)
   {
      val << x;
      finish();
   }

   ~PropertyOut()
   {
      if (__builtin_expect(t != _done, 0)) cancel();
   }

   friend class Object;
};


class ObjectType {
   friend class Value;
   friend class Object;

protected:
   SV* obj_ref;

   bool _isa(const char* type_name, size_t tl) const;
   static SV* find_type(const char* type_name, size_t tl);
   static SV* construct_parameterized_type(const char* type_name, size_t tl);

   template <typename TypeList>
   static SV* construct(const char* type_name, size_t tl)
   {
      Stack stack(false, TypeListUtils<TypeList>::type_cnt);
      if (TypeListUtils<TypeList>::push_types(stack)) {
         return construct_parameterized_type(type_name, tl);
      } else {
         stack.cancel();
         throw exception("one of the type arguments is not declared in the rules");
      }
   }
   explicit ObjectType(SV* r) : obj_ref(r) {}
public:
   template <size_t nl>
   explicit ObjectType(const char (&type_name)[nl]) :
      obj_ref(find_type(type_name, nl-1)) {}

   explicit ObjectType(const std::string& type_name) :
      obj_ref(find_type(type_name.c_str(), type_name.size())) {}

   ObjectType();

   ObjectType(const ObjectType& o);
   ~ObjectType();

   // construct a parameterized object type
   template <typename TypeList, size_t tl>
   static ObjectType construct(const char (&type_name)[tl])
   {
      return ObjectType(construct<TypeList>(type_name, tl-1));
   }

   template <typename TypeList>
   static ObjectType construct(const std::string& type_name)
   {
      return ObjectType(construct<TypeList>(type_name.c_str(), type_name.size()));
   }

   ObjectType& operator= (const ObjectType& o);

   std::string name() const;
   std::string generic_name() const;
 
   bool isa(const ObjectType& o) const;

   template <size_t nl>
   bool isa(const char (&type_name)[nl]) const
   {
      return _isa(type_name, nl-1);
   }
   bool isa(const std::string& type_name) const
   {
      return _isa(type_name.c_str(), type_name.size());
   }
};


class Object {
   friend class Value;  friend class Main;
   template <typename> friend class Array_access;
protected:
   SV* obj_ref;
   mutable bool needs_commit;

   void _create(const ObjectType& type, const char* name=NULL, size_t nl=0);
   void _create_copy(const ObjectType& type, const Object& src);
   bool _isa(const char* type_name, size_t tl) const;

   explicit Object(SV* ref_arg)
      : obj_ref(ref_arg)
      , needs_commit(false) {}

   Object(SV* ref_arg, const Array_access<Object>&);

public:
   void create_new(const ObjectType& type);

   bool valid() const;

   // construct an empty object of the given type
   explicit Object(const ObjectType& type)
      : obj_ref(NULL)
   {
      _create(type);
   }

   template <size_t nl>
   explicit Object(const char (&type_name)[nl])
      : obj_ref(NULL)
   {
      _create(ObjectType(type_name));
   }

   explicit Object(const std::string& type_name)
      : obj_ref(NULL)
   {
      _create(ObjectType(type_name));
   }

   // construct an empty object of the given type and with the given name
   Object(const ObjectType& type, const std::string& name)
      : obj_ref(NULL)
   {
      _create(type, name.c_str(), name.size());
   }

   template <size_t nl>
   explicit Object(const char (&type_name)[nl], const std::string& name)
      : obj_ref(NULL)
   {
      _create(ObjectType(type_name), name.c_str(), name.size());
   }

   explicit Object(const std::string& type_name, const std::string& name)
      : obj_ref(NULL)
   {
      _create(ObjectType(type_name),name.c_str(), name.size());
   }

   // construct a copy of another object with possible property conversion
   Object(const ObjectType& type, const Object& src)
      : obj_ref(NULL)
   {
      _create_copy(type,src);
   }

   template <size_t nl>
   Object(const char (&type_name)[nl], const Object& src)
      : obj_ref(NULL)
   {
      _create_copy(ObjectType(type_name), src);
   }

   Object(const std::string& type_name, const Object& src)
      : obj_ref(NULL)
   {
      _create_copy(ObjectType(type_name), src);
   }

   // construct an exact copy (up to temporary properties and local extensions)
   Object copy() const;

   // construct a really empty object without type
   Object();

   // doesn't copy the Object, but merely creates a second reference to it!
   Object(const Object& o);

   ~Object();

   Object& operator= (const Object& o);

   // change the object type, possibly converting or discarding properties
   Object& cast(const ObjectType& type);

   template <size_t nl>
   Object& cast(const char (&type_name)[nl])
   {
      return cast(ObjectType(type_name));
   }

   Object& cast(const std::string& type_name)
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
   SV* _give(const char* name, size_t nl) const;
   SV* _give_with_property_name(const char* name, size_t nl, std::string& given) const;
   SV* _lookup(const char* name, size_t nl) const;
   SV* _lookup_with_property_name(const char* name, size_t nl, std::string& given) const;
   SV* _lookup(const char* name, size_t nl, const std::string& subobj_name) const;
   SV* _give(const char* name, size_t nl, SV* props, property_type t) const;
   SV* _lookup(const char* name, size_t nl, SV* props) const;
   SV* _give_all(const char* name, size_t nl) const;
   PerlInterpreter* _take(const char* name, size_t nl) const;
   bool _exists(const char* name, size_t nl) const;
   SV* _add(const char* name, size_t nl, SV* sub_obj, property_type t) const;
   void _remove(const char* name, size_t nl) const;
   SV* _get_attachment(const char* name, size_t nl) const;
   void _remove_attachment(const char* name, size_t nl) const;
public:
   template <size_t nl>
   PropertyValue give(const char (&name)[nl]) const
   {
      return PropertyValue(_give(name, nl-1));
   }
   PropertyValue give(const std::string& name) const
   {
      return PropertyValue(_give(name.c_str(), name.size()));
   }

   template <size_t nl>
   PropertyValue give_with_property_name(const char (&name)[nl], std::string& given) const
   {
      return PropertyValue(_give_with_property_name(name, nl-1, given));
   }
   PropertyValue give_with_property_name(const std::string& name, std::string& given) const
   {
      return PropertyValue(_give_with_property_name(name.c_str(), name.size(), given));
   }

   template <size_t nl>
   PropertyValue lookup(const char (&name)[nl]) const
   {
      return PropertyValue(_lookup(name, nl-1), value_allow_undef);
   }
   PropertyValue lookup(const std::string& name) const
   {
      return PropertyValue(_lookup(name.c_str(), name.size()), value_allow_undef);
   }

   template <size_t nl>
   PropertyValue lookup_with_property_name(const char (&name)[nl], std::string& given) const
   {
      return PropertyValue(_lookup_with_property_name(name, nl-1, given), value_allow_undef);
   }
   PropertyValue lookup_with_property_name(const std::string& name, std::string& given) const
   {
      return PropertyValue(_lookup_with_property_name(name.c_str(), name.size(), given), value_allow_undef);
   }

   template <size_t nl>
   PropertyValue lookup(const char (&name)[nl], const std::string& subobj_name) const
   {
      return PropertyValue(_lookup(name, nl-1, subobj_name));
   }
   PropertyValue lookup(const std::string& name, const std::string& subobj_name) const
   {
      return PropertyValue(_lookup(name.c_str(), name.size(), subobj_name));
   }

   template <size_t nl>
   PropertyValue give(const char (&name)[nl], const Hash& props, property_type t=_normal) const
   {
      return PropertyValue(_give(name, nl-1, props.get(), t));
   }
   PropertyValue give(const std::string& name, const Hash& props, property_type t=_normal) const
   {
      return PropertyValue(_give(name.c_str(), name.size(), props.get(), t));
   }

   template <size_t nl>
   PropertyValue give(const char (&name)[nl], TempOptions& props, property_type t=_normal) const
   {
      return PropertyValue(_give(name, nl-1, props.get_temp(), t));
   }
   PropertyValue give(const std::string& name, TempOptions& props, property_type t=_normal) const
   {
      return PropertyValue(_give(name.c_str(), name.size(), props.get_temp(), t));
   }

   template <size_t nl>
   PropertyValue lookup(const char (&name)[nl], const Hash& props) const
   {
      return PropertyValue(_lookup(name, nl-1, props.get()), value_allow_undef);
   }
   PropertyValue lookup(const std::string& name, const Hash& props) const
   {
      return PropertyValue(_lookup(name.c_str(), name.size(), props.get()), value_allow_undef);
   }

   template <size_t nl>
   PropertyValue lookup(const char (&name)[nl], TempOptions& props) const
   {
      return PropertyValue(_lookup(name, nl-1, props.get_temp()), value_allow_undef);
   }
   PropertyValue lookup(const std::string& name, TempOptions& props) const
   {
      return PropertyValue(_lookup(name.c_str(), name.size(), props.get_temp()), value_allow_undef);
   }

   template <size_t nl>
   pm::Array<Object> give_all(const char (&name)[nl]) const;

   pm::Array<Object> give_all(const std::string& name) const;

   template <size_t nl>
   bool exists(const char (&name)[nl]) const
   {
      return _exists(name, nl-1);
   }
   bool exists(const std::string& name) const
   {
      return _exists(name.c_str(), name.size());
   }

   template <size_t nl>
   PropertyOut take(const char (&name)[nl], property_type t=_normal)
   {
      return PropertyOut(_take(name, nl-1), t);
   }
   PropertyOut take(const std::string& name, property_type t=_normal)
   {
      return PropertyOut(_take(name.c_str(), name.size()), t);
   }

   template <size_t nl>
   Object add(const char (&name)[nl], const Object& sub_obj, property_type t=_normal)
   {
      return Object(_add(name, nl-1, sub_obj.obj_ref, t));
   }
   Object add(const std::string& name, const Object& sub_obj, property_type t=_normal)
   {
      return Object(_add(name.c_str(), name.size(), sub_obj.obj_ref, t));
   }

   template <size_t nl>
   Object add(const char (&name)[nl], property_type t=_normal)
   {
      needs_commit=true;
      return Object(_add(name, nl-1, NULL, t));
   }
   Object add(const std::string& name, property_type t=_normal)
   {
      needs_commit=true;
      return Object(_add(name.c_str(), name.size(), NULL, t));
   }

   template <size_t nl>
   void remove(const char (&name)[nl])
   {
      _remove(name, nl-1);
   }
   void remove(const std::string& name)
   {
      _remove(name.c_str(), name.size());
   }

   void remove(const Object& sub_obj);

   template <size_t nl>
   PropertyValue get_attachment(const char (&name)[nl]) const
   {
      return PropertyValue(_get_attachment(name,nl-1), value_allow_undef);
   }
   PropertyValue get_attachment(const std::string& name) const
   {
      return PropertyValue(_get_attachment(name.c_str(), name.size()), value_allow_undef);
   }

   template <size_t nl>
   PropertyOut attach(const char (&name)[nl])
   {
      return PropertyOut(_take(name, nl-1), attachment);
   }
   PropertyOut attach(const std::string& name)
   {
      return PropertyOut(_take(name.c_str(), name.size()), attachment);
   }

   template <size_t nl>
   void remove_attachment(const char (&name)[nl])
   {
      _remove_attachment(name, nl-1);
   }
   void remove_attachment(const std::string& name)
   {
      _remove_attachment(name.c_str(), name.size());
   }

   ObjectType type() const;
   bool isa(const ObjectType& type) const;

   template <size_t nl>
   bool isa(const char (&type_name)[nl]) const
   {
      return _isa(type_name, nl-1);
   }
   bool isa(const std::string& type_name) const
   {
      return _isa(type_name.c_str(), type_name.size());
   }

   static Object load(const std::string& filename);
   void save(const std::string& filename) const;

   Object parent() const;

   PropertyValue call_polymake_method(const std::string& name, const FunCall& funcall) const
   {
      return PropertyValue(funcall.evaluate_method(obj_ref, name.c_str()),
                           value_flags(value_allow_undef | value_not_trusted));
   }
   PropertyValue call_polymake_method(const char* name, const FunCall& funcall) const
   {
      return PropertyValue(funcall.evaluate_method(obj_ref, name),
                           value_flags(value_allow_undef | value_not_trusted));
   }

   ListResult list_call_polymake_method(const std::string& name, const FunCall& funcall) const
   {
      return ListResult(funcall.list_evaluate_method(obj_ref, name.c_str()), funcall);
   }
   ListResult list_call_polymake_method(const char* name, const FunCall& funcall) const
   {
      return ListResult(funcall.list_evaluate_method(obj_ref, name), funcall);
   }

   void void_call_polymake_method(const std::string& name, const FunCall& funcall) const
   {
      funcall.void_evaluate_method(obj_ref, name.c_str());
   }
   void void_call_polymake_method(const char* name, const FunCall& funcall) const
   {
      funcall.void_evaluate_method(obj_ref, name);
   }

   class Schedule {
      friend class Object;
   protected:
      SV* obj_ref;
   public:
      Schedule();
      Schedule(const Schedule&);
      Schedule& operator= (const Schedule&);

      // swallow the result of CallPolymakeMethod
      Schedule(const PropertyValue&);
      Schedule& operator= (const PropertyValue&);

      bool valid() const;

      ListResult list_new_properties() const
      {
         FunCall fc;
         return ListResult(fc.list_evaluate_method(obj_ref, "list_new_properties"), fc);
      }

      void apply(Object& o) const
      {
         FunCall fc;
         fc.push(o.obj_ref);
         fc.void_evaluate_method(obj_ref, "apply");
      }
   };
};

template <typename Container>
void read_labels(const Object& p, const char* label_prop, Container& labels)
{
   if (!(p.lookup(label_prop) >> labels)) {
      std::ostringstream label;
      int i=0;
      for (typename Entire<Container>::iterator dst=entire(labels); !dst.at_end(); ++i, ++dst) {
         label.str("");
         label << i;
         *dst = label.str();
      }
   }
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

template <>
class Array<perl::Object, void>
   : public perl::ArrayOwner<perl::Object> {
   typedef perl::ArrayOwner<perl::Object> super;
   friend class perl::Object;  friend class perl::Value;
protected:
   mutable bool needs_commit;

   explicit Array(SV* sv_ref)
      : super(sv_ref)
      , needs_commit(false) {}
public:
   /// Create an empty array.
   /// Can be expanded later with resize() or push_back().
   Array() : needs_commit(true) {}

   /// Create an array of the given size and populate it with Objects in invalid state.
   /// Each object must later be initialized with create_new() or assigned from another valid object.
   explicit Array(int n)
      : super(n)
      , needs_commit(true) {}

   /// Create an array of the given size and populate it with valid, empty objects of the given type.
   Array(int n, const perl::ObjectType& type)
      : super(n)
      , needs_commit(true)
   {
      for (Entire<Array>::iterator it=entire(*this); !it.at_end(); ++it)
         it->create_new(type);
   }

   /// This does *not* copy the single objects from the source array.
   /// Instead, the original array gets one more reference pointing to it.
   Array(const Array& other)
      : super(other)
      , needs_commit(other.needs_commit)
   {
      other.needs_commit=false;
   }

   /// This does *not* copy the single objects from the source array.
   /// Instead, the original array gets one more reference pointing to it.
   Array& operator= (const Array& other)
   {
      super::operator=(other);
      needs_commit=other.needs_commit;
      other.needs_commit=false;
      return *this;
   }
};

namespace perl {

template <size_t nl> inline
pm::Array<Object> Object::give_all(const char (&name)[nl]) const
{
   return pm::Array<Object>(_give_all(name, nl-1));
}

inline
pm::Array<Object> Object::give_all(const std::string& name) const
{
   return pm::Array<Object>(_give_all(name.c_str(), name.size()));
}

inline
False* Value::retrieve(pm::Array<Object>& x) const
{
   return retrieve(static_cast<Array&>(static_cast<ArrayHolder&>(x)));
}

} }

#endif // POLYMAKE_PERL_OBJECT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
