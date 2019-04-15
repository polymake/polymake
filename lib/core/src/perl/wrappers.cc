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
#include "polymake/perl/macros.h"
#include "polymake/perl/wrappers.h"

namespace pm { namespace perl {

void FunctionWrapperBase::register_it(bool is_template,
                                      wrapper_type wrapper, const AnyString& uniq_name, const AnyString& cpperl_file, int inst_num,
                                      SV* arg_types, SV* cross_apps, type_reg_fn_type result_type_reg) const
{
   // FIXME: arg_types and cross_apps may be created by ArrayHolder which does not own the SV, and this is bad design.
   // When that's fixed, refcounts of both must be incremented here.

   dTHX;
   AV* const descr = newAV();
   av_fill(descr, glue::FuncDescr_fill);
   AvFILLp(descr) = glue::FuncDescr_fill_visible;
   SV** const descr_array = AvARRAY(descr);
   SV* descr_ref=sv_bless(newRV_noinc((SV*)descr), glue::FuncDescr_stash);

   descr_array[glue::FuncDescr_arg_types_index] = arg_types;   // for regular functions: number of arguments
   descr_array[glue::FuncDescr_wrapper_index] = reinterpret_cast<SV*>(wrapper);
   descr_array[glue::FuncDescr_return_type_reg_index] = reinterpret_cast<SV*>(result_type_reg);

   if (is_template) {
      descr_array[glue::FuncDescr_name_index] = Scalar::const_string(uniq_name.ptr, uniq_name.len);
      descr_array[glue::FuncDescr_cpperl_file_index] = Scalar::const_string_with_int(cpperl_file.ptr, cpperl_file.len, inst_num);
      if (cross_apps)
         descr_array[glue::FuncDescr_cross_apps_index] = cross_apps;

      AV* const functions = (AV*)queue;
      av_push(functions, descr_ref);
   } else {
      AV* const regular_functions = (AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_regular_functions_index]);
      av_push(regular_functions, descr_ref);
      const int index=AvFILLp(regular_functions);

      AV* const embedded_rules = (AV*)queue;
      av_push(embedded_rules, newSVpv(cpperl_file.ptr, cpperl_file.len));   // source line
      av_push(embedded_rules, newSVpvf(uniq_name.ptr, index));
   }
}

void EmbeddedRule::add__me(const AnyString& text, const AnyString& source_line) const
{
   dTHX;
   AV* embedded_rules=(AV*)queue;
   av_push(embedded_rules, newSVpv(source_line.ptr, source_line.len));
   av_push(embedded_rules, newSVpv(text.ptr, text.len));
}

AnyString class_with_prescribed_pkg(nullptr, 1);
AnyString relative_of_known_class(nullptr, 0);

SV* ClassRegistratorBase::register_class(const AnyString& name, const AnyString& cpperl_file, int inst_num,
                                         SV* someref, SV* generated_by,
                                         const char* typeid_name,
                                         bool is_mutable, ClassFlags class_flags,
                                         SV* vtbl_sv)
{
   dTHX;
   AV* const descr=newAV();
   av_fill(descr, glue::TypeDescr_fill);
   SV** const descr_array=AvARRAY(descr);

   const size_t typeid_len=strlen(typeid_name);

   SV* const descr_ref=*hv_fetch((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_typeids_index]), typeid_name, typeid_len, true);
   if (SvOK(descr_ref)) {
      // already exists; someref -> list of duplicate classes
      if (!name.ptr)
         Perl_croak(aTHX_ "internal error: duplicate call of register_class for an undeclared type");
      if (!cpperl_file.ptr)
         Perl_croak(aTHX_ "multiple definition of an opaque class %.*s", (int)name.len, name.ptr);
      assert(SvTYPE(someref)==SVt_PVAV);
      SV* const dup_ref=newRV_noinc((SV*)descr);
      sv_bless(dup_ref, glue::TypeDescr_stash);

      AV* orig_descr=(AV*)SvRV(descr_ref);
      descr_array[glue::TypeDescr_pkg_index]=SvREFCNT_inc_simple_NN(AvARRAY(orig_descr)[glue::TypeDescr_pkg_index]);
      descr_array[glue::TypeDescr_cpperl_file_index]=Scalar::const_string_with_int(cpperl_file.ptr, cpperl_file.len, inst_num);
      av_push((AV*)someref, dup_ref);
      return descr_ref;
   }

   sv_upgrade(descr_ref, SVt_RV);
   SvRV_set(descr_ref, (SV*)descr);
   SvROK_on(descr_ref);
   sv_bless(descr_ref, glue::TypeDescr_stash);

   glue::base_vtbl* const vtbl = reinterpret_cast<glue::base_vtbl*>(SvPVX(vtbl_sv));
   vtbl->const_ref_typeid_name_sv = Scalar::const_string_with_int(typeid_name, typeid_len, arg_is_const_ref);
   vtbl->mutable_ref_typeid_name_sv = is_mutable ? Scalar::const_string_with_int(typeid_name, typeid_len, arg_is_lval_ref) : vtbl->const_ref_typeid_name_sv;
   vtbl->typeid_name_sv = Scalar::const_string_with_int(typeid_name, typeid_len, arg_is_univ_ref);
   vtbl->flags = class_flags;

