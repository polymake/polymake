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
#include "polymake/perl/macros.h"
#include "polymake/perl/wrappers.h"

namespace pm { namespace perl {

namespace {

inline
int register_function(pTHX_ SV *wrap_sv, SV *func_ptr_sv, SV *name_sv, SV *file_sv, SV *arg_types, int flist_index)
{
   AV* const descr=newAV();
   av_fill(descr,glue::FuncDescr_fill);
   SV** array=AvARRAY(descr);
   *array++=wrap_sv;
   *array++=func_ptr_sv;
   *array++=name_sv;
   *array++=file_sv;
   *array  =SvREFCNT_inc_simple_NN(arg_types);

   AV* const flist=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[flist_index]);
   av_push(flist, sv_bless(newRV_noinc((SV*)descr), glue::FuncDescr_stash));
   return AvFILLp(flist);
}

inline
SV* pointer_as_const_string(wrapper_type p)
{
   SV *sv=Scalar::const_string((char*)p, sizeof(p));
   SvFLAGS(sv) &= ~SVf_POK;
   return sv;
}

}

int FunctionBase::register_func(wrapper_type wrapper, const char *sig, size_t siglen, const char *file, size_t filelen, int line,
                                SV *arg_types, void *func_ptr, const char *func_ptr_type)
{
   dTHX;
   SV* const wrap_sv=pointer_as_const_string(wrapper);
   SV* const file_sv= file ? Scalar::const_string_with_int(file, filelen, line) : &PL_sv_undef;
   if (func_ptr) {
      // a regular function which will need an indirect wrapper later
      return register_function(aTHX_ wrap_sv, Scalar::const_string_with_int((char*)func_ptr, -1), Scalar::const_string(func_ptr_type),
                               file_sv, arg_types, glue::CPP_regular_functions_index);
   } else {
      // a function template
      return register_function(aTHX_ wrap_sv, newSViv(-1), sig ? newSVpvn(sig,siglen) : &PL_sv_undef,
                               file_sv, arg_types, glue::CPP_functions_index);
   }
}

void FunctionBase::register_disabled(const char *sig, size_t siglen, const char *file, size_t filelen, int line, SV *arg_types)
{
   dTHX;
   register_function(aTHX_ &PL_sv_undef, &PL_sv_undef, newSVpvn(sig,siglen),
                     file ? Scalar::const_string_with_int(file, filelen, line) : &PL_sv_undef,
                     arg_types, glue::CPP_functions_index);
}

void FunctionBase::add_rules(const char *file, int line, const char *text, ...)
{
   dTHX;
   AV *embedded_rules=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_embedded_rules_index]);
   va_list args;
   va_start(args, text);
   av_push(embedded_rules, newSVpvf("#line %d \"%s\"\n",line,file));
   av_push(embedded_rules, vnewSVpvf(text,&args));
   va_end(args);
}

void EmbeddedRule::add(const char *file, int line, const char *text, size_t l)
{
   dTHX;
   AV *embedded_rules=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_embedded_rules_index]);
   av_push(embedded_rules, newSVpvf("#line %d \"%s\"\n",line,file));
   av_push(embedded_rules, newSVpv(text,l));
}

