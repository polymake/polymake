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

#ifndef POLYMAKE_PERL_CALLS_H
#define POLYMAKE_PERL_CALLS_H

namespace pm { namespace perl {

class PropertyValue;
PropertyValue load_data(const std::string& filename);

template <typename T> inline
void save_data(const std::string& filename, const T& data, const std::string& description=std::string());

SV* get_custom_var(const char* name, size_t ll, const char* key, size_t kl);

template <size_t ll>
PropertyValue get_custom(const char (&name)[ll]);
PropertyValue get_custom(const std::string& name);
template <size_t ll>
PropertyValue get_custom(const char (&name)[ll], const std::string& key);
PropertyValue get_custom(const std::string& name, const std::string& key);

class PropertyValue : public Value {
   friend class Object;   friend class ObjectType;
   friend class FunCall;

   friend PropertyValue load_data(const std::string& filename);

   template <typename T> friend
   void save_data(const std::string& filename, const T& data, const std::string& description);

   template <size_t ll> friend
   PropertyValue get_custom(const char (&name)[ll]);
   friend
   PropertyValue get_custom(const std::string& name);
   template <size_t ll> friend
   PropertyValue get_custom(const char (&name)[ll], const std::string& key);
   friend
   PropertyValue get_custom(const std::string& name, const std::string& key);

   PropertyValue() {}
   explicit PropertyValue(SV* sv_arg, value_flags opt_arg=value_flags(0)) :
      Value(sv_arg, opt_arg) {}

   // inhibited
   void operator= (const PropertyValue&);

   static SV* _load_data(const std::string& filename);
   void _save_data(const std::string&, const std::string&);
public:
   PropertyValue(const PropertyValue& x);
   ~PropertyValue();
};

inline
Value::AnchorChain Value::put(const PropertyValue& x, const char*, int)
{
   set_copy(x);
   return AnchorChain();
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

class ListResult : protected Array {
   friend class FunCall;  friend class Object;
protected:
   ListResult(int items, const FunCall&);
public:
   template <typename Target>
   ListResult& operator>> (Target& x)
   {
      if (size() == 0)
         throw std::runtime_error("no more values in the result list");
      Value v(shift(), value_flags(value_allow_undef | value_not_trusted));
      v >> x;
      v.forget();
      return *this;
   }

   using Array::size;
   using Array::operator[];
   using Array::begin;
   using Array::end;
};

class TempOptionsPendingVal;

class TempOptions : protected HashHolder {
   friend class FunCall;  friend class Object;
protected:
   Value val;
public:
   TempOptions() : val(NULL) {};

   // const return types prevent an odd option list from being consumed by FunCall

   const TempOptionsPendingVal& operator, (const std::string& key);

   template <size_t ll>
   const TempOptionsPendingVal& operator, (const char (&key)[ll]);
};

class TempOptionsPendingVal : public TempOptions {
protected:
   TempOptionsPendingVal();
   ~TempOptionsPendingVal();
public:
   template <typename T>
   TempOptions& operator, (const T& x) const
   {
      val << x;
      return const_cast<TempOptionsPendingVal&>(*this);
   }
};

inline
const TempOptionsPendingVal& TempOptions::operator, (const std::string& key)
{
   val=_access(key.c_str(), key.size(), true);
   return static_cast<const TempOptionsPendingVal&>(*this);
}

template <size_t ll> inline
const TempOptionsPendingVal& TempOptions::operator, (const char (&key)[ll])
{
   val=_access(key, ll-1, true);
   return static_cast<const TempOptionsPendingVal&>(*this);
}

class FunCall : protected Stack {
   friend class Object;  friend class ListResult;
protected:
   mutable SV* func;
   void prepare_function_call(const char* name, size_t nl);

   SV* call() const;
   int list_call() const;
   SV* evaluate_method(SV* obj, const char* name) const;
   int list_evaluate_method(SV* obj, const char* name) const;
   void void_evaluate_method(SV* obj, const char* name) const;
   void push_arg_list(SV* args) const;
private:
   // inhibited
   void operator= (const FunCall&);
public:
   // prepare method call
   FunCall();
   
   explicit FunCall(const std::string& name) : Stack(false,1)
   {
      prepare_function_call(name.c_str(), name.size());
   }

   template <size_t nl>
   explicit FunCall(const char (&name)[nl]) : Stack(false,1)
   {
      prepare_function_call(name, nl-1);
   }

   ~FunCall()
   {
      if (__builtin_expect(func != NULL, 0)) cancel();
   }

   // gathering arguments
   template <typename Arg>
   const FunCall& operator, (const Arg& arg) const
   {
      Value v(value_allow_non_persistent);
      v << arg;
      push(v.get_temp());
      return *this;
   }

   const FunCall& operator, (ArgList& args) const
   {
      push_arg_list(args.get_temp());
      return *this;
   }

   const FunCall& operator, (TempOptions& args) const
   {
      push(args.get_temp());
      return *this;
   }

   PropertyValue evaluate() const
   {
      return PropertyValue(call(), value_flags(value_allow_undef | value_not_trusted));
   }

   ListResult list_evaluate() const
   {
      return ListResult(list_call(), *this);
   }

   void void_evaluate() const;
};


template <size_t ll> inline
PropertyValue get_custom(const char (&name)[ll])
{
   return PropertyValue(get_custom_var(name, ll-1, NULL, 0), value_allow_undef);
}
inline
PropertyValue get_custom(const std::string& name)
{
   return PropertyValue(get_custom_var(name.c_str(), name.size(), NULL, 0), value_allow_undef);
}
template <size_t ll> inline
PropertyValue get_custom(const char (&name)[ll], const std::string& key)
{
   return PropertyValue(get_custom_var(name, ll-1, key.c_str(), key.size()), value_allow_undef);
}
inline
PropertyValue get_custom(const std::string& name, const std::string& key)
{
   return PropertyValue(get_custom_var(name.c_str(), name.size(), key.c_str(), key.size()), value_allow_undef);
}

int get_debug_level();

} }

#endif // POLYMAKE_PERL_CALLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