   HV* stash=nullptr;

   if (name) {
      // a known persistent class declared in the rules or an instance of a declared class template used in the rules
      stash=gv_stashpvn(name.ptr, name.len, TRUE);
      (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_type_descr_index]), name.ptr, name.len, newRV((SV*)descr), 0);
      vtbl->flags |= ClassFlags::is_declared;
      if (generated_by)
         Perl_croak(aTHX_ "internal error: wrong call of register_class");
      generated_by=PmEmptyArraySlot;

   } else if (name.len) {
      if (SvROK(someref)) {
         // a member of an abstract class family with prescribed perl package name; someref -> PropertyType
         AV* const proto=(AV*)SvRV(someref);
         SV* const pkg=AvARRAY(proto)[glue::PropertyType_pkg_index];
         size_t pkgnamelen;
         const char* pkgname=SvPV(pkg, pkgnamelen);
         stash = gv_stashpvn(pkgname, pkgnamelen, true);
         // (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_type_descr_index]), pkgname, pkgnamelen, newRV((SV*)descr), 0);
         vtbl->flags |= ClassFlags::is_declared;
         SvREFCNT_inc_simple_void_NN(generated_by);
      } else {
         Perl_croak(aTHX_ "internal error: wrong call of register_class");
      }

   } else {
      // non-declared class created in a client; someref -> PropertyType for the persistent (declared) type
      if (!someref)
         Perl_croak(aTHX_ "internal error: wrong call of register_class");
      AV* const proto=(AV*)SvRV(someref);
      SV* const pkg=AvARRAY(proto)[glue::PropertyType_pkg_index];
      size_t pkgnamelen;
      const char* pkgname=SvPV(pkg, pkgnamelen);
      stash = gv_stashpvn(pkgname, pkgnamelen, false);
      if (generated_by)
         SvREFCNT_inc_simple_void_NN(generated_by);
      else if (glue::cur_class_vtbl)
         generated_by=newSVsv(glue::cur_class_vtbl->typeid_name_sv);
      else if (glue::cur_wrapper_cv)
         generated_by=newRV((SV*)CvXSUBANY(glue::cur_wrapper_cv).any_ptr);
      else
         // non-declared class created in an external application: currently don't know how to handle this case
         generated_by=PmEmptyArraySlot;
   }

   if ((class_flags & ClassFlags::kind_mask)==ClassFlags::is_container) {
      glue::container_vtbl* const t=static_cast<glue::container_vtbl*>(vtbl);
      if (class_flags * ClassFlags::is_assoc_container) {
         t->assoc_methods=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_auto_assoc_methods_index]);
         t->svt_free     =&glue::destroy_canned_assoc_container;
         t->svt_copy     =&glue::canned_assoc_container_access;
         t->svt_clear    =&glue::clear_canned_assoc_container;
         t->sv_maker     =&glue::create_assoc_container_magic_sv;
         t->sv_cloner    =&glue::clone_assoc_container_magic_sv;
      } else {
         if (class_flags * ClassFlags::is_set)
            t->assoc_methods=(AV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_auto_set_methods_index]);
         t->svt_copy     =&glue::canned_container_access;
         t->svt_clear    =&glue::clear_canned_container;
         t->sv_maker     =&glue::create_container_magic_sv;
         t->sv_cloner    =&glue::clone_container_magic_sv;
         if (vtbl->flags * ClassFlags::is_declared) {
            GV *neg_ind_gv=(GV*)HeVAL(hv_fetch_ent(stash, glue::negative_indices_key, true, SvSHARED_HASH(glue::negative_indices_key)));
            if (SvTYPE(neg_ind_gv) != SVt_PVGV)
               gv_init_pvn(neg_ind_gv, stash, SvPVX(glue::negative_indices_key), SvCUR(glue::negative_indices_key), GV_ADDMULTI);
            sv_setiv(GvSVn(neg_ind_gv), 1);
         }
      }
   }

   descr_array[glue::TypeDescr_pkg_index]=newRV((SV*)stash);
   descr_array[glue::TypeDescr_vtbl_index]=vtbl_sv;
   if (cpperl_file)
      descr_array[glue::TypeDescr_cpperl_file_index]=Scalar::const_string_with_int(cpperl_file.ptr, cpperl_file.len, inst_num);
   descr_array[glue::TypeDescr_typeid_index]=vtbl->const_ref_typeid_name_sv;
   descr_array[glue::TypeDescr_generated_by_index]=generated_by;

   SvREFCNT_inc_simple_void_NN(vtbl_sv);       // let it survive all objects pointing to it via magic
   SvREADONLY_on(vtbl_sv);
   return descr_ref;
}

void ClassTemplate::add__me(const AnyString& name)
{
   dTHX;
   (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[glue::CPP_templates_index]), name.ptr, name.len, &PL_sv_yes, 0);
}

