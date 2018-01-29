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

#include "polymake/perl/glue.h"
#include "polymake/perl/calls.h"

namespace pm { namespace perl {

namespace {

glue::cached_cv load_data_cv{ "Polymake::User::load_data" },
                save_data_cv{ "Polymake::User::save_data" },
               get_custom_cv{ "Polymake::Core::Application::get_custom_var" };
}

PropertyValue::PropertyValue(const PropertyValue& x)
   : Value(x.sv)
{
   SvREFCNT_inc_simple_void(sv);
}

PropertyValue::~PropertyValue()
{
   dTHX;
   SvREFCNT_dec(sv);
}

SV* PropertyValue::_load_data(const std::string& filename)
{
   dTHX;
   PmStartFuncall(1);
   mPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   return glue::call_func_scalar(aTHX_ load_data_cv);
}

void PropertyValue::_save_data(const std::string& filename, const std::string& description)
{
   dTHX;
   PmStartFuncall(3);
   PUSHs(sv);
   mPUSHp(filename.c_str(), filename.size());
   mPUSHp(description.c_str(), description.size());
   PUTBACK;
   glue::call_func_void(aTHX_ save_data_cv);
}

PropertyValue get_custom(const AnyString& name, const AnyString& key)
{
   dTHX;
   PmStartFuncall(2);
   mPUSHp(name.ptr, name.len);
   if (key) mPUSHp(key.ptr, key.len);
   PUTBACK;
   return PropertyValue(glue::call_func_scalar(aTHX_ get_custom_cv), value_allow_undef);
}

FunCall::FunCall(bool is_method, const AnyString& name, int reserve)
   : Stack(reserve)
   , func(nullptr)
   , method_name(nullptr)
{
   dTHXa(pi);
   if (is_method) {
      func=&PL_sv_undef;
      method_name=name.ptr;
   } else {
      dSP;
      SP=glue::push_current_application(aTHX_ SP);
      SV* const app=POPs;
      PUTBACK;
      if (!(func=(SV*)glue::namespace_lookup_sub(aTHX_ glue::User_stash, name.ptr, name.len, (CV*)SvRV(PmArray(app)[glue::Application_eval_expr_index])))) {
         PmCancelFuncall;
         throw std::runtime_error("polymake function " + name + " not found");
      }
   }
}

FunCall::~FunCall()
{
   if (func) {
      dTHXa(pi);
      if (std::uncaught_exception()) {
         // error during preparation of arguments
         PmCancelFuncall;
      } else {
         // call not triggered because result not consumed yet: must be a void context
         if (method_name) {
            func=nullptr;
            glue::call_method_void(aTHX_ method_name);
         } else {
            SV* sv=func;  func=nullptr;
            glue::call_func_void(aTHX_ sv);
         }
      }
   }
}

SV* FunCall::call_scalar_context()
{
   dTHXa(pi);
   if (method_name) {
      func=nullptr;
      return glue::call_method_scalar(aTHX_ method_name);
   } else {
      SV* sv=func;  func=nullptr;
      return glue::call_func_scalar(aTHX_ sv);
   }
}

int FunCall::call_list_context()
{
   dTHXa(pi);
   if (method_name) {
      func=nullptr;
      return glue::call_method_list(aTHX_ method_name);
   } else {
      SV* sv=func;  func=nullptr;
      return glue::call_func_list(aTHX_ sv);
   }
}

void FunCall::push_arg(FunCall&& x)
{
   dTHXa(pi);
   push(sv_2mortal(x.call_scalar_context()));
}

ListResult::ListResult(int items, FunCall& fc)
   : Array(items)
{
   if (items) {
      dTHXa(fc.pi);
      dSP;
      SV **dst=PmArray(sv)+items-1;
      do {
         if (SvTEMP(*SP)) SvREFCNT_inc_simple_void_NN(*SP);
         *dst=*SP;
         --dst; --SP;
      } while (--items>0);
      PUTBACK; FREETMPS; LEAVE;
   }
}

int get_debug_level()
{
   SV* dbg=GvSV(glue::Debug_level);
   return SvIOK(dbg) ? SvIVX(dbg) : 0;
}

namespace glue {

HV* current_application_pkg(pTHX)
{
   if (cur_wrapper_cv) {
      return CvSTASH(glue::cur_wrapper_cv);
   } else {
      SV* const current_app=GvSV(User_application);
      if (current_app && SvROK(current_app)) {
         return gv_stashsv(PmArray(current_app)[Application_pkg_index], false);
      } else {
         PmCancelFuncall;
         throw std::runtime_error("current application not set");
      }
   }
}

SV* fetch_typeof_gv(pTHX_ const char* class_name, size_t class_namelen)
{
   HV* const app_stash=current_application_pkg(aTHX);
   HV* const stash=glue::namespace_lookup_class(aTHX_ app_stash, class_name, class_namelen, 0);
   if (__builtin_expect(!stash, 0)) {
      sv_setpvf(ERRSV, "unknown perl class %.*s::%.*s", PmPrintHvNAME(app_stash), int(class_namelen), class_name);
      PmCancelFuncall;
      throw exception();
   }
   SV** const gvp=hv_fetch(stash, "typeof", 6, false);
   if (__builtin_expect(!gvp, 0)) {
      sv_setpvf(ERRSV, "%.*s is not an Object or Property type", PmPrintHvNAME(stash));
      PmCancelFuncall;
      throw exception();
   }
   return *gvp;
}

SV** push_current_application(pTHX_ SV **SP)
{
   if (cur_wrapper_cv) {
      PUSHMARK(SP);
      PUTBACK;
      call_sv(*hv_fetch(CvSTASH(cur_wrapper_cv), "self", 4, 0), G_SCALAR | G_EVAL);
      SPAGAIN;
      if (__builtin_expect(SvTRUE(ERRSV), 0)) {
         PmCancelFuncall;
         throw exception();
      }
      PUTBACK;
   } else {
      SV* current_app=GvSV(User_application);
      if (current_app && SvROK(current_app)) {
         XPUSHs(current_app);
      } else {
         PmCancelFuncall;
         throw exception("current application not set");
      }
   }
   return SP;
}

void fill_cached_cv(pTHX_ cached_cv& cv)
{
   if (!(cv.addr=(SV*)get_cv(cv.name, false))) {
      sv_setpvf(ERRSV, "unknown perl subroutine %s", cv.name);
      PmCancelFuncall;
      throw exception();
   }
}

SV* call_func_scalar(pTHX_ SV* cv, bool undef_to_null)
{
   call_sv(cv, G_SCALAR | G_EVAL);
   dSP;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      PmFuncallFailed;
   }
   SV* ret=POPs;
   if (undef_to_null && !SvOK(ret)) {
      ret=nullptr;
   } else if (SvTEMP(ret)) {
      SvREFCNT_inc_simple_void_NN(ret);  // prevent from being destroyed in FREETMPS
   }
   PmFinishFuncall;
   return ret;
}

std::string call_func_string(pTHX_ SV* cv, bool protect_with_eval)
{
   call_sv(cv, protect_with_eval ? G_SCALAR | G_EVAL : G_SCALAR);
   dSP;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      PmFuncallFailed;
   }
   SV *retval=POPs;
   size_t l=0;
   const char* s=SvPV(retval, l);
   std::string ret(s, l);
   PmFinishFuncall;
   return ret;
}

bool call_func_bool(pTHX_ SV* cv, int boolean_check)
{
   call_sv(cv, G_SCALAR | G_EVAL);
   dSP;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      PmFuncallFailed;
   }
   SV *retval=POPs;
   const bool ret=boolean_check ? SvTRUE(retval) : SvOK(retval);
   PmFinishFuncall;
   return ret;
}

