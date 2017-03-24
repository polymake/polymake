/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_PERL_CALLS_H
#define POLYMAKE_PERL_CALLS_H

namespace pm { namespace perl {

class PropertyValue;
PropertyValue load_data(const std::string& filename);

template <typename T> inline
void save_data(const std::string& filename, const T& data, const std::string& description=std::string());

PropertyValue get_custom(const AnyString& name, const AnyString& key=AnyString());

class FunCall;

class PropertyValue : public Value {
   friend class Object;   friend class ObjectType;
   friend class FunCall;

   friend PropertyValue load_data(const std::string& filename);

   template <typename T> friend
   void save_data(const std::string& filename, const T& data, const std::string& description);

   friend
   PropertyValue get_custom(const AnyString& name, const AnyString& key);

   PropertyValue() {}

   template <typename... TOptions>
   explicit PropertyValue(SV* sv_arg, TOptions... options)
      : Value(sv_arg)
   {
      set_options(options...);
   }

   void set_options() {}

   template <typename... TMoreOptions>
   void set_options(value_flags flag, TMoreOptions... more_options)
   {
      options |= flag;
      set_options(more_options...);
   }

   template <typename... TMoreOptions>
   void set_options(allow_conversion, TMoreOptions... more_options)
   {
      options |= value_allow_conversion;
      set_options(more_options...);
   }

   void operator= (const PropertyValue&) = delete;

   static SV* _load_data(const std::string& filename);
   void _save_data(const std::string&, const std::string&);
public:
   PropertyValue(const PropertyValue& x);
   ~PropertyValue();
};

inline
Value::NoAnchors Value::put_val(const PropertyValue& x, int, int)
{
   set_copy(x);
   return NoAnchors();
}

inline
PropertyValue load_data(const std::string& filename)
{
   return PropertyValue(PropertyValue::_load_data(filename));
}

template <typename T> inline
void save_data(const std::string& filename, const T& data, const std::string& description)
{
   PropertyValue v;
   v << data;
   v._save_data(filename, description);
}

template <typename TContainerRef>
struct Unrolled {
   explicit Unrolled(TContainerRef c)
      : container(c) {}

   TContainerRef container;
};

template <typename TContainer>
inline
Unrolled<TContainer&&> unroll(TContainer&& c)
{
   return Unrolled<TContainer&&>(std::forward<TContainer>(c));
}

class ListResult
   : protected Array {
public:
   ListResult(ListResult&&) = default;
   ListResult& operator= (ListResult&&) = default;

   template <typename Target>
   ListResult& operator>> (Target& x)
   {
      if (!empty()) {
         Value v(shift(), value_allow_undef | value_not_trusted);
         v >> x;
         v.forget();
      }
      return *this;
   }

   template <typename TContainerRef>
   void operator>> (Unrolled<TContainerRef>&& c)
   {
      Value v(sv, value_not_trusted);
      v.retrieve_nomagic(c.container);
      forget();
      sv=nullptr;
   }

   void operator>> (Unrolled<Array&>&& c)
   {
      c.container=static_cast<Array&&>(*this);
   }

protected:
   ListResult(int items, FunCall& fc);

   friend class Object;
   friend class FunCall;

private:
   ListResult(const ListResult&) = delete;
   void operator=(const ListResult&) = delete;
};

class FunCall
   : protected Stack {
   void operator= (const FunCall&) = delete;
   FunCall(const FunCall&) = delete;   
public:
   FunCall(FunCall&&) = default;
   ~FunCall();

   template <typename... TArgs>
   static
   FunCall call_function(const AnyString& name, TArgs&&... args)
   {
      FunCall fc(false, name, count_args(std::forward<TArgs>(args)...));
      fc.push_args(std::forward<TArgs>(args)...);
      return fc;
   }

   template <typename... TArgs>
   static
   FunCall call_method(const AnyString& name, SV* obj_ref, TArgs&&... args)
   {
      FunCall fc(true, name, 1+count_args(std::forward<TArgs>(args)...));
      fc.push(obj_ref);
      fc.push_args(std::forward<TArgs>(args)...);
      return fc;
   }

   // single consumer: call in a scalar context
   template <typename Target,
             typename=typename std::enable_if<Concrete<Target>::value>::type>
   operator Target ()
   {
      return PropertyValue(call_scalar_context(), value_not_trusted);
   }

   operator ListResult ()
   {
      return ListResult(call_list_context(), *this);
   }

   template <typename Target>
   ListResult operator>> (Target&& x)
   {
      return std::move(ListResult(call_list_context(), *this) >> std::forward<Target>(x));
   }

protected:
   FunCall(bool is_method, const AnyString& name, int reserve);

   template <typename... TArgs>
   static
   typename std::enable_if<!mlist_or<is_instance_of<pure_type_t<TArgs>, Unrolled>...>::value, int>::type
   count_args(TArgs&&... args) { return sizeof...(TArgs); }

   template <typename TContainer, typename... MoreArgs>
   static
   int count_args(Unrolled<TContainer>&& c, MoreArgs&&... more_args)
   {
      return c.container.size()+count_args(std::forward<MoreArgs>(more_args)...);
   }

   template <typename FirstArg, typename... MoreArgs>
   static
   typename std::enable_if<!is_instance_of<FirstArg, Unrolled>::value && mlist_or<is_instance_of<pure_type_t<MoreArgs>, Unrolled>...>::value, int>::type
   count_args(FirstArg&& first_arg, MoreArgs&&... more_args)
   {
      return 1+count_args(std::forward<MoreArgs>(more_args)...);
   }

   void push_args() {}

   template <typename FirstArg, typename... MoreArgs>
   void push_args(FirstArg&& first_arg, MoreArgs&&... more_args)
   {
      push_arg(std::forward<FirstArg>(first_arg));
      push_args(std::forward<MoreArgs>(more_args)...);
   }

   template <typename TArg>
   typename std::enable_if<!is_instance_of<pure_type_t<TArg>, Unrolled>::value &&
                           !std::is_same<pure_type_t<TArg>, OptionSet>::value>::type
   push_arg(TArg&& arg)
   {
      Value v(value_allow_non_persistent | value_allow_store_any_ref);
      v.put(std::forward<TArg>(arg), 0);
      push_temp(v);
   }

   template <typename TContainerRef>
   void push_arg(Unrolled<TContainerRef>&& c)
   {
      for (auto it=entire(c.container); !it.at_end(); ++it) {
         Value v(value_allow_non_persistent | value_allow_store_any_ref);
         v.put(*it, 0);
         push_temp(v);
      }
   }

   void push_arg(FunCall&& x);

   void push_arg(const OptionSet& options)
   {
      push(options.get());
   }

   void push_arg(SV* sv)
   {
      push(sv);
   }

   SV* call_scalar_context();
   int call_list_context();

   SV* func;
   const char* method_name;

   friend class ListResult;
};

int get_debug_level();

} }

namespace polymake {

template <typename... Args>
pm::perl::FunCall call_function(const AnyString& name, Args&&... args)
{
   return pm::perl::FunCall::call_function(name, std::forward<Args>(args)...);
}

template <typename... Args>
[[deprecated("CallPolymakeFunction macros are deprecated, please use call_function() instead")]]
pm::perl::FunCall call_function_deprecated(const AnyString& name, Args&&... args)
{
   return pm::perl::FunCall::call_function(name, std::forward<Args>(args)...);
}

namespace perl {
using pm::perl::unroll;
}

}

#endif // POLYMAKE_PERL_CALLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