namespace {

template <typename vtbl_type>
SV* allocate_vtbl(vtbl_type*& t, STRLEN size=0)
{
   dTHX;
   char* t_area;
   size += sizeof(vtbl_type);
   Newxz(t_area, size, char);
   SV* vtbl=newSV_type(SVt_PV);
   SvPVX(vtbl)=t_area;
   SvLEN(vtbl)=size;
   SvPOKp_on(vtbl);
   t=reinterpret_cast<vtbl_type*>(t_area);
   return vtbl;
}

}

SV* ClassRegistratorBase::create_builtin_vtbl(
   const std::type_info& type,
   size_t obj_size,
   int primitive_lvalue,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor)
{
   glue::base_vtbl* t;
   SV* vtbl=allocate_vtbl(t);
   t->type             =&type;
   t->obj_size         =obj_size;
   t->obj_dimension    =0;
   t->copy_constructor =copy_constructor;
   t->assignment       =assignment;
   t->svt_free         =&glue::destroy_canned;
   if (primitive_lvalue) {
      t->svt_set       =&glue::assigned_to_primitive_lvalue;
   } else {
      t->svt_dup       =&glue::canned_dup;
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
   conv_to_string_type to_string,
   conv_to_serialized_type to_serialized,
   type_reg_fn_type provide_serialized_type,
   conv_to_int_type to_int,
   conv_to_float_type to_float)
{
   glue::scalar_vtbl* t;
   SV* vtbl=allocate_vtbl(t);
   t->svt_free                =&glue::destroy_canned;
   t->svt_dup                 =&glue::canned_dup;
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
   t->to_float                =to_float;
   return vtbl;
}


SV* ClassRegistratorBase::create_opaque_vtbl(
   const std::type_info& type,
   size_t obj_size,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_string_type to_string,
   conv_to_serialized_type to_serialized,
   type_reg_fn_type provide_serialized_type)
{
   glue::common_vtbl* t;
   SV* vtbl=allocate_vtbl(t);
   t->svt_free                =&glue::destroy_canned;
   t->svt_dup                 =&glue::canned_dup;
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
   conv_to_int_type at_end,
   conv_to_int_type index)
{
   glue::iterator_vtbl* t;
   SV* vtbl=allocate_vtbl(t);
   t->svt_free         =&glue::destroy_canned;
   t->svt_dup          =&glue::canned_dup;
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
   t->index            =index;
   return vtbl;
}

SV* ClassRegistratorBase::create_container_vtbl(
   const std::type_info& type,
   size_t obj_size, int total_dimension, int own_dimension,
   copy_constructor_type copy_constructor,
   assignment_type assignment,
   destructor_type destructor,
   conv_to_string_type to_string,
   conv_to_serialized_type to_serialized,
   type_reg_fn_type provide_serialized_type,
   conv_to_int_type size,
   container_resize_type resize,
   container_store_type store_at_ref,
   type_reg_fn_type provide_key_type,
   type_reg_fn_type provide_value_type)
{
   glue::container_vtbl* t;
   SV* vtbl=allocate_vtbl(t);
   t->svt_len                 =&glue::canned_container_size;
   t->svt_free                =&glue::destroy_canned_container;
   t->svt_dup                 =&glue::canned_dup;
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
   glue::container_vtbl* t=reinterpret_cast<glue::container_vtbl*>(SvPVX(vtbl));
   glue::container_access_vtbl* acct=t->acc+i;
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
   glue::container_vtbl* t=reinterpret_cast<glue::container_vtbl*>(SvPVX(vtbl));
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
   conv_to_string_type to_string,
   conv_to_serialized_type to_serialized,
   type_reg_fn_type provide_serialized_type,
   int n_members,
   provide_type provide_member_types,
   provide_type provide_member_descrs,
   provide_type provide_member_names,
   void (*fill)(composite_access_vtbl*))
{
   glue::composite_vtbl* t;
   SV* vtbl=allocate_vtbl(t, (n_members-1)*sizeof(t->acc));
   t->svt_len                 =&glue::canned_composite_size;
   t->svt_copy                =&glue::canned_composite_access;
   t->svt_free                =&glue::destroy_canned;
   t->svt_dup                 =&glue::canned_dup;
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
   t->n_members               =n_members;
   t->provide_member_types    =provide_member_types;
   t->provide_member_descrs   =provide_member_descrs;
   t->provide_member_names    =provide_member_names;
   fill(t->acc);
   return vtbl;
}

SV* Unprintable::impl(const char*)
{
   Value v;
   v.put("<UNPRINTABLE OBJECT>");
   return v.get_temp();
}

RegistratorQueue::RegistratorQueue(const AnyString& name, Kind kind)
{
   dTHX;
   queue=(SV*)newAV();
   (void)hv_store((HV*)SvRV(PmArray(GvSV(glue::CPP_root))[static_cast<int>(kind)]), name.ptr, name.len, newRV_noinc(queue), 0);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