SV* ClassRegistratorBase::register_class(const char *name, size_t namelen, const char *file, size_t filelen, int line,
                                         SV *someref,
                                         const char *typeid_name, const char *const_typeid_name,
                                         bool is_mutable, class_kind kind,
                                         SV *vtbl_sv)
{
   dTHX;
   AV* const descr=newAV();
   av_fill(descr,glue::TypeDescr_fill);
   SV** descr_array=AvARRAY(descr);

   const size_t typeid_len=strlen(typeid_name);
   const size_t const_typeid_len= typeid_name==const_typeid_name ? typeid_len : strlen(const_typeid_name);

   SV* const descr_ref=*hv_fetch((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_typeids_index]), typeid_name, typeid_len, true);
   if (SvOK(descr_ref)) {
      // already exists
      SV* const dup_ref=newRV_noinc((SV*)descr);
      sv_bless(dup_ref, glue::TypeDescr_stash);

      *descr_array++=Scalar::const_string(name, namelen);
      *descr_array  =Scalar::const_string_with_int(file, filelen, line);
      av_push((AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_duplicate_class_instances_index]), dup_ref);
      return descr_ref;
   }

   sv_upgrade(descr_ref, SVt_RV);
   SvRV_set(descr_ref, (SV*)descr);
   SvROK_on(descr_ref);
   sv_bless(descr_ref, glue::TypeDescr_stash);

   glue::base_vtbl* const vtbl=(glue::base_vtbl*)SvPVX(vtbl_sv);
   SV* const typeid_sv=Scalar::const_string_with_int(typeid_name, typeid_len, !is_mutable);
   vtbl->typeid_name_sv=typeid_sv;
   vtbl->const_typeid_name_sv= is_mutable ? Scalar::const_string_with_int(const_typeid_name, const_typeid_len, 1) : typeid_sv;
   vtbl->flags=kind;

   HV *stash;
   SV* generated_by;

   if (name) {
      // a known persistent class declared in the rules or an instance of a declared class template used in the rules
      stash=gv_stashpvn(name, namelen, TRUE);
      (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_classes_index]), name, namelen, newRV((SV*)descr), 0);
      vtbl->flags |= class_is_declared;
      generated_by=&PL_sv_undef;

   } else if (namelen) {
      // a member of an abstract class family with prescribed perl package name; someref -> PropertyType
      if (SvROK(someref)) {
         AV* const proto=(AV*)SvRV(someref);
         SV* const pkg=AvARRAY(proto)[glue::PropertyType_pkg_index];
         name=SvPV(pkg, namelen);
         stash=gv_stashpvn(name, namelen, true);
         (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_classes_index]), name, namelen, newRV((SV*)descr), 0);
         vtbl->flags |= class_is_declared;
         // prescribed package can only appear in the context of an auto-function, no container access method or such
         generated_by= SvROK(glue::cur_wrapper_cv) ? SvREFCNT_inc_simple_NN(glue::cur_wrapper_cv)
                                                   : newRV((SV*)CvXSUBANY(glue::cur_wrapper_cv).any_ptr);
      } else {
         Perl_croak(aTHX_ "internal error: wrong call of register_class");
      }

   } else {
      // non-declared class created in a client; someref -> PropertyType for the persistent (declared) type
      AV* const proto=(AV*)SvRV(someref);
      SV* const pkg=AvARRAY(proto)[glue::PropertyType_pkg_index];
      name=SvPV(pkg, namelen);
      stash=gv_stashpvn(name, namelen, false);
      if (glue::cur_class_vtbl != NULL)
         generated_by=newSVsv(glue::cur_class_vtbl->typeid_name_sv);
      else if (glue::cur_wrapper_cv != NULL)
         generated_by=newRV((SV*)CvXSUBANY(glue::cur_wrapper_cv).any_ptr);
      else
         // non-declared class created in an external application: currently don't know how to handle this case
         generated_by=&PL_sv_undef;
   }

   if ((kind & class_is_kind_mask)==class_is_container) {
      glue::container_vtbl* const t=static_cast<glue::container_vtbl*>(vtbl);
      if (kind & class_is_assoc_container) {
         t->assoc_methods=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_auto_assoc_methods_index]);
         t->std.svt_free     =&glue::destroy_canned_assoc_container;
         t->std.svt_copy     =&glue::canned_assoc_container_access;
         t->std.svt_clear    =&glue::clear_canned_assoc_container;
         t->sv_maker         =&glue::create_assoc_container_magic_sv;
         t->sv_cloner        =&glue::clone_assoc_container_magic_sv;
      } else {
         if (kind & class_is_set)
            t->assoc_methods=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_auto_set_methods_index]);
         t->std.svt_copy     =&glue::canned_container_access;
         t->std.svt_clear    =&glue::clear_canned_container;
         t->sv_maker         =&glue::create_container_magic_sv;
         t->sv_cloner        =&glue::clone_container_magic_sv;
         if (vtbl->flags & class_is_declared) {
            GV *neg_ind_gv=(GV*)HeVAL(hv_fetch_ent(stash, glue::negative_indices_key, true, SvSHARED_HASH(glue::negative_indices_key)));
            if (SvTYPE(neg_ind_gv) != SVt_PVGV)
               gv_init(neg_ind_gv, stash, SvPVX(glue::negative_indices_key), SvCUR(glue::negative_indices_key), GV_ADDMULTI);
            sv_setiv(GvSVn(neg_ind_gv), 1);
         }
      }
   }

   *descr_array++=newRV((SV*)stash);
   *descr_array++=vtbl_sv;
   *descr_array++=typeid_sv;
   *descr_array++=newSViv(kind);
   *descr_array  =generated_by;

   SvREFCNT_inc_simple_void_NN(vtbl_sv);       // let it survive all objects pointing to it via magic
   SvREADONLY_on(vtbl_sv);
   return descr_ref;
}

