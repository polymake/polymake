/* Copyright (c) 1997-2023
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

#include "polymake/perl/Ext.h"

// defuse internal perl macros interfering with C++ library
#ifdef do_open
#undef do_open
#endif
#ifdef do_close
#undef do_close
#endif
#ifdef random
#undef random
#endif
#ifdef seed
#undef seed
#endif
#ifdef call_method
#undef call_method
#endif

#define PmStartFuncall(n) \
   dSP; \
   ENTER; SAVETMPS; \
   if (n) EXTEND(SP, n); \
   PUSHMARK(SP)

#define PmCancelFuncall \
   dMARK; \
   PL_stack_sp = MARK; \
   FREETMPS; LEAVE

#define PmFinishFuncall \
   PUTBACK; FREETMPS; LEAVE

#define PmFuncallFailed \
   (void)POPs; \
   PmFinishFuncall; \
   throw exception()

#define PmPopLastResult(sv) \
   SV *sv = POPs; \
   if (SvTEMP(sv)) SvREFCNT_inc_simple_void_NN(sv); \
   PmFinishFuncall

#define PmArray(avref) AvARRAY((AV*)SvRV(avref))

// Be careful! Tons of perl macros out there!
#define POLYMAKE_WITHIN_PERL 1

#include "polymake/perl/constants.h"
#include "polymake/perl/Value.h"
#include "polymake/perl/types.h"

namespace pm { namespace perl { namespace glue {

struct cached_cv {
   const char* name;
   SV* addr=nullptr;
};

SV* get_current_application(pTHX);

void fill_cached_cv(pTHX_ cached_cv& cv);
SV*  call_func_scalar(pTHX_ SV* cv, bool undef_to_null=false);
std::string call_func_string(pTHX_ SV* cv, bool protect_with_eval=true);
bool call_func_bool(pTHX_ SV* cv);
int  call_func_list(pTHX_ SV* cv);
void call_func_void(pTHX_ SV* cv);
SV*  call_method_scalar(pTHX_ const char* method, bool undef_to_null=false);
int  call_method_list(pTHX_ const char* method);
void call_method_void(pTHX_ const char* method);

// find the given class in the given application namespace and fetch the GV of its typeof() method
SV* fetch_typeof_gv(pTHX_ HV* app_stash, const char* class_name, size_t class_namelen);

inline
SV* call_func_scalar(pTHX_ cached_cv& cv, bool undef_to_null = false)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_scalar(aTHX_ cv.addr, undef_to_null);
}

inline
std::string call_func_string(pTHX_ cached_cv& cv, bool protect_with_eval = true)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_string(aTHX_ cv.addr, protect_with_eval);
}

inline
bool call_func_bool(pTHX_ cached_cv& cv)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_bool(aTHX_ cv.addr);
}

inline
int call_func_list(pTHX_ cached_cv& cv)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_list(aTHX_ cv.addr);
}

inline
void call_func_void(pTHX_ cached_cv& cv)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   call_func_void(aTHX_ cv.addr);
}

struct base_vtbl : public MGVTBL {
   const std::type_info *type;
   SV* const_ref_typeid_name_sv;
   SV* mutable_ref_typeid_name_sv;
   SV* typeid_name_sv;
   size_t obj_size;
   ClassFlags flags;
   int obj_dimension;
   SV* (*sv_maker)(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
   SV* (*sv_cloner)(pTHX_ SV* src);
   copy_constructor_type copy_constructor;
   assignment_type assignment;
   destructor_type destructor;
};

struct common_vtbl : public base_vtbl {
   conv_to_string_type to_string;
   conv_to_serialized_type to_serialized;
   type_reg_fn_type provide_serialized_type;
};

struct scalar_vtbl : public common_vtbl {
   conv_to_Int_type to_Int;
   conv_to_Float_type to_Float;
};

struct container_access_vtbl {
   size_t obj_size;
   destructor_type destructor;
   container_begin_type begin;
   container_access_type deref;
   container_access_type random;
};

struct container_vtbl : public common_vtbl {
   int own_dimension;
   conv_to_Int_type size;
   container_resize_type resize;
   container_store_type store_at_ref;
   type_reg_fn_type provide_key_type;
   type_reg_fn_type provide_value_type;
   container_access_vtbl acc[4];
   AV *assoc_methods;
};

struct iterator_vtbl : public base_vtbl {
   iterator_deref_type deref;
   iterator_incr_type incr;
   conv_to_bool_type at_end;
   conv_to_Int_type index;
};

struct composite_vtbl : public common_vtbl {
   int n_members;
   provide_type provide_member_types;
   provide_type provide_member_descrs;
   provide_type provide_member_names;
   composite_access_vtbl acc[1];
};

template <typename vtbl_type>
const vtbl_type* as_vtbl(const MAGIC* mg)
{
   return static_cast<const vtbl_type*>(mg->mg_virtual);
}

// anchors are stored immediately behind the standard MAGIC structure in a common chunk of memory
struct MagicAnchors {
   MAGIC magic;
   Value::Anchor anchors[1];

   static
   Value::Anchor* first(MAGIC* mg)
   {
      return reverse_cast(mg, &MagicAnchors::magic)->anchors;
   }
};

using mg_size_ret_t = func_return_t<decltype(std::declval<MGVTBL*>()->svt_len)>;
using mg_copy_index_t = typename mlist_at_rev<func_args_t<decltype(std::declval<MGVTBL*>()->svt_copy)>, 0>::type;

MAGIC* allocate_canned_magic(pTHX_ SV* sv, SV* descr, ValueFlags flags, unsigned int n_anchors);
int destroy_canned(pTHX_ SV* sv, MAGIC* mg);
int destroy_canned_container(pTHX_ SV* sv, MAGIC* mg);
int destroy_canned_assoc_container(pTHX_ SV* sv, MAGIC* mg);
int clear_canned_container(pTHX_ SV* sv, MAGIC* mg);
int clear_canned_assoc_container(pTHX_ SV* sv, MAGIC* mg);
int assigned_to_primitive_lvalue(pTHX_ SV* sv, MAGIC* mg);
mg_size_ret_t canned_container_size(pTHX_ SV* sv, MAGIC* mg);
int canned_container_access(pTHX_ SV* sv, MAGIC* mg, SV* nsv, const char* dummy, mg_copy_index_t index);
int canned_assoc_container_access(pTHX_ SV* sv, MAGIC* mg, SV* val_sv, const char* key, mg_copy_index_t klen);
mg_size_ret_t canned_composite_size(pTHX_ SV* sv, MAGIC* mg);
int canned_composite_access(pTHX_ SV* sv, MAGIC* mg, SV* nsv, const char *dummy, mg_copy_index_t index);
MAGIC* upgrade_to_builtin_magic_sv(pTHX_ SV* sv, SV* descr_ref, unsigned int n_anchors);
SV* create_builtin_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
SV* create_scalar_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
SV* create_container_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
SV* create_assoc_container_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
SV* create_composite_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, ValueFlags flags, unsigned int n_anchors);
SV* clone_builtin_magic_sv(pTHX_ SV* src);
SV* clone_scalar_magic_sv(pTHX_ SV* src);
SV* clone_container_magic_sv(pTHX_ SV* src);
SV* clone_assoc_container_magic_sv(pTHX_ SV* src);
SV* clone_composite_magic_sv(pTHX_ SV* src);

void connect_cout(pTHX);

extern const CV* cur_wrapper_cv;
extern SV *negative_indices_key,
          *Serializer_Sparse_dim_key,
          *temporary_value_flag;

extern HV *FuncDescr_stash,
          *TypeDescr_stash,
          *User_stash,
          *Object_InitTransaction_stash;

extern MGVTBL sparse_input_vtbl;

extern GV *CPP_root,
          *PropertyType_nested_instantiation,
          *User_application,
          *Debug_level;
extern const base_vtbl* cur_class_vtbl;

extern int Object_name_index, Object_description_index,
           Object_parent_index, Object_transaction_index, Object_attachments_index,
           Application_pkg_index, Application_eval_expr_index,
           TypeDescr_pkg_index, TypeDescr_vtbl_index, TypeDescr_cpperl_file_index, TypeDescr_typeid_index, TypeDescr_generated_by_index,
           CPPOptions_builtin_index, CPPOptions_descr_index,
           FuncDescr_wrapper_index, FuncDescr_return_type_reg_index, FuncDescr_name_index, FuncDescr_cpperl_file_index,
           FuncDescr_arg_types_index, FuncDescr_cross_apps_index, FuncDescr_return_type_index,
           PropertyType_pkg_index, PropertyType_cppoptions_index, PropertyType_params_index,
           CPP_functions_index, CPP_regular_functions_index, CPP_embedded_rules_index,
           CPP_duplicate_class_instances_index, CPP_type_descr_index, CPP_builtins_index,
           CPP_templates_index, CPP_typeids_index,
           CPP_auto_assignment_index, CPP_auto_conversion_index,
           CPP_auto_assoc_methods_index, CPP_auto_set_methods_index,
           FuncDescr_fill, FuncDescr_fill_visible, TypeDescr_fill;
} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
