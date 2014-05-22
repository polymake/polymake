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

#include "polymake/perl/glue.h"
#include "polymake/Main.h"

namespace pm { namespace perl {

namespace {

glue::cached_cv application_cv={ "Polymake::User::application", 0 },
            app_from_object_cv={ "Polymake::Main::application_from_object", 0 },
                  new_scope_cv={ "Polymake::Main::createNewScope", 0 },
                 set_custom_cv={ "Polymake::Main::set_custom", 0 },
               reset_custom_cv={ "Polymake::Main::reset_custom", 0 },
               local_custom_cv={ "Polymake::Main::local_custom", 0 };

const char Extension[]="Polymake::Core::Extension";
}

void Main::set_application(const char* name, size_t nl)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(name, nl);
   PUTBACK;
   glue::call_func_void(aTHX_ application_cv);
}

void Main::set_application_of(const Object& x)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(x.obj_ref);
   PUTBACK;
   glue::call_func_void(aTHX_ app_from_object_cv);
}

void Main::add_extension(const char* path, size_t pl)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(Extension, sizeof(Extension)-1);
   mXPUSHp(path, pl);
   PUTBACK;
   glue::call_method_void(aTHX_ "add");
}

SV* Main::lookup_extension(const char* path, size_t pl)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(Extension, sizeof(Extension)-1);
   mXPUSHp(path, pl);
   PUTBACK;
   return glue::call_method_scalar(aTHX_ "lookup");
}

void Main::call_app_method(const char* method, const char *arg, size_t argl)
{
   dTHX;
   PmStartFuncall;
   SP=glue::push_current_application(aTHX_ SP);
   mXPUSHp(arg, argl);
   PUTBACK;
   glue::call_method_void(aTHX_ method);
}

void Main::_set_custom(const char* name, size_t ll, const char* key, size_t kl, Value& x)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(name, ll);
   if (key) mXPUSHp(key, kl);
   XPUSHs(x.get_temp());
   PUTBACK;
   glue::call_func_void(aTHX_ set_custom_cv);
}

void Main::_reset_custom(const char* name, size_t ll, const char* key, size_t kl)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(name, ll);
   if (key) mXPUSHp(key, kl);
   PUTBACK;
   glue::call_func_void(aTHX_ reset_custom_cv);
}

Scope Main::newScope()
{
   dTHX;
   PmStartFuncall;
   PUTBACK;
   return call_func_scalar(aTHX_ new_scope_cv);
}

void Scope::_set_custom(const char* name, size_t ll, const char* key, size_t kl, Value& x)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(name, ll);
   if (key) mXPUSHp(key, kl);
   XPUSHs(x.get_temp());
   PUTBACK;
   glue::call_func_void(aTHX_ local_custom_cv);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
