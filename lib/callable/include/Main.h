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
   //! Initialize polymake for use in the current thread
   //!
   //! Only the first invocation in the program evaluates its arguments,
   //! all subsequent invocations are only needed to register already initialized polymake system with other threads.
   //! The calling application is responsible to prevent simultaneous execution of any polymake functions in several threads,
   //! which includes all methods of this class, Scope, BigObject, BigObjectType, as well as call_function, prepare_call_function and
   //! any other functions requiring interaction with polymake perl interpreter.
   //!
   //! @param user_opts configuration path, corresponds to --config-path option of the standalone main script
   //!                  several path elements should be separated with ;
   //! @param install_top override the location of polymake installation top (shared) directory specified during initial pre-build configuration
   //! @param install_arch override the location of polymake installation binary directory specified during initial pre-build configuration
   explicit Main(const std::string& user_opts = "user",
                 std::string install_top = "",
                 std::string install_arch = "");

   //! Get a greeting message
   //! @param verbose integer between 0 and 2, larger value means longer text
   std::string greeting(int verbose = 2);

   //! Select the current application by name
   void set_application(const AnyString& appname);

   //! Select the current application by a big object belonging to it
   void set_application_of(const BigObject& x);

   //! Load an extension installed at the given path
   //! shell_enable() must be called prior to this
   void add_extension(const AnyString& path);

   //! Load a rulefile specified by given path
   void include(const AnyString& path);

   //! Set a preference list
   //! @param label_exp same as in the interactive shell command
   void set_preference(const AnyString& label_exp);

   //! Reset a preference list to default value as defined in the rules
   //! @param label_exp same as in the interactive shell command
   void reset_preference(const AnyString& label_exp);

   //! Set a custom variable to a given value
   template <typename T>
   void set_custom(const AnyString& name, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, AnyString(), x);
   }

   //! Set a custom hash entry to a given value
   template <typename T>
   void set_custom(const AnyString& name, const AnyString& key, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, key, x);
   }

   //! Reset a custom variable or a custom hash entry to its default value as defined in the rules
   void reset_custom(const AnyString& name, const AnyString& key = AnyString());

   //! Construct a new Scope object
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
   using shell_execute_t = std::tuple<bool, std::string, std::string, std::string>;
   shell_execute_t shell_execute(const std::string& input);

   //! get all possible completions for the partial input string, as if the TAB key has been pressed at the end of the string
   //! @return a tuple of three elements:
   //!         <0> offset from the end of the input string to the position where all completion proposals can be applied;
   //!             in other words, length of the intersection of the input string and the proposals
   //!         <1> optional delimiter character which can be appended after every completion proposal
   //!         <2> array of completion proposals
   using shell_complete_t = std::tuple<int, char, std::vector<std::string>>;
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

   //! Install a signal handler for the given signal.
   //!
   //! The signal can be SIGINT or a custom interceptible signal like SUGUSR1.
   //!
   //! If the application handles signals in a thread other than where polymake functions are executed,
   //! it must deliver the signal agreed upon here to polymake thread using pthread_kill().
   //! It is in the caller's responsibility to ensure that the signal is not blocked by polymake's thread signal mask.
   //!
   //! If the signal arrives while polymake is busy e.g. computing object properties or executing a user function,
   //! it will eventually stop and throw an exception, returning to the caller.  If the signal arrives during shell_execute,
   //! the exception will be reported as part of the result tuple.
   //! Please be aware that the break can happen with arbitrarily large delay, because inner loops in pure C/C++ code not communicating
   //! with polymake server are not interruptible yet.
   void set_interrupt_signal(int signum);

   //! Uninstall the interrupt signal handler installed earlier via set_interrupt_signal().
   void reset_interrupt_signal();

private:
   SV* lookup_extension(const AnyString& path); // currently unused
   void call_app_method(const char* method, const AnyString& arg);

   void set_custom_var(const AnyString& name, const AnyString& key, Value& x);
};

class Scope {
   friend class Main;
public:
   Scope(Scope&& s)
      : pm_main(s.pm_main)
      , saved(s.saved)
      , id(s.id)
   {
      s.saved = nullptr;
   }

   ~Scope();

   //! Set a temporary preference list
   //! It is reverted when this Scope object is destroyed
   //! @param labels same as in the interactive shell command
   void prefer_now(const AnyString& labels) const;

   //! Assign a temporary value to a custom variable
   //! It is reverted when this Scope object is destroyed
   template <typename T>
   void set_custom(const AnyString& name, T&& value)
   {
      Value x;
      x << std::forward<T>(value);
      set_custom_var(name, AnyString(), x);
   }

   //! Assign a temporary value to a custom hash entry
   //! It is reverted when this Scope object is destroyed
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
using pm::perl::Scope;

}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
