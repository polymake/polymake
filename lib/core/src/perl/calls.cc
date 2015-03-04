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

#include "polymake/perl/glue.h"
#include "polymake/perl/calls.h"

namespace pm { namespace perl {

static glue::cached_cv
   load_data_cv={ "Polymake::User::load_data", 0 },
   save_data_cv={ "Polymake::User::save_data", 0 },
   get_custom_cv={ "Polymake::Core::Application::get_custom_var", 0 };

PropertyValue::PropertyValue(const PropertyValue& x) :
   Value(x.sv)
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
   PmStartFuncall;
   mXPUSHp(filename.c_str(), filename.size());
   PUTBACK;
   return glue::call_func_scalar(aTHX_ load_data_cv);
}

void PropertyValue::_save_data(const std::string& filename, const std::string& description)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(sv);
   mXPUSHp(filename.c_str(), filename.size());
   mXPUSHp(description.c_str(), description.size());
   PUTBACK;
   glue::call_func_void(aTHX_ save_data_cv);
}

SV* get_custom_var(const char* name, size_t ll, const char* key, size_t kl)
{
   dTHX;
   PmStartFuncall;
   mXPUSHp(name, ll);
   if (key != NULL) mXPUSHp(key, kl);
   PUTBACK;
   return glue::call_func_scalar(aTHX_ get_custom_cv);
}

FunCall::FunCall() : Stack(true,1)
{
   dTHXa(pi);
   func=&PL_sv_undef;
}

void FunCall::prepare_function_call(const char* name, size_t nl)
{
   dTHXa(pi);
   dSP;
   SP=glue::push_current_application(aTHX_ SP);
   SV* const app=POPs;
   PUTBACK;
   if ((func=(SV*)pm_perl_namespace_lookup_sub(aTHX_ glue::User_stash, name, nl, (CV*)SvRV(PmArray(app)[glue::Application_eval_expr_index]))) == NULL) {
      PmCancelFuncall;
      throw std::runtime_error(std::string("polymake function ") + name + " not found");
   }
}

void FunCall::push_arg_list(SV *args) const
{
   dTHXa(pi);
   dSP;
   AV* const av=(AV*)SvRV(args);
   int n=AvFILL(av)+1;
   EXTEND(SP, n);
   AvREAL_off(av);
   for (int i=0; i<n; ++i)
      PUSHs(sv_2mortal(AvARRAY(av)[i]));
   PUTBACK;
}

SV* FunCall::call() const
{
   // all arguments are already on the stack
   dTHXa(pi);
   SV *sv=func;  func=NULL;
   return glue::call_func_scalar(aTHX_ sv);
}

int FunCall::list_call() const
{
   // all arguments are already on the stack
   dTHXa(pi);
   SV *sv=func;  func=NULL;
   return glue::call_func_list(aTHX_ sv);
}

void FunCall::void_evaluate() const
{
   // all arguments are already on the stack
   dTHXa(pi);
   SV *sv=func;  func=NULL;
   glue::call_func_void(aTHX_ sv);
}

SV* FunCall::evaluate_method(SV* obj, const char* name) const
{
   // all arguments are already on the stack, an empty slot is waiting for the object
   dTHXa(pi);
   PL_stack_base[TOPMARK+1]=obj;
   func=NULL;
   return glue::call_method_scalar(aTHX_ name);
}

int FunCall::list_evaluate_method(SV* obj, const char* name) const
{
   // all arguments are already on the stack, an empty slot is waiting for the object
   dTHXa(pi);
   PL_stack_base[TOPMARK+1]=obj;
   func=NULL;
   return glue::call_method_list(aTHX_ name);
}

void FunCall::void_evaluate_method(SV* obj, const char* name) const
{
   // all arguments are already on the stack, an empty slot is waiting for the object
   dTHXa(pi);
   PL_stack_base[TOPMARK+1]=obj;
   func=NULL;
   glue::call_method_void(aTHX_ name);
}

ListResult::ListResult(int items, const FunCall& fc) : Array(items)
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
   HV* const stash=pm_perl_namespace_lookup_class(aTHX_ app_stash, class_name, class_namelen, 0);
   if (__builtin_expect(!stash, 0)) {
      sv_setpvf(ERRSV, "unknown perl class %s::%.*s", HvNAME(app_stash), int(class_namelen), class_name);
      PmCancelFuncall;
      throw exception();
   }
   SV** const gvp=hv_fetch(stash, "typeof", 6, false);
   if (__builtin_expect(!gvp, 0)) {
      sv_setpvf(ERRSV, "%s is not an Object or Property type", HvNAME(stash));
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

SV* call_func_scalar(pTHX_ SV* cv, SV** dst)
{
   call_sv(cv, G_SCALAR | G_EVAL);
   dSP;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      PmFuncallFailed;
   }
   SV* ret=POPs;
   if (dst) {
      if (*dst) {
         sv_setsv(*dst, ret);
      } else {
         if (SvTEMP(ret)) SvREFCNT_inc_simple_void_NN(ret);  // prevent from being destroyed in FREETMPS
         *dst=ret;
      }
      ret=NULL;
   } else {
      if (SvTEMP(ret)) SvREFCNT_inc_simple_void_NN(ret);
   }
   PUTBACK; FREETMPS; LEAVE;
   return ret;
}

bool call_func_bool(pTHX_ SV *cv, int boolean_check)
{
   call_sv(cv, G_SCALAR | G_EVAL);
   dSP;
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      PmFuncallFailed;
   }
   SV *retval=POPs;
   const bool ret=boolean_check ? SvTRUE(retval) : SvOK(retval);
   PUTBACK; FREETMPS; LEAVE;
   return ret;
}

int call_func_list(pTHX_ SV *cv)
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

void call_func_void(pTHX_ SV *cv)
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

SV* call_method_scalar(pTHX_ const char *name)
{
   call_method(name, G_SCALAR | G_EVAL);
   dSP;
   if (SvTRUE(ERRSV)) {
      PmFuncallFailed;
   }
   SV* const ret=POPs;
   if (SvTEMP(ret)) SvREFCNT_inc_simple_void_NN(ret);  // prevent from being destroyed in FREETMPS
   PUTBACK; FREETMPS; LEAVE;
   return ret;
}

int call_method_list(pTHX_ const char *name)
{
   const int ret=call_method(name, G_ARRAY | G_EVAL);
   if (__builtin_expect(SvTRUE(ERRSV), 0)) {
      FREETMPS; LEAVE;
      throw exception();
   }
   if (ret==0) {
      FREETMPS; LEAVE;
   }
   return ret;
}

void call_method_void(pTHX_ const char *name)
{
   const int ret=call_method(name, G_VOID | G_EVAL);
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
