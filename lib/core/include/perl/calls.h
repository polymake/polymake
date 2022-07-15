/* Copyright (c) 1997-2022
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

namespace pm { namespace perl {

class PropertyValue;
PropertyValue load_data(const std::string& filename);

template <typename T>
void save_data(const std::string& filename, const T& data, const std::string& description=std::string());

PropertyValue get_custom(const AnyString& name, const AnyString& key=AnyString());

class FunCall;

class PropertyValue : public Value {
   friend class BigObject;   friend class BigObjectType;
   friend class FunCall;

   friend PropertyValue load_data(const std::string& filename);

   template <typename T> friend
   void save_data(const std::string& filename, const T& data, const std::string& description);

   friend
   PropertyValue get_custom(const AnyString& name, const AnyString& key);

   PropertyValue() {}

   template <typename... Options>
   explicit PropertyValue(SV* sv_arg, Options... option_args)
      : Value(sv_arg)
   {
      set_options(option_args...);
   }

   void set_options() {}

   template <typename... MoreOptions>
   void set_options(ValueFlags flag, MoreOptions... more_options)
   {
      options |= flag;
      set_options(more_options...);
   }

   template <typename... MoreOptions>
   void set_options(allow_conversion, MoreOptions... more_options)
   {
      options |= ValueFlags::allow_conversion;
      set_options(more_options...);
   }

   void operator= (const PropertyValue&) = delete;

   static SV* load_data_impl(const std::string& filename);
   void save_data_impl(const std::string&, const std::string&);
public:
   PropertyValue(const PropertyValue& x);
   ~PropertyValue();
};

inline
Value::NoAnchors Value::put_val(const PropertyValue& x, int)
{
   set_copy(x);
   return NoAnchors();
}

inline
PropertyValue load_data(const std::string& filename)
{
   return PropertyValue(PropertyValue::load_data_impl(filename));
}

template <typename T>
void save_data(const std::string& filename, const T& data, const std::string& description)
{
   PropertyValue v;
   v << data;
   v.save_data_impl(filename, description);
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
         Value v(shift(), ValueFlags::allow_undef | ValueFlags::not_trusted);
         v >> x;
         v.forget();
      }
      return *this;
   }

   template <typename TContainerRef>
   void operator>> (Unrolled<TContainerRef>&& c)
   {
      Value v(sv, ValueFlags::not_trusted);
      v.retrieve_nomagic(c.container);
      forget();
      sv = nullptr;
   }

   void operator>> (Unrolled<Array&>&& c)
   {
      c.container = static_cast<Array&&>(*this);
   }

protected:
   ListResult(int items, FunCall& fc);

   friend class BigObject;
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
   FunCall(FunCall&& other)
      : Stack(std::move(other))
      , func(other.func)
      , method_name(other.method_name)
      , val_flags(other.val_flags)
   {
      other.val_flags = ValueFlags::is_mutable;
   }

   ~FunCall();

   template <typename... Args>
   static
   FunCall call_function(const AnyString& name, Args&&... args)
   {
      FunCall fc(false, name, count_args(std::forward<Args>(args)...));
      (void)std::initializer_list<bool>{ (fc.push_arg(std::forward<Args>(args)), true)... };
      return fc;
   }

   template <typename... Args>
   static
   FunCall call_method(const AnyString& name, SV* obj_arg, Args&&... args)
   {
      FunCall fc(true, name, 1 + count_args(std::forward<Args>(args)...));
      fc.push(obj_arg);
      (void)std::initializer_list<bool>{ (fc.push_arg(std::forward<Args>(args)), true)... };
      return fc;
   }

   // single consumer: call in a scalar context
   template <typename Target,
             typename=std::enable_if_t<Concrete<Target>::value>>
   operator Target ()
   {
      return PropertyValue(call_scalar_context(), ValueFlags::not_trusted);
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
   FunCall(std::nullptr_t, ValueFlags val_flags_, Int reserve);

   FunCall(std::nullptr_t, Int reserve)
      : FunCall(nullptr, ValueFlags::allow_non_persistent | ValueFlags::allow_store_any_ref, reserve) {}

   FunCall(bool is_method, ValueFlags val_flags_, const AnyString& name, Int reserve);

   FunCall(bool is_method, const AnyString& name, Int reserve)
      : FunCall(is_method, ValueFlags::allow_non_persistent | ValueFlags::allow_store_any_ref, name, reserve) {}

   template <typename Arg>
   using count_separately = typename mlist_or< is_instance_of<pure_type_t<Arg>, Unrolled>,
                                               is_instance_of<pure_type_t<Arg>, mlist> >::type;

   template <typename... Args>
   static constexpr
   std::enable_if_t<!mlist_or<count_separately<Args>...>::value, Int>
   count_args(Args&&... args)
   {
      return sizeof...(Args);
   }

   template <typename Container, typename... MoreArgs>
   static
   Int count_args(Unrolled<Container>&& c, MoreArgs&&... more_args)
   {
      return c.container.size() + count_args(std::forward<MoreArgs>(more_args)...);
   }

   template <typename... Types, typename... MoreArgs>
   static
   Int count_args(const mlist<Types...>&, MoreArgs&&... more_args)
   {
      return sizeof...(Types) + count_args(std::forward<MoreArgs>(more_args)...);
   }

   template <typename FirstArg, typename... MoreArgs>
   static
   std::enable_if_t<!count_separately<FirstArg>::value && mlist_or<count_separately<MoreArgs>...>::value, Int>
   count_args(FirstArg&& first_arg, MoreArgs&&... more_args)
   {
      return 1 + count_args(std::forward<MoreArgs>(more_args)...);
   }

   template <typename Arg>
   std::enable_if_t<!count_separately<Arg>::value &&
                    !is_among<pure_type_t<Arg>, OptionSet, AnyString, FunCall, SV*>::value>
   push_arg(Arg&& arg)
   {
      Value v(val_flags);
      v.put(std::forward<Arg>(arg));
      push(v.get_temp());
   }

   template <typename ContainerRef>
   void push_arg(Unrolled<ContainerRef>&& c)
   {
      for (auto it=entire(c.container); !it.at_end(); ++it) {
         Value v(val_flags);
         v.put(*it);
         push(v.get_temp());
      }
   }

   template <typename... Types>
   void push_types(const mlist<Types...>&)
   {
      (void)std::initializer_list<bool>{ (push_type(type_cache<pure_type_t<Types>>::get_proto()), true)... };
   }

   void push_type(SV* proto)
   {
      if (proto)
         push(proto);
      else
         throw Undefined();
   }

   template <typename... Types>
   void push_arg(const mlist<Types...>& t)
   {
      push_types(t);
      if (sizeof...(Types) != 0) create_explicit_typelist(sizeof...(Types));
   }

   void push_current_application();
   void push_current_application_pkg();
   void create_explicit_typelist(size_t size);

   void push_arg(FunCall&& x);

   void push_arg(const OptionSet& options)
   {
      push(options.get());
   }

   void push_arg(const AnyString& s)
   {
      push(s);
   }

   void push_arg(SV* sv)
   {
      push(sv);
   }

   void check_call();

   SV* call_scalar_context();
   int call_list_context();

   SV* func;
   const char* method_name;
   ValueFlags val_flags;

   friend class ListResult;
};


class VarFunCall
   : protected FunCall {
   void operator= (const VarFunCall&) = delete;
   VarFunCall(const VarFunCall&) = delete;
public:
   VarFunCall(VarFunCall&&) = default;

   static
   VarFunCall prepare_call_function(const AnyString& name)
   {
      return VarFunCall(false, ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref, name, 0);
   }

   template <typename TypeParams>
   static
   VarFunCall prepare_call_function(const AnyString& name, const TypeParams& explicit_type_params)
   {
      VarFunCall fc{prepare_call_function(name)};
      if (explicit_type_params.size() != 0) {
         fc.begin_type_params(explicit_type_params.size());
         for (const auto& param : explicit_type_params)
            fc.push_type_param(param);
         fc.end_type_params();
      }
      return fc;
   }

   static
   VarFunCall prepare_call_method(const AnyString& name, SV* obj_arg)
   {
      VarFunCall fc(true, ValueFlags::allow_non_persistent | ValueFlags::allow_store_ref, name, 1);
      fc.push(obj_arg);
      return fc;
   }

   template <typename Arg>
   VarFunCall& operator<< (Arg&& arg)
   {
      check_push();
      extend(count_args(std::forward<Arg>(arg)));
      push_arg(std::forward<Arg>(arg));
      return *this;
   }

   // seal the argument list, allowing calls
   FunCall operator()()
   {
      // the actual call is performed in the conversion operator which depends on the context
      return std::move(*this);
   }

   template <typename Target>
   operator Target () = delete;

   template <typename Target>
   ListResult operator>> (Target&& x) = delete;

protected:
   using FunCall::FunCall;

   void begin_type_params(size_t n);
   void push_type_param(const AnyString& param);
   void end_type_params();

   void check_push() const;
};


template <typename T, typename... Params>
class CachedObjectPointer {
   CachedObjectPointer(const CachedObjectPointer&) = delete;
public:
   CachedObjectPointer(CachedObjectPointer&& other)
      : ptr(std::move(other.ptr))
      , reset_on_destruction(other.reset_on_destruction)
   {
      other.reset_on_destruction = false;
   }

   CachedObjectPointer& operator= (const CachedObjectPointer& other)
   {
      ptr = other.ptr;
      return *this;
   }

   explicit CachedObjectPointer(T* ptr_, bool reset_on_destruction_=false)
      : ptr(std::make_shared<std::unique_ptr<T>>(ptr_))
      , reset_on_destruction(reset_on_destruction_) {}

   explicit CachedObjectPointer(const AnyString& function_name_)
      : function_name(function_name_)
      , ptr(std::make_shared<std::unique_ptr<T>>(nullptr))
      , reset_on_destruction(false) {}

   ~CachedObjectPointer()
   {
      if (reset_on_destruction) ptr->reset(nullptr);
   }

   template <typename... Args>
   T& get(Args&&... args)
   {
      if (!(*ptr)) FunCall::call_function(function_name, mlist<Params...>(), std::forward<Args>(args)...) >> *this;
      return **ptr;
   }

   void set(T* obj) { ptr.reset(obj); }

private:
   const AnyString function_name;
   std::shared_ptr<std::unique_ptr<T>> ptr;
   bool reset_on_destruction = false;
};

Int get_debug_level();

class PropertyTypeBuilder
   : protected FunCall {
public:
   using FunCall::FunCall;

   template <typename... Params, bool exact_match>
   static SV* build(const AnyString& pkg_name, const mlist<Params...>& params, bool_constant<exact_match>)
   {
      try {
         PropertyTypeBuilder b(true, "typeof", 1 + sizeof...(Params));
         b.push_arg(pkg_name);
         b.push_types(params);
         if (!exact_match) b.nonexact_match();
         return b.call_scalar_context();
      }
      catch (const Undefined&) {
         return nullptr;
      }
   }
private:
   void nonexact_match();
};

} }

namespace polymake {

template <typename... Args>
pm::perl::FunCall call_function(const AnyString& name, Args&&... args)
{
   return pm::perl::FunCall::call_function(name, std::forward<Args>(args)...);
}

inline
pm::perl::VarFunCall prepare_call_function(const AnyString& name)
{
   return pm::perl::VarFunCall::prepare_call_function(name);
}

template <typename TypeParams, typename = std::enable_if_t<std::is_constructible<AnyString, typename pm::container_traits<TypeParams>::value_type>::value>>
pm::perl::VarFunCall prepare_call_function(const AnyString& name, const TypeParams& explicit_type_params)
{
   return pm::perl::VarFunCall::prepare_call_function(name, explicit_type_params);
}

using pm::perl::CachedObjectPointer;

namespace perl {
using pm::perl::unroll;
}

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
