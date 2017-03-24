/* Copyright (c) 1997-2017
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
public:
   explicit Main(const std::string& user_opts="user",
                 const std::string& install_top="",
                 const std::string& install_arch="");

   std::string greeting(int verbose=2);

   void set_application(const AnyString& appname);

   void set_application_of(const Object& x);

   void add_extension(const AnyString& path);

   void include(const AnyString& path);

   void set_preference(const AnyString& label_exp);

   void reset_preference(const AnyString& label_exp);

   template <typename T>
   void set_custom(const AnyString& name, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, AnyString(), x);
   }

   template <typename T>
   void set_custom(const AnyString& name, const AnyString& key, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, key, x);
   }

   void reset_custom(const AnyString& name, const AnyString& key = AnyString());

   Scope newScope();

   friend class Scope;

   //! evaluate the given perl code in the context of the current application as if it has been entered in an interactive shell
   //! the code must be syntactically complete, that is, no unfinished blocks or dangling operators are allowed
   //! @return the output produced by the code
   //! syntax or runtime errors are reported via exception
   std::string simulate_shell_input(const std::string& input);
private:
   SV* lookup_extension(const AnyString& path); // currently unused
   void call_app_method(const char* method, const AnyString& arg);

   void set_custom_var(const AnyString& name, const AnyString& key, Value& x);

   //! cached address of the perl interpreter object
   void* pi;
};

class Scope {
   friend class Main;
public:
   Scope(Scope&& s)
      : pm_main(s.pm_main)
      , saved(s.saved)
      , id(s.id)
   {
      s.saved=nullptr;
   }

   ~Scope();

   void prefer_now(const AnyString& labels) const;

   template <typename T>
   void set_custom(const AnyString& name, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, AnyString(), x);
   }

   template <typename T>
   void set_custom(const AnyString& name, const AnyString& key, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, key, x);
   }

private:
   Scope(Main* main_arg, SV* sv)
      : pm_main(main_arg)
      , saved(sv)
      , id(++depth) {}

   Scope(const Scope& s) = delete;
   Scope& operator= (const Scope&) = delete;

   void set_custom_var(const AnyString& name, const AnyString& key, Value& value) const;

   static unsigned int depth;
   Main* pm_main;
   SV* saved;
   unsigned int id;
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
