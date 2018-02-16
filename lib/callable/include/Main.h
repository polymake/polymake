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
#include "polymake/vector"
#include <string>

namespace pm { namespace perl {

class Scope;

class Main {
public:
   explicit Main(const std::string& user_opts="user",
                 std::string install_top="",
                 std::string install_arch="");

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

   //! Load additional modules providing shell functionality.
   //! This should be called prior to loading any application, otherwise methods querying
   //! command completion and context help won't work.
   void shell_enable();

   //! Execute a piece of polymake/perl code in the context of the current application as if it has been entered in an interactive shell
   //! The input must be encoded in UTF-8.
   //! @return a tuple of four elements:
   //!         <0> boolean indicating whether the input string could be parsed and executed
   //!         <1> the entire stdout output produced during the execution, in particular, results of print statements;
   //!             empty when <0> is false
   //!         <2> the entire stderr output produced during the execution, this may include various harmless warnings and credit notes;
   //!             empty when <0> is false
   //!         <3> the message of an exception raised during the execution (when <0> is true) or input parse error message (when <0> is false)
   //! Note that the result for a syntactically correct but incomplete input (unfinished statement, open block, etc.)
   //! will be {false, "", "", ""}.
   using shell_execute_t=std::tuple<bool, std::string, std::string, std::string>;
   shell_execute_t shell_execute(const std::string& input);

   //! get all possible completions for the partial input string, as if the TAB key has been pressed at the end of the string
   //! @return a tuple of three elements:
   //!         <0> offset from the end of the input string to the position where all completion proposals can be applied;
   //!             in other words, length of the intersection of the input string and the proposals
   //!         <1> optional delimiter character which can be appended after every completion proposal
   //!         <2> array of completion proposals
   using shell_complete_t=std::tuple<int, char, std::vector<std::string>>;
   shell_complete_t shell_complete(const std::string& input);

   //! get documentation strings describing an item in the input string near given position
   //! multiple results are possible when the input can't be parsed unambiguously,
   //! e.g. when the item in question refers to an overloaded function
   //! @param input a piece of polymake/perl code
   //! @param pos cursor position within the input string; results will describe the item
   //!            under the cursor or at the closest location towards the begin
   //!            By default, end of line is assumed
   //! @param full provide full documentation texts including parameter descriptions, examples, etc.
   //! @param html documentation texts may include HTML markup
   std::vector<std::string> shell_context_help(const std::string& input, size_t pos=std::string::npos, bool full=false, bool html=false);

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
