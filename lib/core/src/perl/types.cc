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

namespace pm { namespace perl {

SV* get_parameterized_type(const char* pkg, size_t pkgl, bool exact_match)
{
   dTHX;
   PL_stack_base[TOPMARK+1]=sv_2mortal(Scalar::const_string(pkg,pkgl));
   if (!exact_match)
      sv_setiv(save_scalar(glue::PropertyType_nesting_level), 1);
   return glue::call_method_scalar(aTHX_ "typeof");
}

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

bool type_infos::set_descr()
{
   dTHX;
   SV* const opts=PmArray(proto)[glue::PropertyType_cppoptions_index];
   if (SvROK(opts)) {
      descr=PmArray(opts)[glue::CPPOptions_descr_index];
      if (!SvROK(descr)) {
         descr=NULL;
      } else if (SvTYPE(SvRV(descr))==SVt_PVCV) {
         PmStartFuncall;
         const int ret=call_sv(descr, G_VOID | G_EVAL);
         if (__builtin_expect(ret>0, 0)) {
            descr=NULL;
            PmFuncallFailed;
         }
         FREETMPS; LEAVE;
         descr=PmArray(opts)[glue::CPPOptions_descr_index];
      }
   }
   return descr != NULL;
}

void type_infos::set_proto(SV* known_proto)
{
   dTHX;
   if (known_proto != NULL) {
      proto=newSVsv(known_proto);
   } else {
      SV** type_gvp=hv_fetch((HV*)SvRV(PmArray(descr)[glue::TypeDescr_pkg_index]), "type", 4, false);
      if (type_gvp) {
         PmStartFuncall;
         call_sv(*type_gvp, G_SCALAR | G_EVAL);
         SPAGAIN;
         if (__builtin_expect(SvTRUE(ERRSV), 0)) {
            PmFuncallFailed;
         }
         proto=POPs;
         assert(SvTEMP(proto));  SvREFCNT_inc_simple_void_NN(proto);       // prevent from being destroyed in FREETMPS
         PUTBACK; FREETMPS; LEAVE;
      }
   }
}

void type_infos::set_proto(SV* prescribed_pkg, const std::type_info& ti, SV* super_proto)
{
   dTHX;
   PmStartFuncall;
   XPUSHs(prescribed_pkg);
   const char* const type_name=ti.name();
   mXPUSHp(type_name, strlen(type_name));
   if (super_proto) XPUSHs(super_proto);
   PUTBACK;
   proto=glue::call_func_scalar(aTHX_ glue::fetch_typeof_gv(aTHX_ SvPVX(prescribed_pkg), SvCUR(prescribed_pkg)));
}

bool type_infos::allow_magic_storage() const
{
   dTHX;
   SV* opts=PmArray(proto)[glue::PropertyType_cppoptions_index];
   if (SvROK(opts)) {
      SV* builtin=PmArray(opts)[glue::CPPOptions_builtin_index];
      return !SvTRUE(builtin);
   }
   return false;
}

namespace {

SV* resolve_auto_function_cv=NULL;
SV* fake_args_ref=NULL;
AV* fake_args=NULL;

}

wrapper_type type_cache_base::get_function_wrapper(SV* src, SV* dst_descr, int auto_func_index)
{
   dTHX; dSP;
   SV* auto_func=PmArray(GvSV(glue::CPP_root))[auto_func_index];
   wrapper_type ret=NULL;
   if (resolve_auto_function_cv == NULL) {
      resolve_auto_function_cv=(SV*)get_cv("Polymake::Core::CPlusPlus::resolve_auto_function", FALSE);
      fake_args=newAV();
      av_extend(fake_args,2);
      AvFILLp(fake_args)=1;
      AvREAL_off(fake_args);
      fake_args_ref=newRV_noinc((SV*)fake_args);
   }
   ENTER; SAVETMPS;
   PUSHMARK(SP);
   XPUSHs(auto_func);
   AvARRAY(fake_args)[0]=dst_descr;
   AvARRAY(fake_args)[1]=src;
   XPUSHs(fake_args_ref);
   PUTBACK;
   call_sv(resolve_auto_function_cv, G_SCALAR | G_EVAL);
   SPAGAIN;
   SV* cv=POPs;
   if (SvROK(cv) && (cv=SvRV(cv), CvISXSUB(cv))) {
      AV* func_descr=(AV*)CvXSUBANY(cv).any_ptr;
      ret=(wrapper_type)SvPVX(AvARRAY(func_descr)[glue::FuncDescr_wrapper_index]);
   }
   PUTBACK; FREETMPS; LEAVE;
   if (__builtin_expect(SvTRUE(ERRSV), 0))
      throw exception();
   return ret;
}

wrapper_type type_cache_base::get_conversion_operator(SV* src, SV* dst_descr)
{
   return dst_descr != NULL ? get_function_wrapper(src, dst_descr, glue::CPP_auto_conversion_index) : NULL;
}

wrapper_type type_cache_base::get_conversion_constructor(SV* src, SV* dst_descr)
{
   return dst_descr != NULL ? get_function_wrapper(src, dst_descr, glue::CPP_auto_convert_constructor_index) : NULL;
}

wrapper_type type_cache_base::get_assignment_operator(SV* src, SV* dst_descr)
{
   return dst_descr != NULL ? get_function_wrapper(src, dst_descr, glue::CPP_auto_assignment_index) : NULL;
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
