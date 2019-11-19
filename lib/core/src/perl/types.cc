/* Copyright (c) 1997-2019
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

#include "polymake/perl/glue.h"

namespace pm { namespace perl {

bool type_infos::set_descr(const std::type_info& ti)
{
   dTHX;
   const char* const type_name=ti.name();
   if (SV** const descr_p=hv_fetch((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_typeids_index]),
                                   type_name, strlen(type_name), false)) {
      descr=*descr_p;
      return true;
   }
   return false;
}

void type_infos::set_descr()
{
   dTHX;
   SV* const opts=PmArray(proto)[glue::PropertyType_cppoptions_index];
   if (SvROK(opts)) {
      descr=PmArray(opts)[glue::CPPOptions_descr_index];
      if (!SvROK(descr)) {
         descr=nullptr;
      } else if (SvTYPE(SvRV(descr))==SVt_PVCV) {
         PmStartFuncall(0);
         const int ret=call_sv(descr, G_VOID | G_EVAL);
         if (__builtin_expect(ret>0, 0)) {
            descr=nullptr;
            PmFuncallFailed;
         }
         FREETMPS; LEAVE;
         descr=PmArray(opts)[glue::CPPOptions_descr_index];
      }
   }
}

void type_infos::set_proto(SV* known_proto)
{
   dTHX;
   if (known_proto) {
      proto=newSVsv(known_proto);
   } else {
      SV** type_gvp=hv_fetch((HV*)SvRV(PmArray(descr)[glue::TypeDescr_pkg_index]), "type", 4, false);
      if (type_gvp) {
         PmStartFuncall(0);
         proto=glue::call_func_scalar(aTHX_ *type_gvp, true);
      } else {
         return;
      }
   }
   SV* opts=PmArray(proto)[glue::PropertyType_cppoptions_index];
   if (SvROK(opts)) {
      SV* builtin=PmArray(opts)[glue::CPPOptions_builtin_index];
      magic_allowed=!SvTRUE(builtin);
   }
}

void type_infos::set_proto_with_prescribed_pkg(SV* prescribed_pkg, SV* app_stash_ref, const std::type_info& ti, SV* super_proto)
{
   dTHX;
   PmStartFuncall(3);
   PUSHs(prescribed_pkg);
   const char* const type_name=ti.name();
   mPUSHp(type_name, strlen(type_name));
   if (super_proto) PUSHs(super_proto);
   PUTBACK;
   proto=glue::call_func_scalar(aTHX_ glue::fetch_typeof_gv(aTHX_ (HV*)SvRV(app_stash_ref), SvPVX(prescribed_pkg), SvCUR(prescribed_pkg)), true);
   magic_allowed=true;
}

namespace {
SV* resolve_auto_function_cv=nullptr;
}

char* type_cache_base::get_function_wrapper(SV* src, SV* dst_descr, int auto_func_index)
{
   dTHX; dSP;
   SV* auto_func = PmArray(GvSV(glue::CPP_root))[auto_func_index];
   char* ret = nullptr;
   if (!resolve_auto_function_cv)
      resolve_auto_function_cv = (SV*)get_cv("Polymake::Core::CPlusPlus::resolve_auto_function", FALSE);

   AV* fake_args = newAV();
   av_extend(fake_args, 2);
   AvFILLp(fake_args) = 1;
   AvREAL_off(fake_args);
   SV* fake_args_ref = newRV_noinc((SV*)fake_args);

   ENTER; SAVETMPS;
   PUSHMARK(SP);
   XPUSHs(auto_func);
   AvARRAY(fake_args)[0] = dst_descr;
   AvARRAY(fake_args)[1] = src;
   XPUSHs(fake_args_ref);
   PUTBACK;
   call_sv(resolve_auto_function_cv, G_SCALAR | G_EVAL);
   SPAGAIN;
   SV* cv = POPs;
   if (SvROK(cv) && (cv = SvRV(cv), CvISXSUB(cv))) {
      AV* func_descr = (AV*)CvXSUBANY(cv).any_ptr;
      ret = reinterpret_cast<char*>(AvARRAY(func_descr)[glue::FuncDescr_wrapper_index]);
   }
   PUTBACK; FREETMPS; LEAVE;
   SvREFCNT_dec(fake_args_ref);

   if (__builtin_expect(SvTRUE(ERRSV), 0))
      throw exception();
   return ret;
}

char* type_cache_base::get_conversion_operator(SV* src, SV* dst_descr)
{
   return dst_descr ? get_function_wrapper(src, dst_descr, glue::CPP_auto_conversion_index) : nullptr;
}

char* type_cache_base::get_assignment_operator(SV* src, SV* dst_descr)
{
   return dst_descr ? get_function_wrapper(src, dst_descr, glue::CPP_auto_assignment_index) : nullptr;
}

namespace {

inline
const char* perl_error_text()
{
   dTHX;
   return SvPV_nolen(ERRSV);
}

}

exception::exception() :
   std::runtime_error(perl_error_text()) {}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