int call_func_list(pTHX_ SV* cv)
{
   const int ret=call_sv(cv, G_ARRAY | G_EVAL);
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      FREETMPS; LEAVE;
      throw exception();
   }
   if (ret==0) {
      FREETMPS; LEAVE;
   }
   return ret;
}

void call_func_void(pTHX_ SV* cv)
{
   const int ret=call_sv(cv, G_VOID | G_EVAL);
   if (ret>0) {
      dSP;
      (void)POPs;
      PUTBACK;
   }
   FREETMPS; LEAVE;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      throw exception();
   }
}

SV* call_method_scalar(pTHX_ const char* name, bool undef_to_null)
{
   Perl_call_method(aTHX_ name, G_SCALAR | G_EVAL);
   dSP;
   if (SvTRUE(ERRSV)) {
      PmFuncallFailed;
   }
   SV* ret=POPs;
   if (undef_to_null && !SvOK(ret)) {
      ret=nullptr;
   } else if (SvTEMP(ret)) {
      SvREFCNT_inc_simple_void_NN(ret);  // prevent from being destroyed in FREETMPS
   }
   PmFinishFuncall;
   return ret;
}

int call_method_list(pTHX_ const char* name)
{
   const int ret=Perl_call_method(aTHX_ name, G_ARRAY | G_EVAL);
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      FREETMPS; LEAVE;
      throw exception();
   }
   if (ret==0) {
      FREETMPS; LEAVE;
   }
   return ret;
}

void call_method_void(pTHX_ const char* name)
{
   const int ret=Perl_call_method(aTHX_ name, G_VOID | G_EVAL);
   if (ret>0) {
      dSP;
      (void)POPs;
      PUTBACK;
   }
   FREETMPS; LEAVE;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      throw exception();
   }
}

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
