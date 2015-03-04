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

#ifndef POLYMAKE_MAIN_H
#define POLYMAKE_MAIN_H

#ifdef POLYMAKE_APPNAME
#error polymake::Main cannot be used in clients
#endif

#ifndef POLYMAKE_DEBUG
#define POLYMAKE_DEBUG 0
#endif

// for classes defined in external application code
#define POLYMAKE_APPNAME unknown

#include "polymake/client.h"
#include <string>

namespace pm { namespace perl {

class Scope;

class Main {
private:
   static void set_application(const char* appname, size_t ll);
   static void add_extension(const char* path, size_t ll);
   static SV* lookup_extension(const char* path, size_t ll); // currently unused
   static void call_app_method(const char* method, const char *arg, size_t argl);

   static void _set_custom(const char* name, size_t ll, const char* key, size_t kl, Value& x);
   static void _reset_custom(const char* name, size_t ll, const char* key, size_t kl);
public:
   explicit Main(const std::string& user_opts="user",
                 const std::string& install_top="",
                 const std::string& install_arch="");

   std::string greeting(int verbose=2);

   void set_application(const std::string& appname)
   {
      set_application(appname.c_str(), appname.size());
   }

   template <size_t ll>
   void set_application(const char (&appname)[ll])
   {
      set_application(appname, ll-1);
   }

   void set_application_of(const Object& x);

   void add_extension(const std::string& path)
   {
      add_extension(path.c_str(), path.size());
   }

   template <size_t ll>
   void add_extension(const char (&path)[ll])
   {
      add_extension(path, ll-1);
   }

   template <size_t pl>
   void include(const char (&path)[pl]) 
   {
      call_app_method("include_rules", path, pl-1); 
   }

   void include(const std::string& path) 
   {
      call_app_method("include_rules", path.c_str(), path.size()); 
   }

   template <size_t ll>
   void set_preference(const char (&label_exp)[ll])
   { 
      call_app_method("set_preference", label_exp, ll-1); 
   }
   void set_preference(const std::string& label_exp)
   { 
      call_app_method("set_preference", label_exp.c_str(), label_exp.size()); 
   }

   template <size_t ll>
   void reset_preference(const char (&label_exp)[ll])
   { 
      call_app_method("reset_preference", label_exp, ll-1); 
   }
   void reset_preference(const std::string& label_exp)
   { 
      call_app_method("reset_preference", label_exp.c_str(), label_exp.size()); 
   }

   template <size_t ll, typename T>
   void set_custom(const char (&name)[ll], const T& value)
   {
      Value x;
      x << value;
      _set_custom(name, ll-1, NULL, 0, x);
   }
   template <typename T>
   void set_custom(const std::string& name, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name.c_str(), name.size(), NULL, 0, x);
   }

   template <size_t ll, typename T>
   void set_custom(const char (&name)[ll], const std::string& key, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name, ll-1, key.c_str(), key.size(), x);
   }
   template <typename T>
   void set_custom(const std::string& name, const std::string& key, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name.c_str(), name.size(), key.c_str(), key.size(), x);
   }

   template <size_t ll>
   void reset_custom(const char (&name)[ll])
   {
      _reset_custom(name, ll-1, NULL, 0);
   }
   void reset_custom(const std::string& name)
   {
      _reset_custom(name.c_str(), name.size(), NULL, 0);
   }
   template <size_t ll>
   void reset_custom(const char (&name)[ll], const std::string& key)
   {
      _reset_custom(name, ll-1, key.c_str(), key.size());
   }
   void reset_custom(const std::string& name, const std::string& key)
   {
      _reset_custom(name.c_str(), name.size(), key.c_str(), key.size());
   }

   Scope newScope();

   friend class Scope;
};

class Scope {
   friend class Main;
private:
   static unsigned int depth;
   mutable SV *saved;
   unsigned int id;
   Scope(SV *sv) : saved(sv), id(++depth) {}

   // inhibited
   Scope& operator= (const Scope&);

   static void _set_custom(const char* name, size_t ll, const char* key, size_t kl, Value& x);

public:
   Scope(const Scope& s) : saved(s.saved), id(s.id) { s.saved=NULL; }

   ~Scope();

   void prefer_now(const std::string& labels) const
   {
      Main::call_app_method("prefer_now", labels.c_str(), labels.size());
   }
   template <size_t ll>
   void prefer_now(const char (&labels)[ll]) const
   {
      Main::call_app_method("prefer_now", labels, ll-1);
   }

   template <size_t ll, typename T>
   void set_custom(const char (&name)[ll], const T& value)
   {
      Value x;
      x << value;
      _set_custom(name, ll-1, NULL, 0, x);
   }
   template <typename T>
   void set_custom(const std::string& name, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name.c_str(), name.size(), NULL, 0, x);
   }

   template <size_t ll, typename T>
   void set_custom(const char (&name)[ll], const std::string& key, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name, ll-1, key.c_str(), key.size(), x);
   }
   template <typename T>
   void set_custom(const std::string& name, const std::string& key, const T& value)
   {
      Value x;
      x << value;
      _set_custom(name.c_str(), name.size(), key.c_str(), key.size(), x);
   }
};

} }

namespace polymake {

using pm::perl::Main;

namespace perl {

using pm::perl::Scope;

} }

#endif // POLYMAKE_MAIN_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
