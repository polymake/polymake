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

#include "polymake/perl/glue.h"
#include "polymake/Main.h"

namespace pm { namespace perl {

namespace {

glue::cached_cv application_cv={ "Polymake::User::application", 0 },
            app_from_object_cv={ "Polymake::Main::application_from_object", 0 },
                  new_scope_cv={ "Polymake::Main::createNewScope", 0 },
                 set_custom_cv={ "Polymake::Main::set_custom", 0 },
               reset_custom_cv={ "Polymake::Main::reset_custom", 0 },
               local_custom_cv={ "Polymake::Main::local_custom", 0 },
               greeting_cv={ "Polymake::Main::greeting", 0 },
               simulate_shell_cv={ "Polymake::Main::simulate_shell_input", 0 };

const char Extension[]="Polymake::Core::Extension";
}

void Main::set_application(const AnyString& appname)
{
   dTHXa(pi);
   PmStartFuncall(1);
   mPUSHp(appname.ptr, appname.len);
   PUTBACK;
   glue::call_func_void(aTHX_ application_cv);
}

void Main::set_application_of(const Object& x)
{
   dTHXa(pi);
   PmStartFuncall(1);
   PUSHs(x.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ app_from_object_cv);
}

void Main::add_extension(const AnyString& path)
{
   dTHXa(pi);
   PmStartFuncall(2);
   mPUSHp(Extension, sizeof(Extension)-1);
   mPUSHp(path.ptr, path.len);
   PUTBACK;
   glue::call_method_void(aTHX_ "add");
}

void Main::include(const AnyString& path)
{
   call_app_method("include_rules", path); 
}

void Main::set_preference(const AnyString& label_exp)
{ 
   call_app_method("set_preference", label_exp); 
}

void Main::reset_preference(const AnyString& label_exp)
{ 
   call_app_method("reset_preference", label_exp); 
}

SV* Main::lookup_extension(const AnyString& path)
{
   dTHXa(pi);
   PmStartFuncall(2);
   mPUSHp(Extension, sizeof(Extension)-1);
   mPUSHp(path.ptr, path.len);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

void Main::call_app_method(const char* method, const AnyString& arg)
{
   dTHXa(pi);
   PmStartFuncall(2);
   SP=glue::push_current_application(aTHX_ SP);
   mPUSHp(arg.ptr, arg.len);
   PUTBACK;
   glue::call_method_void(aTHX_ method);
}

void Main::set_custom_var(const AnyString& name, const AnyString& key, Value& x)
{
   dTHXa(pi);
   PmStartFuncall(3);
   mPUSHp(name.ptr, name.len);
   if (key.ptr) mPUSHp(key.ptr, key.len);
   PUSHs(x.get_temp());
   PUTBACK;
   glue::call_func_void(aTHX_ set_custom_cv);
}

void Main::reset_custom(const AnyString& name, const AnyString& key)
{
   dTHXa(pi);
   PmStartFuncall(2);
   mPUSHp(name.ptr, name.len);
   if (key.ptr) mPUSHp(key.ptr, key.len);
   PUTBACK;
   glue::call_func_void(aTHX_ reset_custom_cv);
}

Scope Main::newScope()
{
   dTHXa(pi);
   PmStartFuncall(0);
   return Scope(this, call_func_scalar(aTHX_ new_scope_cv));
}

void Scope::prefer_now(const AnyString& labels) const
{
   pm_main->call_app_method("prefer_now", labels);
}

void Scope::set_custom_var(const AnyString& name, const AnyString& key, Value& x) const
{
   dTHXa(pm_main->pi);
   PmStartFuncall(3);
   mPUSHp(name.ptr, name.len);
   if (key.ptr) mPUSHp(key.ptr, key.len);
   PUSHs(x.get_temp());
   PUTBACK;
   glue::call_func_void(aTHX_ local_custom_cv);
}

std::string Main::greeting(int verbose)
{
   dTHXa(pi);
   PmStartFuncall(1);
   mPUSHi(verbose);
   PUTBACK;
   return glue::call_func_string(aTHX_ greeting_cv);
}

std::string Main::simulate_shell_input(const std::string& input)
{
   if (input.empty()) return input;
   dTHXa(pi);
   PmStartFuncall(1);
   mPUSHp(input.c_str(), input.size());
   PUTBACK;
   return glue::call_func_string(aTHX_ simulate_shell_cv, false);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