void ClassTemplate::register_class(const char *name, size_t namelen)
{
   dTHX;
   (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_templates_index]), name, namelen, &PL_sv_yes, 0);
}

#define AllocateVtbl(vtbl_type, t) \
   dTHX;                           \
   vtbl_type* t;                   \
   SV *vtbl=newSV(0);              \
   Newz(0, t, 1, vtbl_type);       \
   sv_upgrade(vtbl, SVt_PV);       \
   SvPVX(vtbl)=(char*)t;           \
   SvLEN(vtbl)=sizeof(vtbl_type)

SV* ClassRegistratorBase::create_builtin_vtbl(
   const std::type_info& type,
   size_t obj_size,
   int primitive_lvalue,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor)
{
   AllocateVtbl(glue::base_vtbl, t);
   t->type             =&type;
   t->obj_size         =obj_size;
   t->obj_dimension    =0;
   t->copy_constructor =copy_constructor;
   t->assignment       =assignment;
   if (primitive_lvalue) {
      t->std.svt_set   =&glue::assigned_to_primitive_lvalue;
   } else {
      t->std.svt_free  =&glue::destroy_canned;
      t->std.svt_dup   =&pm_perl_canned_dup;
      t->destructor    =destructor;
      t->sv_maker      =&glue::create_builtin_magic_sv;
      t->sv_cloner     =&glue::clone_builtin_magic_sv;
   }
   return vtbl;
}

SV* ClassRegistratorBase::create_scalar_vtbl(
   const std::type_info& type,
   size_t obj_size,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_SV_type to_string,
   conv_to_SV_type to_serialized,
   provide_type provide_serialized_type,
   conv_to_int_type to_int,
   conv_to_double_type to_double)
{
   AllocateVtbl(glue::scalar_vtbl, t);
   t->std.svt_free            =&glue::destroy_canned;
   t->std.svt_dup             =&pm_perl_canned_dup;
   t->type                    =&type;
   t->obj_size                =obj_size;
   t->obj_dimension           =0;
   t->copy_constructor        =copy_constructor;
   t->assignment              =assignment;
   t->destructor              =destructor;
   t->sv_maker                =&glue::create_scalar_magic_sv;
   t->sv_cloner               =&glue::clone_scalar_magic_sv;
   t->to_string               =to_string;
   t->to_serialized           =to_serialized;
   t->provide_serialized_type =provide_serialized_type;
   t->to_int                  =to_int;
   t->to_double               =to_double;
   return vtbl;
}

SV* ClassRegistratorBase::create_opaque_vtbl(
   const std::type_info& type,
   size_t obj_size,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_SV_type to_string,
   conv_to_SV_type to_serialized,
   provide_type provide_serialized_type)
{
   AllocateVtbl(glue::common_vtbl, t);
   t->std.svt_free            =&glue::destroy_canned;
   t->std.svt_dup             =&pm_perl_canned_dup;
   t->type                    =&type;
   t->obj_size                =obj_size;
   t->obj_dimension           =0;
   t->copy_constructor        =copy_constructor;
   t->assignment              =assignment;
   t->destructor              =destructor;
   t->sv_maker                =&glue::create_scalar_magic_sv;
   t->sv_cloner               =&glue::clone_scalar_magic_sv;
   t->to_string               =to_string;
   t->to_serialized           =to_serialized;
   t->provide_serialized_type =provide_serialized_type;
   return vtbl;
}

SV* ClassRegistratorBase::create_iterator_vtbl(
   const std::type_info& type,
   size_t obj_size,
   copy_constructor_type copy_constructor,
   destructor_type destructor,
   iterator_deref_type deref,
   iterator_incr_type incr,
   conv_to_int_type at_end)
{
   AllocateVtbl(glue::iterator_vtbl, t);
   t->std.svt_free     =&glue::destroy_canned;
   t->std.svt_dup      =&pm_perl_canned_dup;
   t->type             =&type;
   t->obj_size         =obj_size;
   t->obj_dimension    =0;
   t->copy_constructor =copy_constructor;
   t->destructor       =destructor;
   t->sv_maker         =&glue::create_scalar_magic_sv;
   t->sv_cloner        =&glue::clone_scalar_magic_sv;
   t->deref            =deref;
   t->incr             =incr;
   t->at_end           =at_end;
   return vtbl;
}

