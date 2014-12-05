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

#ifndef POLYMAKE_PERL_GLUE_H
#define POLYMAKE_PERL_GLUE_H

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

#define PmStartFuncall \
   dSP; \
   ENTER; SAVETMPS; \
   PUSHMARK(SP)

#define PmCancelFuncall \
   dMARK; \
   PL_stack_sp=MARK; \
   FREETMPS; LEAVE

#define PmFuncallFailed \
   (void)POPs; \
   PUTBACK; FREETMPS; LEAVE; \
   throw exception()

#define PmPopLastResult(sv) \
   SV *sv=POPs; \
   if (SvTEMP(sv)) SvREFCNT_inc_simple_void_NN(sv); \
   PUTBACK; FREETMPS; LEAVE

#define PmArray(avref) AvARRAY((AV*)SvRV(avref))

// Be careful! Tons of perl macros out there!
#define POLYMAKE_WITHIN_PERL 1

#include "polymake/perl/constants.h"
#include "polymake/perl/Value.h"
#include "polymake/perl/types.h"

namespace pm { namespace perl { namespace glue {

struct cached_cv {
   const char* name;
   SV* addr;
};

SV** push_current_application(pTHX_ SV **sp);
HV*  current_application_pkg(pTHX);

void fill_cached_cv(pTHX_ cached_cv& cv);
SV*  call_func_scalar(pTHX_ SV* cv, SV** dst=NULL);
bool call_func_bool(pTHX_ SV* cv, int boolean_check);
int  call_func_list(pTHX_ SV* cv);
void call_func_void(pTHX_ SV* cv);
SV*  call_method_scalar(pTHX_ const char* method);
int  call_method_list(pTHX_ const char* method);
void call_method_void(pTHX_ const char* method);

// find the given class in the current application namespace and fetch the GV of its typeof() method
SV* fetch_typeof_gv(pTHX_ const char* class_name, size_t class_namelen);

inline
SV* call_func_scalar(pTHX_ cached_cv& cv, SV** dst=NULL)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_scalar(aTHX_ cv.addr, dst);
}

inline
bool call_func_bool(pTHX_ cached_cv& cv, int boolean_check)
{
   if (__builtin_expect(!cv.addr, 0)) fill_cached_cv(aTHX_ cv);
   return call_func_bool(aTHX_ cv.addr, boolean_check);
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

struct base_vtbl {
   MGVTBL std;
   const std::type_info *type;
   SV *typeid_name_sv;
   SV *const_typeid_name_sv;
   size_t obj_size;
   int flags;
   int obj_dimension;
   SV* (*sv_maker)(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
   SV* (*sv_cloner)(pTHX_ SV* src);
   copy_constructor_type copy_constructor;
   assignment_type assignment;
   destructor_type destructor;
};

struct common_vtbl : base_vtbl {
   conv_to_SV_type to_string;
   conv_to_SV_type to_serialized;
   provide_type provide_serialized_type;
};

struct scalar_vtbl : common_vtbl {
   conv_to_int_type to_int;
   conv_to_double_type to_double;
};

struct container_access_vtbl {
   size_t obj_size;
   destructor_type destructor;
   container_begin_type begin;
   container_access_type deref;
   container_access_type random;
};

struct container_vtbl : common_vtbl {
   int own_dimension;
   conv_to_int_type size;
   conv_to_int_type empty;
   container_resize_type resize;
   container_store_type store_at_ref;
   provide_type provide_key_type;
   provide_type provide_value_type;
   container_access_vtbl acc[4];
   AV *assoc_methods;
};

struct iterator_vtbl : base_vtbl {
   iterator_deref_type deref;
   iterator_incr_type incr;
   conv_to_int_type at_end;
};

struct composite_vtbl : common_vtbl {
   int n_elems;
   provide_type provide_elem_types;
   provide_type provide_field_names;
   composite_access_vtbl acc[1];
};

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

int destroy_canned(pTHX_ SV *sv, MAGIC* mg);
int destroy_canned_container(pTHX_ SV *sv, MAGIC* mg);
int destroy_canned_assoc_container(pTHX_ SV *sv, MAGIC* mg);
int clear_canned_container(pTHX_ SV *sv, MAGIC* mg);
int clear_canned_assoc_container(pTHX_ SV *sv, MAGIC* mg);
int assigned_to_primitive_lvalue(pTHX_ SV *sv, MAGIC* mg);
U32 canned_container_size(pTHX_ SV *sv, MAGIC* mg);
int canned_container_access(pTHX_ SV *sv, MAGIC* mg, SV *nsv, const char *dummy, PM_svt_copy_klen_arg index);
int canned_assoc_container_access(pTHX_ SV *sv, MAGIC* mg, SV *val_sv, const char *key, PM_svt_copy_klen_arg klen);
U32 canned_composite_size(pTHX_ SV *sv, MAGIC* mg);
int canned_composite_access(pTHX_ SV *sv, MAGIC* mg, SV *nsv, const char *dummy, PM_svt_copy_klen_arg index);
MAGIC* upgrade_to_builtin_magic_sv(pTHX_ SV* sv, SV* descr_ref, unsigned int n_anchors);
SV* create_builtin_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
SV* create_scalar_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
SV* create_container_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
SV* create_assoc_container_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
SV* create_composite_magic_sv(pTHX_ SV* dst_ref, SV* descr_ref, unsigned int flags, unsigned int n_anchors);
SV* clone_builtin_magic_sv(pTHX_ SV *src);
SV* clone_scalar_magic_sv(pTHX_ SV *src);
SV* clone_container_magic_sv(pTHX_ SV *src);
SV* clone_assoc_container_magic_sv(pTHX_ SV *src);
SV* clone_composite_magic_sv(pTHX_ SV *src);

extern SV *cur_wrapper_cv,
          *negative_indices_key;
extern HV *FuncDescr_stash,
          *TypeDescr_stash,
          *User_stash;
extern GV *CPP_root,
          *PropertyType_nesting_level,
          *User_application,
          *Debug_level;
extern const base_vtbl *cur_class_vtbl;

extern int Object_name_index, Object_description_index,
           Object_parent_index, Object_transaction_index, Object_attachments_index,
           Application_pkg_index, Application_eval_expr_index,
           TypeDescr_pkg_index, TypeDescr_vtbl_index,
           CPPOptions_builtin_index, CPPOptions_descr_index,
           FuncDescr_wrapper_index,
           PropertyType_pkg_index, PropertyType_cppoptions_index,
           CPP_functions_index, CPP_regular_functions_index, CPP_embedded_rules_index,
           CPP_duplicate_class_instances_index, CPP_classes_index, CPP_templates_index, CPP_typeids_index,
           CPP_auto_convert_constructor_index, CPP_auto_assignment_index, CPP_auto_conversion_index,
           CPP_auto_assoc_methods_index, CPP_auto_set_methods_index,
           FuncDescr_fill, TypeDescr_fill;
} } }

#endif // POLYMAKE_PERL_GLUE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