SV* ClassRegistratorBase::create_container_vtbl(
   const std::type_info& type,
   size_t obj_size, int total_dimension, int own_dimension,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_SV_type to_string,
   conv_to_SV_type to_serialized,
   provide_type provide_serialized_type,
   conv_to_int_type size,
   container_resize_type resize,
   container_store_type store_at_ref,
   provide_type provide_key_type,
   provide_type provide_value_type)
{
   AllocateVtbl(glue::container_vtbl, t);
   t->std.svt_len             =&glue::canned_container_size;
   t->std.svt_free            =&glue::destroy_canned_container;
   t->std.svt_dup             =&pm_perl_canned_dup;
   t->type                    =&type;
   t->obj_size                =obj_size;
   t->obj_dimension           =total_dimension;
   t->copy_constructor        =copy_constructor;
   t->assignment              =assignment;
   t->destructor              =destructor;
   t->to_string               =to_string;
   t->to_serialized           =to_serialized;
   t->provide_serialized_type =provide_serialized_type;
   t->own_dimension           =own_dimension;
   t->size                    =size;
   t->resize                  =resize;
   t->store_at_ref            =store_at_ref;
   t->provide_key_type        =provide_key_type;
   t->provide_value_type      =provide_value_type;
   return vtbl;
}

void ClassRegistratorBase::fill_iterator_access_vtbl(
   SV *vtbl, int i,
   size_t it_size, size_t cit_size,
   destructor_type it_destructor,
   destructor_type cit_destructor,
   container_begin_type begin,
   container_begin_type cbegin,
   container_access_type deref,
   container_access_type cderef)
{
   glue::container_vtbl *t=(glue::container_vtbl*)SvPVX(vtbl);
   glue::container_access_vtbl *acct=t->acc+i;
   acct->obj_size     =it_size;
   acct->destructor   =it_destructor;
   acct->begin        =begin;
   acct->deref        =deref;
   ++acct;
   acct->obj_size     =cit_size;
   acct->destructor   =cit_destructor;
   acct->begin        =cbegin;
   acct->deref        =cderef;
}

void
ClassRegistratorBase::fill_random_access_vtbl(
   SV *vtbl,
   container_access_type random,
   container_access_type crandom)
{
   glue::container_vtbl *t=(glue::container_vtbl*)SvPVX(vtbl);
   t->acc[0].random=random;
   t->acc[1].random=crandom;
}

SV* ClassRegistratorBase::create_composite_vtbl(
   const std::type_info& type,
   size_t obj_size,
   int obj_dimension,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_SV_type to_string,
   conv_to_SV_type to_serialized,
   provide_type provide_serialized_type,
   int n_elems,
   provide_type provide_elem_types,
   provide_type provide_field_names,
   void (*fill)(composite_access_vtbl*))
{
   dTHX;
   char *ct;
   size_t s=sizeof(glue::composite_vtbl)+(n_elems-1)*sizeof(composite_access_vtbl);
   Newz(0, ct, s, char);
   SV *vtbl=newSV(0);
   sv_upgrade(vtbl, SVt_PV);
   SvPVX(vtbl)=ct;
   SvLEN(vtbl)=s;
   glue::composite_vtbl *t=(glue::composite_vtbl*)ct;
   t->std.svt_len             =&glue::canned_composite_size;
   t->std.svt_copy            =&glue::canned_composite_access;
   t->std.svt_free            =&glue::destroy_canned;
   t->std.svt_dup             =&pm_perl_canned_dup;
   t->type                    =&type;
   t->obj_size                =obj_size;
   t->obj_dimension           =obj_dimension;
   t->copy_constructor        =copy_constructor;
   t->assignment              =assignment;
   t->destructor              =destructor;
   t->sv_maker                =&glue::create_composite_magic_sv;
   t->sv_cloner               =&glue::clone_composite_magic_sv;
   t->to_string               =to_string;
   t->to_serialized           =to_serialized;
   t->provide_serialized_type =provide_serialized_type;
   t->n_elems                 =n_elems;
   t->provide_elem_types      =provide_elem_types;
   t->provide_field_names     =provide_field_names;
   fill(t->acc);
   return vtbl;
}

SV* Unprintable::to_string(const char*, const char*)
{
   Value v;
   v.put("<UNPRINTABLE OBJECT>");
   return v.get_temp();
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
